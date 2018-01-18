/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <ssv_timer.h>
#include <os.h>
#include <log.h>

#include "host_cmd_engine_priv.h"
#include <dev.h>
#include <cmd_def.h>
#include "mlme.h"

#define TIMER_COUNT_DOWN    (5*60*1000) //(ms)
#define TIME_STAMP_EXPIRE   (10*60*1000) //(ms)
//#define CHECK_OLDEST

extern u8 g_max_num_of_ap_list;
extern struct task_info_st g_host_task_info[];

#if (MLME_TASK==1)
struct task_info_st g_mlme_task_info[] =
{
    { "SSV_MLME",    (OsMsgQ)0, 8,   OS_MLME_TASK_PRIO,   MLME_TASK_STACK_SIZE, NULL, mlme_task},
};

s32 mlme_host_event_handler_(struct cfg_host_event *pPktInfo);
void sta_mode_handler(MsgEvent *pMsgEv);
void ap_mode_handler(MsgEvent *pMsgEv);
#endif

extern void os_timer_init(void);
extern void _cmd_wifi_scan (s32 argc, char *argv[]);

void sta_mode_ap_list_handler (void *data,struct ssv6xxx_ieee80211_bss *ap_list);
static void refresh_timer(u8 total_ap);
void sta_mode_scan_timer(void * data1,void * data2);
static void update_ap_info_evt (u16 apIndex ,enum ap_act apAct);

static inline u32 fine_tune_rcpi(struct ssv6xxx_ieee80211_bss * ap)
{
    if(ap->prev_rcpi)
    {
        ap->prev_rcpi = ((ap->rxphypad.rpci << RSSI_DECIMAL_POINT_SHIFT)
            + (((ap->prev_rcpi)<<RSSI_SMOOTHING_SHIFT) - (ap->prev_rcpi))) >> RSSI_SMOOTHING_SHIFT;
        ap->rxphypad.rpci = ((ap->prev_rcpi) >> RSSI_DECIMAL_POINT_SHIFT);
    }
    else
        ap->prev_rcpi = (ap->rxphypad.rpci << RSSI_DECIMAL_POINT_SHIFT);

    return (ap->rxphypad.rpci);
}

bool mlme_remove_ap(struct ssv6xxx_ieee80211_bss *bss)
{
	u32 i = 0;
    if(bss==NULL)
        return FALSE;

    OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);
    for(i=0;i<g_max_num_of_ap_list;i++)
    {
        if(MEMCMP((void*)(gDeviceInfo->ap_list[i].bssid.addr), (void*)(bss->bssid.addr), ETHER_ADDR_LEN) == 0)
        {
            update_ap_info_evt(i,AP_ACT_REMOVE_AP);
    		MEMSET((void *)&(gDeviceInfo->ap_list[i]),0,sizeof(struct ssv6xxx_ieee80211_bss));
        }
    }
    OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);
    return TRUE;
}
#if(ENABLE_DYNAMIC_RX_SENSITIVE==1)
bool dynamic_cci_update=TRUE;
#endif
void sta_mode_ap_list_handler (void *data,struct ssv6xxx_ieee80211_bss *ap_list)
{
    struct resp_evt_result *scan_res;
    struct ssv6xxx_ieee80211_bss *bss=NULL;
    struct ssv6xxx_ieee80211_bss *prev_bss=NULL;
    u32    current_tick=OS_GetSysTick();
    u8     i,j;
    u8     empty = 0xff;
    u8     duplicate = 0xff;
    
    u8     total_ap=g_max_num_of_ap_list;

#ifdef CHECK_OLDEST
	u8     oldest = 0;
#else
    struct ssv6xxx_ieee80211_bss *low_bss=NULL;        
    u8     low_rcip=0;
#endif
    char bss_mem[sizeof(struct ssv6xxx_ieee80211_bss)] = {0};

    if(data!=NULL)
    {
        scan_res = (struct resp_evt_result *)data;
        bss =&scan_res->u.scan.bss_info;
        //Hidden AP
        if(0==bss->ssid.ssid_len){
            //LOG_PRINTF("Ignore the hidden AP\r\n");
            return;
        }
    }
    //LOG_PRINTF("ssid=%s, channel id=%d\r\n",bss->ssid.ssid,bss->channel_id);
	/*get status*/
    for(i=0;i<g_max_num_of_ap_list;i++)
    {
        /*check empty by using channel id*/
        if(ap_list[i].channel_id==0)
        {
            empty=i;
            total_ap--;
        }
        /*check duplicate signal by using bssid*/
        else if((bss != NULL) &&
            MEMCMP((void*)(ap_list[i].bssid.addr), (void*)(bss->bssid.addr), ETHER_ADDR_LEN) == 0)
        {
            duplicate=i;
        }
        else
        {
            for(j=0;j<MAX_VIF_NUM;j++)
            {
                if(gDeviceInfo->vif[j].hw_mode != SSV6XXX_HWM_STA)
                    continue;
                /*check whether time stamp is expire or not*/
                if(time_after((unsigned long)(current_tick),
                            (unsigned long)(ap_list[i].last_probe_resp+TIME_STAMP_EXPIRE))
                    &&(STRCMP((char *)ap_list[i].bssid.addr,(char *)gDeviceInfo->vif[j].m_info.StaInfo->joincfg->bss.bssid.addr)!=0)
                    )
                {
                    update_ap_info_evt(i,AP_ACT_REMOVE_AP);
                    OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);
                    MEMSET((void *)&(ap_list[i]),0,sizeof(struct ssv6xxx_ieee80211_bss));
                    OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);
                    empty=i;
                }
            }
        }
    }
    
