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
#include "drv_l1_conv422to420.h"
#include "drv_l1_interrupt.h"

#if (defined _DRV_L1_CONV422TO420) && (_DRV_L1_CONV422TO420 == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CONV422_AUTO_CLEAR		(1 << 8)
#define CONV422_BYPASS_EN		(1 << 9)
#define CONV422_FRAME_MODE_EN	(1 << 10)
#define CONV422_YUV422_FORMAT	(1 << 11)
#define CONV422_IRQ_EN			(1 << 12)
#define CONV422_IRQ_FIFO_CHANGE	(1 << 13)
#define CONV422_IRQ_FIFO_BUF_A	(1 << 14)
#define CONV422_IRQ_FIFO_BUF_B	(1 << 15)
#define CONV422_IRQ_FRAME_END	(1 << 16) 
#define CONV422_IRQ_ALL			(0x0F << 13)

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static void (*conv422_callback)(INT32U event);

static void conv422_isr(void)
{
	INT32U event = 0;
	INT32U reg = R_CONV422_CTRL;
	
	// clear irq flag
	R_CONV422_CTRL = reg;

	if(reg & CONV422_IRQ_FRAME_END) {
		event |= CONV422_FRAME_END;
	}
	
	if(reg & CONV422_IRQ_FIFO_BUF_B) {
		event |= CONV422_IRQ_BUF_B;
	}
	
	if(reg & CONV422_IRQ_FIFO_BUF_A) {
		event |= CONV422_IRQ_BUF_A;
	}
	
	if(reg & CONV422_IRQ_FIFO_CHANGE) {
		event |= CONV422_CHANGE_FIFO;
	}
	
	if(conv422_callback && event)	{
		conv422_callback(event);
	}
}

/**
 * @brief   initial conv422
 * @param   none
 * @return 	none
 */
void drv_l1_conv422_init(void)
{	
	R_CONV422_CTRL = CONV422_IRQ_ALL;	// clear irq flag
	vic_irq_register(VIC_CSI422TO420_HDMI, conv422_isr);
	vic_irq_enable(VIC_CSI422TO420_HDMI);	
}

/**
 * @brief   reset conv422
 * @param   none
 * @return 	none
 */
void drv_l1_conv422_reset(void)
{
	R_CONV422_CTRL |= CONV422_AUTO_CLEAR;// software reset
}

/**
 * @brief   set conv422 input image format
 * @param   format[in]: input format, see CONV422_YUV_FORMAT
 * @return 	none
 */
void drv_l1_conv422_input_fmt_set(INT32U format)
{
	R_CONV422_CTRL &= ~(0x03 << 1);
	R_CONV422_CTRL |= (format & 0x03) << 1;
}

/**
 * @brief   set conv422 bypass mode
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_conv422_bypass_enable(INT32U enable)
{
	if(enable) {
		R_CONV422_CTRL |= CONV422_BYPASS_EN;	// bypass
	} else {
		R_CONV422_CTRL &= ~CONV422_BYPASS_EN;	// disable bypass 
	}
}

/**
 * @brief   set conv422 fifo or frame mode
 * @param   mode[in]: 0: fifo mode, 1: frame mode, see CONV422_MODE
 * @return 	none
 */
void drv_l1_conv422_mode_set(INT32U mode)
{
	if(mode == CONV422_FRAME_MODE) {
		R_CONV422_CTRL |= CONV422_FRAME_MODE_EN;// frame mode
	} else {
		R_CONV422_CTRL &= ~CONV422_FRAME_MODE_EN;// fifo mode
	}
}

/**
 * @brief   set conv422 output format
 * @param   mode[in]: output format, see CONV422_FORMAT
 * @return 	none
 */
void drv_l1_conv422_output_format_set(INT32U mode)
{
	if(mode == CONV422_FMT_422) {		
		R_CONV422_CTRL |= CONV422_YUV422_FORMAT;	//format is yuv422 
	} else {
		R_CONV422_CTRL &= ~CONV422_YUV422_FORMAT;	//format is yuv420 
	}	
}

/**
 * @brief   set conv422 irq enable
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_conv422_irq_enable(INT32U enable)
{
	// clear irq flag
	R_CONV422_CTRL |= CONV422_IRQ_ALL;
	 
	if(enable) {
		R_CONV422_CTRL |= CONV422_IRQ_EN;	//enable irq
	} else {
		R_CONV422_CTRL &= ~CONV422_IRQ_EN;	//disable irq
	}	
}

/**
 * @brief   set conv422 output a address
 * @param   addr[in]: fifo line number 
 * @return 	none
 */
void drv_l1_conv422_output_A_addr_set(INT32U addr)
{
	R_CONV422_BUF_A_ADDR = addr;
}

/**
 * @brief   get conv422 output a address
 * @param   none
 * @return 	buffer a address
 */
INT32U drv_l1_conv422_output_A_addr_get(void)
{
	return R_CONV422_BUF_A_ADDR;
}

/**
 * @brief   set conv422 output b address
 * @param   addr[in]: fifo line number 
 * @return 	none
 */
void drv_l1_conv422_output_B_addr_set(INT32U addr)
{
	R_CONV422_BUF_B_ADDR = addr;	
}

/**
 * @brief   get conv422 output a address
 * @param   none
 * @return 	buffer b address
 */
INT32U drv_l1_conv422_output_B_addr_get(void)
{
	return R_CONV422_BUF_B_ADDR;
}

/**
 * @brief   set conv422 size
 * @param   inWidth[in]: image width
 * @param   inHeight[in]: image height 
 * @return 	none
 */
void drv_l1_conv422_input_pixels_set(INT32U inWidth, INT32U inHeight)
{
	R_CONV422_IMG_WIDTH = inWidth & 0xFFF;
	R_CONV422_IMG_HEIGHT = inHeight & 0xFFF;	
}

/**
 * @brief   set conv422 line size
 * @param   line[in]: fifo line number 
 * @return 	none
 */
void drv_l1_conv422_fifo_line_set(INT32U line)
{
	R_CONV422_DEPTH = line & 0xFFF;
}

/**
 * @brief   wait conv422 done
 * @param   none
 * @return 	interrupt flag
 */
void drv_l1_conv422_register_callback(void (*isr_handle)(INT32U event))
{
	conv422_callback = isr_handle;
}
#endif //(defined _DRV_L1_CONV422TO420) && (_DRV_L1_CONV422TO420 == 1)


