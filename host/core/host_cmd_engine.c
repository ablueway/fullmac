/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#include <linux/kthread.h>
#endif

#define __SFILE__ "host_cmd_engine.c"

#include <host_config.h>
#include <ssv_types.h>
#include <os_wrapper.h>
#include <host_apis.h>
#include <ssv_timer.h>
#include <ssv_dev.h>
#include <ssv_hal.h>
#if (AP_MODE_ENABLE == 1)        
#include <ap/ap.h>
#endif
#include <pbuf.h>
#include <msgevt.h>
#include <cmd_def.h>
#include <ssv_drv.h>		// for ssv6xxx_drv_send()
#include <txrx_hdl.h>
#include <log.h>

#if (_WIN32 == 1)
#include <wtypes.h>
#include <Dbt.h>
#endif

#include "host_cmd_engine.h"
#include "host_cmd_engine_priv.h"
#include "host_cmd_engine_sm.h"

#define FREE_FRM_NUM 5
extern struct task_info_st g_host_task_info[];

struct CmdEng_st
{
    ModeType mode;
    u32 BlkCmdNum;
    bool BlkCmdIn;
};


HostCmdEngInfo_st *gHCmdEngInfo;
#define STATE_MACHINE_DATA struct HostCmdEngInfo




extern void os_timer_init(os_timer_st *timer);






//-------------------------------------------------------------
extern void CmdEng_RxHdlEvent(struct cfg_host_event *pHostEvt);
extern void CmdEng_TxHdlCmd(struct cfg_host_cmd *pPktInfo);






/*
void pendingcmd_expired_handler(void* data1, void* data2)
{
    HostCmdEngInfo_st *info = (HostCmdEngInfo_st *)data1;
    OS_MutexLock(info->CmdEng_mtx);
    if(info->debug != false)
        LOG_DEBUG("[CmdEng]: pending cmd %d timeout!! \n", info->pending_cmd_seqno);

    info->pending_cmd_seqno = 0;
    info->blockcmd_in_q = false;
    OS_MutexUnLock(info->CmdEng_mtx);
}
*/
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void CmdEng_HandleQueue(void *frame)
{

	//event/frames(data/ctrl/mgmt)/cmd
	//pPktInfo;
	CFG_HOST_RXPKT * rxpkt = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);//(struct cfg_host_rxpkt *)pPktInfo;
	u8 c_type=ssv_hal_get_rxpkt_ctype(rxpkt);
	switch (c_type)
	{
		//-------------
		//RX data
#if(MLME_TASK==0)
	case M0_RXEVENT:
    {
        if ((SSV6XXX_HWM_AP == gDeviceInfo->vif[0].hw_mode)||
            (SSV6XXX_HWM_AP == gDeviceInfo->vif[1].hw_mode))
        {
#if (AP_MODE_ENABLE == 1)            
            AP_RxHandleAPMode(frame);
#endif
        }
		else
        {
            LOG_DEBUGF(LOG_CMDENG|LOG_LEVEL_WARNING, ("Unhandle M0_RXEVENT \r\n"));
            os_frame_free(frame);
        }
        //    CmdEng_RxHdlData(frame);
		break;
    }
#endif
	case HOST_EVENT:
		CmdEng_RxHdlEvent(frame);
		break;
		//-------------
		//TX
	case HOST_CMD:
    {
        FrmQ *pcmd = NULL;
        struct cfg_host_cmd *hCmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(frame);

        if ((gHCmdEngInfo->blockcmd_in_q == true)&&
            (hCmd->h_cmd != SSV6XXX_HOST_CMD_SET_STA_CFG)&&
            (hCmd->h_cmd != SSV6XXX_HOST_CMD_SET_AP_CFG))
        {
            if( (pcmd = (FrmQ *)list_q_deq_safe(&gHCmdEngInfo->free_FrmQ, &gHCmdEngInfo->CmdEng_mtx)) == NULL)
                        pcmd = (FrmQ *)MALLOC(sizeof(FrmQ));
            if(pcmd!=NULL)
            {
                                pcmd->frame = frame;
                list_q_qtail_safe(&gHCmdEngInfo->pending_cmds, (struct ssv_list_q *)pcmd, &gHCmdEngInfo->CmdEng_mtx);

                LOG_DEBUGF(LOG_CMDENG, ("[CmdEng]: Pending cmd %d\r\n", hCmd->cmd_seq_no));
            }
            else
            {
                LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
            }
            return;
        }
        else
            CmdEng_TxHdlCmd(frame);
		break;
    }
	default:
	    hex_dump(rxpkt,128);
		LOG_FATAL("Unexpect c_type %d appeared\r\n", c_type);
		ASSERT(0);
	}

}

