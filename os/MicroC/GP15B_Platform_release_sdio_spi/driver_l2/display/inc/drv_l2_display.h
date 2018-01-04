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
#ifndef __drv_l2_DISPLAY_H__
#define __drv_l2_DISPLAY_H__
#include "driver_l2.h"

// TFT Device, only one tft can be enable
#define TPO_TD025THD1		1
#define GPM_LM765A0			0

// data type
typedef enum
{
	DISDEV_TV_QVGA = 0,
	DISDEV_TV_VGA,
	DISDEV_TV_D1,
	DISDEV_TFT,
	DISDEV_HDMI_480P,
	DISDEV_HDMI_720P,
	DISDEV_MAX
} DISP_DEV;

typedef enum
{
	DISP_FMT_RGB565 = 0,
	DISP_FMT_BGRG,
	DISP_FMT_GBGR,
	DISP_FMT_RGBG,
	DISP_FMT_GRGB,
	DISP_FMT_VYUV,
	DISP_FMT_YVYU,
	DISP_FMT_UYVY,
	DISP_FMT_YUYV,
	DISP_FMT_GP420,
	DISP_FMT_MAX
} DISP_FMT;

typedef struct DispCtrl_s
{
	INT16U width;
	INT16U height;
	INT32S (*init)(void);
} DispCtrl_t;

extern INT32S drv_l2_display_init(void);
extern void drv_l2_display_uninit(void);
extern INT32S drv_l2_display_start(DISP_DEV disp_device, DISP_FMT color_mode);
extern INT32S drv_l2_display_stop(DISP_DEV disp_dev);
extern void drv_l2_display_get_size(DISP_DEV disp_dev, INT16U *width, INT16U *height);
extern INT32U drv_l2_display_get_fmt(DISP_DEV disp_dev);
extern INT32S drv_l2_display_update(DISP_DEV disp_dev, INT32U buffer);
#endif  //__drv_l2_DISPLAY_H__
