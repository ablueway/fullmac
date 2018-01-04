/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "drv_l1_sfr.h" 
#include "drv_l1_conv420to422.h"
#include "drv_l1_interrupt.h"

#if (defined _DRV_L1_CONV420TO422) && (_DRV_L1_CONV420TO422 == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CONV420_ENABLE				(1 << 0)
#define CONV420_8BEAT_BURST			(1 << 4)
#define CONV420_DEST_SCALE0			(1 << 1)
#define CONV420_DO_START			(1 << 8)
#define CONV420_DO_RESET			(1 << 9)
#define CONV420_IRQ_EN				(1 << 20)
#define CONV420_MASTER_MASK_EN		(1 << 21)
#define CONV420_IRQ_FIFO_PEND		(1 << 29)
#define CONV420_BUSY_FLAG			(1 << 30)
#define CONV420_IRQ_FIFO_BUF		((INT32U)1<<31)

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static void (*conv420_callback)(INT32U event);

 
static void conv420_isr(void)
{
	INT32U event = 0;
	INT32U reg = R_CONV420_CTRL;

	// isr indicator flag
	if((reg & CONV420_IRQ_FIFO_BUF) == 0) {
		return;
	}	
	
	// clear isr indicator flag	
	R_CONV420_CTRL = reg;
	
	// busy flag 
	if((reg & CONV420_BUSY_FLAG) == 0) {	
		event |= CONV420_IRQ_FRAME_END;
	}
	
	// fifo flag
	if((reg & CONV420_IRQ_FIFO_PEND) == 0) {
		event |= CONV420_IRQ_FIFO_A;
	} else {
		event |= CONV420_IRQ_FIFO_B;
	} 
	
	if(conv420_callback && event) {
		conv420_callback(event);
	}
}

/**
 * @brief   initial conv420 
 * @param   none
 * @return 	none
 */
void drv_l1_conv420_init(void)
{
	R_CONV420_CTRL = CONV420_IRQ_FIFO_BUF;
	vic_irq_register(VIC_CONV420TO422, conv420_isr);
	vic_irq_enable(VIC_CONV420TO422);
}

/**
 * @brief   uninitial conv420 
 * @param   none
 * @return 	none
 */
void drv_l1_conv420_uninit(void)
{
	R_CONV420_CTRL = 0;
	vic_irq_unregister(VIC_CONV420TO422);
	vic_irq_disable(VIC_CONV420TO422);
}

/**
 * @brief   enable convert 420 to 422 
 * @param   none
 * @return 	none
 */
void drv_l1_conv420_convert_enable(INT32U enable)
{
	if(enable) {
		R_CONV420_CTRL |= CONV420_ENABLE;	
	} else {
		R_CONV420_CTRL &= ~CONV420_ENABLE;	
	}
}

/**
 * @brief   set convert 420 to 422 path 
 * @param   path[in]: output path, see CONV420_PATH
 * @return 	none
 */
void drv_l1_conv420_path(INT32U path)
{
	if(path == CONV420_TO_TV_HDMI) {
		R_CONV420_CTRL &= ~CONV420_DEST_SCALE0;
	} else {
		R_CONV420_CTRL |= CONV420_DEST_SCALE0;
	}
}

/**
 * @brief   set output format
 * @param   format[in]: output format, see CONV420_FORMAT 
 * @return 	none
 */
void drv_l1_conv420_output_fmt_set(INT32U format)
{
	R_CONV420_CTRL &= ~(0x03 << 2);
	R_CONV420_CTRL |= (format & 0x03) << 2;
}

/**
 * @brief   start convert 420 to 422 
 * @param   none
 * @return 	none
 */
void drv_l1_conv420_start(void)
{
	R_CONV420_CTRL |= CONV420_DO_START;
}

/**
 * @brief   reset convert 420 to 422 
 * @param   none
 * @return 	none
 */
void drv_l1_conv420_reset(void)
{
	R_CONV420_CTRL |= CONV420_DO_RESET;
}

/**
 * @brief   set fifo interrupt enable
 * @param   enable[in]: enable or disable
 * @return 	STATUS_OK / STATUS_FAIL
 */
void drv_l1_conv420_fifo_irq_enable(INT32U enable)
{
	if(enable) {
		R_CONV420_CTRL |= CONV420_IRQ_EN;			//interrupt enable
		R_CONV420_CTRL |= CONV420_MASTER_MASK_EN;	//master mask enable 
	} else {
		R_CONV420_CTRL &= ~CONV420_IRQ_EN;			//interrupt disable
		R_CONV420_CTRL &= ~CONV420_MASTER_MASK_EN;	//master mask disable 
	}
}

/**
 * @brief   set input a address
 * @param   path[in]: output path
 * @return 	none
 */
void drv_l1_conv420_input_A_addr_set(INT32U addr)
{
	R_CONV420_IN_A_ADDR = addr;
	R_CONV420_MASTER_MASK_INT = 0;
}

/**
 * @brief   get conv420 input a address
 * @param   none
 * @return 	buffer a address
 */
INT32U drv_l1_conv420_input_A_addr_get(void)
{
	return R_CONV420_IN_A_ADDR;
}

/**
 * @brief   set input b address
 * @param   path[in]: output path
 * @return 	none
 */
void drv_l1_conv420_input_B_addr_set(INT32U addr)
{	
	R_CONV420_IN_B_ADDR = addr;
	R_CONV420_MASTER_MASK_INT = 0;	
}

/**
 * @brief   get conv420 input b address
 * @param   none
 * @return 	buffer b address
 */
INT32U drv_l1_conv420_input_B_addr_get(void)
{
	return R_CONV420_IN_B_ADDR;
}

/**
 * @brief   set input width
 * @param   path[in]: output path
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_conv420_input_pixels_set(INT32U inWidth)
{
	if(inWidth & 0x03) {
		return STATUS_FAIL;
	}
	
	R_CONV420_LINE_CTRL &= ~(0x1FF);
	R_CONV420_LINE_CTRL |= ((inWidth >> 2) - 1);
	return STATUS_OK;	
}

/**
 * @brief   set fifo line size
 * @param   path[in]: output path
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_conv420_fifo_line_set(INT32U line)
{
	if(line == 0) {
		return STATUS_FAIL;
	}
	
	R_CONV420_LINE_CTRL &= ~(0xFF << 16);
	R_CONV420_LINE_CTRL |= ((line * 3) >> 1) << 16;	// conv420 need 1.5x	
	return STATUS_OK;	
}

/**
 * @brief   register isr callback function 
 * @param   isr_handle[in]: callback function
 * @return 	none
 */
void drv_l1_register_callback(void (*isr_handle)(INT32U event))
{
	conv420_callback = isr_handle;
}
#endif //(defined _DRV_L1_CONV420TO422) && (_DRV_L1_CONV420TO422 == 1)      



