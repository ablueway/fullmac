/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#endif

#include <os_wrapper.h>

#include <txrx_hdl.h>
#include <txrx_hdl_tx.h>
#include <txrx_hdl_rx.h>


#include <ssv_dev.h>
#include <ssv_hal.h>
#include <hdr80211.h>
#include <ieee80211.h>
#include "log.h"
#include "ssv_timer.h"

extern int ssv_hal_get_rxpkt_seqnum(CFG_HOST_RXPKT *p);
extern struct rx_ba_session_desc g_ba_rx_session_desc[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS];
extern int ssv_hal_get_rxpkt_wsid(CFG_HOST_RXPKT *p);
extern int ssv_hal_get_rxpkt_tid(CFG_HOST_RXPKT *p);
extern struct Host_cfg g_host_cfg;

extern struct task_info_st g_host_task_info[];

extern s32 TXRXTask_Init(void);

static inline int seq_less(u16 sq1, u16 sq2)
{
	return ((sq1 - sq2) & SEQ_MASK) > (SEQ_MODULO >> 1);//2048
}

static inline u16 seq_inc(u16 sq)
{
	return (sq + 1) & SEQ_MASK;
}

static inline u16 seq_sub(u16 sq1, u16 sq2)
{
	return (sq1 - sq2) & SEQ_MASK;
}


void send_to_data_handler(void* frame, u8 vif_idx)
{
    u32 i;
    ssv6xxx_data_result data_ret = SSV6XXX_DATA_CONT;
	//remove ssv descriptor(RxInfo), just leave raw data.
	os_frame_pull(frame, RxHdl_GetRawRxDataOffset((CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame)));

	for (i=0;i<HOST_DATA_CB_NUM;i++)
	{
		data_handler handler = gDeviceInfo->data_cb[i];
		if (handler)
		{
			data_ret = handler(frame, OS_FRAME_GET_DATA_LEN(frame), vif_idx);
			if (SSV6XXX_DATA_ACPT==data_ret)
				break;
		}
	}//-----------------

	if(SSV6XXX_DATA_ACPT != data_ret)
		os_frame_free(frame);

}



void ieee80211_release_reorder_frame(struct rx_ba_session_desc *tid_agg_rx, int index)
{
	//struct ieee80211_local *local = hw_to_local(hw);
	//struct sk_buff *skb = tid_agg_rx->reorder_buf[index];
	//struct ieee80211_rx_status *status;
	void *frame = tid_agg_rx->reorder_buf[index];
    CFG_HOST_RXPKT *pkt_id = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);
    u8 bssid_idx= ssv_hal_get_rxpkt_bssid_idx(pkt_id);

    ssv_vif* vif = &gDeviceInfo->vif[bssid_idx];
    if(bssid_idx>=MAX_VIF_NUM)
    {
        LOG_PRINTF("ieee80211_release_reorder_frame vif invalid =%d\r\n",bssid_idx);
        ASSERT(0);
    }

	//lockdep_assert_held(tid_agg_rx->reorder_lock);

	if (!frame)
		goto no_frame;

	/* release the frame from the reorder ring buffer */
	tid_agg_rx->stored_mpdu_num--;
	tid_agg_rx->reorder_buf[index] = NULL;

    if((vif->hw_mode==SSV6XXX_HWM_STA)&&(vif->m_info.StaInfo->status==DISCONNECT))
    {
        os_frame_free(frame);
    }
    else
    {
        send_to_data_handler(frame,bssid_idx);
    }

no_frame:
	tid_agg_rx->head_seq_num = seq_inc(tid_agg_rx->head_seq_num);
}

void ieee80211_release_reorder_frames(struct rx_ba_session_desc *tid_agg_rx,
	u16 head_seq_num)
{
	int index;

	//lockdep_assert_held(tid_agg_rx->reorder_lock);

	while (seq_less(tid_agg_rx->head_seq_num, head_seq_num)) {
		index = seq_sub(tid_agg_rx->head_seq_num, tid_agg_rx->ssn) %
			tid_agg_rx->buf_size;
		ieee80211_release_reorder_frame(tid_agg_rx, index);
	}
}

