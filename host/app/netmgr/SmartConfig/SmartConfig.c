/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <log.h>
#include "os.h"
#include "porting.h"
#include "SmartConfig.h"
#include <ssv_lib.h>
#include "netmgr/net_mgr.h"
#include <cmd_def.h>

#if(ENABLE_SMART_CONFIG==1)
#define MAX_RETRY_AFTER_LOCK 3
SMART_COFIG_RESULT sres;

/*
Set default status
*/
EN_SCONFIG_STATUS enSCstat=EN_SCONFIG_NOT_READY;

/*
Local variables
*/
OsMutex SCMutex=0;
u32 StartTick=0;
u32 LastTick=0;
u32 CurTick=0;
u8 last_channel=0;
u8 lock_channel=0;
u8 current_channel=0;
u16 g_SconfigChannelMask=0;
u32 g_Sconfig5gChannelMask=0;
u16 _g_SconfigChannelMask=0;
u32 _g_Sconfig5gChannelMask=0;
u16 ChannelMask=0;
u32 Channel5gMask=0;
u32 g_channelInterval=10;
u32 g_restartInterval=6000;
u8 g_retryCountAfterLock=0;
extern int CusSmartConfigInit(void);
extern int CusSmartConfigDeInit(void);
extern int CusSmartConfigFlushData(void);
extern EN_SCONFIG_STATUS CusSmartConfigRxData(u8 *rx_buf, u32 rx_len);
extern int CustSmartConfigDone(void);
extern int CusSmartGetResult(SMART_COFIG_RESULT *sres);

u16 get_next_channel(void)
{
    u16 i=STA_DEFAULT_CHANNEL,j=0,k=0;
    u8 chose_2g=0;
REPEAT:
    i=STA_DEFAULT_CHANNEL; //start from the default channel
    j=0;

    for(;(i<MAX_2G_CHANNEL_NUM)&&(j<MAX_2G_CHANNEL_NUM);i=((i+1)%(MAX_2G_CHANNEL_NUM)),j++){
        if((ChannelMask&(1<<i))==0){
            continue;
        }
        ChannelMask=ChannelMask&(~(1<<i));
        chose_2g=1;
        break;
    }

    if(chose_2g!=1)
    {
        if(TRUE==ssv6xxx_wifi_support_5g_band())
        {
            for(k=0;k<MAX_5G_CHANNEL_NUM;k++)
            {
                if((Channel5gMask&(1<<k))==0){
                    continue;
                }
                Channel5gMask=Channel5gMask&(~(1<<k));  
                break;
            }
        }
        else
        {
            k=MAX_5G_CHANNEL_NUM;    
        }

        if((j==MAX_2G_CHANNEL_NUM)&&(k==MAX_5G_CHANNEL_NUM)){
            ChannelMask=_g_SconfigChannelMask;
            Channel5gMask=_g_Sconfig5gChannelMask;
            goto REPEAT;
        }
    }
        
    OS_MutexLock(SCMutex);
    if(chose_2g==1)
    {
        current_channel=ssv6xxx_wifi_ch_bitmask_to_num(FALSE, i);
    }
    else
    {
        current_channel=ssv6xxx_wifi_ch_bitmask_to_num(TRUE, k);    
    }
    OS_MutexUnLock(SCMutex);
    LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("==SmartConfig==: Change channel to %d\r\n",current_channel));
    return current_channel;
}

