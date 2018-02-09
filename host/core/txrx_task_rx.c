/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifdef __linux__
#include <linux/kernel.h>
#include <linux/llist.h>
#include <linux/kthread.h>
#endif


#include <cmd_def.h>

#if (AP_MODE_ENABLE == 1)        
#include <ap_info.h>
#endif


#include "host_cmd_engine_priv.h"
#include <txrx_task_rx.h>

#include <ssv_drv.h>
#include <txrx_hdl.h>
#include <txrx_hdl_rx.h>


extern struct Host_cfg g_host_cfg;
extern ModeType curr_mode;
extern struct ssv_hif_drv *s_drv_cur;

OsSemaphore rx_frm_sphr;
bool RxTask_Done = false;
u32 chip_interrupt;
static u8 *hcirxAggtempbuf;
static u32 g_TXSEMA_MAX_NUM;

#ifdef __linux__
int TXRXTask_RxTask(void *args);
#else
void TXRXTask_RxTask(void *args);
#endif

struct task_info_st g_rx_task_info[] =
{
    { "txrxtask_rxtask", (OsMsgQ)0, 0, OS_RX_TASK_PRIO, WIFI_RX_STACK_SIZE, NULL, TXRXTask_RxTask },
};

#ifdef __linux__
int TXRXTask_RxTask(void *args)
{
    void *rFrame = NULL;
    u8  *msg_data = NULL;
    s32 recv_len = 0;
    s32 retry = 0;
    size_t next_pkt_len = 0;

	while (!kthread_should_stop())
	{
	    while (curr_mode != MT_EXIT)
	    {
			if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
						&& (s_drv_cur->drv_info.fields.hw_type == DRV_INFO_FLAG_HW_TYPE_USB))
			{
				/* TODO(aaron): use timeout sem to avoid rx work queue polling mechanism */
			    printk("%s()at line(%d)\n",__FUNCTION__,__LINE__);
				OS_SemWait(rx_frm_sphr, 0);
			}
			else
			{
				OS_SemWait(rx_frm_sphr, 0);			
			}

	        for (retry = 0; retry < 32; retry++)
	        {
	            if (g_host_cfg.hci_rx_aggr)
	            {
	                msg_data = hcirxAggtempbuf;
	                recv_len = ssv6xxx_drv_recv(msg_data, next_pkt_len);
	            }
	            else
	            {
	                if (rFrame == NULL)
	                {
#if (SW_8023TO80211 == 1)
						while ((rFrame = (u8 *)os_frame_alloc((g_host_cfg.recv_buf_size-g_host_cfg.trx_hdr_len),TRUE)) == NULL)
#else
						while ((rFrame = (u8 *)os_frame_alloc(g_host_cfg.recv_buf_size, TRUE)) == NULL)
#endif
	                    {
	                        OS_TickDelay(1);
	                       	LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING, 
								("[RxTask]: wakeup from sleep!\r\n"));
	                        //continue;
	                        printk("[RxTask]: wakeup from sleep!\r\n");
	                    }
	                    os_frame_push(rFrame, g_host_cfg.trx_hdr_len);
	                    msg_data = OS_FRAME_GET_DATA(rFrame);
	                }
                   	printk("[RxTask]: wakeup from sleep!\n");
					
	                recv_len = ssv6xxx_drv_recv(msg_data, OS_FRAME_GET_DATA_LEN(rFrame));
	            }

	            if (recv_len > 0)
	            {
	                if (g_host_cfg.hci_rx_aggr)
	                {
	                    next_pkt_len = ssv_hal_process_hci_rx_aggr(msg_data,recv_len,(RxPktHdr)TXRXTask_RxFrameProc);
	                }
	                else
	                {
	                    OS_FRAME_SET_DATA_LEN(rFrame, recv_len);
	                    TXRXTask_RxFrameProc(rFrame);
	                }
	                rFrame = NULL;
	                msg_data = NULL;
	            }
	            else if (recv_len == 0)
	            {
	                LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_SERIOUS, 
						("[TxRxTask]: recv_len == 0 at frame %p, first 8 bytes content is dumpped as below:\r\n", rFrame));
					printk("%s() recv_len(0) at frame(%p)\n",__FUNCTION__,rFrame);
					printk("the pkt first 8 bytes content as below:\n");
					hex_dump(msg_data, 8);
					break;
	            }
	            else
	            {
					//LOG_DEBUG("[TxRxTask]: Rx semaphore comes in, but not frame receive\r\n");
					printk("[TxRxTask]: Rx semaphore comes in, but not frame receive\r\n");
#if(RECOVER_ENABLE == 1)
#if(RECOVER_MECHANISM == 0)
	                u32 i;
	                if (retry == 0) // when interrupt always high
	                {
	                    if (TRUE == ssv_hal_get_diagnosis()) // watchdog wack up & reset
	                    {
	                        u32 fw_status = 0;
	                        ssv_hal_get_fw_status(&fw_status);

	                        //LOG_PRINTF("***********************************************g_hw_enable:%d fw_status:%08x\r\n", g_hw_enable, fw_status);
	                        if (fw_status == 0x5A5AA5A5 ||g_hw_enable == false)
							{
	                            break;
	                        }

	                        ssv_hal_reset_soc_irq();//  system reset interrupt for host

	                        for (i = 0; i < MAX_VIF_NUM; i++)
	                        {
	                            if (gDeviceInfo->vif[i].hw_mode == SSV6XXX_HWM_AP)
								{
	                                ssv6xxx_wifi_ap_recover(i);
	                            }
	                            else if(gDeviceInfo->vif[i].hw_mode == SSV6XXX_HWM_STA)
	                            {
	                                ssv6xxx_wifi_sta_recover(i);
	                            }
	                        }
	                    }
	                }
#elif(RECOVER_MECHANISM == 1)
	                if (retry == 0)
	                {
	                    // handle ipc interrupt for check fw ceash
	                    //LOG_PRINTF("is heartbeat\r\n");
	                    if (1 == ssv_hal_is_heartbeat())
	                    {
	                        //LOG_PRINTF("%08x: host_isr\r\n",isr_status);
	                        gDeviceInfo->fw_timer_interrupt ++;
	                        chip_interrupt = os_tick2ms(OS_GetSysTick());
	                        ssv_hal_reset_heartbeat();
	                    }
	                }
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE == 1)
	                break;
	            }
	        }
	        //RxTaskRetryCount[retry]++;
	        ssv6xxx_drv_irq_enable(false);
	    }
	    RxTask_Done = true;

	    if (rFrame != NULL)
	    {
	        os_frame_free(rFrame);
	        rFrame = NULL;
	    }
	}
	return 0;
}

