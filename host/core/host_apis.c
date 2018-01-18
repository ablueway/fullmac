/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
//#include <linux/semaphore.h>
//#include <linux/mutex.h>
#endif

#include <pbuf.h>
#include <log.h>
#include <hdr80211.h>
#include <ieee80211.h>
#include <cmd_def.h>
#include <host_apis.h>
#include <ssv_drv.h>
#include <mlme.h>
#include <os_wrapper.h>
#include <ssv_timer.h>
#include <dev.h>
#include <txrx_hdl.h>
#include <cmd_def.h>
#include <ssv_dev.h>
#include <ssv_hal.h>

#include "ap_config.h"


/* TODO: aaron */
#if 0 //(CONFIG_SIM_PLATFORM == 1)
#include  <wsimp/wsimp_lib.h>
#endif

#if (AP_MODE_ENABLE == 1)
#include <ap_tx.h>
#include <ap_info.h>
#include <ap_mlme.h>
#ifndef CONFIG_NO_WPA2
#include <common/wpa_common.h>
#include <wpa_auth.h>
#include <wpa_auth_i.h>
#endif
#endif

#include <cli_cmd_wifi.h>
#include <ssv_hal.h>
#include <channel.h>
#include <country_cfg.h>
#include <Regulatory.h>

#if (AP_MODE_ENABLE == 1)
extern void reset_ap_info(void);
#endif

#if (AP_MODE_ENABLE == 1)
extern s32 send_deauth_and_remove_sta(u8 *hwaddr);
#endif
extern bool freq_40MHZ_legal(u32 center_freq,const struct ieee80211_regdomain *regd);
extern ssv6xxx_result check_efuse_chip_id(void);
extern int ssv6xxx_start(ssv_vif *vif);
extern int ssv_hal_remove_interface(u8 itf_idx);


//Mac address
extern u8 config_mac[];
extern u8 g_max_num_of_ap_list;

HOST_API_STATE active_host_api = HOST_API_ACTIVE;
static u32 sg_host_cmd_seq_no;
#define INI_CNT 20


OsMutex	g_host_api_mutex = NULL;
OsSemaphore ap_sta_on_off_sphr = NULL;
OsSemaphore scanning_sphr = NULL;


OsSemaphore ap_sta_on_off_sphr;
OsSemaphore scanning_sphr;


#if (AP_MODE_ENABLE == 1)  
static s32 _ssv6xxx_wifi_auto_channel_selection(u16 channel_mask, u32 channel_5g_mask,u8 vif_idx);
#endif

s32 os_hcmd_msg_send(void* msg, void *pBuffer);
void ap_sta_mode_off(void);
void sta_mode_off(u8 vif_idx);
void ap_mode_off(u8 vif_idx);
void sconfig_mode_off(u8 vif_idx);
extern struct Host_cfg g_host_cfg;


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

vif_state get_current_vif_state(void)
{
    int i;
    vif_state vst=VIF_ALL_DEACTIVE;
    for(i=0;i<MAX_VIF_NUM;i++)
    {
        ssv6xxx_hw_mode hmd = (ssv6xxx_hw_mode)gDeviceInfo->vif[i].hw_mode;
        switch (hmd)
        {
            case SSV6XXX_HWM_STA:
            {
                if((gDeviceInfo->vif[i].m_info.StaInfo)&&
                   (gDeviceInfo->vif[i].m_info.StaInfo->status != DISCONNECT))
                   vst = VIF_ACTIVE_STA;
                   
                return vst;
            }
#if (AP_MODE_ENABLE == 1)
            case SSV6XXX_HWM_AP:
            {
                if((gDeviceInfo->vif[i].m_info.APInfo) && (gDeviceInfo->vif[i].m_info.APInfo->current_st != AP_STATE_IDLE))
                   vst = VIF_ACTIVE_AP;
                   
                return vst;
            }
#endif
			default:
			{
				if((gDeviceInfo->vif[i].m_info.StaInfo)&&
					(gDeviceInfo->vif[i].m_info.StaInfo->status != DISCONNECT))
                   vst = VIF_ACTIVE_STA;
                   
                return vst;
            }
        }
    }
    return vst;
}

struct StaInfo *_alloc_sta_info(void)
{
    struct StaInfo *SInfo = NULL;

    SInfo = (struct StaInfo *)OS_MemAlloc(sizeof(struct StaInfo));
    if(NULL==SInfo)
    {
        return NULL;
    }
    MEMSET(SInfo, 0 , sizeof(struct StaInfo));

    SInfo->joincfg= (struct cfg_join_request *)MALLOC(sizeof(struct cfg_join_request));
    if(NULL==SInfo->joincfg)
    {
        return NULL;
    }

    MEMSET(SInfo->joincfg, 0 , sizeof(struct cfg_join_request));

    SInfo->joincfg_backup= (struct cfg_join_request *)MALLOC(sizeof(struct cfg_join_request));
    if(NULL==SInfo->joincfg_backup)
    {
        return NULL;
    }

    MEMSET(SInfo->joincfg_backup, 0 , sizeof(struct cfg_join_request));
    return SInfo;

}

s32 _ssv6xxx_wifi_ioctl_Ext(u32 cmd_id, void *data, u32 len, bool blocking,const bool mutexLock);
static bool _ssv6xxx_wakeup_wifi(void);
s32 _ssv6xxx_send_msg_to_hcmd_eng(void *data)
{
	s32 ret = SSV6XXX_SUCCESS;
	void *msg = os_msg_alloc();
	if (msg)
		os_msg_send(msg, data);
	else
		ret = SSV6XXX_NO_MEM;
	return ret;
}


s32 _ssv6xxx_wifi_send_cmd(void *pCusData, int nCuslen, ssv6xxx_host_cmd_id eCmdID)
{
  	struct cfg_host_cmd *host_cmd;
	u32 nSize = HOST_CMD_HDR_LEN+nCuslen;
	ssv6xxx_result ret = SSV6XXX_SUCCESS;
	u8* frame;

	do
	{

		frame = os_frame_alloc((u32)nCuslen,FALSE);

        if (frame == NULL) {
            ret = SSV6XXX_NO_MEM;
            break;
        }
		host_cmd = os_frame_push(frame, HOST_CMD_HDR_LEN);
		host_cmd->c_type = HOST_CMD;
        host_cmd->RSVD0 = 0;//(eCmdID>>8)&0x1F;
		host_cmd->h_cmd = (u8)eCmdID;
		host_cmd->len = nSize;
		sg_host_cmd_seq_no++;
		host_cmd->cmd_seq_no=sg_host_cmd_seq_no;
		MEMCPY(host_cmd->un.dat8, pCusData, (u32)nCuslen);

		ret = _ssv6xxx_send_msg_to_hcmd_eng((void *)frame);
	} while (0);

	return ret;
}

s32 _ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len, const bool mutexLock)
{
	ssv6xxx_result ret = SSV6XXX_SUCCESS;

    //Detect host API status
    if (mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {

        if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }

        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if((ssv6xxx_result)FALSE == ret)
                break;
        }

        ret = _ssv6xxx_wifi_send_cmd(data, len, cmd_id);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;
}


H_APIs s32 ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len)
{
	return _ssv6xxx_wifi_ioctl(cmd_id, data, len, TRUE);
}

#if(ENABLE_SMART_CONFIG==1)
/**
 * H_APIs s32 ssv6xxx_wifi_sconfig() - Smart config Request from host to wifi controller
 *                                               to collect nearby APs.
 *
 * This API triggers wifi controller to broadcast probe request frames and
 * wait for the probe response frames from APs - active scan.
 * This API also can issue passive scan request and monitor bradocast
 * beacon from nearby APs siliently.
 */

s32 _ssv6xxx_wifi_sconfig(struct cfg_sconfig_request *csreq,const bool mutexLock)
{
//	struct cfg_host_cmd *host_cmd = NULL;
	s32 size,ret=SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }
        if (csreq == NULL)
        {
            ret = SSV6XXX_INVA_PARAM;
            break;
        }

        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if(FALSE == ret)
                break;
        }

    	size = sizeof(struct cfg_sconfig_request);
        ret = _ssv6xxx_wifi_send_cmd(csreq, size, SSV6XXX_HOST_CMD_SMART_CONFIG);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;


}
H_APIs s32 ssv6xxx_wifi_sconfig(struct cfg_sconfig_request *csreq)
{
    return _ssv6xxx_wifi_sconfig(csreq, TRUE);
}
#endif

static bool _ssv6xxx_wakeup_wifi(void)
{
    bool ret;

    ret=ssv6xxx_drv_wakeup_wifi(TRUE);
    if(TRUE == ret)
    {
        OS_MsDelay(100);
        while(1)
        {
            if(active_host_api != HOST_API_ACTIVE)
            {
                ssv6xxx_drv_wakeup_wifi(TRUE);
                OS_MsDelay(200);
                //LOG_PRINTF("+ ");
            }
            else
                break;
        }
        if(FALSE == ssv6xxx_drv_wakeup_wifi(FALSE))
            LOG_PRINTF("clear PMU wakeup signal fail!!\r\n");
    }
    else
    {
        LOG_PRINTF("Wakeup wifi fail!!\r\n");
    }
    return ret;
}

H_APIs bool ssv6xxx_wifi_wakeup(void)
{
    bool ret;
        
    ret=_ssv6xxx_wakeup_wifi();
    return ret;
}


/**
 * H_APIs s32 ssv6xxx_wifi_pwr_saving() - Info FW to start power saving procedure
 *                                               .
 */

s32 _ssv6xxx_wifi_pwr_saving(struct cfg_ps_request* wowreq,const bool mutexLock)
{
    s32 size,ret=SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do
    {
        if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }
        size = sizeof(struct cfg_ps_request);
        ret = _ssv6xxx_wifi_send_cmd(wowreq, size, SSV6XXX_HOST_CMD_SET_PWR_SAVING);
        if (ret == SSV6XXX_SUCCESS)
        {
            if(wowreq->host_ps_st == HOST_PS_START)
            {
                active_host_api = HOST_API_PWR_SAVING;
                LOG_PRINTF("PS Setup done\r\n");
            }
        }
        else
        {
            active_host_api = HOST_API_ACTIVE;
            LOG_PRINTF("Send CMD_SET_PWR_SAVING fail!!\r\n");
        }
    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;


}
extern void check_watchdog_timer(void *data1, void *data2);
extern struct task_info_st g_host_task_info[];
extern void recovery_pause_resume(bool resume);

H_APIs s32 ssv6xxx_wifi_pwr_saving(struct cfg_ps_request* wowreq, bool on)
{
    u32 ret=SSV6XXX_SUCCESS;
    OS_MutexLock(&g_host_api_mutex);
    if(on == TRUE)
    {
        if(active_host_api == HOST_API_PWR_SAVING)
        {
            LOG_PRINTF("%s fail\r\n",__func__);
            OS_MutexUnLock(&g_host_api_mutex);
            return SSV6XXX_FAILED;
        }
        
        if(wowreq->host_ps_st == HOST_PS_SETUP)
        {
#if(RECOVER_ENABLE == 1)
        ssv_hal_watchdog_disable();

#if(RECOVER_MECHANISM == 1)
        os_cancel_timer(check_watchdog_timer,(u32)NULL,(u32)NULL);
        recovery_pause_resume(false);
#endif //#if(RECOVER_MECHANISM == 1)
        //LOG_PRINTF("PS pause Recovery\r\n");
#endif //#if(RECOVER_ENABLE  == 1)
            ssv_hal_l2_off(0);            
        }
        ret = _ssv6xxx_wifi_pwr_saving(wowreq, FALSE);
        if(ret!=SSV6XXX_SUCCESS)
        {
            LOG_PRINTF("PS fail resume Recovery\r\n");
#if(RECOVER_ENABLE == 1)
            ssv_hal_watchdog_enable();
#if(RECOVER_MECHANISM == 1)
            os_create_timer(IPC_CHECK_TIMER, check_watchdog_timer, NULL, NULL, (void*)TIMEOUT_TASK);
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE  == 1)
        }
    }
    else
    {
        wowreq->host_ps_st = HOST_PS_WAKEUP_OK;
        ret = _ssv6xxx_wifi_pwr_saving(wowreq, FALSE);
        ssv_hal_l2_on(0);

        active_host_api = HOST_API_ACTIVE;
        //LOG_PRINTF("PS resume Recovery\r\n");
#if(RECOVER_ENABLE == 1)
        ssv_hal_watchdog_enable();
#if(RECOVER_MECHANISM == 1)
        recovery_pause_resume(true);
        os_create_timer(IPC_CHECK_TIMER*2, check_watchdog_timer, NULL, NULL, (void*)TIMEOUT_TASK);
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE  == 1)
    }
    OS_MutexUnLock(&g_host_api_mutex);

    return ret;
}
/**
 * H_APIs s32 ssv6xxx_wifi_scan() - Scan Request from host to wifi controller
 *                                               to collect nearby APs.
 *
 * This API triggers wifi controller to broadcast probe request frames and
 * wait for the probe response frames from APs - active scan.
 * This API also can issue passive scan request and monitor bradocast
 * beacon from nearby APs siliently.
 */

s32 _ssv6xxx_wifi_scan(struct cfg_scan_request *csreq,const bool mutexLock)
{
//	struct cfg_host_cmd *host_cmd = NULL;
	s32 size,ret=SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }
        if (csreq == NULL)
        {
            ret = SSV6XXX_INVA_PARAM;
            break;
        }

        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if(FALSE == ret)
                break;
        }

        if((gDeviceInfo->vif[csreq->vif_idx].hw_mode==SSV6XXX_HWM_STA)&&
           (gDeviceInfo->vif[csreq->vif_idx].m_info.StaInfo->status==DISCONNECT))
        {
            MEMSET((void*)gDeviceInfo->ap_list,0,sizeof(struct ssv6xxx_ieee80211_bss)*g_max_num_of_ap_list);
            ssv_hal_sta_rcv_all_bcn();
        }

        //#if(ENABLE_DYNAMIC_RX_SENSITIVE==1)
        //ssv_hal_reduce_phy_cca_bits(); //Only use this function on SmartConfig mode
        //#endif
    	size = sizeof(struct cfg_scan_request) +
    		sizeof(struct cfg_80211_ssid) * csreq->n_ssids;
        ret = _ssv6xxx_wifi_send_cmd(csreq, size, SSV6XXX_HOST_CMD_SCAN);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;


}

#define SSV6200_RX_HIGHEST_RATE             72
typedef struct icomm_hw_info_st
{
    bool ht_supported;
    u8 ampdu_factor;
    u8 ampdu_density;
    u16 ht_capability;
    struct ieee80211_mcs_info mcs;
}icomm_hw_info;

H_APIs void get_icomm_hw_info (icomm_hw_info *hw_info)
{
    //ampdu only set RX, it didnot consider TX
    hw_info->ampdu_density = Minimum_MPDU_Start_Spacing;
    hw_info->ampdu_factor = Maximum_AMPDU_Length_Exponent;

    //you can find rate table in host side
    MEMSET((void *)&hw_info->mcs, 0, sizeof(hw_info->mcs));
    hw_info->mcs.rx_mask[0] = 0xff;
    hw_info->mcs.tx_params |= IEEE80211_HT_MCS_TX_DEFINED;
    hw_info->mcs.rx_highest = cpu_to_le16(SSV6200_RX_HIGHEST_RATE);

    //check support n-mode
    if(g_host_cfg.support_ht)
    {
        hw_info->ht_supported = true;//AP_SUPPORT_80211N

        //icomm support ht_capability
        hw_info->ht_capability = (LDPC_Coding_Capability<<0)
                        |(Supported_Channel_Width_Set<<1)
                        |(SM_Power_Save<<2)
                        |(HT_Greenfield<<4)
                        |(g_host_cfg.ap_rx_short_GI<<5)
                        |(Short_GI_for_40_MHz<<6)
                        |(Tx_STBC<<7)
                        |(Rx_STBC<<8)
                        |(HT_Delayed_Block_Ack<<10)
                        |(Maximum_AMSDU_Length<<11)
                        |(DSSS_CCK_Mode_in_40_MHz<<12)
                        |(Forty_MHz_Intolerant<<14)
                        |(L_SIG_TXOP_Protection_Support<<15);//0x2C
    }
    else
    {
        hw_info->ht_supported = false;
    }

}

