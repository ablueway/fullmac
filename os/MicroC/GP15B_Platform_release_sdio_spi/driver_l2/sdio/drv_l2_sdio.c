/*
* Purpose: Driver layer2 for controlling SDIO cards
*
* Author: Dunker Chen
*
* Date: 2013/10/31
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.0
*/

#include <stdio.h>
#include <string.h>
#include "gplib_mm_gplus.h"
#include "drv_l1_sdc.h"
#include "drv_l2_sd.h"
#include "driver_l2_cfg.h"
#include "drv_l2_sdio.h"
#include "drv_l1_timer.h"

#include "drv_l1_ext_int.h"
#include "drv_l1_dma.h"
#include "drv_l1_interrupt.h"
#include "board_config.h"
#include "drv_l1_gpio.h"
#include "project.h"

extern INT32S drvl1_sdc_command_issue(sd_l1_ctrl_t *ctrl);
extern void drvl1_sdio_install_1_bit_isr(INT32U device_id, INT8U extInt, INT8U extMux, void (*user_isr)(INT32U));
extern void drvl1_sdio_config_one_bit_intr_get(INT32U device_id, INT8U *extInt, INT8U *extMux);

#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)

//#ifndef __CS_COMPILER__
//const INT8U DRVL2_SDIO[] = "GLB_GP-S2_0610L_SDIO-L2-ADS_1.0.0";
//#else
//const INT8U DRVL2_SDIO[] = "GLB_GP-S2_0610L_SDIO-L2-CS_1.0.0";
//#endif
/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

#define SDC_TOTAL		2

#define SDIO_OCR	0x1C0000		// Operation conditions register (OCR) 3.0~3.3 V

#define SDIO_DMA_WAIT_STATUS    1
/**************************************************************************
 *                              M A C R O S                               *
**************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
**************************************************************************/

typedef struct gpSDCtrl_s{
	sd_l1_ctrl_t	l1_ctrl;
	sd_l1_data_t	l1_data;
	volatile INT32U	int_notify;
#if _OPERATING_SYSTEM == _OS_UCOS2
	void *			int_buf;
	OS_EVENT*		int_queue;
#endif
}gpSDCtrl_t;

typedef int (tpl_parse_t)( gpSDIOInfo_t *sdio, struct sdio_func *,
			   const INT8U *, INT32U);

struct cis_tpl {
	INT8U code;
	INT8U min_size;
	tpl_parse_t *parse;
};

typedef void (*DRV_L2_SDIO_ISR)(void);
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/

extern void print_string(CHAR *fmt, ...);
//extern void sdio_host_isr(INT32U device_id);
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
**************************************************************************/
static void sdio_host_isr(INT32U device_id)
{
}
/**************************************************************************
 *                         G L O B A L    D A T A                         *
**************************************************************************/

static gpSDIOInfo_t gp_sdio_info[2] = {0};
static gpSDCtrl_t gp_sdio_ctrl[2] = {0};

#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
static	OS_EVENT *gSD_sem[SDC_TOTAL] = {NULL, NULL};
#elif _OPERATING_SYSTEM == _OS_FREERTOS
static	xSemaphoreHandle gSD_sem[SDC_TOTAL] = {NULL, NULL};
#endif
#endif

static DRV_L2_SDIO_ISR sdio_isr_handler[SDC_TOTAL] = {NULL, NULL};
/*
static SD_CARD_STATE_ENUM sd_card_state[SDC_TOTAL] = {SD_CARD_STATE_INACTIVE, SD_CARD_STATE_INACTIVE};
static INT32U sd_card_rca[SDC_TOTAL];			// 16 bits (31-16)
static INT32U sd_card_csd[SDC_TOTAL][4];		// 128 bits
static INT32U sd_card_scr[SDC_TOTAL][2];		// 64 bits
static INT32U sd_card_cid[SDC_TOTAL][4];		// 128 bits
static INT32U sd_card_ocr[SDC_TOTAL];			// 32 bits
static INT32U sd_card_total_sector[SDC_TOTAL];
static INT32U sd_card_speed[SDC_TOTAL];
static INT8U sd_card_type[SDC_TOTAL];
static INT8U sd_card_bus_width[SDC_TOTAL];
static INT8U sd_card_protect[SDC_TOTAL];
static INT8U sd_card_user[SDC_TOTAL] = {0, 0};
*/
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/
static INT32S sd_sema_count[SDC_TOTAL] = {0, 0};

static __inline INT32S drvl2_sd_sem_init(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE
  #if _OPERATING_SYSTEM == _OS_UCOS2
	if(gSD_sem[device_id]==NULL)
		gSD_sem[device_id] = OSSemCreate(1);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
	if(gSD_sem[device_id]==NULL)
		gSD_sem[device_id] = xSemaphoreCreateMutex();
  #endif
#endif
    return (0);
}