ssv6xxx_result CmdEng_GetStatus(void *stp)
{
    struct CmdEng_st *st = (struct CmdEng_st *)stp;
    if(st == NULL)
        return SSV6XXX_INVA_PARAM;

    OS_MutexLock(&gHCmdEngInfo->CmdEng_mtx);
    st->mode = gHCmdEngInfo->curr_mode;
    st->BlkCmdIn = gHCmdEngInfo->blockcmd_in_q;
    st->BlkCmdNum = list_q_len(&gHCmdEngInfo->pending_cmds);
    OS_MutexUnLock(&gHCmdEngInfo->CmdEng_mtx);

    return SSV6XXX_SUCCESS;
}








//---------------------------------------------------------------------------------------------
extern s32 AP_Start(void);
extern s32 AP_Stop( void );
#ifdef __linux__
int CmdEng_Task(void *data)
{
	MsgEvent *MsgEv;
	s32 res;
	extern u32 g_RunTaskCount;
    CFG_HOST_RXPKT *rxpkt= NULL;
    u32 msgData = 0;
	LOG_TRACE("%s() Task started.\r\n", __FUNCTION__);
	g_RunTaskCount++;

	while(!kthread_should_stop())
	{
        if ((gHCmdEngInfo->blockcmd_in_q == false) 
			&& (list_q_len_safe(&gHCmdEngInfo->pending_cmds, &(gHCmdEngInfo->CmdEng_mtx)) > 0))
        {
            //Proceeding pending cmds
            FrmQ *pcmd = NULL;
            struct cfg_host_cmd *hCmd = NULL;

            pcmd = (FrmQ *)list_q_deq_safe(&gHCmdEngInfo->pending_cmds, &gHCmdEngInfo->CmdEng_mtx);
            hCmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(pcmd->frame);
            LOG_DEBUGF(LOG_CMDENG, ("[CmdEng]: Pop pending cmd %d to execute\r\n", hCmd->cmd_seq_no));
            CmdEng_TxHdlCmd(pcmd->frame);

			OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
            if (list_q_len(&gHCmdEngInfo->free_FrmQ) < FREE_FRM_NUM)
            {    
            	list_q_qtail(&gHCmdEngInfo->free_FrmQ, (struct ssv_list_q *)pcmd);
            }
			else 
			{
	            FREE(pcmd);
			}
			OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);
        }
        else
        {
            /* Wait Message: */
            res = msg_evt_fetch(MBOX_CMD_ENGINE, &MsgEv);
            ASSERT(res == OS_SUCCESS);

    		//LOG_TRACE("AP needs to handle msg:%d.\n", MsgEv->MsgType);
            switch (MsgEv->MsgType)
            {
            case MEVT_PKT_BUF:
                rxpkt=(CFG_HOST_RXPKT *)MsgEv->MsgData;
                os_msg_free(MsgEv);
                CmdEng_HandleQueue(rxpkt);
                break;

            /**
            *  Message from software timer timeout event.
            */
            case MEVT_HOST_TIMER:
                os_timer_expired((void *)MsgEv);
                os_msg_free(MsgEv);
                break;


            case MEVT_HOST_CMD:
                msgData=MsgEv->MsgData;
                msg_evt_free(MsgEv);
                switch(msgData)
                {
#if (AP_MODE_ENABLE == 1)
                case AP_CMD_AP_MODE_ON:
                    AP_Start();
                    break;

                case AP_CMD_AP_MODE_OFF:
                    AP_Stop();
                    break;
#endif                    
#ifdef __TEST_DATA__
                case AP_CMD_ADD_STA:
                    TestCase_AddAPSta();
                    break;

                case AP_CMD_PS_POLL_DATA:
                    TestCase_SendPSPoll();
                    break;

                case AP_CMD_PS_TRIGGER_FRAME:
                    TestCase_SendTriggerFrame();
                    break;
#endif//__TEST_DATA__
                default:
                    break;
                }

                break;
            default:
                //SoftMac_DropPkt(MsgEv);
                LOG_DEBUGF(LOG_CMDENG, ("%s(): unknown message type(%02x) !!\r\n",
                    __FUNCTION__, MsgEv->MsgType));
                break;
            };
        }
	}
	return 0;
}
#else
void CmdEng_Task(void *args)
{
	MsgEvent *MsgEv;
	s32 res;
	extern u32 g_RunTaskCount;
    CFG_HOST_RXPKT *rxpkt=NULL;
    u32 msgData=0;
	LOG_TRACE("%s() Task started.\r\n", __FUNCTION__);
	g_RunTaskCount++;

#ifdef __TEST__
	//_Cmd_CreateSocketClient(0, NULL);

#endif

	//SSVHostCmdEng_Start();
	//	SM_ENTER(HCMDE, IDLE, NULL);

	while (1)
	{
        if ((gHCmdEngInfo->blockcmd_in_q == false) && (list_q_len_safe(&gHCmdEngInfo->pending_cmds, &(gHCmdEngInfo->CmdEng_mtx)) > 0))
        {
            //Proceeding pending cmds
            FrmQ *pcmd = NULL;
            struct cfg_host_cmd *hCmd = NULL;

            pcmd = (FrmQ *)list_q_deq_safe(&gHCmdEngInfo->pending_cmds, &gHCmdEngInfo->CmdEng_mtx);
            hCmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(pcmd->frame);
            LOG_DEBUGF(LOG_CMDENG, ("[CmdEng]: Pop pending cmd %d to execute\r\n", hCmd->cmd_seq_no));
            CmdEng_TxHdlCmd(pcmd->frame);

			OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
            if (list_q_len(&gHCmdEngInfo->free_FrmQ) < FREE_FRM_NUM)
            {    
            	list_q_qtail(&gHCmdEngInfo->free_FrmQ, (struct ssv_list_q *)pcmd);
            }
			else 
			{
	            FREE(pcmd);
			}
			OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);
        }
        else
        {
            /* Wait Message: */
            res = msg_evt_fetch(MBOX_CMD_ENGINE, &MsgEv);
            ASSERT(res == OS_SUCCESS);

    		//LOG_TRACE("AP needs to handle msg:%d.\n", MsgEv->MsgType);
            switch (MsgEv->MsgType)
            {
            case MEVT_PKT_BUF:
                rxpkt=(CFG_HOST_RXPKT *)MsgEv->MsgData;
                os_msg_free(MsgEv);
                CmdEng_HandleQueue(rxpkt);
                break;

                /**
                *  Message from software timer timeout event.
                */
            case MEVT_HOST_TIMER:
                os_timer_expired((void *)MsgEv);
                os_msg_free(MsgEv);
                break;


            case MEVT_HOST_CMD:
                msgData=MsgEv->MsgData;
                msg_evt_free(MsgEv);
                switch(msgData)
                {
#if (AP_MODE_ENABLE == 1)
                case AP_CMD_AP_MODE_ON:
                    AP_Start();
                    break;

                case AP_CMD_AP_MODE_OFF:
                    AP_Stop();
                    break;
#endif                    
#ifdef __TEST_DATA__
                case AP_CMD_ADD_STA:
                    TestCase_AddAPSta();
                    break;

                case AP_CMD_PS_POLL_DATA:
                    TestCase_SendPSPoll();
                    break;

                case AP_CMD_PS_TRIGGER_FRAME:
                    TestCase_SendTriggerFrame();
                    break;
#endif//__TEST_DATA__
                default:
                    break;
                }

                break;
            default:
                //SoftMac_DropPkt(MsgEv);
                LOG_DEBUGF(LOG_CMDENG, ("%s(): unknown message type(%02x) !!\r\n",
                    __FUNCTION__, MsgEv->MsgType));
                break;
            };
        }
	}
}
#endif