#else
void TXRXTask_RxTask(void *args)
{
    void * rFrame = NULL;
    u8  *msg_data = NULL;
    s32 recv_len = 0;
    s32 retry = 0;
    size_t next_pkt_len = 0;
    while(curr_mode != MT_EXIT)
    {
        OS_SemWait(&rx_frm_sphr, 0);

        for(retry=0;retry<32;retry++)
        {
            if(g_host_cfg.hci_rx_aggr)
            {
                msg_data = hcirxAggtempbuf;
                recv_len = ssv6xxx_drv_recv(msg_data, next_pkt_len);
            }
            else
            {
                if(rFrame == NULL)
                {
                    #if(SW_8023TO80211==1)
                    while ((rFrame = (u8 *)os_frame_alloc((g_host_cfg.recv_buf_size-g_host_cfg.trx_hdr_len),TRUE)) == NULL)
                    #else
                    while ((rFrame = (u8 *)os_frame_alloc(g_host_cfg.recv_buf_size,TRUE)) == NULL)
                    #endif
                    {
                        OS_TickDelay(1);
                       	LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING, ("[RxTask]: wakeup from sleep!\r\n"));
                        //continue;
                    }
                    os_frame_push(rFrame, g_host_cfg.trx_hdr_len);
                    msg_data = OS_FRAME_GET_DATA(rFrame);
                }
                recv_len = ssv6xxx_drv_recv(msg_data, OS_FRAME_GET_DATA_LEN(rFrame));
            }

            if(recv_len > 0)
            {
                if(g_host_cfg.hci_rx_aggr)
                {
                    next_pkt_len = ssv_hal_process_hci_rx_aggr(msg_data,recv_len,(RxPktHdr)TXRXTask_RxFrameProc);
                }
                else
                {
                    OS_FRAME_SET_DATA_LEN(rFrame, recv_len);
                    TXRXTask_RxFrameProc(rFrame);
                }
                rFrame = NULL;
                msg_data = NULL;
            }
            else if(recv_len == 0)
            {
                LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_SERIOUS, ("[TxRxTask]: recv_len == 0 at frame %p, first 8 bytes content is dumpped as below:\r\n", rFrame));
                hex_dump(msg_data, 8);
            }
            else
            {
                //LOG_DEBUG("[TxRxTask]: Rx semaphore comes in, but not frame receive\r\n");
#if(RECOVER_ENABLE == 1)
#if(RECOVER_MECHANISM == 0)
                u32 i;
                if(retry ==0) // when interrupt always high
                {
                    if(TRUE==ssv_hal_get_diagnosis()) // watchdog wack up & reset
                    {
                        u32 fw_status = 0;
                        ssv_hal_get_fw_status(&fw_status);

                        //LOG_PRINTF("***********************************************g_hw_enable:%d fw_status:%08x\r\n", g_hw_enable, fw_status);
                        if(fw_status == 0x5A5AA5A5 ||g_hw_enable == false){
                            break;
                        }

                        ssv_hal_reset_soc_irq();//  system reset interrupt for host

                        for(i=0;i<MAX_VIF_NUM;i++)
                        {
                            if(gDeviceInfo->vif[i].hw_mode == SSV6XXX_HWM_AP){
                                ssv6xxx_wifi_ap_recover(i);
                            }
                            else if(gDeviceInfo->vif[i].hw_mode == SSV6XXX_HWM_STA)
                            {
                                ssv6xxx_wifi_sta_recover(i);
                            }
                        }
                    }
                }
#elif(RECOVER_MECHANISM == 1)
                if(retry ==0)
                {
                    // handle ipc interrupt for check fw ceash
                    //LOG_PRINTF("is heartbeat\r\n");
                    if(1==ssv_hal_is_heartbeat())
                    {
                        //LOG_PRINTF("%08x: host_isr\r\n",isr_status);
                        gDeviceInfo->fw_timer_interrupt ++;
                        chip_interrupt = os_tick2ms(OS_GetSysTick());
                        ssv_hal_reset_heartbeat();
                    }
                }
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE == 1)
                break;
            }
        }
        //RxTaskRetryCount[retry]++;
        ssv6xxx_drv_irq_enable(false);
    }
    RxTask_Done = true;

    if(rFrame != NULL)
    {
        os_frame_free(rFrame);
        rFrame = NULL;
    }	
}
#endif


