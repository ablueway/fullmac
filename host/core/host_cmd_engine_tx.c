/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#endif

#define __SFILE__ "host_cmd_engine_tx.c"

#include <log.h>
#include <ssv_types.h>
#include <ssv_hal.h>
#include <pbuf.h>
#include <msgevt.h>
#include <cmd_def.h>
#if (AP_MODE_ENABLE == 1)        
#include <ap/ap.h>
#include <ap_info.h>
#include <ap_sta_info.h>
#endif
#include <os_wrapper.h>
#include <host_apis.h>
#include "host_cmd_engine.h"
#include "host_cmd_engine_priv.h"
#include "host_cmd_engine_sm.h"
#include "host_cmd_engine_tx.h"

#include <ssv_drv.h>
#include <dev.h>
#include <txrx_hdl.h>

extern s32 TXRXTask_FrameEnqueue(void* frame, u32 priority);
extern OsSemaphore ap_sta_on_off_sphr;
extern tx_result ssv6xxx_data_could_be_send(void *frame, bool bAPFrame, u32 TxFlags);

static const HCmdEng_TrapHandler sgTrapHandler[] =
{
	//{ SSV6XXX_HOST_CMD_SET_CONFIG, HCmdEng_set_config },
	//{ SSV6XXX_HOST_CMD_SET_OPMODE, HCmdEng_set_opmode },
	{ SSV6XXX_HOST_CMD_SET_CONFIG, NULL },
	{ SSV6XXX_HOST_CMD_SET_OPMODE, NULL },


};



void CmdEng_TxHdlCmd(void *frame)
{
    struct cfg_host_cmd *hCmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(frame);
    const HCmdEng_TrapHandler  *trap_entry = sgTrapHandler;
    u32 i;

    if(hCmd->h_cmd < SSV6XXX_HOST_SOC_CMD_MAXID)
    {
        trap_entry = sgTrapHandler;
        for(i=0; i<sizeof(sgTrapHandler)/sizeof(HCmdEng_TrapHandler); i++) {
            if (sgTrapHandler[i].hCmdID != hCmd->h_cmd)
                continue;

            if(sgTrapHandler[i].hTrapHandler)
                sgTrapHandler[i].hTrapHandler(hCmd);
            break;

        }

        if ((hCmd->h_cmd == SSV6XXX_HOST_CMD_SCAN) || (hCmd->h_cmd == SSV6XXX_HOST_CMD_JOIN) || (hCmd->h_cmd == SSV6XXX_HOST_CMD_LEAVE) )
        {

            OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
            gHCmdEngInfo->pending_cmd_seqno = hCmd->cmd_seq_no;
            gHCmdEngInfo->blockcmd_in_q = true;
            LOG_DEBUGF(LOG_CMDENG, ("[CmdEng]: Got Block cmd %d, start to pending cmd\r\n", gHCmdEngInfo->pending_cmd_seqno));
            OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);
        }
        //==============================================================
        {
            //Send to tx driver task
            if(false == TXRXTask_FrameEnqueue(frame, 0))
            {
                LOG_PRINTF("[CmdEng]snd cmd %d fail\r\n",hCmd->h_cmd);
                os_frame_free(frame);
            }
        }
        //==============================================================
        return;
    }
    else
    {
        if(hCmd->h_cmd == SSV6XXX_HOST_CMD_SET_STA_CFG)
        {
            struct stamode_setting * sta_mod = NULL;
            sta_mod = (struct stamode_setting *)hCmd->un.dat8;
            ssv6xxx_wifi_station(sta_mod->mode,sta_mod->sta_cfg);
            OS_SemSignal(ap_sta_on_off_sphr);
            os_frame_free(frame);
            return;
        }
#if (AP_MODE_ENABLE == 1)		
        else if (hCmd->h_cmd == SSV6XXX_HOST_CMD_SET_AP_CFG)
        {
            struct apmode_setting * ap_mod = NULL;
            ap_mod = (struct apmode_setting *)hCmd->un.dat8;
            ssv6xxx_wifi_ap((Ap_setting *)ap_mod->ap_cfg,ap_mod->step);
            OS_SemSignal(ap_sta_on_off_sphr);
            os_frame_free(frame);
            return;

        }
#endif
    }
#if (AP_MODE_ENABLE == 1)        
#if (AUTO_BEACON != 0)
    if (hCmd->h_cmd > SSV6XXX_HOST_SOC_CMD_MAXID)
#else
    if (hCmd->h_cmd == SSV6XXX_HOST_CMD_UPDATE_BEACON)
    {
        u8 update_bcn = 0;

        OS_MutexLock(gDeviceInfo->APInfo->g_dev_bcn_mutex);
        update_bcn = gDeviceInfo->APInfo->beacon_upd_need;
        OS_MutexUnLock(gDeviceInfo->APInfo->g_dev_bcn_mutex);

        if (update_bcn == 1)
        {
            if (0==ssv_hal_beacon_set(gDeviceInfo->APInfo->bcn, gDeviceInfo->APInfo->bcn_info.tim_cnt_oft))
            {
                OS_MutexLock(gDeviceInfo->APInfo->g_dev_bcn_mutex);
                gDeviceInfo->APInfo->beacon_upd_need = 0;
                OS_MutexUnLock(gDeviceInfo->APInfo->g_dev_bcn_mutex);

            }
            else
            {
                //post message to cmd_engine
                ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_UPDATE_BEACON, NULL, 0);
                LOG_DEBUGF(LOG_CMDENG|LOG_LEVEL_WARNING, ("Failed to complete the cmd. Send it again\n\r"));
            }
        }       
    }
    else if (hCmd->h_cmd > SSV6XXX_HOST_SOC_CMD_MAXID)
#endif
#endif
    {
        LOG_DEBUGF(LOG_CMDENG|LOG_LEVEL_WARNING, ("%s(): invalid host cmd: %d\r\n", __FUNCTION__, hCmd->h_cmd));
        os_frame_free(frame);
        return;
    }

    os_frame_free(frame);
}