static __inline void drvl2_sd_sem_lock(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE

  #if _OPERATING_SYSTEM == _OS_UCOS2
        INT8U err = NULL;
	OSSemPend(gSD_sem[device_id], 0, &err);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
        //BaseType_t err;
        xSemaphoreTake(gSD_sem[device_id], portMAX_DELAY);
  #endif

  if (sd_sema_count[device_id] != 0)
    DBG_PRINT("sd_sema_count not 0 before lock. %d\r\n", sd_sema_count[device_id]);
  sd_sema_count[device_id]++;

#endif
}

static __inline void drvl2_sd_sem_unlock(INT32U device_id)
{
#if _OPERATING_SYSTEM != _OS_NONE

    sd_sema_count[device_id]--;
    if (sd_sema_count[device_id] != 0)
        DBG_PRINT("sd_sema_count not 0 after unlock. %d\r\n", sd_sema_count[device_id]);

  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSSemPost(gSD_sem[device_id]);
  #elif _OPERATING_SYSTEM == _OS_FREERTOS
        xSemaphoreGive(gSD_sem[device_id]);
  #endif

#endif
}

/**
* @brief 	Get SDIO information(handle).
* @param 	device_id[in]:  Index of SD controller.
* @return 	 SDIO information.
*/
gpSDIOInfo_t* drvl2_sdio_get_info(
	INT32U device_id )
{
	return &gp_sdio_info[device_id];
}

/**
* @brief 	Set l1 driver data information.
* @param 	data[in]: Data information pointer.
* @param 	buf[in]: Data buffer pointer.
* @param 	blksz[in]: Data block size.
* @param 	blocks[in]: Number of blocks.
* @param 	dir[in]: Data direction.
* @param 	dmaen[in]: Dma enable.
* @return 	None.
*/
void drvl2_sd_set_data(
	sd_l1_data_t *data,
	INT32U *buf,
	INT32U blksz,
	INT32U blocks,
	INT32U dir,
	INT32U dmaen)
{
	data->buf = buf;
	data->blksz = blksz;
	data->blocks = blocks;
	data->dir = dir;
	data->dmaen = dmaen;
	data->bytes_xfered = 0;
}

/**
* @brief 	Send SD/MMC command with or without data.
* @param 	device_id[in]: Index of SD controller.
* @param 	cmd[in]: SD/MMC command.
* @param 	arg[in]: Argument.
* @param 	resp_type[in]: Response type.
* @param 	data_separate[in]: Data separate. If set, driver will send commmand and do not process data.
* @param 	data[in]: Data information.
* @return 	0: success, -1: timeout or status fail.
*/
INT32S drvl2_sd_send_command(
	INT32U device_id,
	INT32U cmd,
	INT32U arg,
	INT32U resp_type,
	INT32U data_separate,
	sd_l1_data_t *data)
{
	sd_l1_ctrl_t *l1_ctrl = &gp_sdio_ctrl[device_id].l1_ctrl;

	l1_ctrl->device_id = device_id;
	l1_ctrl->cmd = cmd;
	l1_ctrl->arg = arg;
	l1_ctrl->resp_type = resp_type;
	l1_ctrl->data_separate = data_separate;
	l1_ctrl->data = data;
	return drvl1_sdc_command_issue( l1_ctrl );
//	DBG_PRINT("[drvl1_sdc_command_issue]=%d\r\n", retn);
}

/**
* @brief 	Send SDIO IO_RX_DIRECT command(CMD52).
* @param 	sdio[in]: SDIO handle.
* @param 	write[in]: The direction of io operation. 0 for read, and 1 for write.
* @param 	fn[in]: Function number.
* @param 	addr[in]: Register address.
* @param 	in[in]: Register data when write operation.
* @param 	out[out]: Read back register data after read/write operation.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_rw_direct(
	gpSDIOInfo_t *sdio,
	INT32U write,
	INT32U fn,
	INT32U addr,
	INT8U in,
	INT8U* out)
{
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[sdio->device_id];
	sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
	INT32U arg;

	if( gp_sdio_info[sdio->device_id].present==0 )
		return -1;

	if ((fn >7) || (addr & ~0x1FFFF))
		return -1;
	/* ----- Setting argument ----- */
	arg = write ? 0x80000000 : 0x00000000;
	arg |= fn << 28;
	arg |= (write && out) ? 0x08000000 : 0x00000000;
	arg |= addr << 9;
	arg |= in;
	/* ----- Send CMD52 ----- */
	if( drvl2_sd_send_command( sdio->device_id, MASK_CMD52, arg, SDC_RESPTYPE_R5, 0, NULL ) < 0 )
		return -1;
	/* ----- Check response error ----- */
	if( l1_ctrl->resp[0] & ( R5_COM_CRC_ERROR | R5_ILLEGAL_COMMAND | R5_ERROR | R5_FUNCTION_NUMBER | R5_OUT_OF_RANGE ))
	{
		DBG_PRINT("IO_RW_DIRECT error 0x%x\r\n", l1_ctrl->resp[0] );
		return -1;
	}

	if (out)
		*out = l1_ctrl->resp[0] & 0xFF;
	return 0;
}

