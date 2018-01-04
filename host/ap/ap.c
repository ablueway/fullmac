/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#define __SFILE__ "ap.c"




#include <log.h>

#include <ssv_types.h>
#include <msgevt.h>
#include <hdr80211.h>
#include <pbuf.h>
#include <ssv_timer.h>

#include "ap_config.h"

#include "ap.h"
#include <os_wrapper.h>
#include <ssv_ether.h>
#include <ssv_drv.h>
#include "ap_sta_info.h"
#include "ap_info.h"

#include "common/bitops.h"
//#include "common/bitmap.h"

#include <host_apis.h>

#include <ethertype.h>
#include <ssv_dev.h>
#include <ssv_hal.h>
#include "common/ieee802_11_defs.h"
#include <ieee80211.h>

//mgmt function
#include "ap_mlme.h"
#include "ap_rx.h"

#include <cmd_def.h>
#include "ap_drv_cmd.h"
#include "dev.h"



#include <core/recover.h>

#if (AP_MODE_ENABLE == 1)

#ifndef CONFIG_NO_WPA2
#include "wpa_auth.h"
extern int wpa_auth_derive_psk(struct ApInfo *pApInfo);
#endif

extern void AP_RxHandleAPMode(CFG_HOST_TXREQ *pPktInfo);
extern void StartBeacon(void);
extern void StopBeacon(void);
extern ssv6xxx_data_result AP_RxHandleData(void *frame);
extern void sta_info_cleanup(void *data1, void *data2);

#ifdef __TEST_DATA__
extern void TestCase_AddAPSta(void);
#endif



s32 AP_Start( void );
s32 AP_Stop( void );
void host_cmd_cb_handler(ssv6xxx_ap_cmd ap_cmd, ssv6xxx_result result);

//--------------------------------------------------

//=====>ADR_MTX_BCN_EN_MISC

#define MTX_BCN_PKTID_CH_LOCK_SHIFT MTX_BCN_PKTID_CH_LOCK_SFT 	
#define MTX_BCN_CFG_VLD_SHIFT MTX_BCN_CFG_VLD_SFT 
#define MTX_BCN_CFG_VLD_MASK  MTX_BCN_CFG_VLD_MSK 

#define MAX_FAIL_COUNT          100
#define MAX_RETRY_COUNT         20

#define PBUF_BASE_ADDR	            0x80000000
#define PBUF_ADDR_SHIFT	            16

#define PBUF_MapPkttoID(_PKT)		(((u32)_PKT&0x0FFF0000)>>PBUF_ADDR_SHIFT)	
#define PBUF_MapIDtoPkt(_ID)		(PBUF_BASE_ADDR|((_ID)<<PBUF_ADDR_SHIFT))

//=====>ADR_CH0_TRIG_1
#define ADDRESS_OFFSET            16                    //16
#define HW_ID_OFFSET              7

//=====>ADR_MCU_STATUS
#define CH0_FULL_MASK             CH0_FULL_MSK              //0x00000001
//=====>

extern struct Host_cfg g_host_cfg;


void AP_RxHandleEvent(void *frame);


//-------------------------------------------------

// only handle null data event, ps poll control frame
s32 ap_handle_ps(void *frame)
{
    ssv_vif* vif = (ssv_vif*)gDeviceInfo->APInfo->vif;
	
    if (SSV6XXX_HWM_AP==vif->hw_mode)
        AP_RxHandleEvent(frame); // only handle null data event, ps poll control frame

    os_frame_free(frame);
	return SSV6XXX_SUCCESS;
}

extern u8* ssv6xxx_host_rx_data_get_data_ptr(CFG_HOST_RXPKT *rxpkt);

