/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#endif

#include <os_wrapper.h>
#include <dev.h>
#include <txrx_hdl.h>
#include <hctrl.h>
#if (AP_MODE_ENABLE == 1)        
#include <ap_info.h>
#endif
#include <ssv_dev.h>
#include <ssv_hal.h>
#include <hdr80211.h>
#include <ieee80211.h>


void ieee80211_sta_reorder_release(struct rx_ba_session_desc *tid_agg_rx);
void ieee80211_rx_reorder_ampdu(void * pkt_id);

struct qos_ctrl_st {
    u16                 tid:4;
    u16                 bit4:1;
    u16                 ack_policy:2;
    u16                 rsvd:1;
    u16                 bit8_15:8;
};

struct ht_ctrl_st {
	u32 				ht;
};


struct a4_ctrl_st {
	ETHER_ADDR			a4;
};


#define CFG_HOST_TXREQ0(host_txreq0,len,c_type,f80211,qos,ht,use_4addr,RSVD0,padding,bc_queue,security,more_data,sub_type,extra_info) do{\
    u32 *temp;                                       \
    temp = (u32*)host_txreq0 ;                       \
    *temp = (len<<0) |                              \
            (c_type<<16)|                           \
            (f80211<<19)|                           \
            (qos<<20)|                              \
            (ht<<21)|                               \
            (use_4addr<<22)|                        \
            (RSVD0<<23)|                            \
            (padding<<24)|                          \
            (bc_queue<<26)|                         \
            (security<<27)|                         \
            (more_data<<28)|                        \
            (sub_type<<29)|                         \
            (extra_info<<31);                        \
            }while(0)

#define RXPKTFLT_NUM 5
OsMutex rxhdl_mtx;
u8  wifi_flt_num;
u8  eth_flt_num;
static struct wifi_flt rx_wifi_flt[RXPKTFLT_NUM];
static struct eth_flt rx_eth_flt[RXPKTFLT_NUM];
extern struct rx_ba_session_desc g_ba_rx_session_desc[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS];
extern struct Host_cfg g_host_cfg;
bool TxHdl_FrameProc(void *frame, bool apFrame, u32 priority, u32 flags, ssv_vif* vif);



#include "log.h"
#include "ssv_timer.h"
extern s32 TXRXTask_FrameEnqueue(void *frame, u32 priority);
extern s32 TXRXTask_Init(void);
extern int ssv_hal_get_rxpkt_wsid(CFG_HOST_RXPKT *p);
extern u8 *ssv6xxx_host_rx_data_get_data_ptr(CFG_HOST_RXPKT *rxpkt);
extern s32 os_create_timer(u32 ms, timer_handler handler, void *data1, void *data2, void *mbx);
int ssv_hal_get_rxpkt_seqnum(CFG_HOST_RXPKT *p);
extern int ssv_hal_get_rxpkt_tid(CFG_HOST_RXPKT *p);


u16 RxHdl_GetRawRxDataOffset(CFG_HOST_RXPKT *p)
{
    u16 offset=0;
    offset=(u32)ssv_hal_get_rxpkt_data_ptr(p)-(u32)p;
    return offset;
}