void _dq_status_handler(void *data)
{
#if (AP_MODE_ENABLE == 1)    
    struct resp_evt_result *dqStatus = (struct resp_evt_result *)data;
    APStaInfo_st *sta = &gDeviceInfo->APInfo->StaConInfo[dqStatus->u.dq_status.wsid];
    OS_MutexLock(sta->apsta_mutex);
    sta->fw_q_len = dqStatus->u.dq_status.len;
    OS_MutexUnLock(sta->apsta_mutex);
    if(dqStatus->u.dq_status.len > 0)
    {
        extern void recalc_tim_gen_beacon(APStaInfo_st * sta);
        // indicate tim and generate beacon
        recalc_tim_gen_beacon(sta);
    }    
#endif    
}

//TXRX_RxFrameProc Process rx frame
s32 TXRXTask_RxFrameProc(void* frame)
{
    //send to CmdEngine if it is host_event
    //pass to RxHdl if it is data frame
    s32 ret = SSV6XXX_SUCCESS;
    u8 c_type=0;
    CFG_HOST_RXPKT * rxpkt = (CFG_HOST_RXPKT *)OS_FRAME_GET_DATA(frame);
    c_type=ssv_hal_get_rxpkt_ctype(rxpkt);
    if(c_type == M0_RXEVENT)
    {
        if(curr_mode == MT_RUNNING)
        {

            ret = RxHdl_FrameProc(frame);
            return ret;
        }
        else
        {
            os_frame_free(frame);
            LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING,("[TXRXTask]: free rx pkt due to mode = stop\r\n"));
            return SSV6XXX_FAILED;
        }
    }
    else if(c_type == HOST_EVENT)
    {
        struct cfg_host_event *pPktInfo = (struct cfg_host_event *)OS_FRAME_GET_DATA(frame);

        switch(pPktInfo->h_event)
        {
            case SOC_EVT_PS_POLL:
            case SOC_EVT_NULL_DATA:
#if (AP_MODE_ENABLE == 1)                
                ap_handle_ps(frame); //handle null data and ps_poll
#endif
                break;
            case SOC_EVT_DATA_QUEUE_STATUS:
                _dq_status_handler(pPktInfo->dat);
                os_frame_free(frame);
                break;
            case SOC_EVT_TX_LOOPBACK_DONE:
            {
                ssv_hal_tx_loopback_done(pPktInfo->dat);
                os_frame_free(frame);
                break;
            }
            case SOC_EVT_ADD_BA:
            {
                if(g_host_cfg.ampdu_rx_enable)
                    ieee80211_addba_handler(pPktInfo->dat);
                os_frame_free(frame);
                break;
            }
            case SOC_EVT_ACK:
            {
                LOG_PRINTF("Bee!!Bee!!Bee!!\r\n");
                os_frame_free(frame);
                break;
            }
            default:
            {
                void *msg = os_msg_alloc();
                if (msg)
                {
                    os_msg_send(msg, frame);
                }
                else
                {
                    os_frame_free(frame);
                    ret = SSV6XXX_NO_MEM;
                }
                break;
            }

        }
        return ret;

    }
    else
    {
        LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING,("[TXRXTask]: unavailable type of rx pkt\r\n"));
        os_frame_free(frame);
        return SSV6XXX_INVA_PARAM;
    }

    return ret;
}