#if SDIO_DMA_WAIT_STATUS
static volatile INT8S sdio_wdma_result[SDC_TOTAL] = {C_DMA_STATUS_DONE, C_DMA_STATUS_DONE};
static volatile INT8S sdio_rdma_result[SDC_TOTAL] = {C_DMA_STATUS_DONE, C_DMA_STATUS_DONE};
#endif
/**
* @brief 	Send SDIO IO_RX_EXTENDED command(CMD53).
* @param 	sdio[in]: SDIO handle.
* @param 	write[in]: The direction of io operation. 0 for read, and 1 for write.
* @param 	fn[in]: Function number.
* @param 	addr[in]: Register address.
* @param 	incr_addr[in]: Increment address, 0 for R/W fixed address, 1 for R/W incrementing address.
* @param 	buf[in]: Buffer pointer ( must be 4 byte alignment).
* @param 	blocks[in]: Block number.
* @param 	blksz[in]: Block size.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_rw_extended(
	gpSDIOInfo_t *sdio,
	INT32U write,
	INT32U fn,
	INT32U addr,
	INT32U incr_addr,
	INT32U *buf,
	INT32U blocks,
	INT32U blksz)
{
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[sdio->device_id];
	sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
	sd_l1_data_t *l1_data = &ctrl->l1_data;
	INT32U arg, dma_seperate = 0, dma_ena = 1;

	if( gp_sdio_info[sdio->device_id].present==0 )
		return -1;

	if ((fn >7) || (addr & ~0x1FFFF))
		return -1;

	if ((blocks == 0) || (blksz == 0))
		return -1;
	/* ----- Setting argument ----- */
	arg = write ? 0x80000000 : 0x00000000;
	arg |= fn << 28;
	arg |= incr_addr ? 0x04000000 : 0x00000000;
	arg |= addr << 9;

	if (blocks == 1 && blksz <= 512)
		arg |= (blksz == 512) ? 0 : blksz;	/* byte mode */
	else
		arg |= 0x08000000 | blocks;			/* block mode */

    if(blocks == 1 /*&& write */&& (blksz == 2 || blksz == 4))//(JC)using PIO mode in byte mode
	{
        dma_ena = 0;
    }

	drvl2_sd_set_data( l1_data, buf, blksz, blocks, write/* ? SDC_WRITE : SDC_READ*/, /*1*/dma_ena);
	/* ----- Send CMD53 ----- */
	if( drvl2_sd_send_command( sdio->device_id, MASK_CMD53, arg, SDC_RESPTYPE_R5, /*1*/dma_seperate, l1_data ) < 0 )
	{
        DBG_PRINT("CMD53 fail return -8. id=%d\r\n", sdio->device_id);
        DBG_PRINT("buf=0x%08x, blksz=%u blocks=%u dma_ena=%u arg=0x%08x separate=%d\r\n", buf, blksz, blocks, dma_ena, arg, dma_seperate);
		return -8;
    }
	/* ----- Check response error ----- */
	if( l1_ctrl->resp[0] & ( R5_COM_CRC_ERROR | R5_ILLEGAL_COMMAND | R5_ERROR | R5_FUNCTION_NUMBER | R5_OUT_OF_RANGE ))
	{
		DBG_PRINT("IO_RW_EXTENDED error 0x%x\r\n", l1_ctrl->resp[0] );
		//drvl2_sd_sem_unlock(sdio->device_id);
		return -2;
	}
