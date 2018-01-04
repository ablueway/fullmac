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
#ifndef __drv_l1_CSI_H__
#define __drv_l1_CSI_H__

/****************************************************
*		include file								*
****************************************************/
#include "driver_l1.h"

/****************************************************
*	Definition	 									*
****************************************************/
typedef	void (*CSI_CALLBACK_FUN)(INT32U event);

typedef enum
{
	CSI_NON_INTERLACE = 0,
	CSI_INTERLACE,
	CSI_INTERLACE_INVERT_FIELD
} CSI_INTERLACE_MODE_DEF;

typedef enum
{
	CSI_IN_RGB888_BGRG = 0,
	CSI_IN_RGB888_GBGR,
	CSI_IN_RGB565_BGRG,
	CSI_IN_RGB565_GBGR,
	CSI_IN_UYVY,
	CSI_IN_UYVY_INVERT_UV7,
	CSI_IN_YUYV,
	CSI_IN_YUYV_INVERT_UV7
} CSI_INPUT_DATA_DEF;

typedef enum
{
	CSI_OUT_RGB565 = 0,
	CSI_OUT_RGB1555,
	CSI_OUT_VYUY,
	CSI_OUT_VYUY_INVERT_UV7,
	CSI_OUT_YUYV,
	CSI_OUT_YUYV_INVERT_UV7,
	CSI_OUT_Y_ONLY
} CSI_OUTPUT_DATA_DEF;

typedef enum
{
	CSI_HREF = 0,
	CSI_HSYNC_CCIR_601,
	CSI_HSYNC_CCIR_656
} CSI_INPUT_INTERFACE_DEF;

typedef enum
{
	CSI_SENSOR_FPS_7 = 0, 
	CSI_SENSOR_FPS_10, 
	CSI_SENSOR_FPS_15, 
	CSI_SENSOR_FPS_27, 
	CSI_SENSOR_FPS_30 
} CSI_SENSOR_FPS_DEF;

typedef enum
{
	CSI_NONE_EVENT,
	CSI_SENSOR_FRAME_END_EVENT, 
	CSI_MOTION_DET_EVENT, 
	CSI_SENSOR_POSITION_HIT_EVENT, 
	CSI_MOTION_FIFO_UNDERRUN_EVENT, 
	CSI_SENSOR_FIFO_OVERFLOW_EVENT,
	CSI_SENSOR_OUTPUT_FIFO_EVENT 
} CSI_EVENT_DEF;


/********************* Define R_TGR_IRQ_STATUS bit mask (0xD0500238, Sensor IRQ flag register) *****************/
#define MASK_CSI_OUTPUT_FIFO_FLAG		BIT5					/* Sensor output FIFO mode IRQ flag */
#define MASK_CSI_FIFO_OVERFLOW_FLAG		BIT4					/* Sensor receive FIFO overflow IRQ flag */
#define MASK_CSI_MFIFO_UNDERRUN_FLAG	BIT3					/* Motion detect FIFO under-run IRQ flag */
#define MASK_CSI_POSITION_HIT_FLAG		BIT2					/* Sensor position hit IRQ flag */
#define MASK_CSI_MOTION_DET_FLAG		BIT1					/* Motion detect frame end IRQ flag */
#define MASK_CSI_FRAME_END_FLAG			BIT0					/* Sensor frame end IRQ flag */

/********************* Define R_TGR_IRQ_EN bit mask (0xD050023C, Sensor IRQ enable register) *****************/
#define MASK_CSI_OUTPUT_FIFO_ENABLE		BIT5					/* Sensor output FIFO mode IRQ enable */
#define MASK_CSI_FIFO_OVERFLOW_ENABLE	BIT4					/* Sensor receive FIFO overflow IRQ enable */
#define MASK_CSI_MFIFO_UNDERRUN_ENABLE	BIT3					/* Motion detect FIFO under-run IRQ enable */
#define MASK_CSI_POSITION_HIT_ENABLE	BIT2					/* Sensor position hit IRQ enable */
#define MASK_CSI_MOTION_DET_ENABLE		BIT1					/* Motion detect frame end IRQ enable */
#define MASK_CSI_FRAME_END_ENABLE		BIT0					/* Sensor frame end IRQ enable */