//must be data/mgmt frame
ssv6xxx_data_result AP_RxHandleFrame(void *frame)
{
    CFG_HOST_RXPKT *pPktInfo = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);
    ssv6xxx_data_result ret = SSV6XXX_DATA_ACPT;
	

    if(1==ssv_hal_get_rxpkt_f80211(pPktInfo))//MGMT, control
    {   

        u8 *raw = ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);
        u16 fc = (raw[1]<<8) | raw[0];

        //ps poll data send to ap handle ps function, execute on rx task
        if ( WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_CTRL )
        {
            if( !(fc & WLAN_FC_RETRY)&& 
                WLAN_FC_GET_STYPE(fc)== WLAN_FC_STYPE_PSPOLL)
                ap_handle_ps(frame);
            else
                os_frame_free(frame);
        }
        else
        {    
          void *msg;
          int retry = 0;
#if(MLME_TASK ==1)
            ap_mlme_handler(frame,MEVT_PKT_BUF);//send to mlme task
#else                
            retry = 0;
            while (((msg=msg_evt_alloc()) == NULL) && (retry < 10)) {
              OS_TickDelay(1);
              retry++;
            }
            if (retry >= 10)    
            {
                os_frame_free(frame);
                return SSV6XXX_DATA_ACPT;
            }
            
            os_msg_send(msg, frame);

        //AP_RxHandleAPMode(frame);	
#endif	
        }

    }
	else
    {   
		ret = AP_RxHandleData(frame);
    }
	return ret;
}


//ssv6xxx_data_result AP_RxDataCB (void *data, u32 len)
//{
//	return AP_RxHandleFrame((struct cfg_host_rxpkt *)data);
//}


//extern void BeaconGen(void *data1, void *data2);

//Could be null data event, or command response we call before
void AP_RxHandleEvent(void *frame)
{
    struct cfg_host_event *pPktInfo = (struct cfg_host_event *)OS_FRAME_GET_DATA(frame);
    struct ap_rx_desp rx_desp;
    struct cfg_null_data_info *null_data_info;
    struct cfg_ps_poll_info *ps_poll_info;
    MEMSET(&rx_desp, 0, sizeof(struct ap_rx_desp));

	//LOG_TRACE( "Enter AP_RxHandleEvent pHostEvt->c_type %d\n", pHostEvt->c_type);

    if(pPktInfo->c_type == HOST_EVENT)
    {

        switch (pPktInfo->h_event)
        {
            
            case SOC_EVT_NULL_DATA:
                null_data_info = (struct cfg_null_data_info *)(pPktInfo+1);
	            //Frame type, PM/QoS bit/UP
	            rx_desp.flags |= AP_RX_FLAGS_FRAME_TYPE_NULL_DATA;              
	            rx_desp.flags |= (null_data_info->Flags& HOST_EVT_NULL_DATA_PM)?AP_RX_FLAGS_PM_BIT:0;
            	rx_desp.sta = APStaInfo_FindStaByAddr(&null_data_info->SAAddr);
                
            	if(rx_desp.sta)
            	{    
            	    OS_MutexLock(rx_desp.sta->apsta_mutex);                    
                    if(null_data_info->q_len > 0 )
                        rx_desp.sta->fw_q_len = null_data_info->q_len;
                    else
                        rx_desp.sta->fw_q_len = UNKNOWN_FW_QUEUE_LENGTH;

                	rx_desp.sta->arp_retry_count = 0;
                	OS_MutexUnLock(rx_desp.sta->apsta_mutex);
                    
                	rx_desp.flags |= (rx_desp.sta->_flags&WLAN_STA_WMM)?AP_RX_FLAGS_QOS_BIT:0;
                    rx_desp.sta->last_rx = os_sys_jiffies();     
	                 
            	}   
                
	            //rx_desp.data = (void*)pHostEvt;
				rx_desp.UP = null_data_info->Priority;

                if (!rx_desp.sta || !test_sta_flag(rx_desp.sta, WLAN_STA_ASSOC)) {
                    i802_sta_deauth((u8*)gDeviceInfo->APInfo->own_addr, (u8*)&null_data_info->SAAddr, WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA);
                }
                ssv6xxx_data_need_to_be_received(&rx_desp);
                    

                break;
            case SOC_EVT_PS_POLL:		
				ps_poll_info = (struct cfg_ps_poll_info *)(pPktInfo+1);
               
				//Frame type, PM/QoS bit/UP				
				rx_desp.flags |= AP_RX_FLAGS_FRAME_TYPE_PS_POLL;

				rx_desp.sta = APStaInfo_FindStaByAddr(&ps_poll_info->SAAddr);
				if(rx_desp.sta)
				{

                    OS_MutexLock(rx_desp.sta->apsta_mutex);
                    rx_desp.sta->fw_q_len = ps_poll_info->q_len;
                    rx_desp.sta->arp_retry_count = 0;
                    OS_MutexUnLock(rx_desp.sta->apsta_mutex);
					rx_desp.flags |= (rx_desp.sta->_flags&WLAN_STA_WMM)?AP_RX_FLAGS_QOS_BIT:0;
                    rx_desp.sta->last_rx = os_sys_jiffies();
                    
				}	

                if (!rx_desp.sta || !test_sta_flag(rx_desp.sta, WLAN_STA_ASSOC)) {
                    i802_sta_deauth((u8*)gDeviceInfo->APInfo->own_addr, (u8*)&ps_poll_info->SAAddr, WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA);
                }
                ssv6xxx_data_need_to_be_received(&rx_desp);
				//rx_desp.data = (void*)pHostEvt;

                break;
       		default:
			    break;
        }    
    }
    else if (pPktInfo->c_type == M0_RXEVENT)
    {
		#if 0
        struct cfg_host_rxpkt *pPktInfo = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);
        u8 *raw = ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);        
        struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)(raw);

                        
        //Frame type, PM/QoS bit/UP             
        rx_desp.flags |= AP_RX_FLAGS_FRAME_TYPE_PS_POLL;

        rx_desp.sta = APStaInfo_FindStaByAddr((ETHER_ADDR *)mgmt->sa);

      
        if(rx_desp.sta)
        {
            OS_MutexLock(rx_desp.sta->apsta_mutex);
            rx_desp.sta->arp_retry_count = 0;
            OS_MutexUnLock(rx_desp.sta->apsta_mutex);
            rx_desp.flags |= (rx_desp.sta->_flags&WLAN_STA_WMM)?AP_RX_FLAGS_QOS_BIT:0;
        }                   
        
        ssv6xxx_data_need_to_be_received(&rx_desp);     
 		#endif
    }        
    else
    {
        LOG_TRACE("not accept frame type [type:%x]\r\n",pPktInfo->c_type);        
    }

    
	

	

	//FREEIF(pHostEvt);
	
}