//	DBG_PRINT("IO_RW_EXTENDED resp 0x%x\r\n", l1_ctrl->resp[0]);
    if(1 == dma_seperate)
    {
        if(0 == write)
        {
#if (SDIO_DMA_WAIT_STATUS == 1)
            if(drvl1_sdio_read_data_by_dma(sdio->device_id, buf, blocks, blksz, (INT8S *) &sdio_rdma_result[sdio->device_id])<0)
#else
            if(drvl1_sdio_read_data_by_dma(sdio->device_id, buf, blocks, blksz, NULL)<0)
#endif
            {
                DBG_PRINT("IO_RW_EXTENDED rdma error!!\r\n");
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -3;
            }

#if (SDIO_DMA_WAIT_STATUS == 1)
            while (sdio_rdma_result[sdio->device_id] == C_DMA_STATUS_WAITING);
            if (sdio_rdma_result[sdio->device_id] != C_DMA_STATUS_DONE) {
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -4;
            }
#else
            if (drvl1_sdc_command_complete_wait(sdio->device_id, 4300)) {
                return -5;
            }
#endif
        }
        else
        {
#if (SDIO_DMA_WAIT_STATUS == 1)
            if(drvl1_sdio_write_data_by_dma(sdio->device_id, buf, blocks, blksz, (INT8S *) &sdio_wdma_result[sdio->device_id])<0)
#else
            if(drvl1_sdio_write_data_by_dma(sdio->device_id, buf, blocks, blksz, NULL)<0)
#endif
            {
                DBG_PRINT("IO_RW_EXTENDED wdma error!!\r\n");
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -6;
            }
#if (SDIO_DMA_WAIT_STATUS == 1)
            while (sdio_wdma_result[sdio->device_id] == C_DMA_STATUS_WAITING);
            if (sdio_wdma_result[sdio->device_id] != C_DMA_STATUS_DONE) {
                //drvl2_sd_sem_unlock(sdio->device_id);
                return -7;
            }
#else
            if (drvl1_sdc_data_complete_bit_wait(sdio->device_id, 1000)) {
                return -8;
            }
#endif
        }
        if (drvl1_sdc_stop_controller(sdio->device_id, 1000)) {
            return -9;
        }
    }
    return 0;
}
/**
* @brief 	enable will be cleared after interrupt occurred.
* @param 	sdio[in]: SDIO handle.
* @param 	en[in]: Enable(1) or disable(0).
* @return 	None.
*/
void drvl2_sdio_irq_en(
	gpSDIOInfo_t *sdio,
	INT8U en)
{
	if(en)
	{
		if (drvl1_sdio_config_bit_get(sdio->device_id) == 1)
        {
            INT8U extInt, extMux;

            drvl1_sdio_config_one_bit_intr_get(sdio->device_id, &extInt, &extMux);
            drv_l1_extabc_enable_set(extInt, TRUE);
        }
		drvl1_sdc_ioint_en( sdio->device_id, 1 );
		drvl1_sdc_int_en( sdio->device_id, C_SDC_INT_IO_INT, 1 );
		drvl1_sdc_dummyclk_en(sdio->device_id, 1 ); // williamyeo, ssv require dummy clock always enable, rtk no need
	}
	else
	{
		drvl1_sdc_dummyclk_en(sdio->device_id, 0 );
		drvl1_sdc_int_en( sdio->device_id, C_SDC_INT_IO_INT, 0 );
		drvl1_sdc_ioint_en( sdio->device_id, 0 );
		drvl1_sdc_sdio_int_clear( sdio->device_id );
		if (drvl1_sdio_config_bit_get(sdio->device_id) == 1)
        {
            INT8U extInt, extMux;

            drvl1_sdio_config_one_bit_intr_get(sdio->device_id, &extInt, &extMux);
            drv_l1_extabc_enable_set(extInt, FALSE);
			drv_l1_extabc_int_clr(extInt);
        }
	}
}

/**
* @brief 	Receive SDIO interrupt signal.
* @param 	sdio[in]: SDIO handle.
* @param 	wait[in]: Waiting for irq signal or not.
* @param 	timeout[in]: Timeout value in second.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_receive_irq(
	gpSDIOInfo_t *sdio,
	INT32U wait,
	INT16U timeout)
{
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[sdio->device_id];
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32S result;
	INT8U error;
	if(wait)
		result = (INT32S) OSQPend(ctrl->int_queue, timeout * OS_TICKS_PER_SEC, &error);
	else
		result = (INT32S) OSQAccept(ctrl->int_queue, &error);

	if(error == OS_NO_ERR)
		return 0;
	else
		return -1;
#else
	INT32U notify = ctrl->int_notify;
	if(wait)
	{
		INT32U count = 1000;
		while( notify == 0 && timeout )
		{
			drv_msec_wait(1);
			count --;
			if(count == 0)
			{
				timeout --;
				count = 1000;
			}
			notify = ctrl->int_notify;
		}
	}

	if(notify)
		ctrl->int_notify = 0;
	return (notify)? 0: -1;
#endif
}

/**
* @brief 	Read the common registers.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
static INT32S drvl2_sdio_read_cccr(
	gpSDIOInfo_t *sdio)
{
	INT32S ret;
	INT32S cccr_vsn;
	INT8U data;

	memset(&sdio->cccr, 0, sizeof(sdio_cccr_t));

	ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_CCCR, 0, &data );
	if (ret)
		goto out;

	cccr_vsn = data & 0x0f;

	if (cccr_vsn > SDIO_CCCR_REV_1_20) {
		DBG_PRINT("unrecognised CCCR structure version %d\n", cccr_vsn);
		return -1;
	}

	sdio->cccr.sdio_vsn = (data & 0xf0) >> 4;

	ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_CAPS, 0, &data );
	if (ret)
		goto out;

	if (data & SDIO_CCCR_CAP_SMB)
		sdio->cccr.multi_block = 1;
	if (data & SDIO_CCCR_CAP_LSC)
		sdio->cccr.low_speed = 1;
	if (data & SDIO_CCCR_CAP_4BLS)
		sdio->cccr.wide_bus = 1;

	if (cccr_vsn >= SDIO_CCCR_REV_1_10)
	{
		ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_POWER, 0, &data);
		if (ret)
			goto out;

		if (data & SDIO_POWER_SMPC)
			sdio->cccr.high_power = 1;
	}

	if (cccr_vsn >= SDIO_CCCR_REV_1_20)
	{
		ret = drvl2_sdio_rw_direct( sdio, 0, 0, SDIO_CCCR_SPEED, 0, &data);
		if (ret)
			goto out;

		if (data & SDIO_SPEED_SHS)
			sdio->cccr.high_speed = 1;
	}
out:
	return ret;
}

/**
* @brief 	Enable SDIO high speed mode.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
static INT32S drvl2_sdio_enable_hs(
	gpSDIOInfo_t *sdio)
{
	INT8U speed;
	if (!sdio->cccr.high_speed)
		return -1;

	if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_SPEED, 0, &speed) < 0 )
		return -1;

	speed |= SDIO_SPEED_EHS;

	if( drvl2_sdio_rw_direct(sdio, 1, 0, SDIO_CCCR_SPEED, speed, NULL) < 0 )
		return -1;
	return 0;
}

/**
* @brief 	Enable SDIO 4 bit mode.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
static INT32S  drvl2_sdio_enable_wide(
	gpSDIOInfo_t *sdio)
{
	INT8U ctrl;
	if (sdio->cccr.low_speed && !sdio->cccr.wide_bus)
		return 0;

	if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_IF, 0, &ctrl) < 0 )
		return -1;

	ctrl |= SDIO_BUS_WIDTH_4BIT;

	if( drvl2_sdio_rw_direct(sdio, 1, 0, SDIO_CCCR_IF, ctrl, NULL) < 0 )
		return -1;

//	drvl1_sdc_bus_width_set( sdio->device_id, 1);
	drvl1_sdc_bus_width_set( sdio->device_id, 4);
	return 0;
}

INT32S  drvl2_sdio_card_set_1_bit(
	INT32U device_id)
{
	INT8U ctrl;
    gpSDIOInfo_t *sdio = &gp_sdio_info[device_id];
	if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_IF, 0, &ctrl) < 0 )
		return -1;

    if (ctrl != 0)
        DBG_PRINT("SDIO_CCCR_IF=0x%08x\r\n", ctrl);

	ctrl |= SDIO_BUS_WIDTH_1BIT;

	if( drvl2_sdio_rw_direct(sdio, 1, 0, SDIO_CCCR_IF, ctrl, NULL) < 0 )
		return -1;
	return 0;
}

/**
* @brief 	SDIO interrupt service  function.
* @return 	None.
*/
#if 1
static void drvl2_sdio_isr(void)
{
	INT32U i;
	for( i=0 ; i<2 ; i++)
	{
		gpSDCtrl_t *ctrl = &gp_sdio_ctrl[i];
		if( (drvl1_sdc_int_en_check(i, C_SDC_INT_IO_INT )== 0) && (drvl1_sdc_sdio_int_check(i) == 0) )
		{
			//drvl1_sdc_sdio_int_clear(i);
			drvl2_sdio_irq_en( &gp_sdio_info[i], 0 );
			ctrl->int_notify = 1;
#if _OPERATING_SYSTEM == _OS_UCOS2
			OSQPost( ctrl->int_queue, (void*)1);
#endif
		}
	}
}
#endif