/********************* Define R_CSI_TG_CTRL0 bit mask (0xD0500240, Sensor timing generator control register 0) *****************/
#define MASK_CSI_CTRL0_VGA_TO_D1			BIT17		/* Sensor VGA upscaler to D1 enable.  During this mode, the sensor input must be set to VGA resolution, and the output frame buffer will be D1 size */
#define MASK_CSI_CTRL0_OWN_IRQ				BIT16		/* Sensor・s own interrupt ID enable bit.  When this bit is 1, the sensor related IRQ will go through another interrupt ID and separated from PPU・s IRQ */
#define MASK_CSI_CTRL0_INTL					BIT15		/* Sensor input interlace/non-interlace selection */
#define MASK_CSI_CTRL0_FIELDINV				BIT14		/* Sensor filed invert selection.  Used only in interlace mode */
#define MASK_CSI_CTRL0_YUVTYPE				BIT13		/* YUV input type selection. 0: UYVY or BGRG 1: YUYV or GBGR */
#define MASK_CSI_CTRL0_VRST					BIT12		/* Vertical counter reset selection */
#define MASK_CSI_CTRL0_VADD					BIT11		/* Vertical counter increase selection */
#define MASK_CSI_CTRL0_HRST					BIT10		/* Horizontal counter reset selection */
#define MASK_CSI_CTRL0_FGET					BIT9		/* Field latch timing selection */
#define MASK_CSI_CTRL0_CCIR656				BIT8		/* CCIR656 selection, 0: CCIR601, 1: CCIR656 */
#define MASK_CSI_CTRL0_YUVOUT				BIT6		/* YUV/RGB output selection */
#define MASK_CSI_CTRL0_YUVIN				BIT5		/* YUV/RGB input selection */
#define MASK_CSI_CTRL0_CLKIINV				BIT4		/* Input clock invert selection */
#define MASK_CSI_CTRL0_RGB565				BIT3		/* RGB565 mode input selection.  This bit is useful only when YUVIN is 0 */
#define MASK_CSI_CTRL0_HREF					BIT2		/* HREF mode selection.This bit is useful for OV・s sensor */
#define MASK_CSI_CTRL0_CAP  				BIT1		/* Capture/preview mode selection. This bit must be set to 1 */
#define MASK_CSI_CTRL0_CSIEN 				BIT0		/* Sensor controller enable bit */

/********************* Define R_CSI_TG_CTRL1 bit mask (0xD0500244, Sensor timing generator control register 1) *****************/
#define MASK_CSI_CTRL1_CLK_STOPB			BIT14		/* Clock output stop selection when system is busy, 0: stop clock output when busy, 1: not stop clock output when busy */
#define MASK_CSI_CTRL1_CLK_SEL				BIT11		/* Sensor master clock selection */
#define MASK_CSI_CTRL1_YONLY				BIT10		/* Y output only selection */
#define MASK_CSI_CTRL1_INVYUVI				BIT9		/* UV input invert selection, useful only in YUV input mode */
#define MASK_CSI_CTRL1_CUTEN				BIT8		/* Screen CUT enable */
#define MASK_CSI_CTRL1_CLKOEN				BIT7		/* CSICLKO output clock enable */
#define MASK_CSI_CTRL1_INVYUVO				BIT6		/* Invert UV・s bit 7 at YUV output mode, useless in RGB output mode */
#define MASK_CSI_CTRL1_RGB1555				BIT4		/* RGB1555 mode output selection */
#define MASK_CSI_CTRL1_CLKOINV				BIT3		/* Output clock invert selection */

#define DELAY_1CLOCK         				(0)			/* Data latch delay 1 clock */
#define DELAY_2CLOCK         				(0x01)		/* Data latch delay 2 clock */
#define DELAY_3CLOCK         				(0x02)		/* Data latch delay 3 clock */
#define DELAY_4CLOCK         				(0x03)		/* Data latch delay 4 clock */

/****************************************************
*		external function declarations				*
****************************************************/
extern void drv_l1_csi_isr(void);
extern void drv_l1_register_csi_cbk(CSI_CALLBACK_FUN fun);
extern void drv_l1_csi_set_buf(INT32U buf);
extern INT32U drv_l1_csi_get_buf(void);
#endif	//__drv_l1_CSI_H__