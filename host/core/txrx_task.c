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

#include <txrx_task.h>
#include <txrx_task_tx.h>
#include <txrx_task_rx.h>
#include <ssv_drv.h>
#include <txrx_hdl.h>
#include "ssv_dev.h"


extern OsMutex tx_mtx;
extern u32 tx_data_frm_num;
extern u32 tx_flush_data_frm_num;
extern bool TxTask_Done;
extern bool RxTask_Done;
extern OsSemaphore tx_frm_sphr;
extern OsSemaphore rx_frm_sphr;
extern u16 tx_frame_len[TX_FRAME_ARRAY_SIZE];
extern bool ssv6xxx_drv_irq_status(void);

ModeType curr_mode;

//TXRX_Init Initialize
s32 TXRXTask_Init(void)
{
    u32 res = OS_SUCCESS;

	curr_mode = MT_STOP;

	TXTask_Init();

	RXTask_Init();
    LOG_PRINTF("%s(): end !!\r\n", __FUNCTION__);
    return res;
}

s32 TXRXTask_DeInit(void)
{
	TXTask_DeInit();
	RXTask_DeInit();
	return 0;
}

//TXRX_SetOpMode Set the operation mode
ssv6xxx_result TXRXTask_SetOpMode(ModeType mode)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    if (mode > MT_EXIT)
        return SSV6XXX_INVA_PARAM;

    OS_MutexLock(tx_mtx);

    switch (curr_mode)
    {
        case MT_STOP:
        {
            if(mode == MT_EXIT)
            {
                tx_flush_data_frm_num = tx_data_frm_num;
            }
        }
            break;
        case MT_RUNNING:
            if(mode == MT_STOP)
            {
                //To stop
                tx_flush_data_frm_num = tx_data_frm_num;
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
        curr_mode = mode;

    OS_MutexUnLock(tx_mtx);
    LOG_DEBUGF(LOG_TXRX, ("[TxRx_task]: curr_mode = %d\r\n", curr_mode));

    if(curr_mode == MT_EXIT)
    {

        while ((TxTask_Done != true) || (RxTask_Done != true))
        {
            OS_TickDelay(1);
        }

    }
    return SSV6XXX_SUCCESS;
}

void TXRXTask_Isr(u32 signo, bool isfromIsr)
{
    if (signo & INT_RX)
    {
        ssv6xxx_drv_irq_disable(isfromIsr);
        if (isfromIsr == TRUE)
        {
            if (OS_SemSignal_FromISR(rx_frm_sphr) != 0)
            {
                LOG_PRINTF("1 RX sem cnt=%d\r\n", OS_SemCntQuery(rx_frm_sphr));
            }
        }    
        else
        {
            if (OS_SemSignal(rx_frm_sphr) != 0)
            {
                LOG_PRINTF("2 RX sem cnt=%d\r\n", OS_SemCntQuery(rx_frm_sphr));
            }
        }
    }
}


void TXRXTask_ShowSt(void)
{
    u8 i=0;
    LOG_DEBUG("[TxRx_task]: curr_mode = %d\r\n %d tx frames in q\r\n %d tx frames need to be flushed\r\n",
              curr_mode, tx_data_frm_num, tx_flush_data_frm_num);

    for(i=0;i<TX_FRAME_ARRAY_SIZE;i++){
        LOG_PRINTF("tx frame %d: len=%d\r\n",i,tx_frame_len[i]);
    }
    LOG_PRINTF("TX sem cnt=%d, RX sem cnt=%d,irq_st=%d\r\n", 
		OS_SemCntQuery(tx_frm_sphr),OS_SemCntQuery(rx_frm_sphr),
									ssv6xxx_drv_irq_status());
}

u32 ssv6xxx_wifi_get_fw_interrupt_cnt(void)
{
    return gDeviceInfo->fw_timer_interrupt;
}


