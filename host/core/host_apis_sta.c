/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#endif

#include <host_apis.h>
#include <ssv_drv.h>
#include <mlme.h>
#include <os_wrapper.h>
#include <ssv_timer.h>
#include <dev.h>
#include <txrx_hdl.h>

/* extern variables */
extern struct Host_cfg g_host_cfg;
extern OsMutex g_host_api_mutex;
extern HOST_API_STATE active_host_api;
extern u8 g_max_num_of_ap_list;
extern u8 config_mac[];

/* extern functions */
extern bool _ssv6xxx_wakeup_wifi(void);
extern s32 _ssv6xxx_wifi_send_cmd(void *pCusData, int nCuslen, ssv6xxx_host_cmd_id eCmdID);
extern ssv_vif *_get_ava_vif(void);
extern void _ssv6xxx_wifi_set_rc_values(bool FromAPI);
extern s32 _ssv6xxx_set_ampdu_param(u8 opt, u8 value, u8 vif_idx, bool FromAPI);
	
extern int ssv6xxx_get_cust_mac(u8 *mac);
extern ssv6xxx_result check_efuse_chip_id(void);
extern ssv6xxx_result CmdEng_SetOpMode(ModeType mode);
extern ssv6xxx_result TXRXTask_SetOpMode(ModeType mode);
extern int ssv_hal_remove_interface(u8 itf_idx);
extern int ssv6xxx_start(ssv_vif *vif);

extern H_APIs void get_icomm_hw_info(icomm_hw_info *hw_info);

 
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
        OS_MutexLock(g_host_api_mutex);

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
                OS_MutexUnLock(g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(g_host_api_mutex);

            if(FALSE == ret)
                break;
        }

    	size = sizeof(struct cfg_sconfig_request);
        ret = _ssv6xxx_wifi_send_cmd(csreq, size, SSV6XXX_HOST_CMD_SMART_CONFIG);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(g_host_api_mutex);

    return ret;


}
H_APIs s32 ssv6xxx_wifi_sconfig(struct cfg_sconfig_request *csreq)
{
    return _ssv6xxx_wifi_sconfig(csreq, TRUE);
}
#endif

 
 



 
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
        OS_MutexLock(g_host_api_mutex);

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
                OS_MutexUnLock(g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(g_host_api_mutex);

            if(FALSE == ret)
                break;
        }

        if((gDeviceInfo->vif[csreq->vif_idx].hw_mode==SSV6XXX_HWM_STA)&&
           (gDeviceInfo->vif[csreq->vif_idx].m_info.StaInfo->status==DISCONNECT))
        {
            MEMSET((void*)gDeviceInfo->ap_list,0,sizeof(struct ssv6xxx_ieee80211_bss)*g_max_num_of_ap_list);
            ssv_hal_sta_rcv_all_bcn();
        }
 
    	size = sizeof(struct cfg_scan_request) +
    		sizeof(struct cfg_80211_ssid) * csreq->n_ssids;
        ret = _ssv6xxx_wifi_send_cmd(csreq, size, SSV6XXX_HOST_CMD_SCAN);

    }while(0);
    if(mutexLock)
        OS_MutexUnLock(g_host_api_mutex);

    return ret;


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
        OS_MutexLock(g_host_api_mutex);

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
                OS_MutexUnLock(g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(g_host_api_mutex);

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
        OS_MutexUnLock(g_host_api_mutex);

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
        OS_MutexLock(g_host_api_mutex);

    do {
        if(active_host_api == HOST_API_DEACTIVE)
        {
            ret=SSV6XXX_FAILED;
            break;
        }
        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(g_host_api_mutex);

            if(FALSE == ret)
                break;
        }


        size = HOST_CMD_HDR_LEN	+ sizeof(struct cfg_leave_request);
        ret = _ssv6xxx_wifi_send_cmd(clreq, size, SSV6XXX_HOST_CMD_LEAVE);


    }while(0);

    if(mutexLock)
        OS_MutexUnLock(g_host_api_mutex);

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
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(!vif->m_info.StaInfo)
    {
        LOG_PRINTF("_alloc_sta_info \n\r");
        vif->m_info.StaInfo = _alloc_sta_info();
	    if(!vif->m_info.StaInfo)
		{
			LOG_PRINTF("_alloc_sta_info fail\n\r");
			return SSV6XXX_FAILED;
		}
	}

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);

    if(gDeviceInfo->used_vif==0)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);    
        ssv6xxx_HW_disable();
        if(-1==ssv_hal_chip_init())
        {
            LOG_PRINTF("ssv6xxx_chip_init fail\n\r");
            return SSV6XXX_FAILED;
        }
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    //Set default MAC addr
    ssv6xxx_memcpy(vif->self_mac, config_mac, ETH_ALEN);
    ssv6xxx_get_cust_mac(vif->self_mac);
    if(vif->idx>0)
    {
        vif->self_mac[0] |= 0x02;            
        vif->self_mac[3] &= MAC_MASK;
        vif->self_mac[3] |=((vif->idx-1)<<20);
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(check_efuse_chip_id() != SSV6XXX_SUCCESS)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);    
        return SSV6XXX_FAILED;
    }

    //Set Host API on
    active_host_api = HOST_API_ACTIVE;
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);

    OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
    //Set global variable
    vif->hw_mode = hw_mode;

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);

    if(gDeviceInfo->used_vif==0)
    {
        //Load FW, init mac
        if(ssv6xxx_start(vif)!=SSV6XXX_SUCCESS)
        {
            LOG_PRINTF("ssv6xxx_start fail!!\r\n");

            OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
			LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
            return SSV6XXX_FAILED;
        }
    }
    OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
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

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(vif->hw_mode ==SSV6XXX_HWM_SCONFIG)
	{
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
		ssv_hal_sconfig_rx_data_flow();
        ssv_hal_accept_none_wsid_frame();
        ssv_hal_reduce_phy_cca_bits();        
    }
    else
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
		ssv_hal_update_cci_setting(MAX_CCI_SENSITIVE);    
    }

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);	
    if(gDeviceInfo->used_vif==0)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);    
        _ssv6xxx_wifi_set_rc_values(FALSE);
        //ampdu tx
        _ssv6xxx_set_ampdu_param(AMPDU_TX_OPT_ENABLE,g_host_cfg.ampdu_tx_enable,vif->idx,FALSE);
        //ampdu rx
        _ssv6xxx_set_ampdu_param(AMPDU_RX_OPT_ENABLE,g_host_cfg.ampdu_rx_enable,vif->idx,FALSE);
        //ampdu rx aggr max
        _ssv6xxx_set_ampdu_param(AMPDU_RX_OPT_BUF_SIZE,g_host_cfg.ampdu_rx_buf_size,vif->idx,FALSE);
        ssv6xxx_wifi_set_tx_pwr_mode(g_host_cfg.tx_power_mode);

        ssv_hal_sta_reject_bcn();
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);		
    }

    gDeviceInfo->used_vif++;
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);	
    return SSV6XXX_SUCCESS;
}