void drvl2_sdio_register_isr_handler(INT32U device_id, void* isr)
{
	if(device_id > (SDC_TOTAL - 1))
		return;
	sdio_isr_handler[device_id] = (DRV_L2_SDIO_ISR)isr;
}

static INT8U drvl2_sdio_inited = 0;

/**
* @brief 	Initial SDIO card.
* @param 	device_id[in]:  Index of SD controller.
* @return 	0: success, -1: fail.
*/
INT32S drvl2_sdio_init(
	INT32U device_id )
{
	INT32U response;
	INT32U arg;
	gpSDIOInfo_t *sdio = &gp_sdio_info[device_id];
	gpSDCtrl_t *ctrl = &gp_sdio_ctrl[device_id];
	sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
	sd_l1_data_t *l1_data = &ctrl->l1_data;
    INT32U spend_ms;
    INT32U max_ms;
    INT32U clk = 25000000;
    if (drvl2_sdio_inited)
        return 0;

    drvl2_sd_sem_init(device_id);
	//drvl2_sd_sem_lock(device_id);
/*
	sd_card_rca[device_id] = 0x0;
	sd_card_type[device_id] = C_MEDIA_STANDARD_SD_CARD;
	sd_card_total_sector[device_id] = 0;
	sd_card_speed[device_id] = 400000;				// 400K Hz
	sd_card_bus_width[device_id] = 1;
	sd_card_protect[device_id] = 0;
	sd_card_state[device_id] = SD_CARD_STATE_IDLE;
	drvl1_sdc_init(device_id);
	drvl1_sdc_enable(device_id);
*/

	/* ----- Clear all variable ----- */
	memset( sdio, 0, sizeof(gpSDIOInfo_t) );
	memset( &gp_sdio_ctrl[device_id].l1_ctrl, 0, sizeof(sd_l1_ctrl_t));
	memset( &gp_sdio_ctrl[device_id].l1_data, 0, sizeof(sd_l1_data_t));
	/* ----- Initial queue ----- */
#if _OPERATING_SYSTEM == _OS_NONE
	ctrl->int_queue = OSQCreate(&ctrl->int_buf, 1);
	l1_data->dma_q = OSQCreate(&l1_data->dma_q_buf, 1);
#endif
	sdio->device_id = device_id;
	/* ----- Initial SD controller ----- */
	drvl1_sdc_init(device_id);
	drvl1_sdc_enable(device_id);
	/* ----- Start 74 cycles on SD Clk Bus ----- */
	drvl1_sdc_card_init_74_cycles(device_id);
	//drvl1_sdc_card_init_74_cycles(device_id);
	/* ----- Reset Command, Should be no response ----- */
	if( drvl2_sd_send_command( device_id, MASK_CMD0, 0x00, SDC_RESPTYPE_NONE, 0, NULL ) < 0 )
        goto fail;
    DBG_PRINT("##SDIO CMD0 ok!\r\n");
	/* ----- Skip CMD8, for our application SDIO card does not work with memory card ----- */
	//drvl2_sd_send_command( device_id, MASK_CMD8, 0x1AA, SDC_RESPTYPE_R7, 0, NULL );
	/* ----- Send CMD5 to detect SDIO card ----- */
	drv_msec_wait(1);
    for (max_ms=200, spend_ms=0; spend_ms < max_ms; spend_ms++)
    {
        if( drvl2_sd_send_command( device_id, MASK_CMD5, 0x00, SDC_RESPTYPE_R4, 0, NULL ) < 0 )
            goto fail;
        if (l1_ctrl->resp[0] != 0)
            break;
        drv_msec_wait(1);
    }
    DBG_PRINT("##SDIO CMD5 ok! resp=%x, spend_ms=%u\r\n", l1_ctrl->resp[0], spend_ms);
	/* ----- Check operation voltage ----- */
	if( (l1_ctrl->resp[0] & SDIO_OCR) == 0 )
	{
		DBG_PRINT("Not support OCR 0x%x. spend_ms=%u\r\n", l1_ctrl->resp[0], spend_ms);
		goto fail;
	}

	/* ----- Run CMD5 until card finish power up sequence ----- */
	drv_msec_wait(1);
	for (max_ms=200, spend_ms=0;spend_ms < max_ms; spend_ms++)
	{
		if( drvl2_sd_send_command( device_id, MASK_CMD5, SDIO_OCR, SDC_RESPTYPE_R4, 0, NULL ) < 0 )
			goto fail;
        if (( l1_ctrl->resp[0] & 0x80000000 ) != 0)
            break;
        drv_msec_wait(1);
	}
	if (( l1_ctrl->resp[0] & 0x80000000 ) == 0)
	{
        DBG_PRINT("SDIO power-up stable trainning timeout... spend_ms=%u\r\n", spend_ms);
        goto fail;
    }
	DBG_PRINT("##SDIO pwrup stable! spend_ms=%u\r\n", spend_ms);

	/* ----- Step 6, CMD 3 Read New RCA, SD will generate RCA itself ----- */
	drv_msec_wait(1);
	if( drvl2_sd_send_command( device_id, MASK_CMD3, 0x00, SDC_RESPTYPE_R6, 0, NULL) < 0 )
		goto fail;
    DBG_PRINT("##SDIO CMD3 ok! resp=%x\r\n", l1_ctrl->resp[0]);
	sdio->RCA = l1_ctrl->resp[0] & 0xFFFF0000;
	/* ----- Select card ----- */
	if( drvl2_sd_send_command( device_id, MASK_CMD7, sdio->RCA, SDC_RESPTYPE_R1b, 0, NULL) < 0 )
		goto fail;
    DBG_PRINT("##SDIO CMD7 ok!\r\n");
	sdio->present = 1;
	/* ----- Read common reister ----- */
	if(drvl2_sdio_read_cccr(sdio)<0)
		goto fail;
	/* ----- Read the common CIS tuple ----- */
	if(drvl2_sdio_read_common_cis(sdio)<0)
		goto fail;
	/* ----- Switch to high-speed (if supported). */
	if(drvl2_sdio_enable_hs(sdio) == 0){
		//drvl1_sdc_clock_set(device_id, 50000000);
        drvl1_sdc_clock_set(device_id, clk);
        DBG_PRINT("SDIO clk =%d\r\n",clk);        
    }
	else
		drvl1_sdc_clock_set(device_id, sdio->cis.max_dtr);
    {
        INT8U cccr_if_reg;

        if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_IF, 0, &cccr_if_reg) < 0 )
            goto fail;
        DBG_PRINT("SDIO_CCCR_IF value=%u\r\n", cccr_if_reg);
    }

    if (drvl1_sdio_config_bit_get(device_id) == 1)
    {
	    INT8U extInt, extMux;
        /* ----- Switch to 1 bit bus ----- */
        drvl1_sdc_bus_width_set(device_id, 1);

        /* ----- Set 1 bit external interrupt pin ----- */
        // SD0_1, DAT3, EXTC_MUX0, SD1_0, DAT1, EXTB_MUX3
        drvl1_sdio_config_one_bit_intr_get(device_id, &extInt, &extMux);
        drvl1_sdio_install_1_bit_isr(device_id, extInt, extMux, sdio_host_isr);
        DBG_PRINT("SDIO USE 1 BIT, extInt=%u, extMux=%u\r\n", extInt, extMux);
    }
    else
    {
        /* ----- Switch to wider bus (if supported). ----- */
        if(drvl2_sdio_enable_wide(sdio) < 0)
            goto fail;
    }

