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
#include <ieee80211.h>
#include "log.h"

extern s32 TXRXTask_FrameEnqueue(void *frame, u32 priority);

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
			if(1==ssv_hal_get_txreq0_ht(host_txreq)) 
				copy_len+=IEEE80211_HT_CTL_LEN;
			if(1==ssv_hal_get_txreq0_qos(host_txreq)) 
				copy_len+=IEEE80211_QOS_CTL_LEN;
			if(1==ssv_hal_get_txreq0_4addr(host_txreq)) 
				copy_len+=ETHER_ADDR_LEN;
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
	if (TX_DROP == retAP)
		os_frame_free(dup_frame);

    return ret;
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

s32 TxHdl_FlushFrame(void)
{
    //todo: execute pause of txrx task
    //flush the data frames inside txrx task
    //
    return true;
}


s32 TxHdl_Init(void)
{
    return 0;
}