bool TxHdl_prepare_wifi_txreq(u8 vif_idx ,void *frame, u32 len, bool f80211, u32 priority, u8 tx_dscrp_flag)
{
    void *_frame=NULL;
    bool qos = false, ht = false, use_4addr = false;
    u8 security;
    u8 *addr4=NULL;
    u16 *qos_ctrl=NULL;
    u32 *ht_ctrl=NULL;
    ssv_vif* vif=NULL;

    vif = &gDeviceInfo->vif[vif_idx];
    if(gDeviceInfo->recovering==TRUE)
        return FALSE;

	//get info from
	if(vif->hw_mode == SSV6XXX_HWM_STA)
	{
        if((vif->m_info.StaInfo->status==DISCONNECT)||(gDeviceInfo->recovering==TRUE)){
            return FALSE;
        }
		//STA mode
        qos = !!(IS_TXREQ_WITH_QOS(gDeviceInfo));
        ht = !!(IS_TXREQ_WITH_HT(gDeviceInfo));
        use_4addr = !!(IS_TXREQ_WITH_ADDR4(gDeviceInfo));
	}
	else if(vif->hw_mode == SSV6XXX_HWM_AP)
	{
		//AP mode
		//get data from station info(ap mode)
#if (BEACON_DBG == 0)
		if(!f80211)
		{
			//802.3
#if (AP_MODE_ENABLE == 1)
			//get  DA
 			ETHER_ADDR *mac = (ETHER_ADDR *)OS_FRAME_GET_DATA(frame);//
			if(FALSE == ap_sta_info_capability(mac , &ht, &qos, &use_4addr))
				return false;
#endif
		}

#else//#if (BEACON_DBG == 1)
		qos=ht=use_4addr=0;
#endif//#if (BEACON_DBG == 1)


	}
	else
	{;}

#if (AP_MODE_ENABLE == 1)
    if(vif->hw_mode == SSV6XXX_HWM_AP)
    {
        security = (gDeviceInfo->APInfo->sec_type!= SSV6XXX_SEC_NONE);
    }
    else
#endif
    {
        security = (vif->m_info.StaInfo->joincfg->sec_type != SSV6XXX_SEC_NONE);
    }

    if (use_4addr)
    {
        addr4=(u8 *)(&gDeviceInfo->addr4);
    }

    if (qos)
    {
        qos_ctrl = (u16 *)(&gDeviceInfo->qos_ctrl);
    }

    if (ht)
    {
        ht_ctrl = (u32 *)(&gDeviceInfo->ht_ctrl);
    }

    #if(SW_8023TO80211==1)
    if(0!=ssv_hal_tx_8023to80211(frame,OS_FRAME_GET_DATA_LEN(frame),priority,qos_ctrl,ht_ctrl,addr4,f80211,security))
    {
        return FALSE;
    }
    #endif

    _frame=(void *)ssv_hal_fill_txreq0(frame,len,priority,qos_ctrl,ht_ctrl,addr4,f80211,security,tx_dscrp_flag,vif_idx);

    if(_frame==NULL)
    {
        return FALSE;
    }

    return (TxHdl_FrameProc(_frame, false, priority, 0, vif));
}

extern tx_result ssv6xxx_data_could_be_send(void *frame, bool bAPFrame, u32 TxFlags);

/*
 * Be noticed that you have to process the frame if the return value of TxHdl_FrameProc is FALSE.
 * The FALSE return value means that the frame is not sent to txrx_task due to some reason.
*/
bool TxHdl_FrameProc(void *frame, bool apFrame, u32 priority, u32 flags, ssv_vif* vif)
{
    s32 retAP = TX_CONTINUE;
    void *dup_frame = frame;
    bool ret = true;
    CFG_HOST_TXREQ *host_txreq=(CFG_HOST_TXREQ *)OS_FRAME_GET_DATA(frame);
    u32 copy_len=0;
    u32 len=0;
    u32 padding=ssv_hal_get_txreq0_padding(host_txreq);
    u8 *pS=NULL,*pD=NULL;

	do
	{

#if( BEACON_DBG == 0)
#if (AP_MODE_ENABLE == 1)
		if (SSV6XXX_HWM_AP == vif->hw_mode)
  		{
  			//Ap mode data could be drop or queue(power saving)
  			if (TX_CONTINUE != (retAP = ssv6xxx_data_could_be_send(frame, apFrame, flags)))
  				break;
  		}
#endif
#endif//#if( BEACON_DBG == 1)

#if(SW_8023TO80211==1)
        if ((padding!=0)&&(1==ssv_hal_get_txreq0_f80211(host_txreq)))
#else
        if ((padding!=0)&&(0==ssv_hal_get_txreq0_f80211(host_txreq)))
#endif
        {
            ssv_hal_set_txreq0_padding(host_txreq,0);// Avoid FW rate control used RSVD_0 error. It need reset to zero.
            copy_len=ssv_hal_get_valid_txinfo_size();
            #if(SW_8023TO80211==0)
            if(1==ssv_hal_get_txreq0_ht(host_txreq)) copy_len+=IEEE80211_HT_CTL_LEN;
            if(1==ssv_hal_get_txreq0_qos(host_txreq)) copy_len+=IEEE80211_QOS_CTL_LEN;
            if(1==ssv_hal_get_txreq0_4addr(host_txreq)) copy_len+=ETHER_ADDR_LEN;
            #endif
            len=ssv_hal_get_txreq0_len(host_txreq);
            len-=padding;
            ssv_hal_set_txreq0_len(host_txreq,len);
            pS=(u8 *)((u32)OS_FRAME_GET_DATA(frame)+copy_len-1);
            pD=pS+padding;
            do{ *pD--=*pS--;}while(--copy_len);
            os_frame_pull(frame,padding);
        }
#if (AP_MODE_ENABLE == 1)
		//Send to tx driver task
        if(apFrame)
        {
            u8 retry = 0;
            while (((dup_frame = os_frame_dup(frame)) == NULL) && (retry < 10))
            {
                OS_TickDelay(1);
                retry++;
            }
        }
#endif

        if(dup_frame)
        {
            ret = TXRXTask_FrameEnqueue(dup_frame, 0);
        }
        else
        {
            ret = false;
            LOG_ERROR("%s can't duplicated frame\n",__func__);
        }
	} while (0);

	//reuse frame buffer in AP mgmt and beacon frame. no need to release
	if (/*!bAPFrame && TX_CONTINUE == retAP ||*/
		TX_DROP == retAP)
		os_frame_free(dup_frame);

    return ret;
}