//    if( drvl2_sd_send_command( device_id, MASK_CMD52, 0x00, SDC_RESPTYPE_R5, 0, NULL ) < 0 )
//		goto fail;
//    DBG_PRINT("##SDIO CMD52 ok! resp=%x\r\n", l1_ctrl->resp[0]);
	/* ----- Register and enable interrupt function ----- */
//	vic_irq_register( (device_id==0)? VIC_SD:VIC_SD1, drvl2_sdio_isr);
//  vic_irq_enable((device_id==0)? VIC_SD:VIC_SD1);

#if (_OPERATING_SYSTEM == _OS_FREERTOS)
    NVIC_SetPriority((device_id == 0) ? SD0_IRQn : SD1_IRQn, 5);
    NVIC_EnableIRQ((device_id == 0) ? SD0_IRQn : SD1_IRQn);
#elif (_OPERATING_SYSTEM == _OS_UCOS2)
	if(sdio_isr_handler[device_id] != NULL)
	{
		vic_irq_register( (device_id==0)? VIC_SD1:VIC_SD2, sdio_isr_handler[device_id]);
		vic_irq_enable((device_id==0)? VIC_SD1:VIC_SD2);
	}
	else
	{
		DBG_PRINT("drvl2 SDIO[%d] isr handler is NULL!\r\n", device_id);
	}
