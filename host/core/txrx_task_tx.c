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
#include "host_cmd_engine_priv.h"
#include <txrx_task.h>
#include <txrx_task_tx.h>

#include <ssv_drv.h>
#include <txrx_hdl.h>
#include <txrx_hdl_tx.h>


extern ModeType curr_mode;
extern struct Host_cfg g_host_cfg;

u16 tx_frame_len_count=0;
u16 tx_frame_len[TX_FRAME_ARRAY_SIZE];
OsMutex tx_mtx;
u32 tx_data_frm_num;
u32 tx_flush_data_frm_num;

static struct ssv_llist_head tx_hwq[PRI_Q_NUM];
static struct ssv_llist_head free_frmQ;

OsSemaphore tx_frm_sphr;

static u32 g_TXSEMA_MAX_NUM;

FrmL *g_freeFrmList = NULL;
bool TxTask_Done = false;



#ifdef __linux__
int TXRXTask_TxTask(void *args);
#else
void TXRXTask_TxTask(void *args);
#endif

struct task_info_st g_tx_task_info[] =
{
    { "txrxtask_txtask", (OsMsgQ)0, 0, OS_TX_TASK_PRIO, WIFI_TX_STACK_SIZE, NULL, TXRXTask_TxTask },
};

#ifdef __linux__
int TXRXTask_TxTask(void *args)
{
    void *tFrame = NULL;
    FrmL *outFrm = NULL;
    s8 i = 0, prc_count = 0;
    bool flush_frm = false;
    u32 aggr_n=0,aggr_len=0;
    void *aggr_buf=NULL;
    u32 sleep_tick=0;

	while (!kthread_should_stop())
	{
		while (curr_mode != MT_EXIT)
	    {
	        prc_count = 0;
	        OS_SemWait(tx_frm_sphr, sleep_tick);
	        for (i = 1; i <= PRI_Q_NUM; i++)
	        {
	            while ((outFrm = (FrmL *)llist_pop_safe(&tx_hwq[(PRI_Q_NUM-i)], &tx_mtx)) != NULL)
	            {
	                tFrame = outFrm->frame;
	                if (tFrame)
					{
	                    while (ssv6xxx_drv_tx_resource_enough(OS_FRAME_GET_DATA_LEN(tFrame)) != TRUE)
						{
	                        if (g_host_cfg.tx_sleep)
	                        {
	                            OS_TickDelay(g_host_cfg.tx_sleep_tick);
	                        }
						};

	                    OS_MutexLock(tx_mtx);
	                    flush_frm = (tx_flush_data_frm_num == 0)?false:true;
	                    tx_data_frm_num --;
	                    if (flush_frm == true)
	                    {    
	                    	tx_flush_data_frm_num--;
	                    }
						
						OS_MutexUnLock(tx_mtx);
	                    if (flush_frm == false)
	                    {
TX_RESEND:
	                        if (g_host_cfg.hci_aggr_tx ==1)
	                        {
	                            if (!aggr_buf)
	                            {
	                                aggr_buf = OS_MemAlloc(MAX_HCI_AGGR_SIZE);
	                                if (!aggr_buf)
	                                {
	                                    LOG_PRINTF("Allocate hci tx buf fail. Disable HCI TX Aggregation\r\n");
	                                    g_host_cfg.hci_aggr_tx=0;
	                                    goto TX_RESEND;
	                                }
	                                OS_MemSET(aggr_buf,0,MAX_HCI_AGGR_SIZE);
	                                //LOG_PRINTF("Alloc aggr tx buffer=%x\r\n",(u32)aggr_buf);
	                            }
	                            if ((aggr_len+OS_FRAME_GET_DATA_LEN(tFrame))<MAX_HCI_AGGR_SIZE)
	                            {
	                                aggr_n = ssv_hal_process_hci_aggr_tx(tFrame,aggr_buf,&aggr_len);
	                            }

	                            if (aggr_n > 4)
	                            {
	                                if(ssv6xxx_drv_send(aggr_buf, aggr_len) <0)
	                                {
	                                    LOG_PRINTF("%s: ssv6xxx_drv_send() AGGR data failed !!\r\n", __FUNCTION__);
	                                }
	                                //LOG_PRINTF("AGGR T aggr_n=%d, len=%d\r\n",aggr_n,aggr_len);
	                                aggr_n = 0;
									aggr_len = 0;
	                                OS_MemSET(aggr_buf,0,MAX_HCI_AGGR_SIZE);
	                            }
	                            else
	                            {
	                                sleep_tick = 1;
	                            }
	                        }
	                        else
	                        {
	                            u16 len1=OS_FRAME_GET_DATA_LEN(tFrame);
	                            u16 len2=*((u16 *)OS_FRAME_GET_DATA(tFrame));
	                            if (len1 != len2)
								{
	                                LOG_PRINTF("\33[31m Wrong TX len (len1=%d,len2=%d) \33[0m\r\n",len1,len2);
	                            }
	                            tx_frame_len[tx_frame_len_count] = len2;
	                            tx_frame_len_count++;
	                            tx_frame_len_count = tx_frame_len_count % TX_FRAME_ARRAY_SIZE;
  
	                            if (ssv6xxx_drv_send(OS_FRAME_GET_DATA(tFrame), OS_FRAME_GET_DATA_LEN(tFrame)) <0)
	                            {
	                                LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_SERIOUS, ("%s: ssv6xxx_drv_send() data failed !!\r\n", __FUNCTION__));
	                            }
	    						//LOG_PRINTF("T\r\n");
	                        }
	                    }

	                    os_frame_free(tFrame);
	                    outFrm->frame = NULL;
	                    //periodic_pktnum++;
	                    prc_count++;
	                    llist_push_safe(&free_frmQ,(struct ssv_llist *)outFrm, &tx_mtx);

	                    tFrame = NULL;
	                    outFrm = NULL;

	                    if (prc_count > 1)
	                    {
	                        OS_SemWait(tx_frm_sphr, sleep_tick); //More frames are sent and resource should be pulled out.
	                        //LOG_PRINTF("******************%s: push back due to multi-send:%d !!\r\n", __FUNCTION__, tx_count);
	                    }

	                }
	                else
	                {
	                    LOG_ERROR("outFrm without pbuf?? \r\n");
	                }
	            }
	        }
	        if (!prc_count)
	        {
	            if (aggr_n)
	            {
	                if (ssv6xxx_drv_send(aggr_buf, aggr_len) <0)
	                {
	                    LOG_PRINTF("%s: ssv6xxx_drv_send() AGGRT data failed !!\r\n", __FUNCTION__);
	                }
	                OS_MemSET(aggr_buf,0,MAX_HCI_AGGR_SIZE);
	                aggr_n=aggr_len=0;
	                sleep_tick=0;
	            }
	        }
	    }
	    TxTask_Done = true;
	}
	return 0;
}