static inline void create_sta_ht_cap_ie(u8* ht_cap_ie,icomm_hw_info *hw_info,u16 cap)
{
    u8 *pos;
    u16 tmp;

    if((!ht_cap_ie) || (!hw_info) || (!cap))
    {
        LOG_PRINTF("wrong ht cap ie\r\n");
        return;
    }

    pos = (u8 *)ht_cap_ie;

//	*pos++ = WLAN_EID_HT_CAPABILITY;
//	*pos++ = sizeof(struct ieee80211_ht_cap);
    MEMSET(pos, 0, sizeof(ht_cap_ie));

	//MEMSET(pos, 0, sizeof(struct ieee80211_ht_cap));

	/* capability flags */
	tmp = cpu_to_le16(cap);
	MEMCPY(pos, &tmp, sizeof(u16));
	pos += sizeof(u16);

	/* AMPDU parameters */
	*pos++ = hw_info->ampdu_factor |
		 (hw_info->ampdu_density <<
			IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);

	/* MCS set */
	MEMCPY((void *)pos, (void *)&hw_info->mcs, sizeof(hw_info->mcs));
	pos += sizeof(hw_info->mcs);

	/* extended capabilities */
	pos += sizeof(u16);

	/* BF capabilities */
	pos += sizeof(u32);

	/* antenna selection */
	pos += sizeof(u8);

}

static inline bool build_sta_ht_ie(u8 *ht_cap_ie)
{
    icomm_hw_info icomm_hw_info;
    MEMSET(&icomm_hw_info,0,sizeof(icomm_hw_info));
    get_icomm_hw_info(&icomm_hw_info);

    if(icomm_hw_info.ht_supported)
    {
       create_sta_ht_cap_ie((u8 *)ht_cap_ie,&icomm_hw_info,icomm_hw_info.ht_capability);
       //hex_dump(ht_cap_ie,sizeof(struct ssv_ht_cap));
       return TRUE;
    }else{
       //MEMSET((u8 *)ht_cap_ie,0,sizeof(struct ieee80211_ht_cap));
       return FALSE;
    }

}

H_APIs s32 ssv6xxx_wifi_scan(struct cfg_scan_request *csreq)
{
//    int i;
    ssv_vif* vif = NULL;//(ssv_vif*)gDeviceInfo->StaInfo->vif;
    
    //for(i=0;i<MAX_VIF_NUM;i++)
    //{
    if(gDeviceInfo->vif[csreq->vif_idx].hw_mode == SSV6XXX_HWM_STA)
    {
        //LOG_PRINTF("Invalid arguments.\n");
        //return SSV6XXX_FAILED;
        vif = &gDeviceInfo->vif[csreq->vif_idx];
        //break;
    }
    //}
    //csreq->ht_supported=build_sta_ht_ie(csreq->ht_cap_ie);
    if(vif)
    {
        build_sta_ht_ie(csreq->ht_cap_ie);
        //csreq->vif_idx = vif->idx;

        return _ssv6xxx_wifi_scan(csreq, TRUE);
    }
    else
    {
        LOG_PRINTF("vif(%d) not STA\r\n",csreq->vif_idx);
        return SSV6XXX_FAILED;
    }
}

/**
 * H_APIs s32 ssv6xxx_wifi_join() - Request a AP to create a link between
 *                                             requested AP and a STA.
 *
 * This API triggers authentication and association reqests to an
 * explicitly specify AP and wait for authentication and association
 * response from the AP.
 */
s32 _ssv6xxx_wifi_join(struct cfg_join_request *cjreq, const bool mutexLock)
{
    ssv_vif* vif = &gDeviceInfo->vif[cjreq->vif_idx];
//    struct cfg_host_cmd *host_cmd = NULL;
    s32 size,ret=SSV6XXX_SUCCESS;

    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        //if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }
        if (cjreq == NULL)
        {
            ret = SSV6XXX_INVA_PARAM;
            break;
        }
        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if(FALSE == ret)
                break;
        }

        size =   HOST_CMD_HDR_LEN
            + sizeof(struct cfg_join_request)
            + sizeof(struct cfg_80211_ssid);
        ret = _ssv6xxx_wifi_send_cmd(cjreq, size, SSV6XXX_HOST_CMD_JOIN);

        if (ret == SSV6XXX_SUCCESS)
            MEMCPY(vif->m_info.StaInfo->joincfg_backup, cjreq, sizeof(struct cfg_join_request));
            //_join_security = (cjreq->sec_type != SSV6XXX_SEC_NONE);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;


}
H_APIs s32 ssv6xxx_wifi_join(struct cfg_join_request *cjreq)
{
    ssv_vif* vif = &gDeviceInfo->vif[cjreq->vif_idx];

    if(vif->hw_mode != SSV6XXX_HWM_STA)
    {
        LOG_PRINTF("Invalid arguments.\n");
        return SSV6XXX_FAILED;
    }

    cjreq->ht_supported=build_sta_ht_ie(cjreq->ht_cap_ie);

    return _ssv6xxx_wifi_join(cjreq, TRUE);
}


/**
 * H_APIs s32 ssv6xxx_wifi_leave() - Request wifi controller to leave an
 *                                                explicitly speicify BSS.
 *
 */
s32 _ssv6xxx_wifi_leave(struct cfg_leave_request *clreq,const bool mutexLock)
{

//    struct cfg_host_cmd *host_cmd = NULL;
    s32 size,ret=SSV6XXX_SUCCESS;

    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }
        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if(FALSE == ret)
                break;
        }


        size = HOST_CMD_HDR_LEN	+ sizeof(struct cfg_leave_request);
        ret = _ssv6xxx_wifi_send_cmd(clreq, size, SSV6XXX_HOST_CMD_LEAVE);


    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;

}
H_APIs s32 ssv6xxx_wifi_leave(struct cfg_leave_request *clreq)
{
    ssv_vif *vif = &gDeviceInfo->vif[clreq->vif_idx];
    if(vif->hw_mode != SSV6XXX_HWM_STA)
    {
        LOG_PRINTF("vif[%d] mode[%d] not STA.\n",clreq->vif_idx,vif->hw_mode);
        return SSV6XXX_FAILED;
    }

    return _ssv6xxx_wifi_leave(clreq, TRUE);
}


#if (AP_MODE_ENABLE == 1)
extern tx_result ssv6xxx_data_could_be_send(struct cfg_host_txreq0 *host_txreq0, bool bAPFrame, u32 TxFlags);
#endif

extern void CmdEng_TxHdlData(void *frame);
static inline s32 _ssv6xxx_wifi_send_ethernet(u8 vif_idx, void *frame, s32 len, 
	enum ssv6xxx_data_priority priority, u8 tx_dscrp_flag,const bool mutexLock)
{

	ssv6xxx_result ret = SSV6XXX_SUCCESS;
	//Detect host API status
	if (mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if (active_host_api == HOST_API_DEACTIVE)
        {
    		os_frame_free(frame);
	        ret=SSV6XXX_FAILED;
            break;
        }

        if (active_host_api == HOST_API_PWR_SAVING)
        {
            if (mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if (mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if ((ssv6xxx_result)FALSE == ret)
                break;
        }

        if(TxHdl_prepare_wifi_txreq(vif_idx, frame, (u32)len, FALSE, priority, tx_dscrp_flag)==FALSE)
    	{
    		LOG_TRACE("tx req fail. release frame.\r\n");
    		os_frame_free(frame);
    		ret = SSV6XXX_FAILED;
            break;
    	}
        ret = SSV6XXX_SUCCESS;

    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);



	return ret;
}
H_APIs s32 ssv6xxx_wifi_send_ethernet(u8 vif_idx,void *frame, s32 len, enum ssv6xxx_data_priority priority)
{
	return _ssv6xxx_wifi_send_ethernet(vif_idx, frame, len, priority, FALSE, TRUE);
}

#if 0
static inline s32 _ssv6xxx_wifi_send_80211(void *frame, s32 len, u8 tx_dscrp_flag, const bool mutexLock  )
{

	ssv6xxx_result ret = SSV6XXX_SUCCESS;

    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }

    	if(ssv6xxx_prepare_wifi_txreq(frame, len, TRUE, 0, tx_dscrp_flag)==FALSE)
    	{
    		LOG_TRACE("tx req fail. release frame.\n");
    		os_frame_free(frame);
    		return SSV6XXX_FAILED;
    	}
    	ret = _ssv6xxx_send_msg_to_hcmd_eng(frame);

    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;

}
H_APIs s32 ssv6xxx_wifi_send_80211(void *frame, s32 len)
{
	return _ssv6xxx_wifi_send_80211(frame, len, FALSE, TRUE);
}
#endif


//----------------------------------------------

u8* ssv6xxx_host_tx_req_get_qos_ptr(CFG_HOST_TXREQ *req0)
{
    return ssv_hal_get_txreq0_qos_ptr(req0);
}

u8* ssv6xxx_host_tx_req_get_data_ptr(struct cfg_host_txreq0 *req0)
{
    return ssv_hal_get_txreq0_data_ptr(req0);
}
//----------------------------------------------------------------------------------------------------------

//callback function handle
s32  ssv6xxx_wifi_cb(s32 arraylen, u32 **array, u32 *handler, ssv6xxx_cb_action action)
{
	ssv6xxx_result ret = SSV6XXX_SUCCESS;
	int i, nEmptyIdx=-1;
	bool bFound = FALSE;

	do
	{
		for (i=0;i<arraylen;i++)
		{
			if (NULL == array[i])
			{
				nEmptyIdx = i;
                break;
			}
			else if(handler == array[i])
			{
				nEmptyIdx = i;
				bFound = TRUE;
				break;
			}
		}

		if(action == SSV6XXX_CB_ADD)
		{
			//add
			if (-1 == nEmptyIdx)
			{
				LOG_ERROR("queue is not enough\n");
				ret = SSV6XXX_QUEUE_FULL;
#ifdef __AP_DEBUG__
				ASSERT(FALSE);
#endif//__AP_DEBUG__
				break;
			}
			array[nEmptyIdx] = handler;
		}
		else
		{
			//remove
			if(bFound)
				array[nEmptyIdx] = NULL;
		}

	} while (0);


	return ret;


}
//----------------------------------------------------------------------------------------------------------

//Register handler to get RX data
ssv6xxx_result _ssv6xxx_wifi_reg_rx_cb(data_handler handler,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }

        ret = ssv6xxx_wifi_cb(HOST_DATA_CB_NUM, (u32 **)&gDeviceInfo->data_cb , (u32 *)handler, SSV6XXX_CB_ADD);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;
}


//Register handler to get RX data
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_rx_cb(data_handler handler)
{
	return _ssv6xxx_wifi_reg_rx_cb(handler, TRUE);
}

ssv6xxx_result _ssv6xxx_wifi_reg_evt_cb(evt_handler evtcb, const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }
        ret = ssv6xxx_wifi_cb(HOST_EVT_CB_NUM, 
			(u32 **)&gDeviceInfo->evt_cb , (u32 *)evtcb, SSV6XXX_CB_ADD);

    } while(0);

    if (mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_reg_evt_cb(evt_handler evtcb)
{
	return _ssv6xxx_wifi_reg_evt_cb(evtcb,TRUE);
}


ssv6xxx_result _ssv6xxx_wifi_unreg_rx_cb(data_handler handler,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE )
        {
	        ret=SSV6XXX_FAILED;
            break;
        }

        ret = ssv6xxx_wifi_cb(HOST_DATA_CB_NUM, (u32 **)&gDeviceInfo->data_cb , (u32 *)handler, SSV6XXX_CB_REMOVE);

    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);


	return ret;
}
//Register handler to get RX data
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_rx_cb(data_handler handler)
{
	return _ssv6xxx_wifi_unreg_rx_cb(handler,TRUE);
}


ssv6xxx_result _ssv6xxx_wifi_unreg_evt_cb(evt_handler evtcb,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }

        ret = ssv6xxx_wifi_cb(HOST_EVT_CB_NUM, (u32 **)&gDeviceInfo->evt_cb , (u32 *)evtcb, SSV6XXX_CB_REMOVE);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);


	return ret;
}

//Register handler to get event
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_evt_cb(evt_handler evtcb)
{
	return _ssv6xxx_wifi_unreg_evt_cb(evtcb,TRUE);
}


extern int ssv6xxx_get_cust_mac(u8 *mac);
extern struct rx_ba_session_desc g_ba_rx_session_desc[RX_AGG_RX_BA_MAX_STATION][RX_AGG_RX_BA_MAX_SESSIONS];
extern ssv6xxx_result CmdEng_Init(void);

H_APIs ssv6xxx_result ssv6xxx_wifi_init(void)
{
    ssv6xxx_result res;
    u32 i,j;

    res = CmdEng_Init();
    gDeviceInfo = (DeviceInfo_st *)MALLOC(sizeof(DeviceInfo_st));
    if (gDeviceInfo == NULL)
    {
        res = SSV6XXX_NO_MEM;
        goto END;
    }

    MEMSET((void *)gDeviceInfo, 0 , sizeof(DeviceInfo_st));
    gDeviceInfo->vif[0].hw_mode = SSV6XXX_HWM_INVALID;
    gDeviceInfo->vif[0].idx = 0;
    gDeviceInfo->vif[1].hw_mode = SSV6XXX_HWM_INVALID;
    gDeviceInfo->vif[1].idx = 1;
    gDeviceInfo->used_vif = 0;


	/* g_max_num_of_ap_list = NUM_AP_INFO(10) */
    gDeviceInfo->ap_list = (struct ssv6xxx_ieee80211_bss *)
		MALLOC(sizeof(struct ssv6xxx_ieee80211_bss)*g_max_num_of_ap_list);
    if (gDeviceInfo->ap_list == NULL)
    {
        res = SSV6XXX_NO_MEM;
        goto END;
    }

	/* g_max_num_of_ap_list = NUM_AP_INFO(10) */
    MEMSET((void *)gDeviceInfo->ap_list,0,
		sizeof(struct ssv6xxx_ieee80211_bss)*g_max_num_of_ap_list);

	//init mutex
    OS_MutexInit(&gDeviceInfo->g_dev_info_mutex);

    //init mutex
    OS_MutexInit(&g_host_api_mutex);

    OS_SemInit(&ap_sta_on_off_sphr, 1, 0);

	sg_host_cmd_seq_no = 0;

	// init AMPDU RX
	/* not in, g_host_cfg.ampdu_rx_enable= AMPDU_RX_ENABLE(0) */
    if (g_host_cfg.ampdu_rx_enable)
    {
		/* RX_AGG_RX_BA_MAX_STATION = 8 */
        for(i=0;i<RX_AGG_RX_BA_MAX_STATION;i++)
        {
			/* RX_AGG_RX_BA_MAX_STATIONS = 1 */
            for(j=0;j<RX_AGG_RX_BA_MAX_SESSIONS;j++)
            {
    			struct rx_ba_session_desc *tid_agg_rx = &g_ba_rx_session_desc[i][j];                

                MEMSET((void *)tid_agg_rx, 0, sizeof(struct rx_ba_session_desc));

				/* allocated (ampdu_rx_buf_size * 4) block of buffer */
                tid_agg_rx->reorder_buf = OS_MemAlloc(g_host_cfg.ampdu_rx_buf_size * sizeof(void *));
                ASSERT(tid_agg_rx->reorder_buf);
                MEMSET(tid_agg_rx->reorder_buf,0,(g_host_cfg.ampdu_rx_buf_size * sizeof(void *)));

				tid_agg_rx->reorder_time = OS_MemAlloc(g_host_cfg.ampdu_rx_buf_size * sizeof(unsigned long));
                ASSERT(tid_agg_rx->reorder_time);
                MEMSET(tid_agg_rx->reorder_time,0,(g_host_cfg.ampdu_rx_buf_size * sizeof(unsigned long)));
                OS_MutexInit(&tid_agg_rx->reorder_lock);
            }
        }
        LOG_PRINTF("AMPDU RX init done.\r\n");
    }

    ssv6xxx_channel_table_init();
    
END:
    if (res != SSV6XXX_SUCCESS)
    {
        if (gDeviceInfo->ap_list != NULL)
        {
            FREE((void *)gDeviceInfo->ap_list);
        }

        if (gDeviceInfo!=NULL)
        {
            FREE(gDeviceInfo);
        }
    }

    return res;
}