/*
 * Timeout (in jiffies) for skb's that are waiting in the RX reorder buffer. If
 * the skb was added to the buffer longer than this time ago, the earlier
 * frames that have not yet been received are assumed to be lost and the skb
 * can be released for processing. This may also release other skb's from the
 * reorder buffer if there are no additional gaps between the frames.
 *
 * Callers must hold tid_agg_rx->reorder_lock.
 */

void ieee80211_sta_reorder_release( struct rx_ba_session_desc *tid_agg_rx)
{
	int index, j;

	//lockdep_assert_held(tid_agg_rx->reorder_lock);

	/* release the buffer until next missing frame */
    //OS_MutexLock(tid_agg_rx->reorder_lock);
	index = seq_sub(tid_agg_rx->head_seq_num, tid_agg_rx->ssn) %
						tid_agg_rx->buf_size;

//---------------------------------------------------------------------------------------------------------------
	if (!tid_agg_rx->reorder_buf[index] &&
	    tid_agg_rx->stored_mpdu_num) {
		/*
		 * No buffers ready to be released, but check whether any
		 * frames in the reorder buffer have timed out.
		 */
		int skipped = 1;
		for (j = (index + 1) % tid_agg_rx->buf_size; j != index;
		     j = (j + 1) % tid_agg_rx->buf_size) {
			if (!tid_agg_rx->reorder_buf[j]) {
				skipped++;
				continue;
			}

			if (skipped&&
                !time_after((unsigned long)OS_GetSysTick(), tid_agg_rx->reorder_time[j] + (unsigned long)HT_RX_REORDER_BUF_TIMEOUT))
				goto set_release_timer;


			ieee80211_release_reorder_frame(tid_agg_rx, j);

			/*
			 * Increment the head seq# also for the skipped slots.
			 */
			tid_agg_rx->head_seq_num =
				(tid_agg_rx->head_seq_num + skipped) & SEQ_MASK;
			skipped = 0;
		}
	} else while (tid_agg_rx->reorder_buf[index]) {
		//Release frame until next missing frame
		ieee80211_release_reorder_frame(tid_agg_rx, index);
		index =	seq_sub(tid_agg_rx->head_seq_num, tid_agg_rx->ssn) %
							tid_agg_rx->buf_size;
	}

//---------------------------------------------------------------------------------------------------------------
	if (tid_agg_rx->stored_mpdu_num) {
		j = index = seq_sub(tid_agg_rx->head_seq_num,
				    tid_agg_rx->ssn) % tid_agg_rx->buf_size;

		for (; j != (index - 1) % tid_agg_rx->buf_size;
		     j = (j + 1) % tid_agg_rx->buf_size) {
			if (tid_agg_rx->reorder_buf[j])
				break;
		}

set_release_timer:
		index = 0;
        //os_create_timer(1 + HT_RX_REORDER_BUF_TIMEOUT,timer_sta_reorder_release,tid_agg_rx,NULL,(void*)TIMEOUT_TASK);
	}
	else {
        //os_cancel_timer(timer_sta_reorder_release,tid_agg_rx,NULL);
	}
}


/*
 * As this function belongs to the RX path it must be under
 * rcu_read_lock protection. It returns false if the frame
 * can be processed immediately, true if it was consumed.
 */
static bool ieee80211_sta_manage_reorder_buf(struct rx_ba_session_desc *tid_agg_rx,
					     void *frame)
{
    CFG_HOST_RXPKT *pkt_id = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);
 	u16 mpdu_seq_num =ssv_hal_get_rxpkt_seqnum(pkt_id);//(sc & IEEE80211_SCTL_SEQ) >> 4;
 	u16 head_seq_num, buf_size;
 	int index;
 	bool ret = true;
    //Need to get by FW
    OS_MutexLock(tid_agg_rx->reorder_lock);
 	buf_size=tid_agg_rx->buf_size;//tid_agg_rx->buf_size;
 	head_seq_num=tid_agg_rx->head_seq_num;//tid_agg_rx->head_seq_num;
//--------------------------------------------------------------------------------------------------------------------------------
//c
//	Wstart+2048  <= 	SN		<	  Wsart

 	/* frame with out of date sequence number */
 	if (seq_less(mpdu_seq_num, head_seq_num)) {
		//dev_kfree_skb(skb);
		os_frame_free(frame);
 		goto out;
	}