// void AP_HandleQueue(PKT_RxInfo *pPktInfo)
// {
// 
// 	//event/frames(data/ctrl/mgmt)/cmd
// 	switch (pPktInfo->c_type)
// 	{
// 		
// 	//-------------
// 	//RX
// 		case M0_RXEVENT:		
// 		//case M1_RXEVENT:
// 			AP_RxHandleFrame((struct cfg_host_rxpkt *)pPktInfo);
// 			break;
// 		
// 
// 		case HOST_EVENT:
// 			AP_RxHandleEvent((struct cfg_host_event *)pPktInfo);
// 			break;			
// 	//-------------
// 	//TX
// 		case HOST_CMD:
// 			AP_TxHandleCmd((struct cfg_host_cmd *)pPktInfo);
// 			break;
// 
// 
// 		default:
// 			LOG_WARN( "Unexpect c_type %d appeared", pPktInfo->c_type);
// 			ASSERT(0);
// 	}
// 
// }





void init_host_tx_req(void)
{
	/* TXD(12) */
	CFG_HOST_TXREQ *req = os_frame_push(gDeviceInfo->APInfo->pOSMgmtframe, ssv_hal_get_txreq0_size());
	MEMSET(req, 0, ssv_hal_get_txreq0_size());
	/* not in */
#if(SW_8023TO80211 == 1)
	ssv_hal_set_txreq0_ctype(req, M2_TXREQ);
	/* in */
#else
	ssv_hal_set_txreq0_ctype(req, M0_TXREQ);
#endif

	/* set hci check hdr and do 802.3 hdr to 802.11 hdr translation */
	ssv_hal_set_txreq0_f80211(req, 1);
}