H_APIs ssv6xxx_result ssv6xxx_wifi_deinit(void)
{
    if(active_host_api == HOST_API_DEACTIVE)
    {
        ap_sta_mode_off();
    }
    return SSV6XXX_SUCCESS;
}


static s32 _ssv6xxx_wifi_send_cmd_directly(void *pCusData, int nCuslen, ssv6xxx_host_cmd_id eCmdID)
{
    struct cfg_host_cmd *host_cmd;
    u32 nSize = HOST_CMD_HDR_LEN+nCuslen;
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    u8* frame;

    frame = os_frame_alloc((u32)nCuslen,FALSE);

    if (frame == NULL) {
        return SSV6XXX_NO_MEM;
    }
    host_cmd = os_frame_push(frame, HOST_CMD_HDR_LEN);
    host_cmd->c_type = HOST_CMD;
    host_cmd->RSVD0 = 0;//(eCmdID>>8)&0x1F;
    host_cmd->h_cmd = (u8)eCmdID;
    host_cmd->len = nSize;
    sg_host_cmd_seq_no++;
    host_cmd->cmd_seq_no=sg_host_cmd_seq_no;
    MEMCPY(host_cmd->un.dat8, (u8*)pCusData, (u32)nCuslen);
//#ifndef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 0)
    if(!ssv6xxx_drv_tx_resource_enough(OS_FRAME_GET_DATA_LEN(frame)))
		ret=SSV6XXX_FAILED;
#endif
    if(ret==SSV6XXX_SUCCESS)
    {
        if(ssv6xxx_drv_send(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame)) <0){
            ret=SSV6XXX_FAILED;
        }
    }
    os_frame_free(frame);
    return ret;
}


s32 _ssv6xxx_wifi_ioctl_Ext(u32 cmd_id, void *data, u32 len, bool blocking,const bool mutexLock)
{
	ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }
        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if((ssv6xxx_result)FALSE == ret)
                break;
        }

        if(TRUE == blocking)
        {
            if(g_host_cfg.hci_aggr_tx)
            {
                ssv6xxx_wifi_set_hci_aggr(SSV_HCI_TX, 0);
                ret = _ssv6xxx_wifi_send_cmd_directly(data, len, cmd_id);
                ssv6xxx_wifi_set_hci_aggr(SSV_HCI_TX, 1);
            }
            else
            {
                ret = _ssv6xxx_wifi_send_cmd_directly(data, len, cmd_id);
            }
        }
        else
            ret = _ssv6xxx_wifi_send_cmd(data, len, cmd_id);


    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;

}


H_APIs s32 ssv6xxx_wifi_ioctl_Ext(u32 cmd_id, void *data, u32 len, bool blocking)
{
    return _ssv6xxx_wifi_ioctl_Ext(cmd_id,data,len,blocking,TRUE);
}
extern ssv6xxx_result TXRXTask_SetOpMode(ModeType mode);
extern ssv6xxx_result CmdEng_SetOpMode(ModeType mode);
extern void ssv6xxx_config_init_rates(struct ssv6xxx_host_ap_config *pConfig);
extern u16 g_acs_channel_mask;
extern u32 g_acs_5g_channel_mask;

s32 _ssv6xxx_set_rate_mask(u16 mask, bool FromAPI);
void  _ssv6xxx_wifi_set_rc_values(bool FromAPI);
s32 _ssv6xxx_set_ampdu_param(u8 opt, u8 value, u8 vif_idx, bool FromAPI);
u8 ts_bk=0;
u8 tr_bk=0;
ssv_vif* _get_ava_vif(void)
{
    if(gDeviceInfo->vif[0].hw_mode == SSV6XXX_HWM_INVALID)
        return &gDeviceInfo->vif[0];

    if(gDeviceInfo->vif[1].hw_mode == SSV6XXX_HWM_INVALID)
        return &gDeviceInfo->vif[1];

    return NULL;
}
ssv6xxx_result ap_mode_on(Ap_setting *ap_setting, s32 step)
{
#if (AP_MODE_ENABLE == 1)
    ssv_vif* vif=NULL;    
    s32 channel=0;

    LOG_PRINTF("ap_mode_on(%d),vif_idx=%d,used vif=%d\r\n",step,ap_setting->vif_idx,gDeviceInfo->used_vif);
    if(ap_setting->vif_idx == MAX_VIF_NUM)
    {
        vif=_get_ava_vif();
        if(!vif)
            return SSV6XXX_FAILED;
    }
    else
    {
        vif = &gDeviceInfo->vif[ap_setting->vif_idx];
    }
    if(!vif)
        return SSV6XXX_FAILED;

    if(step == 0)
    {
        reset_ap_info();
        gDeviceInfo->APInfo->vif = (void*)vif;
        vif->m_info.APInfo = gDeviceInfo->APInfo;
        vif->m_info.APInfo->current_st = AP_STATE_INIT;
        //Set default MAC addr
        ssv6xxx_memcpy(vif->self_mac,config_mac,6);
        ssv6xxx_get_cust_mac(vif->self_mac);
        if(vif->idx>0)
        {
            vif->self_mac[0] |= 0x02;            
            vif->self_mac[3] &= MAC_MASK;
            vif->self_mac[3] |=((vif->idx-1)<<20);
        }

        if(gDeviceInfo->used_vif == 0) //First interface be used
        {
            ssv6xxx_HW_disable();
            if(-1==ssv_hal_chip_init())
            {
                LOG_PRINTF("ssv_hal_chip_init fail\n\r");
                return SSV6XXX_FAILED;
            }
        
//#ifndef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 0)
            if(check_efuse_chip_id() != SSV6XXX_SUCCESS)
            {
                return SSV6XXX_FAILED;
            }
#endif  //#ifndef __SSV_UNIX_SIM__
        }

        //Set Host API on
        active_host_api = HOST_API_ACTIVE;
        OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
        //Set default MAC addr
        MEMCPY(gDeviceInfo->APInfo->own_addr,vif->self_mac,6);
        //Set global variable
        vif->hw_mode = SSV6XXX_HWM_AP;
        //SEC
        gDeviceInfo->APInfo->sec_type = ap_setting->security;
#ifndef CONFIG_NO_WPA2
    	//start: modify by angel.chiu for wpa


        MEMSET(&(gDeviceInfo->APInfo->ap_conf),0x0,sizeof(struct wpa_auth_config));
    	if(ap_setting->security == SSV6XXX_SEC_WPA2_PSK)
    	{

    		//struct wpa_authenticator *auth= (struct wpa_authenticator *)&(gDeviceInfo->APInfo->wpa_auth);

     		gDeviceInfo->APInfo->ap_conf.wpa = ap_setting->proto;
            gDeviceInfo->APInfo->ap_conf.eapol_version = 1;
    		gDeviceInfo->APInfo->ap_conf.wpa_key_mgmt =  ap_setting->key_mgmt;
    	    gDeviceInfo->APInfo->ap_conf.wpa_pairwise = ap_setting->pairwise_cipher;
    		gDeviceInfo->APInfo->ap_conf.wpa_group = ap_setting->group_cipher;
            if(ap_setting->proto == WPA_PROTO_RSN)
    			gDeviceInfo->APInfo->ap_conf.rsn_pairwise = ap_setting->pairwise_cipher;
    		else
    			gDeviceInfo->APInfo->ap_conf.rsn_pairwise = WPA_CIPHER_TKIP;
            MEMSET(&(gDeviceInfo->APInfo->ap_group),0x0,sizeof(struct wpa_group));
    	    gDeviceInfo->APInfo->ap_group.GTK_len = wpa_cipher_key_len(ap_setting->group_cipher);
    		gDeviceInfo->APInfo->ap_group.GN = 1;
            gDeviceInfo->APInfo->ap_group.first_sta_seen = FALSE;
        	wpa_auth_gen_wpa_ie(&(gDeviceInfo->APInfo->wpa_auth));


        }

    	//end:modify by angel.chiu for wpa
#endif
        //password
        MEMCPY((void*)gDeviceInfo->APInfo->password, (void*)ap_setting->password, strlen((void*)ap_setting->password));
        gDeviceInfo->APInfo->password[strlen((void*)ap_setting->password)]=0;

        //SSID
        MEMCPY((void*)gDeviceInfo->APInfo->config.ssid, (void*)ap_setting->ssid.ssid, ap_setting->ssid.ssid_len);
        gDeviceInfo->APInfo->config.ssid[ap_setting->ssid.ssid_len]=0;
        gDeviceInfo->APInfo->config.ssid_len = ap_setting->ssid.ssid_len;
        //set channel
        if(ap_setting->channel==EN_CHANNEL_AUTO_SELECT){
            gDeviceInfo->APInfo->nCurrentChannel    =   AP_DEFAULT_CHANNEL;
            gDeviceInfo->APInfo->config.nChannel    =   AP_DEFAULT_CHANNEL;
        }else{
            gDeviceInfo->APInfo->nCurrentChannel    =   ap_setting->channel;
            gDeviceInfo->APInfo->config.nChannel    =   gDeviceInfo->APInfo->nCurrentChannel;
        }

        if(gDeviceInfo->used_vif == 0) //First interface be used
        {
            //Load FW, init mac
            if(ssv6xxx_start(vif)!=SSV6XXX_SUCCESS)
            {
                LOG_PRINTF("ssv6xxx_start fail!!\r\n");

                OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
                return SSV6XXX_FAILED;
            }
            ssv_hal_add_interface(vif->idx,vif->hw_mode,vif->self_mac,gDeviceInfo->APInfo->config.nChannel);
            
            //send host_cmd
            CmdEng_SetOpMode(MT_RUNNING);
            TXRXTask_SetOpMode(MT_RUNNING);
            
            ssv6xxx_HW_enable();
        }        
        else
        {
            ssv_hal_add_interface(vif->idx,vif->hw_mode,vif->self_mac,0);
        }
        ssv_hal_update_cci_setting(MAX_CCI_SENSITIVE);
#ifndef CONFIG_NO_WPA2
        if(ap_setting->security == SSV6XXX_SEC_WPA2_PSK )
            wpa_init(&(gDeviceInfo->APInfo->wpa_auth));
#endif
        OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
        if(gDeviceInfo->used_vif == 0) //First interface be used
        {
            _ssv6xxx_wifi_set_rc_values(FALSE);
            //ampdu tx
            _ssv6xxx_set_ampdu_param(AMPDU_TX_OPT_ENABLE,g_host_cfg.ampdu_tx_enable,vif->idx,FALSE);
            //ampdu rx
            _ssv6xxx_set_ampdu_param(AMPDU_RX_OPT_ENABLE,g_host_cfg.ampdu_rx_enable,vif->idx,FALSE);
            //ampdu rx aggr max
            _ssv6xxx_set_ampdu_param(AMPDU_RX_OPT_BUF_SIZE,g_host_cfg.ampdu_rx_buf_size,vif->idx,FALSE);
            ssv6xxx_wifi_set_tx_pwr_mode(g_host_cfg.tx_power_mode);        
            ts_bk = g_host_cfg.tx_sleep;
            tr_bk = g_host_cfg.tx_retry_cnt;
        }
    }
    else if (step == 1)
    {

        if(ap_setting->channel==EN_CHANNEL_AUTO_SELECT){
            u16 acs_channel_mask=g_acs_channel_mask;
            u32 acs_channel_5g_mask=g_acs_5g_channel_mask;
            ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_AP, &acs_channel_mask, &acs_channel_5g_mask);
            gDeviceInfo->APInfo->acs_start = TRUE;
            channel=_ssv6xxx_wifi_auto_channel_selection(acs_channel_mask,acs_channel_5g_mask,ap_setting->vif_idx);
            if(-1!=channel){
                gDeviceInfo->APInfo->nCurrentChannel=channel;
                gDeviceInfo->APInfo->config.nChannel=gDeviceInfo->APInfo->nCurrentChannel;
                ssv6xxx_wifi_set_channel(gDeviceInfo->APInfo->nCurrentChannel,SSV6XXX_HWM_AP);
            }
            gDeviceInfo->APInfo->acs_start = FALSE;
        }
    }
    else if(step ==2)
    {
        //set ap config
        ssv6xxx_config_init_rates(&gDeviceInfo->APInfo->config);
        AP_Start();
        vif->m_info.APInfo->current_st = AP_STATE_READY;
        //ssv_hal_sta_rcv_all_bcn();
        gDeviceInfo->used_vif++;
    }
#endif    
    return SSV6XXX_SUCCESS;
}
extern void send_deauth_and_remove_all(void);

extern void timer_sta_reorder_release(void* data1, void* data2);
void ap_sta_mode_off(void)
{

    u8 i;

    for(i=0;i<MAX_VIF_NUM;i++)
    {
        switch(gDeviceInfo->vif[i].hw_mode)
        {
            case SSV6XXX_HWM_AP:
                ap_mode_off(i);
                break;
            case SSV6XXX_HWM_STA:
                sta_mode_off(i);
                break;
            case SSV6XXX_HWM_SCONFIG:
                sconfig_mode_off(i);
                break;
			default:
                sta_mode_off(i);
        }
    }
#if 0
    ssv_vif* vif=NULL;
    //flush reordering buffer
    if(g_host_cfg.ampdu_rx_enable)
    {
        ieee80211_delete_all_ampdu_rx();
        os_cancel_timer(timer_sta_reorder_release,(u32)NULL,(u32)NULL);
    }
#if (AP_MODE_ENABLE == 1)    
    //if(gDeviceInfo->hw_mode ==SSV6XXX_HWM_AP){
    if(gDeviceInfo->APInfo)&&(gDeviceInfo->APInfo->vif){
        send_deauth_and_remove_all();
        OS_MsDelay(10); //wait for send deauth frame
    }
#endif
#if (ENABLE_SMART_CONFIG== 1)    
    if(gDeviceInfo->StaInfo->vif){
        vif = (ssv_vif*)gDeviceInfo->StaInfo->vif;
        if(vif->hw_mode == SSV6XXX_HWM_SCONFIG){
            ssv_hal_sta_rx_data_flow();
            ssv_hal_drop_none_wsid_frame();
        }
    }
#endif    
    //send host cmd
    CmdEng_SetOpMode(MT_STOP);
    TXRXTask_SetOpMode(MT_STOP);


    OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
    //Set global variable
    vif = (ssv_vif*)gDeviceInfo->StaInfo->vif;
    ssv6006_hal_remove_interface(vif->idx);
    vif->hw_mode = SSV6XXX_HWM_INVALID;
    
    vif = (ssv_vif*)gDeviceInfo->APInfo->vif;
    ssv6006_hal_remove_interface(vif->idx);
    vif->hw_mode = SSV6XXX_HWM_INVALID;
    
    OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);


    //clean AP mode global variable
#if (AP_MODE_ENABLE == 1)    
    AP_Stop();
#endif
//--------------------------------
    //clean sta mode global variable
    #if(ENABLE_DYNAMIC_RX_SENSITIVE==0)
    gDeviceInfo->StaInfo->status= DISCONNECT;
    MEMSET(gDeviceInfo->StaInfo->joincfg, 0, sizeof(struct cfg_join_request));
    MEMSET(gDeviceInfo->StaInfo->joincfg_backup, 0, sizeof(struct cfg_join_request));
    #else
    ssv6xxx_sta_mode_disconnect();
    #endif
    //mlme station mode deinit
    mlme_sta_mode_deinit();

