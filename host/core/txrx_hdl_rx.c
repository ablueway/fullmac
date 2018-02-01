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
#include <hdr80211.h>
#include <ieee80211.h>
#include "log.h"



#define RXPKTFLT_NUM 5

OsMutex rxhdl_mtx;
u8 wifi_flt_num;
u8 eth_flt_num;

static struct wifi_flt rx_wifi_flt[RXPKTFLT_NUM];
static struct eth_flt rx_eth_flt[RXPKTFLT_NUM];

/* RX_AGG_RX_BA_MAX_STATION(8), RX_AGG_RX_BA_MAX_SESSIONS(1) */
struct rx_ba_session_desc g_ba_rx_session_desc[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS];

extern int ssv_hal_get_rxpkt_wsid(CFG_HOST_RXPKT *p);
extern u8 *ssv6xxx_host_rx_data_get_data_ptr(CFG_HOST_RXPKT *rxpkt);



u16 RxHdl_GetRawRxDataOffset(CFG_HOST_RXPKT *p)
{
    u16 offset=0;
    offset=(u32)ssv_hal_get_rxpkt_data_ptr(p)-(u32)p;
    return offset;
}

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

s32 RxHdl_Init(void)
{
    OS_MutexInit(&rxhdl_mtx);
    MEMSET(&rx_wifi_flt, 0, sizeof(rx_wifi_flt));
    MEMSET(&rx_eth_flt, 0, sizeof(rx_eth_flt));
    wifi_flt_num = 0;
    eth_flt_num = 0;

    return 0;
}
