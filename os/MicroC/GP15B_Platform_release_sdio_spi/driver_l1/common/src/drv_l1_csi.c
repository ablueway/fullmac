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
/******************************************************************** 
* Purpose:  COMS censor interface layer 1 driver
* Author: 
* Date: 	2014/09/09
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/
#include "drv_l1_sfr.h"
#include "drv_l1_i2c.h"
#include "drv_l1_csi.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
/****************************************************
*		Definition 									*
****************************************************/

/****************************************************
*		varaible and data declaration				*
****************************************************/
static	CSI_CALLBACK_FUN csi_callback = NULL;

/*****************************************
*			CSI functions				 *
*****************************************/
void drv_l1_register_csi_cbk(CSI_CALLBACK_FUN fun)
{
	csi_callback = fun;
}

void drv_l1_csi_isr(void)
{
	INT32U flag = R_TGR_IRQ_STATUS;
	INT32U enable = R_TGR_IRQ_EN;
	INT32U event = CSI_NONE_EVENT;
	
	flag &= enable;	
	if(flag & MASK_CSI_OUTPUT_FIFO_FLAG)
	{
		/* Sensor FIFO mode occurred */
		R_TGR_IRQ_STATUS = MASK_CSI_OUTPUT_FIFO_FLAG;
		event = CSI_SENSOR_OUTPUT_FIFO_EVENT;
	}
	else if(flag & MASK_CSI_FIFO_OVERFLOW_FLAG)
	{
		/* Sensor under run */
		R_TGR_IRQ_STATUS = MASK_CSI_FIFO_OVERFLOW_FLAG;
		event = CSI_SENSOR_FIFO_OVERFLOW_EVENT;
	}
	else if(flag & MASK_CSI_FRAME_END_FLAG)
	{
		/* Sensor frame end occurred */
		R_TGR_IRQ_STATUS = MASK_CSI_FRAME_END_FLAG;
		event = CSI_SENSOR_FRAME_END_EVENT;
	}
	else {
		R_TGR_IRQ_STATUS = flag;
	}	 
	
	if(event != CSI_NONE_EVENT && csi_callback != NULL)
	{
		/* Call callback function come from sensor */
		csi_callback(event);
	}	
}
	
 
void drv_l1_csi_set_buf(INT32U buf)
{
	R_CSI_TG_FBSADDR = buf;
}

INT32U drv_l1_csi_get_buf(void)
{
	return (INT32U)R_CSI_TG_FBSADDR;
}