#else

void TXRXTask_TxTask(void *args)
{
    void *tFrame = NULL;
    FrmL *outFrm = NULL;
    s8 i = 0, prc_count = 0;
    bool flush_frm = false;
    u32 aggr_n=0,aggr_len=0;
    void* aggr_buf=NULL;
    u32 sleep_tick=0;
    //u32 start_ticks = 0, account_ticks = 0, wakeup_ticks = 0, wakeup_total = 0, wait_ticks = 0, acc_wait = 0;
    //u32 periodic_pktnum = 0;

    while (curr_mode != MT_EXIT)
    {
        prc_count = 0;
        /*
        account_ticks += (OS_GetSysTick() - start_ticks);
        wakeup_total += (OS_GetSysTick() - wakeup_ticks);
        if(account_ticks > 1000)
        {
            LOG_PRINTF("%s: %d pkt is sent in %d ms, wakeup in %d ms, acc_wait %d ms !!\r\n",
                       __FUNCTION__, periodic_pktnum, account_ticks, wakeup_total, acc_wait);
            periodic_pktnum = 0;
            account_ticks = 0;
            wakeup_total = 0;
            acc_wait = 0;
        }
        start_ticks = OS_GetSysTick();*/
        OS_SemWait(tx_frm_sphr, sleep_tick);
        /*wakeup_ticks = OS_GetSysTick();
        wait_ticks = (wakeup_ticks-start_ticks);
        acc_wait += wait_ticks;*/
        for (i = 1; i <= PRI_Q_NUM; i++)
        {
            while((outFrm = (FrmL *)llist_pop_safe(&tx_hwq[(PRI_Q_NUM-i)], &tx_mtx)) != NULL)
            {
                tFrame = outFrm->frame;
                if(tFrame){
                    while(ssv6xxx_drv_tx_resource_enough(OS_FRAME_GET_DATA_LEN(tFrame)) != TRUE)
					{
                        if(g_host_cfg.tx_sleep)
                        {
                            //LOG_PRINTF("+\r\n");
                            OS_TickDelay(g_host_cfg.tx_sleep_tick);
                        }
					};
                    //LOG_PRINTF("%s:  Send tx frame %08x with FrmQ %08X!!\r\n", __FUNCTION__, tFrame, outFrm);
                    OS_MutexLock(tx_mtx);
                    flush_frm = (tx_flush_data_frm_num == 0)?false:true;
                    tx_data_frm_num --;
                    if(flush_frm == true)
                        tx_flush_data_frm_num--;
                    OS_MutexUnLock(tx_mtx);

                    if (flush_frm == false)
                    {
TX_RESEND:
                        if(g_host_cfg.hci_aggr_tx ==1)
                        {
                            if(!aggr_buf)
                            {
                                aggr_buf = OS_MemAlloc(MAX_HCI_AGGR_SIZE);
                                if(!aggr_buf)
                                {
                                    LOG_PRINTF("Allocate hci tx buf fail. Disable HCI TX Aggregation\r\n");
                                    g_host_cfg.hci_aggr_tx=0;
                                    goto TX_RESEND;
                                }
                                OS_MemSET(aggr_buf,0,MAX_HCI_AGGR_SIZE);
                                //LOG_PRINTF("Alloc aggr tx buffer=%x\r\n",(u32)aggr_buf);
                            }
                            if((aggr_len+OS_FRAME_GET_DATA_LEN(tFrame))<MAX_HCI_AGGR_SIZE)
                            {
                                aggr_n = ssv_hal_process_hci_aggr_tx(tFrame,aggr_buf,&aggr_len);
                            }

                            if(aggr_n>4)
                            {
                                if(ssv6xxx_drv_send(aggr_buf, aggr_len) <0)
                                {
                                    LOG_PRINTF("%s: ssv6xxx_drv_send() AGGR data failed !!\r\n", __FUNCTION__);
                                }
                                //LOG_PRINTF("AGGR T aggr_n=%d, len=%d\r\n",aggr_n,aggr_len);
                                aggr_n=aggr_len=0;
                                OS_MemSET(aggr_buf,0,MAX_HCI_AGGR_SIZE);
                            }
                            else
                            {
                                sleep_tick=1;
                            }
                        }
                        else
                        {
                            u16 len1=OS_FRAME_GET_DATA_LEN(tFrame);
                            u16 len2=*((u16 *)OS_FRAME_GET_DATA(tFrame));
                            if(len1!=len2){
                                LOG_PRINTF("\33[31m Wrong TX len (len1=%d,len2=%d) \33[0m\r\n",len1,len2);
                            }
                            tx_frame_len[tx_frame_len_count]=len2;
                            tx_frame_len_count++;
                            tx_frame_len_count=tx_frame_len_count%TX_FRAME_ARRAY_SIZE;

                                
                            if (ssv6xxx_drv_send(OS_FRAME_GET_DATA(tFrame), OS_FRAME_GET_DATA_LEN(tFrame)) <0)
                            {
                                LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_SERIOUS, ("%s: ssv6xxx_drv_send() data failed !!\r\n", __FUNCTION__));
                            }
    						//LOG_PRINTF("T\r\n");
                        }
                    }

                    os_frame_free(tFrame);
                    outFrm->frame = NULL;
                    //periodic_pktnum++;
                    prc_count++;
                    llist_push_safe(&free_frmQ,(struct ssv_llist *)outFrm, &tx_mtx);

                    tFrame = NULL;
                    outFrm = NULL;

                    if(prc_count > 1)
                    {
                        OS_SemWait(tx_frm_sphr, sleep_tick); //More frames are sent and resource should be pulled out.
                        //LOG_PRINTF("******************%s: push back due to multi-send:%d !!\r\n", __FUNCTION__, tx_count);
                    }

                }
                else
                {
                    LOG_ERROR("outFrm without pbuf?? \r\n");
                }
            }
        }

        if(!prc_count)
        {
            if(aggr_n)
            {
                if (ssv6xxx_drv_send(aggr_buf, aggr_len) <0)
                {
                    LOG_PRINTF("%s: ssv6xxx_drv_send() AGGRT data failed !!\r\n", __FUNCTION__);
                }
                OS_MemSET(aggr_buf,0,MAX_HCI_AGGR_SIZE);
                //LOG_PRINTF("AGGT timeout,aggr_n=%d, len=%d\r\n",aggr_n,aggr_len);
                aggr_n=aggr_len=0;
                sleep_tick=0;
            }
        }
    }
    TxTask_Done = true;
}
#endif

