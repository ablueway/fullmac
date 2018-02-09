/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
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
#include <txrx_hdl_tx.h>
#include <txrx_hdl_rx.h>


#include <cmd_def.h>
#include <ssv_dev.h>
#include <ssv_hal.h>


#if (AP_MODE_ENABLE == 1)
#include "ap_config.h"
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
extern s32 send_deauth_and_remove_sta(u8 *hwaddr);
extern tx_result ssv6xxx_data_could_be_send(struct cfg_host_txreq0 *host_txreq0, 
													bool bAPFrame, u32 TxFlags);
#endif

/* extern variables */
extern struct Host_cfg g_host_cfg;
extern OsMutex g_host_api_mutex;
extern HOST_API_STATE active_host_api;
extern u8 g_max_num_of_ap_list;
//extern u8 config_mac[];

/* extern functions */
extern s32 _ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len, const bool mutexLock);


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
        
            if(check_efuse_chip_id() != SSV6XXX_SUCCESS)
            {
                return SSV6XXX_FAILED;
            }
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
        OS_MutexLock(g_host_api_mutex);

    do {

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

        if(ap_setting->status)//AP on
        {
            if( HOST_API_ACTIVE ==active_host_api&&step==0)
                if(gDeviceInfo->vif[ap_setting->vif_idx].hw_mode == SSV6XXX_HWM_AP)
                    ap_mode_off(ap_setting->vif_idx);


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
        OS_MutexUnLock(g_host_api_mutex);

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

ssv6xxx_result  _ssv6xxx_wifi_ap_pause(bool pause,const bool mutexLock)
{
#if (AP_MODE_ENABLE == 1)    
//    ssv6xxx_result ret = SSV6XXX_SUCCESS;
    //Detect host API status
    if(mutexLock)
        OS_MutexLock(g_host_api_mutex);

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
        OS_MutexUnLock(g_host_api_mutex);
#else
	return SSV6XXX_SUCCESS;
#endif
}

H_APIs ssv6xxx_result ssv6xxx_wifi_pause(bool pause)
{
    _ssv6xxx_wifi_ap_pause(pause,TRUE);
	return 0;
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