//TXRX_Init Initialize
s32 RXTask_Init(void)
{
    u32 i, size, res=OS_SUCCESS;

	/* not in, g_host_cfg.hci_rx_aggr = HCI_RX_AGGR(0) */
    if (g_host_cfg.hci_rx_aggr)
    {        
        hcirxAggtempbuf = (u8 *)OS_MemAlloc(MAX_HCI_AGGR_SIZE);
        assert(hcirxAggtempbuf != NULL);
    }

    LOG_PRINTF("init TX sem_max=%d, RX sem_max=%d\r\n",g_TXSEMA_MAX_NUM, RXSEMA_MAX_NUM);
    OS_SemInit(&rx_frm_sphr, RXSEMA_MAX_NUM, 0);


	/* size = 2 */
	size = sizeof(g_rx_task_info)/sizeof(struct task_info_st);
	for (i = 0; i < size; i++)
    {
		/* Create Registered Task: */
		OS_TaskCreate(g_rx_task_info[i].task_func,
			g_rx_task_info[i].task_name,
			g_rx_task_info[i].stack_size<<4,
			g_rx_task_info[i].args,
			g_rx_task_info[i].prio,
			NULL);
	}

    LOG_PRINTF("%s(): end !!\r\n", __FUNCTION__);
    return res;
}

s32 RXTask_DeInit(void)
{
    OS_SemDelete(rx_frm_sphr);

    OS_MemFree((void*)hcirxAggtempbuf);
    hcirxAggtempbuf=NULL;

	return 0;
}

