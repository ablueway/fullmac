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
#ifndef _USBD_H
#define _USBD_H
/*******************************************************
    Include file
*******************************************************/
#include "drv_l1_usbd.h"
#include "drv_l2_usbd.h"
#include "drv_l2_usbd_msdc.h"
#include "drv_l2_usbd_uvc.h"
#include "drv_l2_usbd_hid_keyboard.h"

/******************************************************
    Definition and variable declaration
*******************************************************/
#define USBD_NONE_MODE			0
#define USBD_MSDC_MODE			1
#define USBD_UVC_MODE			2
#define USBD_HID_KEYBOARD_MODE  3
#define USBD_DETECT_MODE		4

#define USBD_DET_PIN_OFF		0xFF
#define USBD_TASK_LOOP_TIME		10
#define USBD_DETECT_PIN			USBD_DET_PIN_OFF
#define USBD_DET_TIME_CNT  		5	

#define USBD_STORAGE_NO_WPROTECT	0
#define USBD_STORAGE_WPROTECT		1

typedef struct _USBD_CONTROL_BLK
{
    INT32U	device_type;
    void	(*usbd_init)(void);
    void	(*usbd_uninit)(void);
} USBD_CONTROL_BLK;

/******************************************************
    External API functions and variables declaration
*******************************************************/
extern USBD_CONTROL_BLK usbd_msdc_ctl_blk;
extern USBD_CONTROL_BLK usbd_uvc_ctl_blk;
extern USBD_CONTROL_BLK usbd_det_ctl_blk;
extern USBD_CONTROL_BLK usbd_hid_keyboard_ctl_blk;

extern MDSC_LUN_STORAGE_DRV const gp_msdc_ramdisk;
extern MDSC_LUN_STORAGE_DRV const gp_msdc_nand0;
extern MDSC_LUN_STORAGE_DRV const gp_msdc_nandapp0;
extern MDSC_LUN_STORAGE_DRV const gp_msdc_sd0;
extern MDSC_LUN_STORAGE_DRV const gp_msdc_sd1;

extern INT8U Default_Device_Descriptor_TBL[];
extern INT8U Default_Qualifier_Descriptor_TBL[];
extern INT8U Default_Config_Descriptor_TBL[];
extern INT8U Default_String0_Descriptor[];
extern INT8U Default_String1_Descriptor[];
extern INT8U Default_String2_Descriptor[];
extern INT8U Default_scsi_inquirydata[];
extern INT8U Default_scsi_inquirydata_CDROM[];

extern INT8U USB_UVC_DeviceDescriptor[];
extern INT8U USB_UVC_Qualifier_Descriptor_TBL[];
extern INT8U USB_UVC_ConfigDescriptor[];
extern INT8U UVC_String0_Descriptor[];
extern INT8U UVC_String1_Descriptor[];
extern INT8U UVC_String2_Descriptor[];
extern INT8U UVC_String3_Descriptor[];
extern INT8U UVC_String4_Descriptor[];
extern INT8U UVC_String5_Descriptor[];

extern INT8U HID_Device_Descriptor_TBL[];
extern const INT8U HID_Qualifier_Descriptor_TBL[];
extern INT8U HID_Config_Descriptor_TBL[];
extern const INT8U HID_String0_Descriptor[];
extern const INT8U HID_String1_Descriptor[];
extern const INT8U HID_String2_Descriptor[];
extern const INT8U HID_Key_Report_Descriptor[];
extern const INT8U HID_Descriptor[];

extern void usbd_init(void);
extern void usbd_enable(INT32U enable);
extern void usbd_register_class(USBD_CONTROL_BLK* device);
extern void usbd_task_init(void);

extern INT32S drv_l2_usbd_msdc_demo_write(INT32U addr, INT32U len, void* buf, void* pri);
extern INT32S drv_l2_usbd_msdc_demo_read(INT32U addr, INT32U len, void* buf, void* pri);
#endif