//---------------------------------

    //HCI stop
    ssv6xxx_drv_stop();

    //Disable HW
    ssv6xxx_HW_disable();


    //Set API off
    active_host_api = HOST_API_DEACTIVE;
#endif    
}

void sconfig_mode_off(u8 vif_idx)
{
#if (ENABLE_SMART_CONFIG == 1)
    ssv_vif* vif=NULL;    

    ASSERT(vif_idx<MAX_VIF_NUM);
    
    LOG_PRINTF("sconfig_mode_off,vif_idx=%d\r\n",vif_idx);
    vif = &gDeviceInfo->vif[vif_idx];


    if(vif->hw_mode == SSV6XXX_HWM_SCONFIG)
    {
        ssv_hal_sta_rx_data_flow();
        ssv_hal_drop_none_wsid_frame();
}

    if(0 == (gDeviceInfo->used_vif-1))
    {
        //send host cmd
        CmdEng_SetOpMode(MT_STOP);
        TXRXTask_SetOpMode(MT_STOP);
    }
    ssv_hal_remove_interface(vif->idx);
    vif->hw_mode = SSV6XXX_HWM_INVALID;

    if(0 == (gDeviceInfo->used_vif-1))
    {
        //HCI stop
        ssv6xxx_drv_stop();
        //Disable HW
        ssv6xxx_HW_disable();
        //Set API off
        active_host_api = HOST_API_DEACTIVE;
    }
    if(gDeviceInfo->used_vif>0)
        gDeviceInfo->used_vif--;
#endif    
}

void ap_mode_off(u8 vif_idx)
{
#if (AP_MODE_ENABLE == 1)
    ssv_vif* vif=NULL;    

    ASSERT(vif_idx<MAX_VIF_NUM);
    
    LOG_PRINTF("ap_mode_off,vif_idx=%d\r\n",vif_idx);
    vif = &gDeviceInfo->vif[vif_idx];

    vif->m_info.APInfo->current_st = AP_STATE_IDLE;
    if(g_host_cfg.ampdu_rx_enable)
    {
        if(gDeviceInfo->ampduRx_vif == vif_idx)
        {
            ieee80211_delete_all_ampdu_rx();
            os_cancel_timer(timer_sta_reorder_release,(u32)NULL,(u32)NULL);
        }
    }
    if(vif->hw_mode == SSV6XXX_HWM_AP){
        send_deauth_and_remove_all();
        OS_MsDelay(10); //wait for send deauth frame
    }

    if(0 == (gDeviceInfo->used_vif-1))
    {
    //send host cmd
    CmdEng_SetOpMode(MT_STOP);
    TXRXTask_SetOpMode(MT_STOP);
    }
    //clean AP mode global variable
    AP_Stop();

    if(0 == (gDeviceInfo->used_vif-1))
    {
        //HCI stop
        ssv6xxx_drv_stop();

        //Disable HW
        ssv6xxx_HW_disable();


        //Set API off
        active_host_api = HOST_API_DEACTIVE;
    }
    
    if(gDeviceInfo->used_vif>0)
        gDeviceInfo->used_vif--;

    vif->m_info.APInfo = NULL;
    vif->hw_mode = SSV6XXX_HWM_INVALID;
#endif    
}

ssv6xxx_result _ssv6xxx_wifi_ap(Ap_setting *ap_setting,s32 step,const bool mutexLock)
{
#if (AP_MODE_ENABLE == 1)    
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {

        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if(FALSE == ret)
                break;
        }

        if(ap_setting->status)//AP on
        {
            if( HOST_API_ACTIVE ==active_host_api&&step==0)
                if(gDeviceInfo->vif[ap_setting->vif_idx].hw_mode == SSV6XXX_HWM_AP)
                    ap_mode_off(ap_setting->vif_idx);

//#ifdef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 1)

            ret = ap_mode_on(ap_setting,step);

#else //Normal case
            {
                u8 retry_ini=INI_CNT;
                ssv6xxx_wifi_update_available_channel();
                while((SSV6XXX_SUCCESS != (ret=ap_mode_on(ap_setting,step))) &&
                   (retry_ini > 0) && (step==0))
                {
                    retry_ini--;
                    platform_ldo_en(0);
                    OS_MsDelay(10);
                    platform_ldo_en(1);
                    LOG_PRINTF("R\r\n");
                }
            }
#endif
            //ret = ap_mode_on(ap_setting,step);

        }
        else // AP off
        {
            if((step==0)&&(gDeviceInfo->vif[ap_setting->vif_idx].hw_mode == SSV6XXX_HWM_AP))
            {
                ap_mode_off(ap_setting->vif_idx);
            }
        }

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;
#else
    return SSV6XXX_SUCCESS;
#endif

}

/**********************************************************
Function : ssv6xxx_wifi_ap
Description: Setting ap on or off
1. AP on : The current status must be mode off (ap mode or station mode off)
2. AP off : The current status must be AP on
*********************************************************/
H_APIs ssv6xxx_result ssv6xxx_wifi_ap(Ap_setting *ap_setting,s32 step)
{
#if (AP_MODE_ENABLE == 1)    
    return _ssv6xxx_wifi_ap(ap_setting,step,TRUE);
#else
    return SSV6XXX_SUCCESS;
#endif
}

ssv6xxx_result bus_mode_on(u8 vif_idx)
{
    ssv_vif* vif=NULL;
    vif = &gDeviceInfo->vif[vif_idx];
    //ssv6xxx_HW_disable();

    if(-1==ssv_hal_chip_init())
    {
        LOG_PRINTF("ssv6xxx_chip_init fail\n\r");
        return SSV6XXX_FAILED;
    }

    //Set default MAC addr
    ssv6xxx_memcpy(vif->self_mac,config_mac,6);
    ssv6xxx_get_cust_mac(vif->self_mac);

//#ifndef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 0)
    if(check_efuse_chip_id() != SSV6XXX_SUCCESS)
    {
        return SSV6XXX_FAILED;
    }
#endif  //#ifndef __SSV_UNIX_SIM__

    //Set Host API on
    active_host_api = HOST_API_ACTIVE;

    //Set global variable
    vif->hw_mode = SSV6XXX_HWM_STA;

	return SSV6XXX_SUCCESS;
}

ssv6xxx_result sta_mode_on(ssv6xxx_hw_mode hw_mode, u8 vif_idx)
{
    ssv_vif *vif=NULL;    

    LOG_PRINTF("sta_mode_on, vif_idx=%d,usd_vif=%d\r\n",vif_idx,gDeviceInfo->used_vif);
    if(vif_idx == MAX_VIF_NUM)
    {
        vif=_get_ava_vif();
        if(!vif)
            return SSV6XXX_FAILED;
    }
    else
    {
        vif = &gDeviceInfo->vif[vif_idx];
    }

    if(!vif->m_info.StaInfo)
    {
        LOG_PRINTF("_alloc_sta_info \n\r");
        vif->m_info.StaInfo = _alloc_sta_info();
    }
    
    if(!vif->m_info.StaInfo)
        return SSV6XXX_FAILED;
    //gDeviceInfo->StaInfo->vif = (void*)vif;


    if(gDeviceInfo->used_vif==0)
    {
        ssv6xxx_HW_disable();
        if(-1==ssv_hal_chip_init())
        {
            LOG_PRINTF("ssv6xxx_chip_init fail\n\r");
            return SSV6XXX_FAILED;
        }
    }

    //Set default MAC addr
    ssv6xxx_memcpy(vif->self_mac,config_mac,6);
    ssv6xxx_get_cust_mac(vif->self_mac);
    if(vif->idx>0)
    {
        vif->self_mac[0] |= 0x02;            
        vif->self_mac[3] &= MAC_MASK;
        vif->self_mac[3] |=((vif->idx-1)<<20);
    }

//#ifndef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 0)
    if(check_efuse_chip_id() != SSV6XXX_SUCCESS)
    {
        return SSV6XXX_FAILED;
    }
#endif  //#ifndef __SSV_UNIX_SIM__

    //Set Host API on
    active_host_api = HOST_API_ACTIVE;

    OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);
    //Set global variable
    vif->hw_mode = hw_mode;

    if(gDeviceInfo->used_vif==0)
    {
        //Load FW, init mac
        if(ssv6xxx_start(vif)!=SSV6XXX_SUCCESS)
        {
            LOG_PRINTF("ssv6xxx_start fail!!\r\n");

            OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);
            return SSV6XXX_FAILED;
        }
    }
    OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);


    if(gDeviceInfo->used_vif==0)
    {
        ssv_hal_add_interface(vif->idx,SSV6XXX_HWM_STA,vif->self_mac,STA_DEFAULT_CHANNEL);
        //Host cmd
        CmdEng_SetOpMode(MT_RUNNING);
        TXRXTask_SetOpMode(MT_RUNNING);

        //enable RF
        ssv6xxx_HW_enable();
    }
    else
    {
        ssv_hal_add_interface(vif->idx,SSV6XXX_HWM_STA,vif->self_mac,0);
    }

    if(vif->hw_mode ==SSV6XXX_HWM_SCONFIG){
        ssv_hal_sconfig_rx_data_flow();
        ssv_hal_accept_none_wsid_frame();
        ssv_hal_reduce_phy_cca_bits();        
    }
    else
    {
        ssv_hal_update_cci_setting(MAX_CCI_SENSITIVE);    
    }
    if(gDeviceInfo->used_vif==0)
    {
        _ssv6xxx_wifi_set_rc_values(FALSE);
        //ampdu tx
        _ssv6xxx_set_ampdu_param(AMPDU_TX_OPT_ENABLE,g_host_cfg.ampdu_tx_enable,vif->idx,FALSE);
        //ampdu rx
        _ssv6xxx_set_ampdu_param(AMPDU_RX_OPT_ENABLE,g_host_cfg.ampdu_rx_enable,vif->idx,FALSE);
        //ampdu rx aggr max
        _ssv6xxx_set_ampdu_param(AMPDU_RX_OPT_BUF_SIZE,g_host_cfg.ampdu_rx_buf_size,vif->idx,FALSE);
        ssv6xxx_wifi_set_tx_pwr_mode(g_host_cfg.tx_power_mode);

        ssv_hal_sta_reject_bcn();
    }

    gDeviceInfo->used_vif++;
    return SSV6XXX_SUCCESS;
}