/*
Init your SmartConfig solution, and the state machine
*/
int _SmartConfigInit(void)
{
    s32 ret=0;

    /*===========================
    [Start] Init your SmartConfig solution.
    ============================*/
    ret=CusSmartConfigInit();
    if(ret<0){
        LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_WARNING,("==SmartConfig==: Init fail\r\n"));
        return -1;
    }
    /*===========================
    Init your SmartConfig solution. [End]
    ============================*/

    /*
        Init the variables for state machine of SmartConfig
    */
    MEMSET((void *)&sres,0,sizeof(SMART_COFIG_RESULT));

    ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_SCONFIG, &_g_SconfigChannelMask, &_g_Sconfig5gChannelMask);
    StartTick=OS_GetSysTick();
    LastTick=StartTick;
    CurTick=StartTick;
    ChannelMask=_g_SconfigChannelMask;
    Channel5gMask=_g_Sconfig5gChannelMask;
    
    //Force to start from the current channel in the first time
    if(enSCstat==EN_SCONFIG_NOT_READY){
        ChannelMask|=(1<<STA_DEFAULT_CHANNEL);
    }

    /*
        Choose the first channel, and set channel
    */
    lock_channel=0xFF;
    last_channel=current_channel=get_next_channel();
    ssv6xxx_wifi_set_channel(current_channel,SSV6XXX_HWM_SCONFIG);

    /*
        Set status of state machine to "EN_SCONFIG_GOING"
    */
    enSCstat=EN_SCONFIG_GOING;

    return 0;

}

int SmartConfigInit(void)
{
    _g_SconfigChannelMask=g_SconfigChannelMask;
    _g_Sconfig5gChannelMask=g_Sconfig5gChannelMask;
    ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_SCONFIG, &_g_SconfigChannelMask, &_g_Sconfig5gChannelMask);
    
    if(SCMutex==0)
        OS_MutexInit(&SCMutex);

    enSCstat=EN_SCONFIG_NOT_READY;

    return _SmartConfigInit();
}

/*
Deinit your SmartConfig solution, set the status of state machine, and release the resources
*/
void _SmartConfigDeinit(void)
{
    /*===========================
    [Start] Deinit your SmartConfig solution.
    ============================*/
    CusSmartConfigDeInit();
    /*===========================
    DeInit your SmartConfig solution. [End]
    ============================*/

    return;
}
void SmartConfigDeinit(void)
{
    g_retryCountAfterLock=0;

    /*
        Relase the local resources.
    */
    if(SCMutex!=0){
        OS_MutexLock(SCMutex);
        enSCstat=EN_SCONFIG_NOT_READY;
        OS_MutexUnLock(SCMutex);

        OS_MutexDelete(SCMutex);
        SCMutex=0;
    }

    _SmartConfigDeinit();
    return;
}

/*
The state machine of SmartConfig.
*/
extern struct task_info_st st_netmgr_task[];
int SmartConfigSM(void)
{

    /*
        Check the status
    */
    if(enSCstat==EN_SCONFIG_NOT_READY){
        LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_WARNING,("==SmartConfig==: Status is not ready\r\n"));
        return 0;
    }

    /*
        Run the deinit function, and switch to the STA mode when the status is "EN_SCONFIG_DONE"
    */
    if(enSCstat==EN_SCONFIG_DONE){
        struct resp_evt_result *sconfig_done_cpy;
        LOG_PRINTF("==SmartConfig==: status is done\r\n");
        sconfig_done_cpy = OS_MemAlloc(sizeof(struct resp_evt_result));
        if(sconfig_done_cpy!=NULL)
        {
            sconfig_done_cpy->u.sconfig_done.ssid_len=sres.ssid_len;
            MEMCPY(sconfig_done_cpy->u.sconfig_done.ssid,sres.ssid,sres.ssid_len);
            STRNCPY((char *)sconfig_done_cpy->u.sconfig_done.pwd, (const char *)sres.key,sres.key_len);
            sconfig_done_cpy->u.sconfig_done.channel=lock_channel;
            sconfig_done_cpy->u.sconfig_done.result_code=0;

            exec_host_evt_cb(SOC_EVT_SCONFIG_SCAN_DONE,sconfig_done_cpy,sizeof(struct resp_evt_result));
        }
        else
        {
            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        }
        OS_MemFree(sconfig_done_cpy);
        _SmartConfigDeinit();
        return 0;
    }

    CurTick=OS_GetSysTick();

    /*
        If we can't run into "EN_SCONFIG_DONE" in 60s, we restart the SmartConfig
    */
    if(((CurTick-StartTick)>=g_restartInterval)&&
        ((enSCstat==EN_SCONFIG_GOING)||(enSCstat==EN_SCONFIG_LOCK))){ //default setting is 60s
        //Re-start
        LOG_PRINTF("==SmartConfig==: Restart,%d %d\r\n",CurTick,StartTick);

        if((_g_SconfigChannelMask!=g_SconfigChannelMask)&&(_g_Sconfig5gChannelMask!=g_Sconfig5gChannelMask)){
            //In this case, SmartConfig only scan the neighbor channel of lock_channel
            g_retryCountAfterLock++;
        }

        if(g_retryCountAfterLock%MAX_RETRY_AFTER_LOCK==0){
            //reset _g_SconfigChannelMask
            _g_SconfigChannelMask=g_SconfigChannelMask;
            _g_Sconfig5gChannelMask=g_Sconfig5gChannelMask;
            ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_SCONFIG, &_g_SconfigChannelMask, &_g_Sconfig5gChannelMask);
            g_retryCountAfterLock=0;
        }
        _SmartConfigDeinit();
        _SmartConfigInit();
        return 1;
    }

    /*
        Before running into "EN_SCONFIG_LOCK ", we change the channel every 100 ms
    */
    if(((CurTick - LastTick)>=g_channelInterval)&&
        (enSCstat==EN_SCONFIG_GOING)){// default setting is  100ms
        //airkiss_change_channel(&g_smtcfg);
        ssv6xxx_wifi_set_channel(get_next_channel(),SSV6XXX_HWM_SCONFIG);
        LastTick=CurTick;
        return 1;
    }

    return 1;
}

