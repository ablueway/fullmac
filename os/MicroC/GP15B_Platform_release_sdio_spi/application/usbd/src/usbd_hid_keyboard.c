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
/*******************************************************
    Include file
*******************************************************/
#include "drv_l1_sfr.h"
#include "drv_l1_uart.h"
#include "project.h"
#include "gplib.h"
#include "usbd.h"

/******************************************************
    Definition and variable declaration
*******************************************************/
ALIGN4 INT8U hid_buf[8] = {0};
/******************************************************
    Functions declaration
*******************************************************/
void usbd_hid_keyboard_transfer_done(void)
{
}	

void usbd_hid_keyboard_init(void)
{
    INT32S ret;

    /* Init USBD L2 protocol layer first, including control/bulk/ISO/interrupt transfers */
    /******************************* Control transfer ************************************/
    ret = drv_l2_usbd_ctl_init();
    if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l2_usbd_ctl_init failed!\r\n");
        return;
    }
    
    /* Register new descriptor table here, this action must be done after drv_l2_usbd_ctl_init() */
    drv_l2_usbd_register_descriptor(REG_DEVICE_DESCRIPTOR_TYPE, (INT8U *)HID_Device_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_CONFIG_DESCRIPTOR_TYPE, (INT8U *)HID_Config_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_DEVICE_QUALIFIER_DESCRIPTOR_TYPE, (INT8U *)HID_Qualifier_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_STRING0_DESCRIPTOR_TYPE, (INT8U *)HID_String0_Descriptor);
    drv_l2_usbd_register_descriptor(REG_STRING1_DESCRIPTOR_TYPE, (INT8U *)HID_String1_Descriptor);
    drv_l2_usbd_register_descriptor(REG_STRING2_DESCRIPTOR_TYPE, (INT8U *)HID_String2_Descriptor);
	drv_l2_usbd_register_descriptor(REG_REPORT_DESCRIPTOR_TYPE, (INT8U *)HID_Key_Report_Descriptor);
	
   	/* Init USBD L2 of UVC */
    ret = drv_l2_usbd_hid_keyboard_init();
    if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l2_usbd_uvc_init failed!\r\n");
        return;
    }
	
	drv_l2_usbd_register_hid_keyboard_done_cbk((INT32U)usbd_hid_keyboard_transfer_done);

    /* Init USBD L1 register layer */
    ret = drv_l1_usbd_hid_keyboard_init();
     if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l1_usbd_uvc_init failed!\r\n");
        return;
    }
    
	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(drv_l1_usbd_isr_handle);
    
    DBG_PRINT("USB HID Keyborad device init completed!\r\n");
}

void usbd_hid_keyboard_uninit(void)
{
	drv_l2_usbd_ctl_uninit();
	drv_l2_usbd_hid_keyboard_uninit();
}	

USBD_CONTROL_BLK usbd_hid_keyboard_ctl_blk = 
{
	USBD_HID_KEYBOARD_MODE,
	usbd_hid_keyboard_init,
	usbd_hid_keyboard_uninit,
};	