#if (_WIN32 == 1 && CONFIG_RX_POLL == 0)
#define INITGUID
#include <guiddef.h>
WNDPROC wpOrigProc;
DEFINE_GUID(GUID_DEVINTERFACE_ssvsdio_intevent,
			0x76c8ffb9, 0xf552, 0x4398, 0xa3, 0xd6, 0x52, 0x73, 0xfe, 0xc5, 0x5b, 0xe2);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_DEVICECHANGE:
		{
			if ( wParam == DBT_CUSTOMEVENT )
			{
				DEV_BROADCAST_HANDLE* handle = (DEV_BROADCAST_HANDLE*)lParam;
				if ( handle->dbch_devicetype == DBT_DEVTYP_HANDLE && IsEqualGUID(&handle->dbch_eventguid, &GUID_DEVINTERFACE_ssvsdio_intevent ))
				{
					SSV6XXX_Drv_Rx_Task(NULL);
				}
			}
			break;
		}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

HWND CreateMessageOnlyWindow()
{
	WNDCLASSEX wx;
	DEV_BROADCAST_HANDLE handle;
	HWND hwnd;
	ZeroMemory(&wx, sizeof(WNDCLASSEX));
	ZeroMemory(&handle, sizeof(DEV_BROADCAST_HANDLE));

	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = WndProc;
	wx.lpszClassName = L"Message-Only Window";

	if ( RegisterClassEx(&wx) )
		hwnd = CreateWindow(L"Message-Only Window", NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);
	else
		return NULL;

	handle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
	handle.dbch_handle = (HANDLE)ssv6xxx_drv_get_handle();
	handle.dbch_devicetype = DBT_DEVTYP_HANDLE;
	handle.dbch_eventguid = GUID_DEVINTERFACE_ssvsdio_intevent;
	RegisterDeviceNotification(hwnd,&handle,DEVICE_NOTIFY_WINDOW_HANDLE);
}