s32 APInfoInit(u32 max_sta_num)
{
	/* gMaxAID  = WLAN_MAX_STA(8) */
	//Belong to cmd engine
	gMaxAID = max_sta_num;
    gMaxTimLen = (sizeof(unsigned long) * BITS_TO_LONGS(gMaxAID + 1));

    /*  Allocate Memory for STA data structure. */
    gDeviceInfo->APInfo = (struct ApInfo *)MALLOC((u32)sizeof(ApInfo_st));
    ASSERT_RET(gDeviceInfo->APInfo != NULL, OS_FAILED);
    MEMSET(gDeviceInfo->APInfo, 0, (u32)sizeof(ApInfo_st));
    gDeviceInfo->APInfo->tim = (u8*)MALLOC(gMaxTimLen);
    ASSERT_RET(gDeviceInfo->APInfo->tim != NULL, OS_FAILED);
    MEMSET(gDeviceInfo->APInfo->tim, 0, (u32)gMaxTimLen);

    OS_MutexInit(&gDeviceInfo->APInfo->ap_info_ps_mutex);
/* in */
#if (AUTO_BEACON == 0)
    OS_MutexInit(&gDeviceInfo->APInfo->g_dev_bcn_mutex);
#endif

	//------------------------------
    gDeviceInfo->APInfo->pOSMgmtframe = os_frame_alloc(AP_MGMT_PKT_LEN,FALSE);
    ASSERT(NULL != gDeviceInfo->APInfo->pOSMgmtframe);	
    gDeviceInfo->APInfo->pMgmtPkt = OS_FRAME_GET_DATA(gDeviceInfo->APInfo->pOSMgmtframe);

	init_host_tx_req();
	
	//------------------------------
	//Beacon

	//Send beacon by data path
    gDeviceInfo->APInfo->pBeaconHead = (u8 *)gDeviceInfo->APInfo->bcn + pb_offset;
    gDeviceInfo->APInfo->pBeaconTail= (u8 *)MALLOC(AP_BEACON_TAIL_BUF_SIZE);
    ASSERT(NULL != gDeviceInfo->APInfo->pBeaconTail);

	//----------------------
	//timer
    gDeviceInfo->APInfo->sta_pkt_cln_timer =FALSE;

/* in */
#ifndef CONFIG_NO_WPA2
    gDeviceInfo->APInfo->wpa_auth.conf = &(gDeviceInfo->APInfo->ap_conf);
    gDeviceInfo->APInfo->wpa_auth.group = &(gDeviceInfo->APInfo->ap_group);   
#endif
}

extern void os_timer_init(ApInfo_st *pApInfo);


void AP_Config_HW(void)
{

   

}
extern void APTxDespInit(void);
extern u32 gMaxAID;
extern u32 gMaxTimLen;
extern u8 config_mac[];
extern int ssv6xxx_get_cust_mac(u8 *mac);

s32 AP_Init(u32 max_sta_num)
{
	APInfoInit(max_sta_num);
	ssv6xxx_config_init(&gDeviceInfo->APInfo->config);

	//Resource allocate
	APTxDespInit();

	APStaInfo_Init();

	//-----------------------------------------------

	gDeviceInfo->APInfo->eCurrentApMode		= gDeviceInfo->APInfo->config.eApMode;
	gDeviceInfo->APInfo->b80211n			= gDeviceInfo->APInfo->config.b80211n;
	gDeviceInfo->APInfo->nCurrentChannel	= gDeviceInfo->APInfo->config.nChannel;

	//-----------------------------------------------
	//Security
	gDeviceInfo->APInfo->auth_algs = WPA_AUTH_ALG_OPEN|WPA_AUTH_ALG_SHARED;

    gDeviceInfo->APInfo->acs_start = FALSE;

	//Set beacon
   	AP_Config_HW();

	//	LOG_TRACE( "Leave AP_Init\n");
    return OS_SUCCESS;
}




void AP_InfoReset()
{

	
	
	MEMSET(gDeviceInfo->APInfo->password, 0, MAX_PASSWD_LEN+1);
	//SSID
	MEMSET(gDeviceInfo->APInfo->config.ssid, 0, IEEE80211_MAX_SSID_LEN+1);	
	gDeviceInfo->APInfo->config.ssid_len = 0;

	gDeviceInfo->APInfo->auth_algs = 0;
	gDeviceInfo->APInfo->tkip_countermeasures = 0;
	gDeviceInfo->APInfo->sec_type=0;
	MEMSET(gDeviceInfo->APInfo->password, 0, MAX_PASSWD_LEN+1);

}