void sta_mode_off(u8 vif_idx)
{
    ssv_vif* vif=NULL;    

    ASSERT(vif_idx<MAX_VIF_NUM);
    
    LOG_PRINTF("sta_mode_off,vif_idx=%d\r\n",vif_idx);
    vif = &gDeviceInfo->vif[vif_idx];

    if(vif->hw_mode ==SSV6XXX_HWM_SCONFIG){
        ssv_hal_sta_rx_data_flow();
        ssv_hal_drop_none_wsid_frame();
    }
    else
    {
        if(g_host_cfg.ampdu_rx_enable)
        {
            if(gDeviceInfo->ampduRx_vif == vif_idx)
            {
                ieee80211_delete_all_ampdu_rx();
                os_cancel_timer(timer_sta_reorder_release,(u32)NULL,(u32)NULL);
            }
        }
    }

    //send host cmd
    if(0 == (gDeviceInfo->used_vif-1))
    {
        CmdEng_SetOpMode(MT_STOP);
        TXRXTask_SetOpMode(MT_STOP);
        LOG_PRINTF("No vif working. STOP CmdEng & TRX TASK\r\n");
    }

    //clean sta mode global variable
    #if(ENABLE_DYNAMIC_RX_SENSITIVE==0)
    vif->m_info.StaInfo->status= DISCONNECT;
    MEMSET(vif->m_info.StaInfo->joincfg, 0, sizeof(struct cfg_join_request));
    MEMSET(vif->m_info.StaInfo->joincfg_backup, 0, sizeof(struct cfg_join_request));
    #else
    ssv6xxx_sta_mode_disconnect((void*)vif->m_info.StaInfo);
    #endif


    //mlme station mode deinit
    mlme_sta_mode_deinit();

    ssv_hal_remove_interface(vif->idx);
    vif->hw_mode = SSV6XXX_HWM_INVALID;

    if(0 == (gDeviceInfo->used_vif-1))
    {
        //HCI stop
        ssv6xxx_drv_stop();
        //Disable HW
        ssv6xxx_HW_disable();
        //Set API off
        active_host_api = HOST_API_DEACTIVE;
    }

    if(gDeviceInfo->used_vif>0)
        gDeviceInfo->used_vif--;

    if(vif->m_info.StaInfo)
    {
        OS_MemFree(vif->m_info.StaInfo);
        vif->m_info.StaInfo = NULL;
    }

}
ssv6xxx_result _ssv6xxx_wifi_station(u8 hw_mode,Sta_setting *sta_station,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    //Detect host API status
    if((SSV6XXX_HWM_STA != hw_mode)&&(SSV6XXX_HWM_SCONFIG != hw_mode)){
        return SSV6XXX_FAILED;
    }

    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {

        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(&g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(&g_host_api_mutex);

            if((ssv6xxx_result)FALSE == ret)
                break;
        }

        if(sta_station->status) //Station on
        {
            if((HOST_API_ACTIVE==active_host_api)&&
               (gDeviceInfo->vif[sta_station->vif_idx].hw_mode != SSV6XXX_HWM_INVALID))
            {
                //if(gDeviceInfo->vif[sta_station->vif_idx].hw_mode)
                if(gDeviceInfo->vif[sta_station->vif_idx].hw_mode == SSV6XXX_HWM_STA)
                    sta_mode_off(sta_station->vif_idx);
                else if(gDeviceInfo->vif[sta_station->vif_idx].hw_mode == SSV6XXX_HWM_SCONFIG)
                    sconfig_mode_off(sta_station->vif_idx);
            }

//#ifdef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 1)
            ret = sta_mode_on(hw_mode,sta_station->vif_idx);

#else //Normal case
            {
                u8 retry_ini=INI_CNT;
                ssv6xxx_wifi_update_available_channel();
                while((SSV6XXX_SUCCESS != (ret=sta_mode_on(hw_mode,sta_station->vif_idx))) &&
                   (retry_ini > 0))
                {
                    retry_ini--;
                    platform_ldo_en(0);
                    OS_MsDelay(10);
                    platform_ldo_en(1);
                    LOG_PRINTF("R\r\n");
                }
            }
#endif

        }
        else //station off
        {
            if(hw_mode == SSV6XXX_HWM_STA)
                sta_mode_off(sta_station->vif_idx);

            if(hw_mode == SSV6XXX_HWM_SCONFIG)
                sconfig_mode_off(sta_station->vif_idx);

        }
    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;

}

/**********************************************************
Function : ssv6xxx_wifi_station
Description: Setting station on or off
1. station on : The current status must be mode off (ap mode or station mode off)
2. station off : The current status must be Station on
*********************************************************/
H_APIs ssv6xxx_result ssv6xxx_wifi_station(u8 hw_mode,Sta_setting *sta_station)
{
    return _ssv6xxx_wifi_station(hw_mode,sta_station,TRUE);

}

void ap_mode_status(struct ap_sta_status_ap *ap_info)
{
#if (AP_MODE_ENABLE == 1)
    u32 idx=0;
    int sta = 0;
    ssv_vif* vif = (ssv_vif*)gDeviceInfo->APInfo->vif;
    //mac addr
    MEMCPY(ap_info->selfmac, vif->self_mac,ETH_ALEN);
    //ssid
    MEMCPY((void*)ap_info->ssid.ssid,(void*)gDeviceInfo->APInfo->config.ssid,gDeviceInfo->APInfo->config.ssid_len);
    //status_info->u.ap.ssid.ssid[gDeviceInfo->APInfo->config.ssid_len]=0;
    ap_info->ssid.ssid_len = gDeviceInfo->APInfo->config.ssid_len;
    //station number
    ap_info->stanum = gDeviceInfo->APInfo->num_sta;
    //channel
    ap_info->channel = gDeviceInfo->APInfo->nCurrentChannel;
    for(idx=0; idx<gMaxAID; idx++)
    {
        APStaInfo_st *StaInfo= &gDeviceInfo->APInfo->StaConInfo[idx];
		if (!test_sta_flag(StaInfo, WLAN_STA_VALID))
        {
        	continue;
		}
        //station info
        MEMCPY(ap_info->stainfo[sta].Mac,gDeviceInfo->APInfo->StaConInfo[sta].addr,6);
        if(test_sta_flag(StaInfo, WLAN_STA_AUTH))
            ap_info->stainfo[sta].status= AUTH;
        if(test_sta_flag(StaInfo, WLAN_STA_ASSOC))
            ap_info->stainfo[sta].status= ASSOC;
        if(test_sta_flag(StaInfo, WLAN_STA_AUTHORIZED))
            ap_info->stainfo[sta].status= CONNECT;

        sta++;
    }

    switch(gDeviceInfo->APInfo->sec_type)
    {
        case SSV6XXX_SEC_NONE:
			ap_info->key_mgmt = WPA_KEY_MGMT_NONE;
            ap_info->pairwise_cipher = WPA_CIPHER_NONE;
            ap_info->group_cipher = WPA_CIPHER_NONE;
            break;
    	case SSV6XXX_SEC_WEP_40:
			ap_info->key_mgmt = WPA_KEY_MGMT_NONE;
            ap_info->pairwise_cipher = WPA_CIPHER_WEP40;
            ap_info->group_cipher = WPA_CIPHER_WEP40;
            break;
    	case SSV6XXX_SEC_WEP_104:
			ap_info->key_mgmt = WPA_KEY_MGMT_NONE;
            ap_info->pairwise_cipher = WPA_CIPHER_WEP104;
            ap_info->group_cipher = WPA_CIPHER_WEP104;
            break;
    	case SSV6XXX_SEC_WPA_PSK:
            ap_info->proto = WPA_PROTO_WPA;
			ap_info->key_mgmt = WPA_KEY_MGMT_PSK;
            ap_info->pairwise_cipher = WPA_CIPHER_TKIP;
            ap_info->group_cipher = WPA_CIPHER_TKIP;
            break;
    	case SSV6XXX_SEC_WPA2_PSK:
            ap_info->proto = WPA_PROTO_RSN;
			ap_info->key_mgmt = WPA_KEY_MGMT_PSK;
            ap_info->pairwise_cipher = WPA_CIPHER_CCMP;
            ap_info->group_cipher = WPA_CIPHER_CCMP;
            break;
        default:
            break;
    }
    ap_info->current_st = gDeviceInfo->APInfo->current_st;
#endif
}
void sta_mode_status(struct ap_sta_status_station *status_info,u8 vif_idx)
{
    ssv_vif* vif = &gDeviceInfo->vif[vif_idx];
    //mac addr
    MEMCPY(status_info->selfmac, vif->self_mac,ETH_ALEN);
    status_info->apinfo.status = vif->m_info.StaInfo->status;
    if((vif->m_info.StaInfo->status != DISCONNECT)&&(gDeviceInfo->recovering != TRUE))
    {
        //ssid
        MEMCPY((void*)status_info->ssid.ssid,(void*)vif->m_info.StaInfo->joincfg->bss.ssid.ssid,vif->m_info.StaInfo->joincfg->bss.ssid.ssid_len);
        //status_info->u.station.ssid.ssid[gDeviceInfo->joincfg->bss.ssid.ssid_len]=0;
        status_info->ssid.ssid_len = vif->m_info.StaInfo->joincfg->bss.ssid.ssid_len;
        //channel
        status_info->channel = vif->m_info.StaInfo->joincfg->bss.channel_id ;
        //AP info mac
        MEMCPY(status_info->apinfo.Mac,(void*)vif->m_info.StaInfo->joincfg->bss.bssid.addr,6);
        status_info->apinfo.status = vif->m_info.StaInfo->status;
        status_info->capab_info = vif->m_info.StaInfo->joincfg->bss.capab_info;
        switch(vif->m_info.StaInfo->joincfg->sec_type)
        {
            case SSV6XXX_SEC_NONE:
    			status_info->key_mgmt = WPA_KEY_MGMT_NONE;

                break;
        	case SSV6XXX_SEC_WEP_40:
                status_info->proto = 0;
    			status_info->key_mgmt = WPA_KEY_MGMT_NONE;

                break;
        	case SSV6XXX_SEC_WEP_104:
                status_info->proto = 0;
    			status_info->key_mgmt = WPA_KEY_MGMT_NONE;

                break;
        	case SSV6XXX_SEC_WPA_PSK:
                status_info->proto = WPA_PROTO_WPA;
    			status_info->key_mgmt = WPA_KEY_MGMT_PSK;

                break;
        	case SSV6XXX_SEC_WPA2_PSK:
                status_info->proto = WPA_PROTO_RSN;
    			status_info->key_mgmt = WPA_KEY_MGMT_PSK;

                break;
            default:
                break;
        }

        status_info->pairwise_cipher=vif->m_info.StaInfo->joincfg->bss.pairwise_cipher[0];
        status_info->group_cipher = vif->m_info.StaInfo->joincfg->bss.group_cipher;
    }
}
ssv6xxx_result _ssv6xxx_wifi_status(Ap_sta_status *status_info,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        int i;
        status_info->status= active_host_api;
        OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);
        for(i=0;i<MAX_VIF_NUM;i++)
        {
            status_info->vif_operate[i] = gDeviceInfo->vif[i].hw_mode;
#if (AP_MODE_ENABLE == 1)
            if(SSV6XXX_HWM_AP == status_info->vif_operate[i]) // AP  mode
            {
                ap_mode_status((struct ap_sta_status_ap*)(&status_info->vif_u[i]));
            }
            else
#endif    
            if ((SSV6XXX_HWM_STA == status_info->vif_operate[i])||(SSV6XXX_HWM_SCONFIG == status_info->vif_operate[i]))//station mode  & sconfig mode
            {
                sta_mode_status((struct ap_sta_status_station*)(&status_info->vif_u[i]),i);
            }
        }
        OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);
    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return ret;

}

/**********************************************************
Function : ssv6xxx_wifi_status
Description: Return status information about AP or station
*********************************************************/
H_APIs ssv6xxx_result ssv6xxx_wifi_status(Ap_sta_status *status_info)
{
    return _ssv6xxx_wifi_status(status_info,TRUE);
}

H_APIs struct ssv6xxx_ieee80211_bss *ssv6xxx_wifi_find_ap_ssid(struct cfg_80211_ssid *ssid)
{

    int i = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

	OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);

	for (i = 0; i < g_max_num_of_ap_list; i++)
    {
        if(gdeviceInfo->ap_list[i].channel_id!= 0)
		{
            //if (strcmp((const char *) gdeviceInfo->ap_list[i].ssid.ssid, ssid) == 0)
            if (gdeviceInfo->ap_list[i].ssid.ssid_len == ssid->ssid_len
                && MEMCMP((const void *) gdeviceInfo->ap_list[i].ssid.ssid, (const void *)ssid->ssid, ssid->ssid_len) == 0)
            {
                break;
            }
		}
	}

	OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);

    if (i == g_max_num_of_ap_list)
    {
        return NULL;
    }

    return  &(gdeviceInfo->ap_list[i]);
}


H_APIs int ssv6xxx_aplist_get_count(void)
{
    int i = 0;
    int count = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

    if (!gdeviceInfo)
    {
        return 0;
    }

    for (i=0; i<g_max_num_of_ap_list; i++)
    {
        if (gdeviceInfo->ap_list[i].channel_id!= 0)
        {
            count++;
        }
    }

    return (count%SSV6XXX_APLIST_PAGE_COUNT) ? 
		(count/SSV6XXX_APLIST_PAGE_COUNT + 1) : (count/SSV6XXX_APLIST_PAGE_COUNT);
}

H_APIs s32 ssv6xxx_aplist_get_count_by_channel(u8 channel)
{
    int i = 0;
    int count = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

    if (!gdeviceInfo)
    {
        return 0;
    }

    for (i=0; i<g_max_num_of_ap_list; i++)
    {
        if (gdeviceInfo->ap_list[i].channel_id== channel)
        {
            count++;
        }
    }

    return count;
}

H_APIs void ssv6xxx_wifi_set_channel(u8 num, ssv6xxx_hw_mode h_mode)
{
    s32 i=0;
    struct freq_params freq;
    MEMSET((void *)&freq,0,sizeof(struct freq_params));
    freq.current_channel=(u8)num;

    if(SSV6XXX_HWM_STA==h_mode)
    {
        //STA mode always set HT20 on host, and it will change the channel type by AP's beacon and assoc response in fw
        freq.sec_channel_offset=NL80211_CHAN_HT20;    
        
    }
    else if(SSV6XXX_HWM_AP==h_mode)
    {
        if(g_host_cfg.ap_ht20_only==TRUE)
        {
            freq.sec_channel_offset=NL80211_CHAN_HT20;           
        }
        else
        {
            freq.sec_channel_offset=NL80211_CHAN_HT20; 
            if(IS_5G_BAND(num))
            {   
                if(ssv6xxx_wifi_is_40MHZ_legal(num))
                {
                                  
                    i=ssv6xxx_wifi_ch_to_bitmask(num);
                    if(-1!=i)
                    {
                        freq.sec_channel_offset=ssv6xxx_5ghz_chantable[i].flags;
                    }
                }                
            }        
        }
    }
    else if(SSV6XXX_HWM_SCONFIG==h_mode)
    {
       freq.sec_channel_offset=NL80211_CHAN_HT20; 
        if(IS_5G_BAND(num))
        {   
            if(ssv6xxx_wifi_is_40MHZ_legal(num))
            {
                i=ssv6xxx_wifi_ch_to_bitmask(num);
                if(-1!=i)
                {
                    freq.sec_channel_offset=ssv6xxx_5ghz_chantable[i].flags;
                }
            }                
        }         
    }
    else
    {
        freq.sec_channel_offset=NL80211_CHAN_HT20;    
    }
    

    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_CAL, (void *)(&freq), sizeof(struct freq_params), TRUE, FALSE);
}

H_APIs void ssv6xxx_aplist_get_str(char *wifi_list_str, int page)
{
    u32 i = 0;
    u8  temp[128];
    u32 count = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;
    u8 page_num = 1;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};

    if (!wifi_list_str)
    {
	    LOG_PRINTF("wifi_list_str null !!\r\n");
        return;
    }

    if (!gdeviceInfo)
    {
	    LOG_PRINTF("gdeviceInfo null !!\r\n");
        return;
    }

	OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);
    wifi_list_str[0]=0;

    for (i=0; i<g_max_num_of_ap_list; i++)
    {
        if(gdeviceInfo->ap_list[i].channel_id!= 0)
        {
            if (page_num == page)
            {
            
                //sprintf((void *)temp, "BSSID#%02x:%02x:%02x:%02x:%02x:%02x;",
                SPRINTF((void *)temp, "BSSID#%02x:%02x:%02x:%02x:%02x:%02x;",
					                gdeviceInfo->ap_list[i].bssid.addr[0],  
					                gdeviceInfo->ap_list[i].bssid.addr[1], 
					                gdeviceInfo->ap_list[i].bssid.addr[2],  
					                gdeviceInfo->ap_list[i].bssid.addr[3], 
					                gdeviceInfo->ap_list[i].bssid.addr[4],  
					                gdeviceInfo->ap_list[i].bssid.addr[5]);
                STRCAT(wifi_list_str, (void *)temp);
                MEMSET((void*)ssid_buf,0,sizeof(ssid_buf));
                MEMCPY((void*)ssid_buf,(void*)gdeviceInfo->ap_list[i].ssid.ssid,gdeviceInfo->ap_list[i].ssid.ssid_len);
                SPRINTF((void *)temp, "SSID#%s;", ssid_buf);
                STRCAT(wifi_list_str, (void *)temp);
                if(gdeviceInfo->ap_list[i].capab_info&BIT(4))
                {
                    SPRINTF((void *)temp, "proto#%s;",
                       gdeviceInfo->ap_list[i].proto&WPA_PROTO_WPA?"WPA":
                        gdeviceInfo->ap_list[i].proto&WPA_PROTO_RSN?"WPA2":"WEP");
                }
                else
                {
                    SPRINTF((void *)temp, "proto#%s;", "OPEN");
                }
                STRCAT(wifi_list_str, (void *)temp);
                SPRINTF((void *)temp, "channel#%d;\r\n",
                    gdeviceInfo->ap_list[i].channel_id);
                STRCAT(wifi_list_str, (void *)temp);
            }

            count++;
            if (count >= SSV6XXX_APLIST_PAGE_COUNT)
            {
                count = 0;
                page_num++;
            }
        }
    }

	OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);
}

s32 _ssv6xxx_set_ampdu_param(u8 opt, u8 value, u8 vif_idx, bool FromAPI)
{
    u8 param[4];

    param[0] = opt;
    param[1] = value;
    param[2] = vif_idx;

    if ((opt == 0) && (gDeviceInfo->vif[vif_idx].hw_mode == SSV6XXX_HWM_AP))
        ssv_hal_setup_ampdu_wmm((value == 1)?TRUE:FALSE);

    switch(opt)
    {
        case AMPDU_TX_OPT_ENABLE:
            g_host_cfg.ampdu_tx_enable = value;            
            break;
        case AMPDU_RX_OPT_ENABLE:
            g_host_cfg.ampdu_rx_enable = value;
            break;
        case AMPDU_RX_OPT_BUF_SIZE:
            g_host_cfg.ampdu_rx_buf_size = value;
            break;
        default:
            LOG_PRINTF("ampdu opt=%d\r\n",opt);
    }

    return _ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_SET_AMPDU_PARAM, &param, sizeof(param), FromAPI);
}


H_APIs s32 ssv6xxx_set_ampdu_param(u8 opt, u8 vif_idx,u8 value)
{
    return _ssv6xxx_set_ampdu_param(opt, value, vif_idx, TRUE);
}

static inline struct ssv6xxx_ieee80211_bss * sta_mode_find_ap_by_mac(ETHER_ADDR *mac)
{
    int i = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

	OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);

	for (i = 0; i < g_max_num_of_ap_list; i++)
    {
        if(gdeviceInfo->ap_list[i].channel_id!= 0)
		{
            //if (MEMCMP((const void *) gdeviceInfo->ap_list[i].ssid.ssid, (const void *)mac, sizeof(ETHER_ADDR)) == 0)
            if(MEMCMP((void*)(gdeviceInfo->ap_list[i].bssid.addr), (void*)(mac), ETHER_ADDR_LEN) == 0)
            {
                break;
            }
		}
	}

	OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);

    if (i == g_max_num_of_ap_list)
    {
        return NULL;
    }

    return  (void*)&(gdeviceInfo->ap_list[i]);
}

H_APIs s32 ssv6xxx_get_rssi_by_mac(u8 *macaddr, u8 vif_idx)
{
    s32 ret_rssi;
    if(gDeviceInfo->vif[vif_idx].hw_mode == SSV6XXX_HWM_AP)
    {
#if (AP_MODE_ENABLE == 1)        
        extern APStaInfo_st *APStaInfo_FindStaByAddr( ETHER_ADDR *mac );
        APStaInfo_st * sta = APStaInfo_FindStaByAddr( (ETHER_ADDR *)macaddr );

        if(sta)
        {
            OS_MutexLock(sta->apsta_mutex);
            ret_rssi=sta->rcpi;
            OS_MutexUnLock(sta->apsta_mutex);
            return ret_rssi;
        }
#endif        
    }
    else if(gDeviceInfo->vif[vif_idx].hw_mode == SSV6XXX_HWM_STA)
    {
        struct ssv6xxx_ieee80211_bss *ap = sta_mode_find_ap_by_mac((ETHER_ADDR *)macaddr);

        if(ap)
        {
            ret_rssi=ssv_hal_get_rssi_from_reg(vif_idx);
            return ret_rssi;
        }
    }
    return -1;
}