#ifndef CHECK_OLDEST
    low_bss=&ap_list[0];
    low_rcip=0;
#endif    

    for(i=0;i<g_max_num_of_ap_list;i++)
    {
#ifdef CHECK_OLDEST
        /*check whether "i" is the oldest one*/
        if(time_after((unsigned long)(ap_list[oldest].last_probe_resp),
        (unsigned long)(ap_list[i].last_probe_resp)))
        {
            oldest=i;
        }
#else
        /*check whether "i" is the lowest RCPI
        the closer AP, the lower rcpi value
        */
        if(ap_list[i].channel_id!=0)
        {
            if((ap_list[i].rxphypad.rpci)>(low_bss->rxphypad.rpci))
            {
                low_bss=&ap_list[i];
                low_rcip=i;
            }
        }
#endif    
    }
    
    /*record status*/
    if(bss != NULL)
    {
        /*  "CHECK_OLDEST" = priority : duplicate> empty> rcpi>oldest ;  "else" = priority : duplicate> empty> rcpi   */
        if(duplicate!=0xff)
        {
            i=duplicate;
		}
        else if(empty!=0xff)
        {
            i=empty;
        }
#ifdef CHECK_OLDEST
        else
        {
            i = oldest;
            update_ap_info_evt(i,AP_ACT_REMOVE_AP);
            ap_list[i].prev_rcpi=0;
        }
#else
        else if((bss->rxphypad.rpci)<=(ap_list[low_rcip].rxphypad.rpci))
        {
            i=low_rcip;
            update_ap_info_evt(i,AP_ACT_REMOVE_AP);
            ap_list[i].prev_rcpi=0;
        }
        else
        {
            //if rcpi is not good enough, do nothing

            return;
        }
#endif

        #if 0
        prev_bss = (struct ssv6xxx_ieee80211_bss *)MALLOC(sizeof(struct ssv6xxx_ieee80211_bss));
        if (prev_bss==NULL)
        {
            LOG_DEBUGF(LOG_L2_STA|LOG_LEVEL_SERIOUS,("%s(): prev_bss malloc fail\r\n",__FUNCTION__));
            return;
        }
        #else
        prev_bss = (struct ssv6xxx_ieee80211_bss *)bss_mem;
        MEMSET((void *)prev_bss, 0, sizeof(struct ssv6xxx_ieee80211_bss));
        #endif
        OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);

        MEMCPY((void *)prev_bss,(void *)&ap_list[i],sizeof(struct ssv6xxx_ieee80211_bss));
        MEMCPY((void *)&ap_list[i],(void *)bss,sizeof(struct ssv6xxx_ieee80211_bss));

        ap_list[i].last_probe_resp=current_tick;
        ap_list[i].prev_rcpi=prev_bss->prev_rcpi;

        //LOG_PRINTF("a new scan result. prev:%d",ap_list[i].prev_rcpi);
        ap_list[i].rxphypad.rpci=fine_tune_rcpi(&ap_list[i]);
        //LOG_PRINTF(",now:%d (%d)\r\n",ap_list[i].rxphypad.rpci,bss->rxphypad.rpci);
        #if(ENABLE_DYNAMIC_RX_SENSITIVE==1)
        for(j=0;j<MAX_VIF_NUM;j++)
        {
            if((gDeviceInfo->vif[j].hw_mode == SSV6XXX_HWM_STA)&&(gDeviceInfo->vif[j].m_info.StaInfo)
                &&(gDeviceInfo->vif[j].m_info.StaInfo->status == CONNECT)
                &&(0==MEMCMP((void *)(&gDeviceInfo->vif[j].m_info.StaInfo->joincfg->bss.bssid),(void *)(&ap_list[i].bssid),ETHER_ADDR_LEN))
                &&(dynamic_cci_update==TRUE)
                )
            {
                ssv_hal_update_cci_setting(ap_list[i].rxphypad.rpci);
            }
        }
        if(dynamic_cci_update==FALSE)
        {
            ssv_hal_update_cci_setting(MAX_CCI_SENSITIVE);
        }
        #endif
        OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);

        if(duplicate==0xff)
        {
            update_ap_info_evt(i,AP_ACT_ADD_AP);
        }
        else
        {
            //if((STRCMP((char *)ap_list[i].ssid.ssid,(char *)prev_bss->ssid.ssid)!=0)||
            if((MEMCMP((void *)ap_list[i].ssid.ssid,(void *)prev_bss->ssid.ssid,ap_list[i].ssid.ssid_len)!=0)||
                (ap_list[i].channel_id != prev_bss->channel_id)||
                (ap_list[i].proto != prev_bss->proto)
                )
                {
                    update_ap_info_evt(i,AP_ACT_MODIFY_AP);
                }
        }
        #if 0
        FREE((void *)prev_bss);
        #endif
    }


    refresh_timer(total_ap);
    return;
} // end of - _scan_result_handler -