s32 AP_Config(struct cfg_set_ap_cfg *ap_cfg)
{

	//SEC
	gDeviceInfo->APInfo->sec_type = ap_cfg->sec_type;
	
	//password
	MEMCPY((void*)gDeviceInfo->APInfo->password, (void*)ap_cfg->password, strlen((void*)ap_cfg->password));
	

	//SSID
	MEMCPY((void*)gDeviceInfo->APInfo->config.ssid, (void*)ap_cfg->ssid.ssid, ap_cfg->ssid.ssid_len);
	gDeviceInfo->APInfo->config.ssid[ap_cfg->ssid.ssid_len]=0;
	gDeviceInfo->APInfo->config.ssid_len = ap_cfg->ssid.ssid_len;



		
	return 0;
}

//0-->success others-->fail
bool ssv6xxx_beacon_enable( bool bEnable)
{
    int ret=0;
    ret=ssv_hal_beacon_enable(bEnable);
    if(ret==0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


void Send_Beacon()
{
    
    StartBeacon();
    ap_soc_set_bcn(SSV6XXX_SET_INIT_BEACON, gDeviceInfo->APInfo->bcn, &gDeviceInfo->APInfo->bcn_info, AP_DEFAULT_DTIM_PERIOD-1, g_host_cfg.bcn_interval);
            
}
s32 AP_Start( void )
{		
    neighbor_ap_list_init(gDeviceInfo->APInfo);
    
#ifndef CONFIG_NO_WPA2    
	{   
        struct wpa_auth_config *conf= (struct wpa_auth_config *)(gDeviceInfo->APInfo->wpa_auth.conf);        
        if(conf->wpa_key_mgmt == WPA_KEY_MGMT_PSK)    	
    	    wpa_auth_derive_psk(gDeviceInfo->APInfo);
        else            
        {     
			Send_Beacon();
    
        }
    }
#else
	Send_Beacon();	 
    
#endif
    return 1;
}


extern void APStaInfo_Release(void);
extern void release_beacon_info(void);

s32 AP_Stop( void )
{
	
    neighbor_ap_list_deinit(gDeviceInfo->APInfo);  
    APStaInfo_Release();
    // In b mode device will change beacon at APStaInfo_Release(), we need to release beacon after APStaInfo_Release()
    os_cancel_timer(sta_info_cleanup, 0, 0);
    OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
    gDeviceInfo->APInfo->sta_pkt_cln_timer = FALSE;
    OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);

    ssv6xxx_beacon_enable(false); 

    return 1;
}



// void host_cmd_cb_handler(ssv6xxx_ap_cmd ap_cmd, ssv6xxx_result result)
// {
// 	gAPInfo->cb(ap_cmd, result);
// }



	
			

s32 AP_Command(ssv6xxx_ap_cmd cmd, APCmdCb cb, void *data)
{

// 	if (cmd == SSV6XXX_AP_ON)
// 	{
// 		struct cfg_host_cmd *pPktInfo = (struct hw_mode_st *)data;
// 		struct cfg_ioctl_request* ioctlreq = (struct cfg_ioctl_request*) pPktInfo->dat;
// 		struct hw_mode_st *hw_mode = &ioctlreq->u.hw_mode;
// 		u8 len = strlen(hw_mode->ssid);
// 
// 		//Set SSID
// 		memcpy(gAPInfo->config.ssid, hw_mode->ssid, len);			
// 		gAPInfo->config.ssid[len]='\0';
// 		gAPInfo->config.ssid_len = len;
// 
// 		//Set Password
// 		memcpy((void*)&gAPInfo->password, (void*)&hw_mode->password, strlen((const char*)&hw_mode->password));
// 		
// 
// 		//Set Security Type
// 		gAPInfo->sec_type = hw_mode->sec_type;//SSV6XXX_SEC_NONE, SSV6XXX_SEC_WEP_40, SSV6XXX_SEC_WEP_104...
// 
// 		AP_Start();
// 		gAPInfo->cb = cb;
// 	} 
// 	else
// 	{
// 		//AP off
// 		AP_Stop();
// 		gAPInfo->cb = cb;
// 	}

	return SSV6XXX_SUCCESS;
}
#endif