/*
Feed the rx data to your SmartConfig solution.
*/
//u32 rx_count=0;
void SmartConfigPaserData(u8 channel, u8 *rx_buf, u32 len)
{
    EN_SCONFIG_STATUS ret;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};
    int ch_bit=0;

    //LOG_PRINTF("SmartConfig passer data \r\n");
    /*
        Check the status
    */
    if((enSCstat==EN_SCONFIG_NOT_READY)||(enSCstat==EN_SCONFIG_DONE)){
        return;
    }

    /*
        Drop the rx data packet if this packet is not from lock_channel
    */
    if(enSCstat==EN_SCONFIG_LOCK){
        if(lock_channel!=channel){
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("==SmartConfig==: Drop packet.(From channel %d, locking channel %d)\r\n",channel,lock_channel));
            return;
        }
    }

    /*
        Flush channel if your SmartConfig solution offers this function.
    */
    if(last_channel!=channel){
        LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("==SmartConfig==: Flush data.(current channel = %d, last channel = %d)\r\n",channel,last_channel));
        CusSmartConfigFlushData();
        last_channel=channel;
    }

    /*=======================================================
    [Start] Feed the rx data to your SmartConfig solution, and check it's status
    ========================================================*/
    //rx_count++;
    //if(rx_count==30){
    //    LOG_PRINTF("==SmartConfig==: Feed the rx data ... \r\n");
    //    rx_count=0;
    //}
    //LOG_PRINTF("==SmartConfig==: ADDR2:%2x:%2x:%2x:%2x:%2x:%2x\r\n",rx_buf[10],rx_buf[11],rx_buf[12],rx_buf[13],rx_buf[14],rx_buf[15]);
    ret = CusSmartConfigRxData(rx_buf,len);
    /*=======================================================
    Feed the rx data to your SmartConfig solution, and check it's status [End]
    ========================================================*/
    if(EN_SCONFIG_LOCK==ret){
        OS_MutexLock(SCMutex);
        enSCstat=EN_SCONFIG_LOCK;
        OS_MutexUnLock(SCMutex);

        if(lock_channel!=channel){ //There are not any specail reasons for this condiction checking, I just want to output this messages one times.
            LOG_PRINTF("\33[32m==SmartConfig==: Lock on channel %d\33[0m\r\n",channel);
        }

        lock_channel=channel;
        /*
            Sometimes we can't get the complete status after airkiss return the lock.
            The state machine will restart the scanning, I think we don't need to scan whole channel  in this case.
            we just need to scan the previos and after channel of locking channel
        */
        _g_Sconfig5gChannelMask=0;
        _g_SconfigChannelMask=0;
        ch_bit=ssv6xxx_wifi_ch_to_bitmask(lock_channel);
        if(ch_bit!=-1)
        {
            if(IS_5G_BAND(lock_channel))
            {
                _g_Sconfig5gChannelMask|=(1<<ch_bit);

                if(ch_bit>1)
                    _g_Sconfig5gChannelMask|=(1<<(ch_bit-1));                

                if(ch_bit>2)
                    _g_Sconfig5gChannelMask|=(1<<(ch_bit-2)); 
                
                if(ch_bit<(MAX_5G_CHANNEL_NUM-2))
                    _g_Sconfig5gChannelMask|=(1<<(ch_bit+2));
                
                if(ch_bit<(MAX_5G_CHANNEL_NUM-1))
                    _g_Sconfig5gChannelMask|=(1<<(ch_bit+1));
            }
            else
            {
                _g_SconfigChannelMask|=(1<<ch_bit);

                if(ch_bit>1)
                    _g_SconfigChannelMask|=(1<<(ch_bit-1));                

                if(ch_bit>2)
                    _g_SconfigChannelMask|=(1<<(ch_bit-2)); 
                
                if(ch_bit<(MAX_2G_CHANNEL_NUM-2))
                    _g_SconfigChannelMask|=(1<<(ch_bit+2));
                
                if(ch_bit<(MAX_2G_CHANNEL_NUM-1))
                    _g_SconfigChannelMask|=(1<<(ch_bit+1));
            }

            ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_SCONFIG, &_g_SconfigChannelMask, &_g_Sconfig5gChannelMask);
        }
        /*
            Change to lock channel if the lock channel and current channel are not the same
        */
        if(lock_channel!=current_channel){
            OS_MutexLock(SCMutex);
            current_channel=lock_channel;
            OS_MutexUnLock(SCMutex);
            LOG_DEBUGF(LOG_SCONFIG|LOG_LEVEL_ALL,("==SmartConfig==: Change channel to %d\r\n",lock_channel));
            ssv6xxx_wifi_set_channel(lock_channel,SSV6XXX_HWM_SCONFIG);
        }

    }else if(EN_SCONFIG_DONE==ret){
        OS_MutexLock(SCMutex);
        enSCstat=EN_SCONFIG_DONE;
        OS_MutexUnLock(SCMutex);

        LOG_PRINTF("\33[32m==SmartConfig==: Completed\33[0m\r\n");
        /*=============================================
        [Start] Get the AP information from your SmartConfig solution
        ==============================================*/
        CusSmartGetResult(&sres);
        MEMCPY((void*)ssid_buf,(void*)sres.ssid,sres.ssid_len);
        LOG_PRINTF("ssid=%s\r\n",ssid_buf);
        LOG_PRINTF("ssid_length=%u\r\n",sres.ssid_len);
        LOG_PRINTF("pwd=%s\r\n",sres.key);
        LOG_PRINTF("pwd_length=%u\r\n",sres.key_len);
        /*=============================================
        Get the AP information from your SmartConfig solution [End]
        ==============================================*/

    }


    return;
}

/*
    Response the finish status to smart phone
*/
void SmartConfigConnect(void)
{
    /*==================================================================
    [Start] You can do something after get the IP address (ex: broadcast data to smartphone
    ====================================================================*/
    CustSmartConfigDone();
    /*==================================================================
    You can do something after get the IP address (ex: broadcast data to smartphone [End]
    ====================================================================*/
}
#else //#if(ENABLE_SMART_CONFIG==1)
int SmartConfigInit(void)
{
	return 0;
}
int SmartConfigSM(void)
{
	return 0;
}
void SmartConfigDeinit(void)
{
	return;
}
void SmartConfigPaserData(u8 channel, u8 *rx_buf, u32 len)
{
	return;
}
void SmartConfigConnect(void)
{
	return;
}
#endif //#if(ENABLE_SMART_CONFIG==1)