void TXRXTask_TxLock(bool lock)
{
    if(lock)
        OS_MutexLock(tx_mtx);
    else
        OS_MutexUnLock(tx_mtx);
}

void _prepare_tx_frm_list(void)
{
    u32 i;

    llist_head_init(&free_frmQ);
	/* PRI_Q_NUM(5) */
    for(i = 0; i < PRI_Q_NUM; i++)
        llist_head_init(&tx_hwq[i]);

    //MEMSET((void*)g_freeFrmList, 0 , sizeof(FrmL)*g_TXSEMA_MAX_NUM);
    /* g_TXSEMA_MAX_NUM(98) */
    for(i = 0; i < g_TXSEMA_MAX_NUM; i++)
        llist_push(&free_frmQ, &(g_freeFrmList[i]._list));
}

//TXRX_Init Initialize
s32 TXTask_Init(void)
{
    u32 i, size, res=OS_SUCCESS;

//	curr_mode = MT_STOP;
    tx_flush_data_frm_num = 0;
    tx_data_frm_num = 0;

	/* g_TXSEMA_MAX_NUM = 98
	 * g_host_cfg.pool_size = POOL_SIZE(72), 
	 * g_host_cfg.pool_sec_size = POOL_SEC_SIZE(16) 
	 * 10	 
	 */
    g_TXSEMA_MAX_NUM = g_host_cfg.pool_size+g_host_cfg.pool_sec_size+10;
    g_freeFrmList = (FrmL *)OS_MemAlloc(sizeof(FrmL)*g_TXSEMA_MAX_NUM);    
    assert(g_freeFrmList != NULL);
    
    MEMSET((void*)g_freeFrmList, 0, sizeof(FrmL)*g_TXSEMA_MAX_NUM);
    MEMSET(&tx_rcs, 0, sizeof(tx_rcs));
//    OS_MutexInit(&task_mtx);
    OS_MutexInit(&tx_mtx);

    LOG_PRINTF("init TX sem_max=%d\r\n",g_TXSEMA_MAX_NUM);
    OS_SemInit(&tx_frm_sphr, g_TXSEMA_MAX_NUM, 0);

    _prepare_tx_frm_list();

	/* size = 2 */
	size = sizeof(g_tx_task_info)/sizeof(struct task_info_st);
	for (i = 0; i < size; i++)
    {
		/* Create Registered Task: */
		OS_TaskCreate(g_tx_task_info[i].task_func,
			g_tx_task_info[i].task_name,
			g_tx_task_info[i].stack_size<<4,
			g_tx_task_info[i].args,
			g_tx_task_info[i].prio,
			NULL);
	}

    LOG_PRINTF("%s(): end !!\r\n", __FUNCTION__);
    return res;
}