s32 TxHdl_FlushFrame()
{
    //todo: execute pause of txrx task
    //flush the data frames inside txrx task
    //
    return true;
}


s32 TxRxHdl_Init()
{
    OS_MutexInit(&rxhdl_mtx);
    MEMSET(&rx_wifi_flt, 0, sizeof(rx_wifi_flt));
    MEMSET(&rx_eth_flt, 0, sizeof(rx_eth_flt));
    wifi_flt_num = 0;
    eth_flt_num = 0;
    TXRXTask_Init();

    return 0;
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
//u32 Trap7Count=0;
static s32 _RxHdl_FrameProc(void* frame)
{
    ssv6xxx_data_result data_ret = SSV6XXX_DATA_CONT;
    u8 bssid_idx,i=0;
    int wsid;

	//Give it to AP handle firstly.
	//LOG_PRINTF("gHCmdEngInfo->hw_mode: %d \n",gHCmdEngInfo->hw_mode);
    CFG_HOST_RXPKT *pPktInfo = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);
    bssid_idx = ssv_hal_get_rxpkt_bssid_idx(pPktInfo);
    wsid = ssv_hal_get_rxpkt_wsid(pPktInfo);
    if(bssid_idx>=MAX_VIF_NUM)
    {
          
        //LOG_PRINTF("%s bssid_idx invalid =%d\r\n",__func__,bssid_idx);
        if(1==ssv_hal_get_rxpkt_f80211(pPktInfo))
        {
            if (SSV6XXX_HWM_SCONFIG == gDeviceInfo->vif[0].hw_mode)    
            {
#if (ENABLE_SMART_CONFIG == 1)   
                u8 *rx_buf=NULL;
                u8 chl=0;
                u32 len=0;
                for (i=0;i<PRMOISCUOUS_CB_NUM;i++)
                {
                    promiscuous_data_handler handler = gDeviceInfo->promiscuous_cb[i];
                    if (handler)
                    {
                        chl=ssv_hal_get_rxpkt_channel_info(pPktInfo);
                        rx_buf=(u8 *)ssv_hal_get_rxpkt_data_ptr(pPktInfo);
                        len=ssv_hal_get_rxpkt_data_len(pPktInfo);
                        handler(chl,rx_buf,len);
                    }
                }
                data_ret=SSV6XXX_DATA_QUEUED;
#endif
            }
            else
            {
#if (AP_MODE_ENABLE == 1)
                if(gDeviceInfo->APInfo->vif)
                {
                    //AP mode will get 802.11 frame to handle
                    AP_RxHandleFrame(frame);
                }
                else
                {
                    os_frame_free(frame);
                }
#endif
                return 0;            
            }
                
        }        
    }
    else
    {
#ifdef RXFLT_ENABLE
        {
            u8 *raw = (u8 *)ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);

        	//802.11 frame FC from bit 2~bit7 type+subtype
        	u8 b7b2 = raw[0]>>2;


        	//802.11
        	if ((wifi_flt_num != 0) && (1==ssv_hal_get_rxpkt_f80211(pPktInfo)))
            {
                OS_MutexLock(rxhdl_mtx);
                for(;i < RXPKTFLT_NUM; i++)
                {
                    if ((rx_wifi_flt[i].b7b2mask != 0) && ((b7b2&rx_wifi_flt[i].b7b2mask) == rx_wifi_flt[i].fc_b7b2))
                        data_ret = rx_wifi_flt[i].cb_fn(frame, OS_FRAME_GET_DATA_LEN(frame),bssid_idx);
                }
                OS_MutexUnLock(rxhdl_mtx);
            }



        	//Ether type filter. It can register by RxHdl_SetEthRxFlt
            if ((eth_flt_num) != 0 && (1!=ssv_hal_get_rxpkt_f80211(pPktInfo)))
            {
                u16 eth_type = 0;
                u8* eth_type_addr = (u8*) ((u8*)pPktInfo + (RxHdl_GetRawRxDataOffset((CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame)) + 12));
                eth_type = eth_type_addr[0]<<8|eth_type_addr[1];

                OS_MutexLock(rxhdl_mtx);
                for(i = 0;i < RXPKTFLT_NUM; i++)
                {
                    
                    if((eth_type)&&(eth_type == rx_eth_flt[i].ethtype))
                    {
                        LOG_PRINTF("eth_type=0x%x\r\n",eth_type);
                        if(rx_eth_flt[i].cb_fn)
                            data_ret = rx_eth_flt[i].cb_fn(frame, OS_FRAME_GET_DATA_LEN(frame),bssid_idx);
                    }
                }
                OS_MutexUnLock(rxhdl_mtx);
            }

            if (data_ret != SSV6XXX_DATA_CONT)
                return 0;
        }
#endif


    //Handle BAR
    if(1==ssv_hal_get_rxpkt_f80211(pPktInfo))
    {
        u8 *rawdata = (u8 *)ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);
        u16 fc = (rawdata[1]<<8) | rawdata[0];
        if (IS_BAR(fc))
        {
            struct ieee80211_bar *bar_data = (struct ieee80211_bar *)ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);
            struct rx_ba_session_desc  *tid_agg_rx;
            u32 wsid = ssv_hal_get_rxpkt_wsid(pPktInfo);
        	u16 start_seq_num;
        	u16 tid;

            tid = (bar_data->control) >> 12;

            if(wsid < RX_AGG_RX_BA_MAX_STATION&&tid < RX_AGG_RX_BA_MAX_SESSIONS)
            {
                tid_agg_rx = &g_ba_rx_session_desc[wsid][tid];
                //if buf_size=0, it means that this ampdu rx session is deinited.
                if(tid_agg_rx->buf_size!=0)
                {
                    start_seq_num = (bar_data->start_seq_num) >> 4;
                    OS_MutexLock(tid_agg_rx->reorder_lock);
                    ieee80211_release_reorder_frames(tid_agg_rx,start_seq_num);
                    OS_MutexUnLock(tid_agg_rx->reorder_lock);
                }

            }
            os_frame_free(frame);
            return 0;
        }
    }

