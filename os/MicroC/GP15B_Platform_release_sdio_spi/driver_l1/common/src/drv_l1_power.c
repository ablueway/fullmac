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
* Purpose:  LDO power driver
* Author: 
* Date: 	2014/09/17
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/
#include "drv_l1_sfr.h"
#include "drv_l1_power.h"

/****************************************************
*		Definition 									*
****************************************************/


/****************************************************
*		varaible and data declaration				*
****************************************************/
/*****************************************
*			Local functions				 *
*****************************************/
/**
 * @brief   set adc dac codec power on / off
 * @param   enable[in]: 0: diable, 1: enable 
 * @param   mode[in]: voltage select, LDO_CODEC_3P0V ~ LDO_CODEC_3P3V
 * @return 	STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_power_ldo_codec_ctrl(INT32U enable, INT32U mode)
{
	// turn on/off LDO codec
	int sel = 0x100 | mode;
	int reg = R_SYSTEM_POWER_CTRL0;
	
	if(enable)
	{
		reg &= (~0x700);
		reg |= sel;
		R_SYSTEM_POWER_CTRL0 = reg;
	}
	else
	{
		R_SYSTEM_POWER_CTRL0 &= (~0x700);
	}
	return STATUS_OK;
}

/**
 * @brief   set sensor power on / off
 * @param   enable[in]: 0: diable, 1: enable 
 * @param   mode[in]: voltage select, LDO_LDO28_2P7V ~ LDO_LDO28_3P3V
 * @return 	STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_power_ldo_28_ctrl(INT32U enable, INT32U mode)
{
	// turn on/off LDO 2.8v
	int sel = MASK_POWER_CTL_LDO28EN | mode;
	int reg = R_SYSTEM_POWER_CTRL0;

	if(enable)
	{
		reg &= (~0xF0);
		reg |= sel;
		R_SYSTEM_POWER_CTRL0 = reg;
	}
	else
	{
		R_SYSTEM_POWER_CTRL0 &= (~0xF0);
	}
	return STATUS_OK;
}

/**
 * @brief   set lod v3.3 off
 * @param   none
 * @return 	STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_power_ldo33_off(void)
{
	// turn off LDO 3.3v  (turn on in rom code)
	R_SYSTEM_POWER_CTRL0 &= (~0x112);
	R_SYSTEM_POWER_CTRL0 &= (~0x1);	
	return STATUS_OK;
}