#endif
	drvl2_sdio_inited = 1;

	return 0;
fail:
#if (_OPERATING_SYSTEM == _OS_UCOS2)
	{
		INT8U err;
		OSQDel( ctrl->int_queue, OS_DEL_ALWAYS, &err);
		OSQDel( l1_data->dma_q, OS_DEL_ALWAYS, &err);
	}
#endif
	sdio->present = 0;
	return -1;
}

INT32S drvl2_sdio_detect(
	INT32U device_id )
{
    INT32U response;
    INT32U arg;
    gpSDIOInfo_t *sdio = &gp_sdio_info[device_id];
    gpSDCtrl_t *ctrl = &gp_sdio_ctrl[device_id];
    sd_l1_ctrl_t *l1_ctrl = &ctrl->l1_ctrl;
    sd_l1_data_t *l1_data = &ctrl->l1_data;
    INT32U spend_ms;
    INT32U max_ms;

    DBG_PRINT("##SDIO detect\r\n");
    /* ----- Clear all variable ----- */
    memset( sdio, 0, sizeof(gpSDIOInfo_t) );
    memset( &gp_sdio_ctrl[device_id].l1_ctrl, 0, sizeof(sd_l1_ctrl_t));
    memset( &gp_sdio_ctrl[device_id].l1_data, 0, sizeof(sd_l1_data_t));
    /* ----- Initial queue ----- */
#if _OPERATING_SYSTEM == _OS_UCOS2
    ctrl->int_queue = OSQCreate(&ctrl->int_buf, 1);
    l1_data->dma_q = OSQCreate(&l1_data->dma_q_buf, 1);
#endif
    sdio->device_id = device_id;
    /* ----- Initial SD controller ----- */
    drvl1_sdc_init(device_id);
    drvl1_sdc_enable(device_id);
    /* ----- Start 74 cycles on SD Clk Bus ----- */
    drvl1_sdc_card_init_74_cycles(device_id);
    drvl1_sdc_card_init_74_cycles(device_id);
    /* ----- Reset Command, Should be no response ----- */
    if( drvl2_sd_send_command( device_id, MASK_CMD0, 0x00, SDC_RESPTYPE_NONE, 0, NULL ) < 0 )
        goto fail;
    DBG_PRINT("##SDIO CMD0 ok!\r\n");
    /* ----- Skip CMD8, for our application SDIO card does not work with memory card ----- */
    //drvl2_sd_send_command( device_id, MASK_CMD8, 0x1AA, SDC_RESPTYPE_R7, 0, NULL );
    /* ----- Send CMD5 to detect SDIO card ----- */
    for (max_ms=200, spend_ms=0; spend_ms < max_ms; spend_ms++)
    {
        if( drvl2_sd_send_command( device_id, MASK_CMD5, 0x00, SDC_RESPTYPE_R4, 0, NULL ) < 0 )
            goto fail;
        if (l1_ctrl->resp[0] != 0)
            break;
        drv_msec_wait(1);
    }
    DBG_PRINT("##SDIO CMD5 ok! resp=%x, spend_ms=%u\r\n", l1_ctrl->resp[0], spend_ms);
    /* ----- Check operation voltage ----- */
    if( (l1_ctrl->resp[0] & SDIO_OCR) == 0 )
    {
        DBG_PRINT("Not support OCR 0x%x. spend_ms=%u\r\n", l1_ctrl->resp[0], spend_ms);
        goto fail;
    }

    /* ----- Run CMD5 until card finish power up sequence ----- */
    for (max_ms=200, spend_ms=0;spend_ms < max_ms; spend_ms++)
    {
        if( drvl2_sd_send_command( device_id, MASK_CMD5, SDIO_OCR, SDC_RESPTYPE_R4, 0, NULL ) < 0 )
            goto fail;
        if (( l1_ctrl->resp[0] & 0x80000000 ) != 0)
            break;
        drv_msec_wait(1);
    }
    if (( l1_ctrl->resp[0] & 0x80000000 ) == 0)
    {
        DBG_PRINT("SDIO power-up stable trainning timeout... spend_ms=%u\r\n", spend_ms);
        goto fail;
    }
    DBG_PRINT("##SDIO pwrup stable! spend_ms=%u\r\n", spend_ms);

    /* ----- Step 6, CMD 3 Read New RCA, SD will generate RCA itself ----- */
    if( drvl2_sd_send_command( device_id, MASK_CMD3, 0x00, SDC_RESPTYPE_R6, 0, NULL) < 0 )
        goto fail;
    DBG_PRINT("##SDIO CMD3 ok! resp=%x\r\n", l1_ctrl->resp[0]);
    sdio->RCA = l1_ctrl->resp[0] & 0xFFFF0000;
    /* ----- Select card ----- */
    if( drvl2_sd_send_command( device_id, MASK_CMD7, sdio->RCA, SDC_RESPTYPE_R1b, 0, NULL) < 0 )
        goto fail;
    DBG_PRINT("##SDIO CMD7 ok!\r\n");
    sdio->present = 1;
    /* ----- Read common reister ----- */
    if(drvl2_sdio_read_cccr(sdio)<0)
        goto fail;
    /* ----- Read the common CIS tuple ----- */
    if(drvl2_sdio_read_common_cis(sdio)<0)
        goto fail;
    /* ----- Switch to high-speed (if supported). */
    if(drvl2_sdio_enable_hs(sdio) == 0)
        //drvl1_sdc_clock_set(device_id, 50000000);
        drvl1_sdc_clock_set(device_id, 25000000);
    else
        drvl1_sdc_clock_set(device_id, sdio->cis.max_dtr);

    {
        INT8U cccr_if_reg;

        if( drvl2_sdio_rw_direct(sdio, 0, 0, SDIO_CCCR_IF, 0, &cccr_if_reg) < 0 )
            goto fail;
        DBG_PRINT("SDIO_CCCR_IF value=%u\r\n", cccr_if_reg);
    }

    if (drvl1_sdio_config_bit_get(device_id) == 1)
    {
	    INT8U extInt, extMux;
        /* ----- Switch to 1 bit bus ----- */
        drvl1_sdc_bus_width_set(device_id, 1);

        /* ----- Set 1 bit external interrupt pin ----- */
        // SD0_1, DAT3, EXTC_MUX0, SD1_0, DAT1, EXTB_MUX3
        drvl1_sdio_config_one_bit_intr_get(device_id, &extInt, &extMux);
        drvl1_sdio_install_1_bit_isr(device_id, extInt, extMux, sdio_host_isr);
        DBG_PRINT("SDIO USE 1 BIT, extInt=%u, extMux=%u\r\n", extInt, extMux);
    }
    else
    {
        /* ----- Switch to wider bus (if supported). ----- */
        if(drvl2_sdio_enable_wide(sdio) < 0)
            goto fail;
    }
    
    //    if( drvl2_sd_send_command( device_id, MASK_CMD52, 0x00, SDC_RESPTYPE_R5, 0, NULL ) < 0 )
    //      goto fail;
    //    DBG_PRINT("##SDIO CMD52 ok! resp=%x\r\n", l1_ctrl->resp[0]);
    /* ----- Register and enable interrupt function ----- */
    //  vic_irq_register( (device_id==0)? VIC_SD:VIC_SD1, drvl2_sdio_isr);
    //  vic_irq_enable((device_id==0)? VIC_SD:VIC_SD1);

#if (_OPERATING_SYSTEM == _OS_FREERTOS)
    NVIC_SetPriority((device_id == 0) ? SD0_IRQn : SD1_IRQn, 5);
    NVIC_EnableIRQ((device_id == 0) ? SD0_IRQn : SD1_IRQn);
#elif(_OPERATING_SYSTEM == _OS_UCOS2)
	vic_irq_register( (device_id==0)? VIC_SD1:VIC_SD2, drvl2_sdio_isr);
    vic_irq_enable((device_id==0)? VIC_SD1:VIC_SD2);
#endif

    return 0;
    fail:
#if (_OPERATING_SYSTEM == _OS_UCOS2)
    {
          INT8U err;
          OSQDel( ctrl->int_queue, OS_DEL_ALWAYS, &err);
          OSQDel( l1_data->dma_q, OS_DEL_ALWAYS, &err);
    }
#endif
    sdio->present = 0;
    return -1;
}
#endif

/*
void SD0_IRQHandler(void)
{
    gpSDCtrl_t *ctrl = &gp_sdio_ctrl[1];
    if( (drvl1_sdc_int_en_check(1, C_SDC_INT_IO_INT)== 0) && (drvl1_sdc_sdio_int_check(1) == 0) )
	{
		drvl2_sdio_irq_en( &gp_sdio_info[1], 0 );
		ctrl->int_notify = 1;
    }
}
*/

