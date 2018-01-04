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
#include "project.h"
#include "gplib.h"
#include "drv_l1_system.h"
#include "drv_l1_uart.h"
#include "drv_l1_gpio.h"
#include "application.h"
#include "usbd.h"

/******************************************************
    Definition and variable declaration
*******************************************************/
#if (_OPERATING_SYSTEM == _OS_UCOS2)
#define USBDDETStackSize 	512
INT32U USBDDETStack[USBDDETStackSize];
#endif

typedef struct USB_DEVICE_BLOCK_S
{
	INT32U usb_plug_in;
	INT32U usb_det_state;
	USBD_CONTROL_BLK* usb_device_ctl;
} USB_DEVICE_BLOCK_T;	

static USB_DEVICE_BLOCK_T usb_block =
{
	0,
	0,
	NULL
};	

/******************************************************
    USB device application functions
*******************************************************/	
void usbd_enable(INT32U enable)
{
	if(enable)
	{
		/* Enable USB device clock */
		R_SYSTEM_CLK_EN1 |= 0x4; 	
		/* switch to USB device mode  bit8 = 0 */
		R_SYSTEM_MISC_CTRL0 &= ~MASK_SYSMISC_CTL0_HOST_SW;
		
		rUDCCS_UDC |= (MASK_USBD_UDC_CS_FORCE_CONNECT | MASK_USBD_UDC_CS_FORCE_DISCONNECT | MASK_USBD_UDC_CS_SYS_SUSPENDM | MASK_USBD_UDC_CS_SYS_PARTIALM);
		/* Enable USB device controller */
		rUDCCS_UDC &= ~MASK_USBD_UDC_CS_SYS_DISCONNECT;	
	}
	else
	{
		/* switch to USB device mode  bit8 = 0 */
		R_SYSTEM_MISC_CTRL0 &= ~MASK_SYSMISC_CTL0_HOST_SW;
		
		rUDCCS_UDC &= ~(MASK_USBD_UDC_CS_FORCE_CONNECT | MASK_USBD_UDC_CS_FORCE_DISCONNECT | MASK_USBD_UDC_CS_SYS_SUSPENDM);
		
		/* wait for 30ns */
		__asm {NOP};
		__asm {NOP};   
		__asm {NOP};
		__asm {NOP};
		__asm {NOP};
		__asm {NOP};
		__asm {NOP};
		
		rUDLC_SET0 &= ~(MASK_USBD_UDLC_SET0_PWR_SUSPEND_N | MASK_USBD_UDLC_SET0_PWR_PARTIAL_N);
		
		rUDLC_SET0 |= (MASK_USBD_UDLC_SET0_DISCONNECT_SUSPEND_EN | MASK_USBD_UDLC_SET0_CPU_WAKE_UP_EN | MASK_USBD_UDLC_SET0_SOFT_DISCONNECT);
		
		/* Disable USB device controller */
		rUDCCS_UDC |= MASK_USBD_UDC_CS_SYS_DISCONNECT;	
		
		rUDCCS_UDC &= ~(MASK_USBD_UDC_CS_SYS_PARTIALM | MASK_USBD_UDC_CS_SYS_SUSPENDM);
		
		/* Disable USB device clock */
		R_SYSTEM_CLK_EN1 &= ~(0x4);
		/* switch to USB host mode  bit8 = 1 */
		R_SYSTEM_MISC_CTRL0 |= MASK_SYSMISC_CTL0_HOST_SW;
	}
		
}	

void usbd_register_class(USBD_CONTROL_BLK* device)
{
	if(device != NULL)
	{	
		usb_block.usb_device_ctl = device;
	}
}	

void usbd_init(void)
{	
	if(usb_block.usb_device_ctl != NULL)
	{
		usbd_enable(1);
		usb_block.usb_device_ctl->usbd_init();
	}
	else
	{
		DBG_PRINT("Warning: the usb device handle is NULL in usbd_init\r\n");
	}	
}	

void usbd_uninit(void)
{	
	if(usb_block.usb_device_ctl != NULL)
	{
		drv_l1_usbd_uninit();
		/* Turn off USB PHY clock & USB device clk */
		usbd_enable(0);
		usb_block.usb_device_ctl->usbd_uninit();

#if _OPERATING_SYSTEM == _OS_UCOS2	
		/* Delete USB layer 2 task if OS exist */
		drv_l2_usbd_main_task_exit();
#endif	
	}
	else
	{
		DBG_PRINT("Warning: the usb device handle is NULL in usbd_uninit\r\n");
	}	
}

static void usbd_init_detect_pin(void)
{
	if(USBD_DETECT_PIN == USBD_DET_PIN_OFF)
	{
		return;
	}	
		
	gpio_init_io(USBD_DETECT_PIN, GPIO_INPUT);
	gpio_set_port_attribute(USBD_DETECT_PIN, ATTRIBUTE_LOW);
}	

static INT32U usbd_get_detection_status(void)
{
	static INT32U time_cnt = 0;
	BOOLEAN curio_state;
	
	if(USBD_DETECT_PIN == USBD_DET_PIN_OFF)
	{
		return 1;
	}	
	
	curio_state = gpio_read_io(USBD_DETECT_PIN);
	
	if(usb_block.usb_det_state != curio_state)
	{
		if(time_cnt++ == USBD_DET_TIME_CNT)
		{
			if(curio_state)
			{
				DBG_PRINT("USB cable detected!\r\n");
			}
			else
			{
				DBG_PRINT("USB cable removed!\r\n");
			}		
			usb_block.usb_det_state = (INT32U)curio_state;
		}
	}
	else
	{
		time_cnt = 0;
	}
	
	return usb_block.usb_det_state;
}	

void usbd_main_task(void *param)
{
	INT32U cur_ping;
	
	while(1)
	{
#if _OPERATING_SYSTEM == _OS_UCOS2			
		OSTimeDly(USBD_TASK_LOOP_TIME);	/* sleep 10ms */
#endif		
		cur_ping = usbd_get_detection_status();
		if(cur_ping)
		{
			/* USB cable plug-in */
			if(cur_ping != usb_block.usb_plug_in)
			{
				/* just plug in */
				usbd_init();
				usb_block.usb_plug_in = cur_ping;
			}
		}
		else
		{
			/* USB cable removed */
			if(cur_ping != usb_block.usb_plug_in)
			{
				/* just remove */
				usbd_uninit();
				usb_block.usb_plug_in = cur_ping;			
			}	
		}
	}//while(1)	
}	

void usbd_task_init(void)
{
	INT8U err;
	
#if _OPERATING_SYSTEM == _OS_UCOS2	
	err = OSTaskCreate(usbd_main_task, (void *)0, &USBDDETStack[USBDDETStackSize - 1], USBDDETTASKPRIORITY);
	if(err > 0)
		DBG_PRINT("fails to usbd_main_task! err = %d \r\n ", err);
#endif

	/* Init	GPIO for USB device detection PIN */	
	usbd_init_detect_pin();
}	

void usbd_task_exit(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSTaskDel(USBDDETTASKPRIORITY);
#endif	
}	