H_APIs ssv6xxx_result ssv6xxx_get_sta_info_by_aid(struct apmode_sta_info *sta_info,u8 aid_num)
{

#if (AP_MODE_ENABLE == 1)    
    if(aid_num >= WLAN_MAX_STA)
    {
        LOG_PRINTF("aid number is invalid \r\n");
        return SSV6XXX_FAILED;
    }
    else if(!sta_info)
    {
        LOG_PRINTF("sta_info is null \r\n");
        return SSV6XXX_FAILED;
    }
    else
    {
        APStaInfo_st *apStaInfo = NULL;
        OS_MutexLock(gDeviceInfo->g_dev_info_mutex);

        apStaInfo= &gDeviceInfo->APInfo->StaConInfo[aid_num];
        if(!test_sta_flag(apStaInfo, WLAN_STA_VALID))
        {
            LOG_PRINTF("apStaInfo is invalid\r\n");
            OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
            return SSV6XXX_FAILED;
        }

        sta_info->rcpi = apStaInfo->rcpi;
        sta_info->prev_rcpi = apStaInfo->prev_rcpi;
        sta_info->arp_retry_count = apStaInfo->arp_retry_count;
        sta_info->aid = apStaInfo->aid;

        MEMCPY(sta_info->addr,apStaInfo->addr,ETH_ALEN);

        OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
        return SSV6XXX_SUCCESS;
    }
#else
    return SSV6XXX_SUCCESS;
#endif
}


H_APIs u32 ssv6xxx_get_aplist_info(void **ap_list)
{
    void *tmp=NULL;

    tmp = (void*)MALLOC(sizeof(struct ssv6xxx_ieee80211_bss)*g_max_num_of_ap_list);
    if(!tmp)
    {
        LOG_PRINTF("malloc ap_list fail \r\n");
        return 0;
    }
    MEMCPY((void *)tmp,(void *)gDeviceInfo->ap_list, 
		sizeof(struct ssv6xxx_ieee80211_bss) * g_max_num_of_ap_list);
    *ap_list = tmp;
    return g_max_num_of_ap_list;
}




static inline int _ssv6xxx_start_scan(void)
{
    if (NULL == scanning_sphr)
    if (OS_SUCCESS == OS_SemInit(&scanning_sphr, 1, 0))
        return 0;
    else
        return -1;
#else
    if (NULL == scanning_sphr)
    {
        if (OS_SUCCESS == OS_SemInit(&scanning_sphr, 1, 0))
            return 0;
        else
            return -1;
    }
    else
    {
        return 0;
    }
}


static inline bool _ssv6xxx_wait_scan_done(u32 timeOut)
{
    bool ret = FALSE;
    if (OS_SemWait(scanning_sphr, timeOut) == true)
	{
        ret = TRUE;
    } 
	else
	{
        ret = FALSE;
    }
    OS_SemDelete(&scanning_sphr);
	scanning_sphr = NULL;
#endif
    return ret;

}

#if (AP_MODE_ENABLE == 1)    
extern u16 g_acs_channel_scanning_interval;
extern u8  g_acs_channel_scanning_loop;
static s32 _ssv6xxx_wifi_auto_channel_selection(u16 channel_mask,u32 channel_5g_mask,u8 vif_idx)
{
#define INDICATOR_OF_CHANNEL_CHOISE 2 //1: choose by edcca counter, 2: choose by packet counter   3: choose by AP number

    struct cfg_scan_request csreq;
    u32 i=0,j=0;
    u32 count=0xFFFFFFFF,minCount=0xFFFFFFFF, best_channel=0xFFFFFFFF;
    u16 _channel_mask=channel_mask;
    u32 _channel_5g_mask=channel_5g_mask;    
    u32 ap_num1=0,ap_num2=0;
    u8 total_2g_channel=0;
    u8 total_5g_channel=0;
    u8 filter_channel=0;
    u32 agc_gain;
    bool b5G=FALSE;

    //if(gDeviceInfo->hw_mode != SSV6XXX_HWM_AP){
    //    LOG_PRINTF("Invalid arguments.\n");
    //    return SSV6XXX_FAILED;
    //}

    /*
    Count how many channels that user want to scan
    */
    best_channel=6; //Default channel 6
    for(i=0;i<MAX_2G_CHANNEL_NUM;i++){
        if(_channel_mask&(0x01<<i)){
            total_2g_channel++;
            best_channel=i;
        }
    }

    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        for(i=0;i<MAX_5G_CHANNEL_NUM;i++){
            if(_channel_5g_mask&(0x01<<i)){
                total_5g_channel++;
                best_channel=i;
                b5G=TRUE;
            }
        }
    }

    if(((total_2g_channel+total_5g_channel)==1)||((total_2g_channel==0)&&(total_5g_channel==0)))
    {
        return ssv6xxx_wifi_ch_bitmask_to_num(b5G,best_channel);
    }

    
    
    //ssv_hal_reduce_phy_cca_bits(); //Only use this function on SmartConfig mode

    ssv_hal_acs_rx_mgmt_flow();

    ssv_hal_ap_listen_neighborhood(TRUE);

    agc_gain=ssv_hal_get_agc_gain();
    ssv_hal_set_acs_agc_gain();

    ssv6xxx_memset(gDeviceInfo->APInfo->channel_edcca_count,0,sizeof(gDeviceInfo->APInfo->channel_edcca_count));
    ssv6xxx_memset(gDeviceInfo->APInfo->channel_packet_count,0,sizeof(gDeviceInfo->APInfo->channel_packet_count));
    ssv6xxx_memset(gDeviceInfo->APInfo->channel_5g_edcca_count,0,sizeof(gDeviceInfo->APInfo->channel_5g_edcca_count));
    ssv6xxx_memset(gDeviceInfo->APInfo->channel_5g_packet_count,0,sizeof(gDeviceInfo->APInfo->channel_5g_packet_count));


    for(i=0;i<g_acs_channel_scanning_loop;i++){
        csreq.is_active=FALSE;
        csreq.n_ssids=0;
        csreq.dwell_time=g_acs_channel_scanning_interval/10; //due to unit is 10ms in FW scan
        csreq.channel_mask=_channel_mask;
        if(TRUE==ssv6xxx_wifi_support_5g_band())
        {
            csreq.channel_5g_mask=_channel_5g_mask;
        }
        else
        {
            csreq.channel_5g_mask=0;        
        }

        if(-1==_ssv6xxx_start_scan()) {
            LOG_PRINTF("Auto channel selection fail\r\n");
            ssv_hal_ap_rx_mgmt_flow();
            return -1;
        }
        csreq.vif_idx = vif_idx;
        //_ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SCAN, (void *)&csreq, sizeof(csreq), TRUE, FALSE);
        _ssv6xxx_wifi_scan((void *)&csreq,FALSE);

        if(FALSE==_ssv6xxx_wait_scan_done(OS_MS2TICK((g_acs_channel_scanning_interval*4*(total_2g_channel+total_5g_channel))))){
            LOG_PRINTF("Auto channel selection time out\r\n");
            break;
        }
    }

    ssv_hal_set_agc_gain(agc_gain);

    ssv_hal_ap_listen_neighborhood(FALSE);

    ssv_hal_ap_rx_mgmt_flow();

    //ssv_hal_recover_phy_cca_bits(); //Only use this function on SmartConfig mode

    if(i!=g_acs_channel_scanning_loop){
        return ssv6xxx_wifi_ch_bitmask_to_num(b5G,best_channel);;
    }

#if 0
    LOG_PRINTF("2G Channel  edcca count  packet count  AP num\r\n");
    LOG_PRINTF("------------------------------------------------------\r\n");
    for(i=0;i<MAX_2G_CHANNEL_NUM;i++){
        if(_channel_mask&(0x01<<i)){
            LOG_PRINTF("  %3d     %7d       %5d     %5d \r\n",
                ssv6xxx_wifi_ch_bitmask_to_num(FALSE,i),
                gDeviceInfo->APInfo->channel_edcca_count[i],
                gDeviceInfo->APInfo->channel_packet_count[i],
                ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(FALSE,i)));
        }
    }

    LOG_PRINTF("5G Channel  edcca count  packet count  AP num\r\n");
    LOG_PRINTF("------------------------------------------------------\r\n");
    for(i=0;i<MAX_5G_CHANNEL_NUM;i++){
        if(_channel_5g_mask&(0x01<<i)){
            LOG_PRINTF("  %3d     %7d       %5d     %5d \r\n",
                ssv6xxx_wifi_ch_bitmask_to_num(TRUE,i),
                gDeviceInfo->APInfo->channel_5g_edcca_count[i],
                gDeviceInfo->APInfo->channel_5g_packet_count[i],
                ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(TRUE,i)));
        }
    }
    
#endif


    /*
    Filter the top n channels which have the higher edcca count
    */
    #define MAX_COUNT minCount
    #define WORST_CHANNEL best_channel
    MAX_COUNT=0xFFFFFFFF;
    WORST_CHANNEL=0xFFFFFFFF;
    filter_channel=total_2g_channel>>1;

    for(j=0;j<filter_channel;j++){
        for(i=0;i<MAX_2G_CHANNEL_NUM;i++){
            if(_channel_mask&(0x01<<i)){
                count=gDeviceInfo->APInfo->channel_edcca_count[i];

                if((MAX_COUNT==0xFFFFFFFF)&&(WORST_CHANNEL==0xFFFFFFFF)){
                    MAX_COUNT=count;
                    WORST_CHANNEL=i;
                }

                if(count>=MAX_COUNT){
                    MAX_COUNT=count;
                    WORST_CHANNEL=i;
                }
            }
        }
        _channel_mask&=(~(0x01<<WORST_CHANNEL));
        MAX_COUNT=0xFFFFFFFF;
        WORST_CHANNEL=0xFFFFFFFF;
    }
    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        MAX_COUNT=0xFFFFFFFF;
        WORST_CHANNEL=0xFFFFFFFF;
        filter_channel=total_5g_channel>>1;

        for(j=0;j<filter_channel;j++){
            for(i=0;i<MAX_5G_CHANNEL_NUM;i++){
                if(_channel_5g_mask&(0x01<<i)){
                    count=gDeviceInfo->APInfo->channel_5g_edcca_count[i];

                    if((MAX_COUNT==0xFFFFFFFF)&&(WORST_CHANNEL==0xFFFFFFFF)){
                        MAX_COUNT=count;
                        WORST_CHANNEL=i;
                    }

                    if(count>=MAX_COUNT){
                        MAX_COUNT=count;
                        WORST_CHANNEL=i;
                    }
                }
            }
            _channel_5g_mask&=(~(0x01<<WORST_CHANNEL));
            MAX_COUNT=0xFFFFFFFF;
            WORST_CHANNEL=0xFFFFFFFF;
        }
    }
    #undef MAX_COUNT
    #undef WORST_CHANNEL

    /*
     Choose a best channel
    */
    minCount=0xFFFFFFFF;
    best_channel=0xFFFFFFFF;
    //LOG_PRINTF("Channel  edcca count  packet count  AP num\r\n");
    //LOG_PRINTF("------------------------------------------------------\r\n");
    for(i=0;i<MAX_2G_CHANNEL_NUM;i++){
        if(_channel_mask&(0x01<<i)){
            //LOG_PRINTF("  %2d      %7d       %5d     %5d \r\n",i,gDeviceInfo->APInfo->channel_edcca_count[i],gDeviceInfo->APInfo->channel_packet_count[i],ssv6xxx_aplist_get_count_by_channel(i));
            #if(INDICATOR_OF_CHANNEL_CHOISE==1)
            count=gDeviceInfo->APInfo->channel_edcca_count[i];
            #elif(INDICATOR_OF_CHANNEL_CHOISE==2)
            count=gDeviceInfo->APInfo->channel_packet_count[i];
            #elif(INDICATOR_OF_CHANNEL_CHOISE==3)
            count=ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(FALSE,i));
            #else
            error
            #endif

            if((minCount==0xFFFFFFFF)&&(best_channel==0xFFFFFFFF)){
                minCount=count;
                best_channel=i;
            }

            if(count<=minCount){
                minCount=count;
                best_channel=i;
            }

            //If two channels have the same edcca count or packet count, compare their ap num
            #if(INDICATOR_OF_CHANNEL_CHOISE!=3)
            if(count==minCount){
                ap_num1=ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(FALSE,best_channel));
                ap_num2=ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(FALSE,i));
                if(ap_num2<=ap_num1){
                    minCount=count;
                    best_channel=i;
                }
            }
            #endif

        }
    }

    b5G=FALSE;
    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        for(i=0;i<MAX_5G_CHANNEL_NUM;i++){
            if(_channel_5g_mask&(0x01<<i)){
                //LOG_PRINTF("  %2d      %7d       %5d     %5d \r\n",i,gDeviceInfo->APInfo->channel_edcca_count[i],gDeviceInfo->APInfo->channel_packet_count[i],ssv6xxx_aplist_get_count_by_channel(i));
                #if(INDICATOR_OF_CHANNEL_CHOISE==1)
                count=gDeviceInfo->APInfo->channel_5g_edcca_count[i];
                #elif(INDICATOR_OF_CHANNEL_CHOISE==2)
                count=gDeviceInfo->APInfo->channel_5g_packet_count[i];
                #elif(INDICATOR_OF_CHANNEL_CHOISE==3)
                count=ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(TRUE,i));
                #else
                error
                #endif

                if((minCount==0xFFFFFFFF)&&(best_channel==0xFFFFFFFF)){
                    minCount=count;
                    best_channel=i;
                    b5G=TRUE;   
                }

                if(count<=minCount){
                    minCount=count;
                    best_channel=i;
                    b5G=TRUE; 
                    
                }

                //If two channels have the same edcca count or packet count, compare their ap num
                #if(INDICATOR_OF_CHANNEL_CHOISE!=3)
                if(count==minCount){
                    ap_num1=ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(TRUE,best_channel));
                    ap_num2=ssv6xxx_aplist_get_count_by_channel(ssv6xxx_wifi_ch_bitmask_to_num(TRUE,i));
                    if(ap_num2<=ap_num1){
                        minCount=count;
                        best_channel=i;
                        b5G=TRUE; 
                    }
                }
                #endif

            }
        } 
    }
    
    if(best_channel==0xFFFFFFFF)
        best_channel=6;


    LOG_PRINTF("\33[32m The best channel is %d\33[0m\r\n",ssv6xxx_wifi_ch_bitmask_to_num(b5G,best_channel));
    return ssv6xxx_wifi_ch_bitmask_to_num(b5G,best_channel);
}
#endif
ssv6xxx_result _ssv6xxx_wifi_reg_promiscuous_rx_cb(promiscuous_data_handler promiscuous_cb, const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if (mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }
        ret = ssv6xxx_wifi_cb(PRMOISCUOUS_CB_NUM, 
			(u32 **)&gDeviceInfo->promiscuous_cb, (u32 *)promiscuous_cb, SSV6XXX_CB_ADD);

    } while(0);

    if (mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_reg_promiscuous_rx_cb(promiscuous_data_handler promiscuous_cb)
{
	return _ssv6xxx_wifi_reg_promiscuous_rx_cb(promiscuous_cb,TRUE);
}


ssv6xxx_result _ssv6xxx_wifi_unreg_promiscuous_rx_cb(promiscuous_data_handler promiscuous_cb,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE )
        {
	        ret=SSV6XXX_FAILED;
            break;
        }

        ret = ssv6xxx_wifi_cb(PRMOISCUOUS_CB_NUM, (u32 **)&gDeviceInfo->promiscuous_cb , (u32 *)promiscuous_cb, SSV6XXX_CB_REMOVE);

    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);


	return ret;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_promiscuous_rx_cb(promiscuous_data_handler promiscuous_cb)
{
	return _ssv6xxx_wifi_unreg_promiscuous_rx_cb(promiscuous_cb,TRUE);
}
extern u32 g_hw_enable;
extern void check_watchdog_timer(void *data1, void *data2);
extern struct task_info_st g_host_task_info[];
ssv6xxx_result _ssv6xxx_wifi_ap_pause(bool pause,const bool mutexLock)
{
#if (AP_MODE_ENABLE == 1)    
//    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    if(pause)// pause wifi
    {
        send_deauth_and_remove_all();
        OS_MsDelay(10); //wait for send deauth frame

        TXRXTask_SetOpMode(MT_STOP);
        //HCI stop
        ssv6xxx_drv_stop();
        g_hw_enable = FALSE;

#if(RECOVER_ENABLE == 1)
        ssv_hal_watchdog_disable();
#if(RECOVER_MECHANISM == 1)
        os_cancel_timer(check_watchdog_timer,(u32)NULL,(u32)NULL);
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE  == 1)

        ssv6xxx_drv_irq_disable(false);
        ssv_hal_rf_disable();
    }
    else // Go ahead wifi
    {
        TXRXTask_SetOpMode(MT_RUNNING);
        //Enable HW
        ssv_hal_rf_enable();
        ssv6xxx_drv_irq_enable(false);

#if(RECOVER_ENABLE == 1)
        ssv_hal_watchdog_enable();
#if(RECOVER_MECHANISM == 1)
        os_create_timer(IPC_CHECK_TIMER, check_watchdog_timer, NULL, NULL, (void*)TIMEOUT_TASK);
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE  == 1)

        //HCI start
        ssv6xxx_drv_start();

    	g_hw_enable = TRUE;
    }

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);
#else
	return SSV6XXX_SUCCESS;