static inline void refresh_timer(u8 total_ap)
{
    os_cancel_timer(sta_mode_scan_timer, (u32)NULL, (u32)NULL);
#if (CONFIG_AUTO_SCAN==0)
    /* auto flush should be stopped when there is not exist any AP*/
    if(total_ap>0)
#endif
    {
        os_create_timer(TIMER_COUNT_DOWN, sta_mode_scan_timer, NULL, NULL, (void*)TIMEOUT_TASK);
    }
}

void sta_mode_scan_timer(void * data1,void * data2)
{
#if ( CONFIG_AUTO_SCAN == 1)
    /* do regular IW Scan*/
    struct cfg_scan_request *ScanReq = NULL;
    ScanReq = (void *)MALLOC(sizeof(*ScanReq));

    if (ScanReq==NULL)
    {
        LOG_DEBUGF(LOG_L2_STA|LOG_LEVEL_SERIOUS,("%s(): ScanReq malloc fail\r\n",__FUNCTION__));
        return;
    }
    ScanReq->is_active      = true;
    ScanReq->n_ssids        = 0;
    ScanReq->channel_mask   = 0xfff;    // scan channel 1~11
    ScanReq->dwell_time     = 0;

    if (ssv6xxx_wifi_scan(ScanReq) < 0)
    {
       	LOG_DEBUGF(LOG_L2_STA|LOG_LEVEL_SERIOUS,("%s(): ssv6xxx_wifi_scan fail.\r\n",__FUNCTION__));
    }
    FREE(ScanReq);
#else
    /*do regular refresh AP list table*/
    sta_mode_ap_list_handler(NULL,gDeviceInfo->ap_list);
#endif
}

static inline void update_ap_info_evt (u16 apIndex ,enum ap_act apAct)
{
    u8 i;

    ap_info_state apInfoState ;

    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if(gDeviceInfo->vif[i].hw_mode == SSV6XXX_HWM_STA)
        {
            break;
        }
    }
    if(i==MAX_VIF_NUM) //No STA vif
        return;

    apInfoState.apInfo=gDeviceInfo->ap_list;
    apInfoState.index=apIndex;
    apInfoState.act=apAct;

    for (i=0;i<HOST_EVT_CB_NUM;i++)
    {
        evt_handler handler = gDeviceInfo->evt_cb[i];
        if (handler) {
            handler(SOC_EVT_SCAN_RESULT, &apInfoState,(s32)sizeof(ap_info_state));
        }
    }
}

void mlme_clear_ap_list(void)
{
    OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);

    /*clean gDeviceInfo ap_list record*/
    MEMSET((void*)gDeviceInfo->ap_list,0,sizeof(struct ssv6xxx_ieee80211_bss)*g_max_num_of_ap_list);


    OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);
}

void mlme_sta_mode_deinit(void)
{

    /*stop scan timer*/
    os_cancel_timer(sta_mode_scan_timer, (u32)NULL, (u32)NULL);

    mlme_clear_ap_list();
}