void SSV6XXX_drv_msg_only(void *args)
{
	MSG msg;
	HWND m_hWnd;
	m_hWnd = CreateMessageOnlyWindow();



	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(msg.message == WM_QUIT)
		{
			break;
		}
	}
}
#endif

struct task_info_st g_host_task_info[] =
{
	{ "host_cmd",   (OsMsgQ)0, 16, 	OS_CMD_ENG_PRIO, CMD_ENG_STACK_SIZE, NULL, CmdEng_Task},

};

#ifdef RXFLT_ENABLE
ssv6xxx_data_result test_cb(void *data, u32 len)
{
    LOG_DEBUG("[CmdEng]: RXFLT test, data = %x, len = %d\r\n", (u32)data, len);
    return SSV6XXX_DATA_CONT;
}
#endif

s32 CmdEng_Init(void)
{
	u32 i, size, res=OS_SUCCESS;

	size = sizeof(HostCmdEngInfo_st);
	gHCmdEngInfo = (HostCmdEngInfo_st *)MALLOC(size);
    if(NULL==gHCmdEngInfo)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return OS_FAILED;
    }

	MEMSET(gHCmdEngInfo, 0, size);
    OS_MutexInit(&(gHCmdEngInfo->CmdEng_mtx));
    list_q_init(&gHCmdEngInfo->pending_cmds);
    list_q_init(&gHCmdEngInfo->free_FrmQ);
    gHCmdEngInfo->curr_mode = MT_STOP;// 0
    gHCmdEngInfo->blockcmd_in_q = false;
    gHCmdEngInfo->pending_cmd_seqno = 0;


    TxRxHdl_Init();

	/*size(1)*/
	size = sizeof(g_host_task_info)/sizeof(struct task_info_st);
	for(i = 0; i < size; i++)
    {
		if (g_host_task_info[i].qlength> 0)
        {
			ASSERT(OS_MsgQCreate(&g_host_task_info[i].qevt,
				(u32)g_host_task_info[i].qlength)==OS_SUCCESS);
		}

		/* Create Registered Task: */
		OS_TaskCreate(g_host_task_info[i].task_func,
			g_host_task_info[i].task_name,
			g_host_task_info[i].stack_size<<4,
			g_host_task_info[i].args,
			g_host_task_info[i].prio,
			NULL);
	}
	return res;
}

void CmdEng_FlushPendingCmds(void)
{
    //os_cancel_timer(pendingcmd_expired_handler, (u32)gHCmdEngInfo, (u32)NULL);
    gHCmdEngInfo->blockcmd_in_q = false;
    gHCmdEngInfo->pending_cmd_seqno = 0;
    while(list_q_len(&gHCmdEngInfo->pending_cmds) > 0)
    {
        FrmQ *pcmd = (FrmQ *)list_q_deq(&gHCmdEngInfo->pending_cmds);
        os_frame_free(pcmd->frame);
        pcmd->frame = NULL;
        if(list_q_len(&gHCmdEngInfo->free_FrmQ) > FREE_FRM_NUM)
        {
            FREE(pcmd);
        }
        else
        {
            list_q_qtail(&gHCmdEngInfo->free_FrmQ, (struct ssv_list_q *)pcmd);
        }
    }
    //LOG_DEBUG("[CmdEng]: CmdEng_FlushPendingCmds\n");
}

ssv6xxx_result CmdEng_SetOpMode(ModeType mode)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    if(mode > MT_EXIT)
        return SSV6XXX_INVA_PARAM;

    OS_MutexLock(&gHCmdEngInfo->CmdEng_mtx);

    switch (gHCmdEngInfo->curr_mode)
    {
        case MT_STOP:
        {
            switch (mode)
            {
                case MT_RUNNING:
                    // To run
                    break;
                case MT_EXIT:
                    // To end
                    break;
                default:
                    ret = SSV6XXX_INVA_PARAM;
                    break;
            }
        }
            break;
        case MT_RUNNING:
            if(mode == MT_STOP)
            {
                //To stop
                CmdEng_FlushPendingCmds();
            }
            else
            {
                //error handling
                ret = SSV6XXX_INVA_PARAM;
            }
            break;
        case MT_EXIT:
        default:
            //error handling
            ret = SSV6XXX_FAILED;
            break;
    }

    if(ret == SSV6XXX_SUCCESS)
        gHCmdEngInfo->curr_mode = mode;

    OS_MutexUnLock(&gHCmdEngInfo->CmdEng_mtx);

    return ret;
}