//--------------------------------------------------------------------------------------------------------------------------------
//b
//	Wend  < 	   SN	   <	 Wsart+2048
//head_seq_num + buf_size--->Wend+1


	/*
	 * If frame the sequence number exceeds our buffering window
	 * size release some previous frames to make room for this one.
	 */
	if (!seq_less(mpdu_seq_num, head_seq_num + buf_size)) {						//mpdu_seq_num >= Wend+1
		head_seq_num = seq_inc(seq_sub(mpdu_seq_num, buf_size));				//b2
		/* release stored frames up to new head to stack */
		ieee80211_release_reorder_frames(tid_agg_rx, head_seq_num);			//b4
	}

//--------------------------------------------------------------------------------------------------------------------------------
//a
//	Wstart	<=	   SN	   <=	  Wend
//

	/* Now the new frame is always in the range of the reordering buffer */
	index = seq_sub(mpdu_seq_num, tid_agg_rx->ssn) % tid_agg_rx->buf_size;

	/* check if we already stored this frame */
	if (tid_agg_rx->reorder_buf[index]) {
		//dev_kfree_skb(skb);
		os_frame_free(frame);
		goto out;
	}

	/*
	 * If the current MPDU is in the right order and nothing else
	 * is stored we can process it directly, no need to buffer it.
	 * If it is first but there's something stored, we may be able
	 * to release frames after this one.
	 */
	if (mpdu_seq_num == tid_agg_rx->head_seq_num &&
	    tid_agg_rx->stored_mpdu_num == 0) {
		tid_agg_rx->head_seq_num = seq_inc(tid_agg_rx->head_seq_num);
		ret = false;
		goto out;
	}
//
// 	/* put the frame in the reordering buffer */
	tid_agg_rx->reorder_buf[index] = frame;
    tid_agg_rx->reorder_time[index] = OS_GetSysTick();
	tid_agg_rx->stored_mpdu_num++;
    //OS_MutexUnLock(tid_agg_rx->reorder_lock);
	ieee80211_sta_reorder_release(tid_agg_rx);
    //return;
 out:
    OS_MutexUnLock(tid_agg_rx->reorder_lock);
	return ret;
}


/*
 * Reorder MPDUs from A-MPDUs, keeping them on a buffer. Returns
 * true if the MPDU was buffered, false if it should be processed.
 */
void ieee80211_rx_reorder_ampdu(void *frame)
{
    struct rx_ba_session_desc *tid_agg_rx;
    CFG_HOST_RXPKT *pkt_id = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);
    u8 tid;
    u32 wsid = ssv_hal_get_rxpkt_wsid(pkt_id);
    u8 bssid_idx= ssv_hal_get_rxpkt_bssid_idx(pkt_id);

    if(bssid_idx>=MAX_VIF_NUM)
    {
        LOG_PRINTF("%s vif invalid =%d\r\n",__func__,bssid_idx);
        ASSERT(0);
    }

    if(ssv_hal_get_rxpkt_f80211(pkt_id))
        goto dont_reorder;

    if(!ssv_hal_get_rxpkt_qos(pkt_id))
        goto dont_reorder;

    //LOG_PRINTF("seq:%d, tid:%d\r\n",ssv6030_hal_get_rxpkt_seq_num(pkt_id),ssv6030_hal_get_rxpkt_tid(pkt_id));

    /*
    * filter the QoS data rx stream according to
    * STA/TID and check if this STA/TID is on aggregation
    */

    if (wsid >= RX_AGG_RX_BA_MAX_STATION)
        goto dont_reorder;

    tid = ssv_hal_get_rxpkt_tid(pkt_id);
    //LOG_PRINTF("TID:%d\r\n",tid);
    if (tid >= RX_AGG_RX_BA_MAX_SESSIONS)
        goto dont_reorder;

    tid_agg_rx = &g_ba_rx_session_desc[wsid][tid];

    if (tid_agg_rx->buf_size == 0)
        goto dont_reorder;


    if (ieee80211_sta_manage_reorder_buf(tid_agg_rx, frame))
        return;

    dont_reorder:
        send_to_data_handler((void *)frame,bssid_idx);
        return;

}