#if(MLME_TASK==1)

void mlme_sta_mode_init(void)
{
    mlme_clear_ap_list();
}

s32 mlme_init(void)
{
    if (g_mlme_task_info[0].qlength> 0) {
        ASSERT(OS_MsgQCreate(&g_mlme_task_info[0].qevt,
        (u32)g_mlme_task_info[0].qlength)==OS_SUCCESS);
    }

    /* Create Registered Task: */
    OS_TaskCreate(g_mlme_task_info[0].task_func,
    g_mlme_task_info[0].task_name,
    g_mlme_task_info[0].stack_size<<4,
    g_mlme_task_info[0].args,
    g_mlme_task_info[0].prio,
    NULL);

	mlme_sta_mode_init();

	return OS_SUCCESS;
}

void mlme_task( void *args )
{
    s32 res,hw_mode;
    MsgEvent *pMsgEv = NULL;

    while(1)
    {
        res = msg_evt_fetch(MBOX_MLME_TASK, &pMsgEv);
        ASSERT(res == OS_SUCCESS);

		OS_MutexLock(&gDeviceInfo->g_dev_info_mutex);
		hw_mode=gDeviceInfo->hw_mode;
		OS_MutexUnLock(&gDeviceInfo->g_dev_info_mutex);

		if(pMsgEv)
        {
            if(hw_mode == SSV6XXX_HWM_STA)
            {
                sta_mode_handler(pMsgEv);
            }
            else
            {
                ap_mode_handler(pMsgEv);
            }
            os_msg_free(pMsgEv);
        }
    }
}

static inline void sta_mode_handler(MsgEvent *pMsgEv)
{
    void* pMsgData = NULL;

    switch(pMsgEv->MsgType)
    {
        case MEVT_PKT_BUF:
            switch(pMsgEv->MsgData1)
            {
                case SOC_EVT_SCAN_RESULT:
					pMsgData = (void*)pMsgEv->MsgData;
                    sta_mode_ap_list_handler(pMsgData,gDeviceInfo->ap_list);
                    FREE(pMsgData);
                    break;
                default:
                    break;
            }
            break;
        case MEVT_HOST_TIMER:
            {
                os_timer_expired((void *)pMsgEv);
            }
            break;
        default:
            break;
    }
}

static inline void ap_mode_handler(MsgEvent *pMsgEv)
{

    switch(pMsgEv->MsgType)
    {
        case MEVT_PKT_BUF:
            AP_RxHandleAPMode(pMsgEv->MsgData);
            break;
        case MEVT_BCN_CMD:
            GenBeacon();
            break;
        case MEVT_HOST_TIMER:
        {
            os_timer_expired((void *)pMsgEv);
        }
            break;
        default:
            break;
    }


}

s32 mlme_host_event_handler_(struct cfg_host_event *pPktInfo)
{

    MsgEvent *pMsgEv= NULL;
    void     *pRecvData = NULL;
    u32 len = pPktInfo->len-sizeof(HDR_HostEvent);

    pRecvData =  MALLOC(len);
    if(NULL==pRecvData)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return -1;
    }
    MEMCPY(pRecvData,pPktInfo->dat,len);

    pMsgEv = msg_evt_alloc();
    if(NULL!=pMsgEV)
    {
        pMsgEv->MsgType  = MEVT_PKT_BUF;
        pMsgEv->MsgData  = (u32)pRecvData;
        pMsgEv->MsgData1 = SOC_EVT_SCAN_RESULT;
        pMsgEv->MsgData2 = 0;
        pMsgEv->MsgData3 = 0;
        msg_evt_post(MBOX_MLME_TASK, pMsgEv);

        return 0;
    }
    else
    {
        LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }
} // end of - _host_event_handler -

s32 ap_mlme_handler( void *frame, msgevt_type MsgType)
{
    MsgEvent *pMsgEv= NULL;
    pMsgEv = msg_evt_alloc();
    if(NULL!=pMsgEv)
    {
        pMsgEv->MsgType  = MsgType;
        pMsgEv->MsgData  = (u32)frame;
        pMsgEv->MsgData1 = 0;
        pMsgEv->MsgData2 = 0;
        pMsgEv->MsgData3 = 0;
        ASSERT(OS_SUCCESS == msg_evt_post(MBOX_MLME_TASK, pMsgEv));


        return SSV6XXX_SUCCESS;
    }
    else
    {
        LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
        return SSV6XXX_FAILED;
    }
} // end of - _host_event_handler -

#endif