#endif
}

H_APIs ssv6xxx_result ssv6xxx_wifi_pause(bool pause)
{
    _ssv6xxx_wifi_ap_pause(pause,TRUE);
	return 0;
}

//Customized
H_APIs void ssv6xxx_get_tx_info(u16 *tx_cnt, u16 *retry_cnt, u16 *phy_rate)
{
    ssv_hal_get_rc_info(tx_cnt,retry_cnt,phy_rate);
}


ssv6xxx_result _ssv6xxx_wifi_reg_recovery_cb(recovery_handler recovery_cb, const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }
        ret = ssv6xxx_wifi_cb(HOST_RECOVERY_CB_NUM, (u32 **)&gDeviceInfo->recovery_cb , (u32 *)recovery_cb, SSV6XXX_CB_ADD);

    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_reg_recovery_cb(recovery_handler recovery_cb)
{
	return _ssv6xxx_wifi_reg_recovery_cb(recovery_cb,TRUE);
}

ssv6xxx_result _ssv6xxx_wifi_unreg_recovery_cb(recovery_handler recovery_cb, const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
	        ret=SSV6XXX_FAILED;
            break;
        }
        ret = ssv6xxx_wifi_cb(HOST_RECOVERY_CB_NUM, (u32 **)&gDeviceInfo->recovery_cb , (u32 *)recovery_cb, SSV6XXX_CB_REMOVE);

    }while(0);

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

	return ret;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_recovery_cb(recovery_handler recovery_cb)
{
	return _ssv6xxx_wifi_unreg_recovery_cb(recovery_cb,TRUE);
}
HOST_API_STATE _ssv6xxx_wifi_host_api_state(const bool mutexLock)
{
    HOST_API_STATE host_api = HOST_API_ACTIVE;
    if(mutexLock)
        OS_MutexLock(&g_host_api_mutex);

    host_api=active_host_api;

    if(mutexLock)
        OS_MutexUnLock(&g_host_api_mutex);

    return host_api;

}

H_APIs HOST_API_STATE ssv6xxx_wifi_host_api_state()
{
	return _ssv6xxx_wifi_host_api_state(TRUE);
}

H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_pwr_mode(u32 pwr_mode)
{
#if (CONFIG_CHIP_ID==SSV6030P)
    if(pwr_mode == TX_POWER_B_HIGH_ONLY)
    {
        LOG_PRINTF("TX_POWER_B_HIGH_ONLY\r\n");
        g_host_cfg.tx_power_mode = TX_POWER_B_HIGH_ONLY;
        MAC_REG_WRITE(0xce0071bc, 0x79809098);
    }
    else if(pwr_mode == TX_POWER_ENHANCE_ALL)
    {
        LOG_PRINTF("TX_POWER_ENHANCE_ALL\r\n");
        g_host_cfg.tx_power_mode = TX_POWER_ENHANCE_ALL;
        MAC_REG_WRITE(0xce0071bc, 0xa0a08080);
    }
    else //back to normal power
    {
        LOG_PRINTF("TX_POWER_NORMAL\r\n");
        g_host_cfg.tx_power_mode = TX_POWER_NORMAL;
        MAC_REG_WRITE(0xce0071bc, 0x79806C72);//TX gain
    }
#endif
    return SSV6XXX_SUCCESS;
}

void  _ssv6xxx_wifi_set_rc_values(bool FromAPI)
{
    _ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_SET_RC_VALUES, &g_host_cfg, sizeof(struct Host_cfg), FromAPI);
}

H_APIs ssv6xxx_result ssv6xxx_set_rc_value(enum cmdtype_rcinfo cmd, u32 param)
{
    if (cmd == RC_RATEMASK)
    {
        u16 mask = (u16)(param&0x0FFF);
        if (mask == 0)
        {
            LOG_PRINTF("at least one bit is 1 between bit 0~bit 11\r\n");
            return SSV6XXX_INVA_PARAM;
        }
        g_host_cfg.rate_mask = mask;
    }
    else if (cmd == RC_PER)
    {
        u8 up_per = ((param&0xff0000)>>16);
        u8 down_per = param&0xff;
        if ((up_per >= down_per) || (up_per > 50) || (down_per > 50) || (up_per == 0))
        {
            LOG_PRINTF("buggy values of PER\r\n");
            return SSV6XXX_INVA_PARAM;
        }
        g_host_cfg.upgrade_per = up_per;
        g_host_cfg.downgrade_per = down_per;

    }
    else if (cmd == RC_RESENT)
    {
        if (g_host_cfg.resent_fail_report != param)
            g_host_cfg.resent_fail_report = (u8)param;
    }
    else if (cmd == RC_PREPRBFRM)
    {
        if (g_host_cfg.pre_alloc_prb_frm != param)
        {
            g_host_cfg.pre_alloc_prb_frm = (u8)param;
            LOG_PRINTF("After set pre-allocate probe frame, please restart AP/station to enable the feature\r\n");
            return SSV6XXX_SUCCESS;
        }
    }
    else if (cmd == RC_UPPERFASTESTB)
    {
        u8 per = (u8)param;
        if ((per > 50) || (per == 0))
        {
            LOG_PRINTF("buggy values of PER for fastest b mode rate\r\n");
            return SSV6XXX_INVA_PARAM;
        }
        g_host_cfg.upper_fastestb = per;
    }
    else if (cmd == DIRECT_RATE_DW)
    {
        g_host_cfg.direct_rc_down = (bool)param;
    }
    else if (cmd == FORCE_RTSCTS)
    {
        g_host_cfg.force_RTSCTS= (bool)param;
    }
    else if (cmd == RC_DRATE_ENDIAN)
    {
        g_host_cfg.rc_drate_endian= (bool)param;    
    }

    _ssv6xxx_wifi_set_rc_values(TRUE);
    return SSV6XXX_SUCCESS;
}

H_APIs s32 ssv6xxx_wifi_get_mac(u8 *mac,u8 vif_idx)
{
    MEMCPY(mac,gDeviceInfo->vif[vif_idx].self_mac,ETH_ALEN);
    return 0;
}

bool tx_duty_en=0;
void txduty_t_handler(void* data1, void* data2)
{    
    s32 res;

    if(tx_duty_en==TRUE)
    {
        ssv_hal_halt_txq((u32)data1,1); //halt q 1 g_qktime
        OS_MsDelay(g_host_cfg.txduty.qktime);
        ssv_hal_halt_txq((u32)data1,0);//enable q 1
        LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING, ("txduty tick=%d,qidx=%d\r\n",OS_GetSysTick(),(u32)data1));
        res = os_create_timer(g_host_cfg.txduty.txtime,txduty_t_handler,(void*)data1,NULL, (void*)TIMEOUT_TASK);
        while(res != OS_SUCCESS)
        {
            LOG_PRINTF("g_txtime faile\r\n");
            OS_MsDelay(10);
            res = os_create_timer(g_host_cfg.txduty.txtime,txduty_t_handler,(void*)data1,NULL, (void*)TIMEOUT_TASK);
        }
    }
}

s32 _ssv6xxx_set_tx_duty(txduty_cfg* cfg, bool FromAPI)
{

    return _ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_SET_TXDUTY, (void*)cfg, sizeof(txduty_cfg), FromAPI);
}

H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_duty(u32 qidx, u32 txTime, u32 qkTime)
{
    ssv6xxx_result res;
    if(qidx < 5)
    {    
        txduty_cfg dt_cfg;
        g_host_cfg.txduty.qidx = qidx;
        g_host_cfg.txduty.txtime = txTime;
        g_host_cfg.txduty.qktime = qkTime;
        
        if(g_host_cfg.txduty.mode == TXDUTY_AT_FW)
        {
            dt_cfg.qidx = qidx;
            dt_cfg.txtime = txTime;
            dt_cfg.qktime = qkTime;
            if(txTime && qkTime)
            {
                tx_duty_en = TRUE;
            }
            else
            {
                LOG_PRINTF("invalid duty: %d,%d\r\n",txTime, qkTime);
                tx_duty_en = FALSE;
            }

            res = _ssv6xxx_set_tx_duty(&dt_cfg,TRUE);
            if(SSV6XXX_SUCCESS != res)
            {
                LOG_PRINTF("_ssv6xxx_set_tx_duty fail,res=%d\r\n",res);
                return res;
            }
        }
        else if(g_host_cfg.txduty.mode == TXDUTY_AT_HOST)
        {
            os_cancel_timer(txduty_t_handler,qidx,0);
            ssv_hal_halt_txq(qidx,0);//enable q 1
            if((((txTime > 200)&&(txTime < 70)) || ((qkTime > 50)&&(qkTime < 10)))||
                (!txTime||!qkTime))
            {
                tx_duty_en = FALSE;
                LOG_PRINTF("invalid duty: %d,%d\r\n",txTime, qkTime);
                return SSV6XXX_FAILED;
            }
            os_create_timer(txTime,txduty_t_handler,(void*)qidx,NULL, (void*)TIMEOUT_TASK);
            tx_duty_en = TRUE;
        }
        LOG_PRINTF("TX duty: %d,%d,qidx=%d\r\n",g_host_cfg.txduty.txtime, g_host_cfg.txduty.qktime,qidx);
    }
    else
    {
        LOG_PRINTF("TX duty:invalid qidx=%d\r\n",qidx);
    }
    return SSV6XXX_SUCCESS;
}

H_APIs void ssv6xxx_set_TXQ_SRC_limit(u32 qidx,u32 val)
{
    ssv_hal_set_TXQ_SRC_limit(qidx,val);
}