#if (AP_MODE_ENABLE == 1)
	//AP mode will get 802.11 frame to handle
    	if (SSV6XXX_HWM_AP == gDeviceInfo->vif[bssid_idx].hw_mode)
    	data_ret = AP_RxHandleFrame(frame);
#endif
	if (SSV6XXX_DATA_CONT == data_ret)
	{
        ieee80211_rx_reorder_ampdu(frame);
        return 0;
	}
    }
	if(SSV6XXX_DATA_ACPT != data_ret)
		os_frame_free(frame);

    return 0;

}

s32 RxHdl_FrameProc(void* frame)
{
    s32 ret = SSV6XXX_SUCCESS;
#if(SW_8023TO80211==1)
    CFG_HOST_RXPKT * rxpkt = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);

    if (SSV6XXX_HWM_SCONFIG != gDeviceInfo->hw_mode)
    {
        if(-1==ssv_hal_rx_80211to8023(rxpkt))
        {
            os_frame_free(frame);
            LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING,("[TXRXTask]: free rx pkt due 11 to 3 fail\r\n"));
            return SSV6XXX_FAILED;

        }
    }
#endif
    ret = _RxHdl_FrameProc(frame);
    return ret;

}

s32 RxHdl_SetWifiRxFlt(struct wifi_flt *flt, ssv6xxx_cb_action act)
{
    bool ret = false;
    s8 i = 0, empty = -1, exist = -1;

    OS_MutexLock(rxhdl_mtx);
    for (;i < RXPKTFLT_NUM; i++)
    {
        if(rx_wifi_flt[i].cb_fn == NULL)
        {
            empty = i;
            continue;
        }
        if(flt->cb_fn == rx_wifi_flt[i].cb_fn)
            exist = i;
    }

    if(act == SSV6XXX_CB_ADD)
    {
        if(empty >= 0)
        {
            MEMCPY((void *)&rx_wifi_flt[empty], (void *)flt, sizeof(struct wifi_flt));
            wifi_flt_num++;
            ret = true;
        }
    }
    else
    {
        if(exist >= 0)
        {
            if(act == SSV6XXX_CB_REMOVE)
            {
                MEMSET((void *)&rx_wifi_flt[exist], 0, sizeof(struct wifi_flt));
                wifi_flt_num--;
            }
            else
                MEMCPY((void *)&rx_wifi_flt[exist], (void *)flt, sizeof(struct wifi_flt));
            ret = true;
        }
    }
    OS_MutexUnLock(rxhdl_mtx);

    return ret;
}

