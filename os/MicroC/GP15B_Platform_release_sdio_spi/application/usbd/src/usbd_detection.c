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
/**************************************************************************
Purpose: The USB detection mode must cowork with GPIO detection PIN to
		detect what is the object(host or charger) connected to the deivce.
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

/******************************************************
    Functions declaration
*******************************************************/
void usbd_det_isr_cbk(void)
{
	/* Let DP/DM low to disconnect from host */
	drv_l1_usb_soft_disconnect();
	drv_l1_usbd_disable_isr();
	DBG_PRINT("USB cable connect to host\r\n");
}

void usbd_det_init(void)
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

	/* regiset call back function for getting reset signal from layer 2 USB device driver */
	drv_l2_usbd_register_detect_cbk((void*)usbd_det_isr_cbk);
	
    /* Init USBD L1 register layer */
    ret = drv_l1_usbd_init();
     if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l1_usbd_uvc_init failed!\r\n");
        return;
    }
    
	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(drv_l1_usbd_isr_handle);
    	
    DBG_PRINT("USB detection mode init completed!\r\n");
}

void usbd_det_uninit(void)
{
	drv_l2_usbd_ctl_uninit();
}	

USBD_CONTROL_BLK usbd_det_ctl_blk = 
{
	USBD_DETECT_MODE,
	usbd_det_init,
	usbd_det_uninit
};	