void ieee80211_delete_ampdu_rx(u8 wsid)
{
    u16 tid,index;
    struct rx_ba_session_desc * tid_agg_rx;
    if(wsid<RX_AGG_RX_BA_MAX_STATION)
    {
        for(tid=0;tid<RX_AGG_RX_BA_MAX_SESSIONS;tid++)
        {
            //OsMutex reorder_lock_temp;
            tid_agg_rx=&g_ba_rx_session_desc[wsid][tid];
            OS_MutexLock(tid_agg_rx->reorder_lock);
            for(index = 0;index<g_host_cfg.ampdu_rx_buf_size;index++)
            {
                ieee80211_release_reorder_frame(tid_agg_rx, index);
            }
            MEMSET((void *)tid_agg_rx->reorder_time,0,g_host_cfg.ampdu_rx_buf_size*sizeof(unsigned long));
            tid_agg_rx->head_seq_num=0;
        	tid_agg_rx->stored_mpdu_num=0;
        	tid_agg_rx->ssn=0;
        	tid_agg_rx->buf_size=0;
            OS_MutexUnLock(tid_agg_rx->reorder_lock);
        }
    }
    return ;
}

void ieee80211_delete_all_ampdu_rx(void)
{
    u8 wsid=0;
    for(wsid=0; wsid<RX_AGG_RX_BA_MAX_STATION; wsid++)
    {
        ieee80211_delete_ampdu_rx(wsid);
    }
    return;
}

void timer_sta_reorder_release(void* data1, void* data2)
{
    //checkdering buffer
    u16 wsid,tid;
    struct rx_ba_session_desc * tid_agg_rx;
    for(wsid=0; wsid<RX_AGG_RX_BA_MAX_STATION; wsid++)
    {
        for(tid=0;tid<RX_AGG_RX_BA_MAX_SESSIONS;tid++)
        {
            tid_agg_rx=&g_ba_rx_session_desc[wsid][tid];
            if (tid_agg_rx->buf_size == 0)
                continue;

            OS_MutexLock(tid_agg_rx->reorder_lock);
            ieee80211_sta_reorder_release(tid_agg_rx);
            OS_MutexUnLock(tid_agg_rx->reorder_lock);
        }
    }
    os_create_timer(HT_RX_REORDER_BUF_TIMEOUT,timer_sta_reorder_release,NULL,NULL,(void*)TIMEOUT_TASK);
}

void ieee80211_addba_handler (void *data)
{
    struct cfg_addba_info* addbainfo = (struct cfg_addba_info *)data;
    struct rx_ba_session_desc *tid_agg_rx;
    u32 wsid = addbainfo->wsid;
    u32 tid = addbainfo->tid;
    u32 index;
    gDeviceInfo->ampduRx_vif = addbainfo->bssid_idx;
    if ((wsid < RX_AGG_RX_BA_MAX_STATION) && (tid < RX_AGG_RX_BA_MAX_SESSIONS))
    {
        tid_agg_rx=&g_ba_rx_session_desc[wsid][tid];
        OS_MutexLock(tid_agg_rx->reorder_lock);
        for (index = 0; index<g_host_cfg.ampdu_rx_buf_size; index++)//RX_AGG_RX_BA_MAX_BUF_SIZE
            ieee80211_release_reorder_frame(tid_agg_rx, index);
        tid_agg_rx->buf_size = addbainfo->buf_size;
        tid_agg_rx->ssn = addbainfo->ssn;
        tid_agg_rx->head_seq_num = addbainfo->ssn;
        OS_MutexUnLock(tid_agg_rx->reorder_lock);
        LOG_PRINTF("addba: wsid:%d,tid:%d,ssn:%d,buf_size:%d\r\n",
			addbainfo->wsid,addbainfo->tid,addbainfo->ssn,addbainfo->buf_size);
        os_cancel_timer(timer_sta_reorder_release,(u32)NULL,(u32)NULL);
        os_create_timer(HT_RX_REORDER_BUF_TIMEOUT,timer_sta_reorder_release,NULL,NULL,(void*)TIMEOUT_TASK);
    }
}


s32 TxRxHdl_Init(void)
{
	TxHdl_Init();
	RxHdl_Init();
    TXRXTask_Init();
    return 0;
}