s32 TXTask_DeInit(void)
{
//    OS_MutexDelete(task_mtx);
    OS_MutexDelete(tx_mtx);

    OS_SemDelete(tx_frm_sphr);

	return 0;
}

//TXRX_FrameEnqueue Enqueue tx frames
s32 TXRXTask_FrameEnqueue(void *frame, u32 priority)
{
    FrmL *dataFrm = NULL;

    CFG_HOST_TXREQ *req = (CFG_HOST_TXREQ *)OS_FRAME_GET_DATA(frame);
	
#if(SW_8023TO80211==1)
    if ((curr_mode == MT_STOP) && (M2_TXREQ==ssv_hal_get_txreq0_ctype(req)))
#else
    if ((curr_mode == MT_STOP) && (M0_TXREQ==ssv_hal_get_txreq0_ctype(req)))
#endif
    {
        LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_WARNING, 
			("[TxRx_task]: No enqueue frame due to mode = stop and frame type = data\r\n"));
        return false;
    }

    OS_MutexLock(tx_mtx);
    dataFrm = (FrmL *)llist_pop(&free_frmQ);
    if (dataFrm == NULL)
    {
        LOG_DEBUGF(LOG_TXRX|LOG_LEVEL_SERIOUS, 
			("[TxRx_task]:can not get empty llist for dataFrm\r\n"));
        OS_MutexUnLock(tx_mtx);
        return false;
    }


    os_frame_set_debug_flag(frame,SSV_PBUF_DBG_FLAG_L2_TX_DRIVER);

    dataFrm->frame = frame;


    tx_data_frm_num++;

    llist_push(&tx_hwq[priority], &dataFrm->_list);
    OS_MutexUnLock(tx_mtx);

    while(OS_SemSignal(tx_frm_sphr) != 0)
        OS_TickDelay(1);

    return true;
}