s32 RxHdl_SetEthRxFlt(struct eth_flt *flt, ssv6xxx_cb_action act)
{
    bool ret = false;
    s8 i = 0, empty = -1, exist = -1;

    OS_MutexLock(rxhdl_mtx);
    for (;i < RXPKTFLT_NUM; i++)
    {
        if(rx_eth_flt[i].cb_fn == NULL)
        {
            empty = i;
            continue;
        }
        if(flt->cb_fn == rx_eth_flt[i].cb_fn)
            exist = i;
    }

    if(act == SSV6XXX_CB_ADD)
    {
        if(empty >= 0)
        {
            MEMCPY((void *)&rx_eth_flt[empty], (void *)flt, sizeof(struct eth_flt));
            eth_flt_num++;
            ret = true;
        }
    }
    else
    {
        if(exist >= 0)
        {
            if(act == SSV6XXX_CB_REMOVE)
            {
                MEMSET((void *)&rx_eth_flt[exist], 0, sizeof(struct eth_flt));
                eth_flt_num--;
            }
            else
                MEMCPY((void *)&rx_eth_flt[exist], (void *)flt, sizeof(struct eth_flt));
            ret = true;
        }
    }
    OS_MutexUnLock(rxhdl_mtx);

    return ret;
}
/* RX_AGG_RX_BA_MAX_STATION(8), RX_AGG_RX_BA_MAX_SESSIONS(1) */
struct rx_ba_session_desc g_ba_rx_session_desc[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS];
#define SEQ_MODULO 0x1000
#define SEQ_MASK   0xfff

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

	//lockdep_assert_held(&tid_agg_rx->reorder_lock);

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

	//lockdep_assert_held(&tid_agg_rx->reorder_lock);

	while (seq_less(tid_agg_rx->head_seq_num, head_seq_num)) {
		index = seq_sub(tid_agg_rx->head_seq_num, tid_agg_rx->ssn) %
			tid_agg_rx->buf_size;
		ieee80211_release_reorder_frame(tid_agg_rx, index);
	}
}
extern struct task_info_st g_host_task_info[];

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

	//lockdep_assert_held(&tid_agg_rx->reorder_lock);

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
void ieee80211_rx_reorder_ampdu(void * frame)
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
        LOG_PRINTF("addba: wsid:%d,tid:%d,ssn:%d,buf_size:%d\r\n",addbainfo->wsid,addbainfo->tid,addbainfo->ssn,addbainfo->buf_size);
        os_cancel_timer(timer_sta_reorder_release,(u32)NULL,(u32)NULL);
        os_create_timer(HT_RX_REORDER_BUF_TIMEOUT,timer_sta_reorder_release,NULL,NULL,(void*)TIMEOUT_TASK);
    }
}