void sta_mode_off(u8 vif_idx)
{
    ssv_vif *vif=NULL;    

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


ssv6xxx_result _ssv6xxx_wifi_station(u8 hw_mode,Sta_setting *sta_station,const bool mutexLock)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    LOG_PRINTF("%s() at line(%d)\n",__FUNCTION__,__LINE__);

    LOG_PRINTF("%s() at line(%d), hw_mode(%d)\n",__FUNCTION__,__LINE__,hw_mode);

    //Detect host API status
    if ((SSV6XXX_HWM_STA != hw_mode)&&(SSV6XXX_HWM_SCONFIG != hw_mode))
	{
        return SSV6XXX_FAILED;
    }

    if(mutexLock)
        OS_MutexLock(g_host_api_mutex);

    do {

        if(active_host_api == HOST_API_PWR_SAVING)
        {
            if(mutexLock)
                OS_MutexUnLock(g_host_api_mutex);

            ret = _ssv6xxx_wakeup_wifi();

            if(mutexLock)
                OS_MutexLock(g_host_api_mutex);

            if((ssv6xxx_result)FALSE == ret)
                break;
        }

	    LOG_PRINTF("%s() at line(%d),sta_station->status=(%s)\n",
			__FUNCTION__,__LINE__,(sta_station->status == true)?"true":"false");

		if(sta_station->status) //Station on
        {
		    LOG_PRINTF("%s() at line(%d),active_host_api(%s)\n",__FUNCTION__,__LINE__,(active_host_api==HOST_API_ACTIVE)?"ACTIVE":"DE-ACTIVE");
		
            if((HOST_API_ACTIVE==active_host_api)&&
               (gDeviceInfo->vif[sta_station->vif_idx].hw_mode != SSV6XXX_HWM_INVALID))
            {
        		printk("%s() at line(%d)\n",__FUNCTION__,__LINE__);
                //if(gDeviceInfo->vif[sta_station->vif_idx].hw_mode)
                if (gDeviceInfo->vif[sta_station->vif_idx].hw_mode == SSV6XXX_HWM_STA)
            	{
            		printk("%s() at line(%d)\n",__FUNCTION__,__LINE__);
	                sta_mode_off(sta_station->vif_idx);
                }
				else if(gDeviceInfo->vif[sta_station->vif_idx].hw_mode == SSV6XXX_HWM_SCONFIG)
				{
            		printk("%s() at line(%d)\n",__FUNCTION__,__LINE__);
	                sconfig_mode_off(sta_station->vif_idx);
				}
            }
            {
                u8 retry_ini=INI_CNT;

                ssv6xxx_wifi_update_available_channel();
                while ((SSV6XXX_SUCCESS != (ret = sta_mode_on(hw_mode,sta_station->vif_idx))) && (retry_ini > 0))
                {
                    retry_ini--;
					platform_dev_init();
                }
            }
		}
        else //station off
        {
            if(hw_mode == SSV6XXX_HWM_STA)
                sta_mode_off(sta_station->vif_idx);

            if(hw_mode == SSV6XXX_HWM_SCONFIG)
                sconfig_mode_off(sta_station->vif_idx);

        }
    } while(0);

    if (mutexLock)
        OS_MutexUnLock(g_host_api_mutex);

	LOG_PRINTF("%s() at line(%d),ret(%d)\n",__FUNCTION__,__LINE__,ret);        
    return ret;
}

