/******************************************************
* drv_l2_usbd_hid_keyboard.h
*
* Purpose: USB l2 controller driver/interface
*
* Author: Eugene Hsu
*
* Date: 2014/9/23
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
*******************************************************/
#ifndef DRV_L2_USBD_HID_KEYBOARD_H
#define DRV_L2_USBD_HID_KEYBOARD_H
#include "project.h"
#include "Customer.h"
/*********************************************************************
        Structure, enumeration and definition
**********************************************************************/
typedef	void (*HID_TRANSFER_DONE)(void);

/******************************************************
    Variables, functions definitions
******************************************************/
extern const INT8U HID_Descriptor[];

extern INT32S drv_l2_usbd_hid_keyboard_init(void);
extern void drv_l2_usbd_hid_keyboard_uninit(void);
extern INT32S drv_l1_usbd_hid_keyboard_init(void);
extern void drv_l2_usbd_register_hid_keyboard_done_cbk(INT32U cbk);
extern INT8U drv_l2_usbd_get_hid_keyboard_led_status(void);
#endif  //DRV_L2_USBD_HID_KEYBOARD_H
