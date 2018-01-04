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
#ifndef __drv_l1_POWER_H__
#define __drv_l1_POWER_H__

/****************************************************
*		include file								*
****************************************************/
#include "driver_l1_cfg.h"
#include "driver_l1.h"
#include "project.h"

/****************************************************
*	Definition	 									*
****************************************************/

/****************************************************
*		Register bitwise definition 				*
****************************************************/
/********************* Define R_SYSTEM_POWER_CTRL0 bit mask (0xC00B0000, I2C control register) *****************/
#define MASK_POWER_CTL_LDOCODECEN	BIT8		/* LDO codec enable */
#define MASK_POWER_CTL_LDO28EN		BIT4		/* LDO28 enable */
#define MASK_POWER_CTL_POWREN		BIT0		/* LDO33 for IO and LDO12 for core logic enable */

#define	LDO_CODEC_3V		(0<<9)
#define	LDO_CODEC_3P1V		(1<<9)
#define	LDO_CODEC_3P2V		(2<<9)
#define	LDO_CODEC_3P3V		(3<<9)

#define	LDO_LDO28_2P8V		(0<<5)
#define	LDO_LDO28_2P9V		(1<<5)
#define	LDO_LDO28_3V		(2<<5)
#define	LDO_LDO28_3P1V		(3<<5)
#define	LDO_LDO28_3P2V		(4<<5)
#define	LDO_LDO28_3P3V		(5<<5)
#define	LDO_LDO28_2P7V		(6<<5)
#define	LDO_LDO28_2P8V_1	(7<<5)

#define	LDO_LDO33_3V		(0<<2)
#define	LDO_LDO33_3P1V		(1<<2)
#define	LDO_LDO33_3P2V		(2<<2)
#define	LDO_LDO33_3P3V		(3<<2)

/****************************************************
*		Data structure 								*
*****************************************************/

/****************************************************
*		external function declarations				*
****************************************************/
extern INT32S drv_l1_power_ldo_codec_ctrl(INT32U enable, INT32U mode);
extern INT32S drv_l1_power_ldo_28_ctrl(INT32U enable, INT32U mode);
extern INT32S drv_l1_power_ldo33_off(void);
#endif	/*__drv_l1_POWER_H__*/