/**********************************************************
Function : ssv6xxx_wifi_station
Description: Setting station on or off
1. station on : The current status must be mode off (ap mode or station mode off)
2. station off : The current status must be Station on
*********************************************************/
H_APIs ssv6xxx_result ssv6xxx_wifi_station(u8 hw_mode, Sta_setting *sta_station)
{
    return _ssv6xxx_wifi_station(hw_mode,sta_station,TRUE);

}


void sta_mode_status(struct ap_sta_status_station *status_info,u8 vif_idx)
{
    ssv_vif *vif = &gDeviceInfo->vif[vif_idx];

	printk("%s(%d)\n",__FUNCTION__,__LINE__);
	//mac addr
    MEMCPY(status_info->selfmac, vif->self_mac,ETH_ALEN);
    status_info->apinfo.status = vif->m_info.StaInfo->status;

	printk("%s(%d)\n",__FUNCTION__,__LINE__);

    if((vif->m_info.StaInfo->status != DISCONNECT)&&(gDeviceInfo->recovering != TRUE))
    {
		printk("%s(%d)\n",__FUNCTION__,__LINE__);

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

		printk("%s(%d)\n",__FUNCTION__,__LINE__);		
		printk("%s(%d)vif->m_info.StaInfo->joincfg->sec_type((%d)\n",__FUNCTION__,__LINE__, vif->m_info.StaInfo->joincfg->sec_type);
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
		printk("%s(%d)\n",__FUNCTION__,__LINE__);
	}
	printk("%s(%d)\n",__FUNCTION__,__LINE__);
}
 
 
struct ssv6xxx_ieee80211_bss *sta_mode_find_ap_by_mac(ETHER_ADDR *mac)
{
    int i = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

	OS_MutexLock(gDeviceInfo->g_dev_info_mutex);

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

	OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);

    if (i == g_max_num_of_ap_list)
    {
        return NULL;
    }

    return  (void*)&(gdeviceInfo->ap_list[i]);
}
   
H_APIs ssv6xxx_result ssv6xxx_wifi_set_sta_no_bcn_timeout(u8 value)
{
    g_host_cfg.sta_no_bcn_timeout = value;
    LOG_PRINTF("STA no bcn timeout = %d (ms)\r\n",g_host_cfg.sta_no_bcn_timeout*500);
    return SSV6XXX_SUCCESS;
}

H_APIs struct ssv6xxx_ieee80211_bss *ssv6xxx_wifi_find_ap_ssid(struct cfg_80211_ssid *ssid)
{

    int i = 0;
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

	OS_MutexLock(gDeviceInfo->g_dev_info_mutex);

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

	OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);

    if (i == g_max_num_of_ap_list)
    {
        return NULL;
    }

    return  &(gdeviceInfo->ap_list[i]);
}