H_APIs ssv6xxx_result ssv6xxx_wifi_set_trx_res_page(u8 tx_page, u8 rx_page)
{
    if((rx_page+tx_page) > 121)
    {
        LOG_PRINTF("invalid trx page: %d,%d\r\n",tx_page, rx_page);
        return SSV6XXX_FAILED;
    }
    g_host_cfg.tx_res_page = tx_page;
    g_host_cfg.rx_res_page = rx_page;
    LOG_PRINTF("TRX res page: T=%d,R=%d\r\n",g_host_cfg.tx_res_page, g_host_cfg.rx_res_page);
    return SSV6XXX_SUCCESS;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_task_sleep(bool on)
{
    g_host_cfg.tx_sleep = on;
    LOG_PRINTF("tx_sleep: %d\r\n",g_host_cfg.tx_sleep);
    
    return SSV6XXX_SUCCESS;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_task_sleep_tick(u8  tick)
{
    g_host_cfg.tx_sleep_tick = tick;
    LOG_PRINTF("tx_sleep_tick: %d\r\n",g_host_cfg.tx_sleep_tick);
    
    return SSV6XXX_SUCCESS;
}


H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_task_retry_cnt(u8 cnt)
{
    g_host_cfg.tx_retry_cnt= cnt;
    LOG_PRINTF("tx task retry cnt: %d\r\n",g_host_cfg.tx_retry_cnt);
    
    return SSV6XXX_SUCCESS;
}


H_APIs ssv6xxx_result ssv6xxx_wifi_set_ap_erp(bool on)
{
    g_host_cfg.erp = on;
    LOG_PRINTF("ap erp: %d\r\n",g_host_cfg.erp);
    
    return SSV6XXX_SUCCESS;
}
H_APIs ssv6xxx_result ssv6xxx_wifi_set_ap_short_preamble(bool on)
{
    g_host_cfg.b_short_preamble = on;
    LOG_PRINTF("ap b_short_preamble: %d\r\n",g_host_cfg.b_short_preamble);
    
    return SSV6XXX_SUCCESS;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_set_ap_rx_support_rate(u16 legacy_msk,u16 mcs_msk)
{
    g_host_cfg.ap_rx_support_legacy_rate_msk = legacy_msk;
    g_host_cfg.ap_rx_support_mcs_rate_msk = mcs_msk;
    LOG_PRINTF("ap_rx_support_rate_msk: bas:0x%x/mcs:0x%x\r\n",
        g_host_cfg.ap_rx_support_legacy_rate_msk,g_host_cfg.ap_rx_support_mcs_rate_msk);
    
    return SSV6XXX_SUCCESS;
}

H_APIs int ssv6xxx_sw_reset(u32 com)
{
#if(RECOVER_MECHANISM == 1)
    os_cancel_timer(check_watchdog_timer,(u32)NULL,(u32)NULL);
    recovery_pause_resume(false);
#endif //#if(RECOVER_MECHANISM == 1)
    return ssv_hal_sw_reset(com);
}

H_APIs int ssv6xxx_halt_tx_queue(u32 qidx, bool bHalt)
{
    if(qidx < 5)
    {    
        if(bHalt)
        {
            g_host_cfg.tx_sleep = 1;
            g_host_cfg.tx_retry_cnt = 20;
            ssv_hal_halt_txq(qidx,bHalt);
        }
        else
        {
            ssv_hal_halt_txq(qidx,bHalt);
            g_host_cfg.tx_sleep = ts_bk;
            g_host_cfg.tx_retry_cnt = tr_bk;
        }
    }
	return SSV6XXX_SUCCESS;
}

H_APIs ssv6xxx_result ssv6xxx_set_voltage_mode(u32 mode)
{
    if((mode == VOLT_LDO_REGULATOR) || (mode == VOLT_DCDC_CONVERT))
    {
        g_host_cfg.volt_mode = mode;
        return ssv_hal_set_voltage_mode(mode);
    }
    return SSV6XXX_FAILED;
}

H_APIs bool ssv6xxx_wifi_support_5g_band(void)
{
    if((RF_5G_BAND==g_host_cfg.support_rf_band)&&
        (TRUE==ssv_hal_support_5g_band())&&
        (g_host_cfg.support_ht==1))
        return TRUE;
    else
        return FALSE;
}


H_APIs bool ssv6xxx_wifi_enable_5g_band(bool en)
{
    if(en==TRUE)
    {
        if(g_host_cfg.support_ht==1)
        {
            if(TRUE==ssv_hal_support_5g_band())
            {
                g_host_cfg.support_rf_band=RF_5G_BAND;
                LOG_PRINTF("Enable 5G Band, please restart the AP/STA mode\r\n");
                return TRUE;
            }
            else
            {
                g_host_cfg.support_rf_band=RF_2G_BAND;        
                LOG_PRINTF("This HW doesn't support 5G Band\r\n");
                return FALSE;
            }
        }
        else
        {
            g_host_cfg.support_rf_band=RF_2G_BAND;
            LOG_PRINTF("Enable 5G band fail, beacuse the system only support bg mode now\r\n");
            return FALSE;
        }
    }
    else
    {
        g_host_cfg.support_rf_band=RF_2G_BAND;
        LOG_PRINTF("Enable 2G Band, please restart the AP/STA mode\r\n");        
        return FALSE;
    
    }
}

H_APIs int ssv6xxx_wifi_ch_to_bitmask(u8 ch)
{
    int i=0;
    if(IS_5G_BAND(ch))
    {
        for(i=0;i<MAX_5G_CHANNEL_NUM;i++)
        {
            if(ssv6xxx_5ghz_chantable[i].hw_value==ch)
            {
                return i;
            }
        }
    }
    else
    {
        for(i=0;i<MAX_2G_CHANNEL_NUM;i++)
        {
            if(ssv6xxx_2ghz_chantable[i].hw_value==ch)
            {
                return i;
            }
        }
    
    }
    return -1;
}

H_APIs int ssv6xxx_wifi_ch_bitmask_to_num(bool b5GBand, u8 idx)
{
    if(b5GBand==TRUE)
    {
        if(idx<MAX_5G_CHANNEL_NUM)
            return ssv6xxx_5ghz_chantable[idx].hw_value;
    }
    else
    {
        if(idx<MAX_2G_CHANNEL_NUM)
            return ssv6xxx_2ghz_chantable[idx].hw_value;
    }
    return -1;
}


H_APIs int ssv6xxx_wifi_show_reg_list(void)
{
    u32 i=0,j=0;
    for(i=0;i<(sizeof(regdom)/sizeof(struct ieee80211_regdomain));i++)
    {
        if(regdom[i].n_reg_rules!=0)
        {
            LOG_PRINTF("country=%c%c\r\n",regdom[i].alpha2[0],regdom[i].alpha2[1]);   
            for(j=0;j<regdom[i].n_reg_rules;j++)
            {                
                LOG_PRINTF("(%d - %d) @ %d %s\r\n",
                    KHZ_TO_MHZ(regdom[i].reg_rules[j].freq_range.start_freq_khz),
                    KHZ_TO_MHZ(regdom[i].reg_rules[j].freq_range.end_freq_khz),
                    KHZ_TO_MHZ(regdom[i].reg_rules[j].freq_range.max_bandwidth_khz),
                    ((regdom[i].reg_rules[j].flags&NL80211_RRF_DFS)?"DFS":""));               
            }
        }
    }
    return 0;
}

H_APIs int ssv6xxx_wifi_get_reg(char *reg, u8 len)
{
    if(len<sizeof(gDeviceInfo->alpha2))
    {
        LOG_PRINTF("len is not invalid\r\n");
        return -1;
    }

    reg[0]=gDeviceInfo->alpha2[0];
    reg[1]=gDeviceInfo->alpha2[1];
    reg[2]=gDeviceInfo->alpha2[2];    
    return 0;
}

static struct ieee80211_regdomain *_ssv6xxx_wifi_get_regdomain(char *reg)
{
    u32 i=0;
    for(i=0;i<(sizeof(regdom)/sizeof(struct ieee80211_regdomain));i++)
    {
        if(regdom[i].n_reg_rules!=0)
        {
            if((regdom[i].alpha2[0]==reg[0])&&(regdom[i].alpha2[1]==reg[1]))
            {
                break;
            }
        }        
    }

    if(i==(sizeof(regdom)/sizeof(struct ieee80211_regdomain)))
    {
        return NULL;
    }
    return &regdom[i];
}

H_APIs struct ieee80211_regdomain *ssv6xxx_wifi_get_current_regdomain(void)
{
    u32 i=0;
    for(i=0;i<(sizeof(regdom)/sizeof(struct ieee80211_regdomain));i++)
    {
        if(regdom[i].n_reg_rules!=0)
        {
            if((regdom[i].alpha2[0]==gDeviceInfo->alpha2[0])&&(regdom[i].alpha2[1]==gDeviceInfo->alpha2[1]))
            {
                break;
            }
        }        
    }

    if(i==(sizeof(regdom)/sizeof(struct ieee80211_regdomain)))
    {
        return NULL;
    }
    return &regdom[i];
}


H_APIs int ssv6xxx_wifi_set_reg(char *reg)
{
    if(NULL==_ssv6xxx_wifi_get_regdomain(reg))
    {
        LOG_PRINTF("set reg %c%c fail \r\n",reg[1],reg[0]);
        return -1;
    }

    gDeviceInfo->alpha2[2]=' ';    
    gDeviceInfo->alpha2[1]=reg[1];
    gDeviceInfo->alpha2[0]=reg[0];  

    ssv6xxx_wifi_update_available_channel();
    return 0;
    
}

H_APIs int ssv6xxx_wifi_update_available_channel(void)
{
    u32 i=0;//,j=0;
    u16 available_2g_channel_mask=0;
    u32 available_5g_channel_mask=0;    
    char current_reg[3];
    struct ieee80211_regdomain *preg=NULL;
    ssv6xxx_wifi_get_reg(current_reg,sizeof(current_reg));

    preg=_ssv6xxx_wifi_get_regdomain(current_reg);
    if(preg==NULL)
    {
        LOG_PRINTF("update support channel fail\r\n");
        return -1;    
    }
    for(i=0;i<MAX_2G_CHANNEL_NUM;i++)
    {
        if(0==freq_reg_info_regd(ssv6xxx_2ghz_chantable[i].center_freq,BW,preg))
            available_2g_channel_mask=(available_2g_channel_mask|(1<<i));
    }

    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        for(i=0;i<MAX_5G_CHANNEL_NUM;i++)
        {
            if(0==freq_reg_info_regd(ssv6xxx_5ghz_chantable[i].center_freq,BW,preg))
                available_5g_channel_mask=(available_5g_channel_mask|(1<<i));
        }
    }
    else
    {
        available_5g_channel_mask=0;
    }
    
    gDeviceInfo->available_2g_channel_mask=available_2g_channel_mask;
    gDeviceInfo->available_5g_channel_mask=available_5g_channel_mask;    
    //LOG_PRINTF("available 2g channel mask=%x\r\n",gDeviceInfo->available_2g_channel_mask);
    //LOG_PRINTF("available 5g channel mask=%x\r\n",gDeviceInfo->available_5g_channel_mask);    
    return 0;
}


H_APIs int ssv6xxx_wifi_show_available_channel(void)
{
    u32 i=0;
    struct ieee80211_regdomain *reg=NULL;
    
    ssv6xxx_wifi_update_available_channel();
    
    reg =ssv6xxx_wifi_get_current_regdomain() ;

    LOG_PRINTF("2G Band:%X\r\n",gDeviceInfo->available_2g_channel_mask);
    for(i=0;i<MAX_2G_CHANNEL_NUM;i++)
    {
        if(gDeviceInfo->available_2g_channel_mask&(1<<i))
        {
            LOG_PRINTF("ch=%d(%d)\r\n",ssv6xxx_2ghz_chantable[i].hw_value,ssv6xxx_2ghz_chantable[i].center_freq);
        }
    }

    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        LOG_PRINTF("5G Band:%X\r\n",gDeviceInfo->available_5g_channel_mask);
        for(i=0;i<MAX_5G_CHANNEL_NUM;i++)
        {
            if(gDeviceInfo->available_5g_channel_mask&(1<<i))
            {
                if(1==g_host_cfg.ap_no_dfs)
                {
                    //if this channel need to support DFS function, skip it
                    if(TRUE==freq_need_dfs(ssv6xxx_5ghz_chantable[i].center_freq, BW, reg))
                    {
                        LOG_PRINTF("ch=%d(%d), DFS AP mode not support\r\n",ssv6xxx_5ghz_chantable[i].hw_value,ssv6xxx_5ghz_chantable[i].center_freq);
                        continue;
                    }
                }

                LOG_PRINTF("ch=%d(%d)\r\n",ssv6xxx_5ghz_chantable[i].hw_value,ssv6xxx_5ghz_chantable[i].center_freq);
            }
        
        }
    }

    return 0;
}

H_APIs bool ssv6xxx_wifi_is_40MHZ_legal(u8 ch)
{
    int i=0;
    struct ieee80211_regdomain *reg=NULL;
    reg =ssv6xxx_wifi_get_current_regdomain() ;
    i=ssv6xxx_wifi_ch_to_bitmask(ch);
    
    if(IS_5G_BAND(ch))
    {
        if(TRUE==freq_40MHZ_legal(ssv6xxx_5ghz_chantable[i].center_freq,reg))
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        return FALSE;
    }
}

H_APIs bool ssv6xxx_wifi_is_available_channel(ssv6xxx_hw_mode mode, u8 ch)
{
    int bit_num;
    
    struct ieee80211_regdomain *reg=NULL;
    reg =ssv6xxx_wifi_get_current_regdomain() ;
    bit_num = ssv6xxx_wifi_ch_to_bitmask(ch);    

    if(bit_num==-1)
    {
        LOG_PRINTF("ch(%d) is not available channel\r\n",ch);
        return FALSE;
    }

    ssv6xxx_wifi_update_available_channel();

    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {

        if(IS_5G_BAND(ch))
        {
            
            if(gDeviceInfo->available_5g_channel_mask&(1<<bit_num))
            {
                if(1==g_host_cfg.ap_no_dfs)
                {
                    // if this channel need to support DFS, skip it
                    if((TRUE==freq_need_dfs(ssv6xxx_5ghz_chantable[bit_num].center_freq, BW, reg))&&(mode==SSV6XXX_HWM_AP))
                        return FALSE;
                    else
                        return TRUE;
                }
                else
                {
                    return TRUE;
                }
            }
        }
        else
        {
            if(gDeviceInfo->available_2g_channel_mask&(1<<bit_num))
            {
                return TRUE;
            }
        
        }
    }
    else
    {
        if(gDeviceInfo->available_2g_channel_mask&(1<<bit_num))
        {
            return TRUE;
        }
    
    }
    LOG_PRINTF("ch(%d) is not available channel\r\n",ch);
    return FALSE;

}

H_APIs int ssv6xxx_wifi_align_available_channel_mask(ssv6xxx_hw_mode mode,u16 *channel_2g_mask, u32 *channel_5g_mask)
{

#if 1
    int i=0;
    u16 _channel_2g_mask;
    u32 _channel_5g_mask;
    struct ieee80211_regdomain *reg=NULL;

    reg =ssv6xxx_wifi_get_current_regdomain();
    _channel_2g_mask=*channel_2g_mask;
    _channel_5g_mask=*channel_5g_mask;

    _channel_2g_mask=_channel_2g_mask&gDeviceInfo->available_2g_channel_mask;

    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        _channel_5g_mask=_channel_5g_mask&gDeviceInfo->available_5g_channel_mask;

        if(1==g_host_cfg.ap_no_dfs)
        {
            //if this channel need to support DFS, skip it
            for(i=0;i<MAX_5G_CHANNEL_NUM;i++)
            {
                if(_channel_5g_mask&(1<<i))
                {
                    if((TRUE==freq_need_dfs(ssv6xxx_5ghz_chantable[i].center_freq, BW, reg))&&(mode==SSV6XXX_HWM_AP))
                    {
                        _channel_5g_mask&=~(1<<i);
                    }

                }
            }
        }        
    }
    else
    {
        _channel_5g_mask=0;
    }
    *channel_2g_mask=_channel_2g_mask;
    *channel_5g_mask=_channel_5g_mask;    
    
#else
#define IS_CHANNEL_AVAILABLE(a,b,c) (((a)&(1<<(c)))&&((b)&(1<<(c))))
    u16 _channel_2g_mask=*channel_2g_mask;
    u32 _channel_5g_mask=*channel_5g_mask;

    u8 i=0;
    for(i=0;i<MAX_2G_CHANNEL_NUM;i++)
    {
        if(!IS_CHANNEL_AVAILABLE(_channel_2g_mask,gDeviceInfo->available_2g_channel_mask,i))
        {
            _channel_2g_mask&=(~(1<<i));
        }
    }
    //LOG_PRINTF("channel_2g_mask=%x\r\n",_channel_2g_mask);
    
    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        for(i=0;i<MAX_5G_CHANNEL_NUM;i++)
        {
            if(!IS_CHANNEL_AVAILABLE(_channel_5g_mask,gDeviceInfo->available_5g_channel_mask,i))
            {
                _channel_5g_mask&=(~(1<<i));
            }
        }
        //LOG_PRINTF("channel_5g_mask=%x\r\n",_channel_5g_mask);            
    }
    else
    {
        _channel_5g_mask=0;
    }
    *channel_2g_mask=_channel_2g_mask;
    *channel_5g_mask=_channel_5g_mask;    
    
#endif    
    return 0;
}

H_APIs int ssv6xxx_wifi_set_ap_no_dfs(bool no_dfs)
{

    if(no_dfs==TRUE)
    {
        g_host_cfg.ap_no_dfs=1;    
        LOG_PRINTF("AP mode don't support DFS\r\n");
    }
    else
    {
        g_host_cfg.ap_no_dfs=0;    
        LOG_PRINTF("AP mode support DFS\r\n");
    }
    
    return 0;
}

H_APIs int ssv6xxx_wifi_set_ap_ht20_only(bool en)
{
    g_host_cfg.ap_ht20_only=en;
    LOG_PRINTF("set ap channel width = %s\r\n",(en==TRUE)?"HT20 Only":"HT2040");    
    return SSV6XXX_SUCCESS;
}

H_APIs ssv6xxx_result ssv6xxx_wifi_set_sta_no_bcn_timeout(u8 value)
{
    g_host_cfg.sta_no_bcn_timeout=value;
    LOG_PRINTF("STA no bcn timeout = %d (ms)\r\n",g_host_cfg.sta_no_bcn_timeout*500);
    return SSV6XXX_SUCCESS;
}

H_APIs int ssv6xxx_wifi_set_hci_aggr(HCI_AGGR_HW trx, bool en)
{
#if((CONFIG_CHIP_ID==SSV6006B)||(CONFIG_CHIP_ID==SSV6006C))
    if(trx == SSV_HCI_RX)
    {
        ssv_hal_hci_aggr_en(SSV_HCI_RX,en);
        g_host_cfg.hci_rx_aggr = en;
        //LOG_PRINTF("hci rx aggr en=%d\r\n",g_host_cfg.hci_rx_aggr);
    }
    else if(trx == SSV_HCI_TX)
    {
        ssv_hal_hci_aggr_en(SSV_HCI_TX,en);
        g_host_cfg.hci_aggr_tx = en;
        //LOG_PRINTF("hci aggr tx en=%d\r\n",g_host_cfg.hci_aggr_tx);
    }
#endif
    return 0;
}

H_APIs HOST_API_STATE ssv6xxx_wifi_get_host_api_state(void)
{
    return active_host_api;
}

H_APIs bool ssv6xxx_wifi_set_eco_mode(void)
{
    struct cfg_ps_request wowreq;
    if(gDeviceInfo->vif[0].hw_mode == SSV6XXX_HWM_AP)
    {
        wowreq.ipv4addr = 0;
        wowreq.dtim_multiple = 0;
        wowreq.host_ps_st = HOST_PS_SETUP;
        ssv6xxx_wifi_pwr_saving(&wowreq,TRUE);
        return TRUE;
    }
    else
    {
        LOG_PRINTF("ECO function is only for AP mode\r\n");
        return FALSE;
    }

}
//For instance ,hwaddr:a[0]~a[5]=60:11:22:33:44:55
H_APIs s32 ssv6xxx_wifi_ap_del_sta(u8 *hwaddr)
{
#if (AP_MODE_ENABLE == 1)
    return send_deauth_and_remove_sta(hwaddr);
#else
	/* TODO: aaron */
	return -1;
#endif
}
