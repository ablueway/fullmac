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
#include "drv_l2_sd.h"
#include "project.h"
#include "gplib.h"
#include "usbd.h"

/******************************************************
    Definition and variable declaration
*******************************************************/
#define USBD_MSDC_DMA_BUF_ORDER	6
#define USBD_MSDC_DMA_BUF_SIZE	((1 << USBD_MSDC_DMA_BUF_ORDER)*512)	/* Support 512/1K/2K/4K/8K/16K/32K/64K/128K data length */
#define USBD_MSDC_SYS_CLOCK		72

extern void drv_l1_system_clk_set(INT32U SysClk, INT32U SDramPara);
extern void Debug_UART_Port_Enable(void);
/******************************************************
    Functions declaration
*******************************************************/
static INT32S _usbd_set_system_clk(INT32U clk_mhz)
{
	INT32U SysClk, SDramPara;

	switch(clk_mhz)
	{
		case 48:
			SysClk = 0x02;
			SDramPara = 0x11;
			break;
		case 72:
			SysClk = 0x08;
			SDramPara = 0x711;
			break;
		case 144:
		default:
			clk_mhz = 144;
			SysClk = 0x1A;
			SDramPara = 0x711;
	}
	MCLK = clk_mhz * 1000000;	
	MHZ	  = clk_mhz;
	drv_l1_system_clk_set(SysClk, SDramPara);
	
	Debug_UART_Port_Enable();

#if 1	
	if (drvl2_sd_init()==0)
	{
		drvl2_sd_bus_clock_set(50000000); 
	}
	else
	{
		DBG_PRINT("SD 0 Initial Fail\r\n");
	}

	if (drvl2_sd1_init()==0)
	{
		drvl2_sd1_bus_clock_set(50000000); 
	}
	else
	{
		DBG_PRINT("SD 1 Initial Fail\r\n");
	}

#endif	
	return 0;
}
void usbd_msdc_init(void)
{
    INT32S ret;

	/* Change system clock to 48Mhz to fix the BULK IN DMA can not be done issue */
	_usbd_set_system_clk(USBD_MSDC_SYS_CLOCK);

    /* Init USBD L2 protocol layer first, including control/bulk/ISO/interrupt transfers */
    /******************************* Control transfer ************************************/
    ret = drv_l2_usbd_ctl_init();
    if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l2_usbd_ctl_init failed!\r\n");
        return;
    }
    
    /* Register new descriptor table here, this action must be done after drv_l2_usbd_ctl_init() */
    drv_l2_usbd_register_descriptor(REG_DEVICE_DESCRIPTOR_TYPE, (INT8U*)Default_Device_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_CONFIG_DESCRIPTOR_TYPE, (INT8U*)Default_Config_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_DEVICE_QUALIFIER_DESCRIPTOR_TYPE, (INT8U*)Default_Qualifier_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_STRING0_DESCRIPTOR_TYPE, (INT8U*)Default_String0_Descriptor);
    drv_l2_usbd_register_descriptor(REG_STRING1_DESCRIPTOR_TYPE, (INT8U*)Default_String1_Descriptor);
    drv_l2_usbd_register_descriptor(REG_STRING2_DESCRIPTOR_TYPE, (INT8U*)Default_String2_Descriptor);

#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	(defined(NAND2_EN) && (NAND2_EN == 1)) || \
	(defined(NAND3_EN) && (NAND3_EN == 1)) || \
	((defined NAND_APP_EN) && (NAND_APP_EN == 1))
	//drv_l2_usbd_msdc_set_lun(LUN_NF_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_nand0);
	//drv_l2_usbd_msdc_set_lun(LUN_NF_TYPE, LUN_NUM_0, USBD_STORAGE_WPROTECT, &gp_msdc_nandapp0);
#else
#if defined(SD_EN) && (SD_EN == 1)
    drv_l2_usbd_msdc_set_lun(LUN_SDC_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_sd0);
#else
    drv_l2_usbd_msdc_set_lun(LUN_RAM_DISK_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_ramdisk);
#endif
#endif
	
	/* Set MSDC A/B buffer size */
	drv_l2_usbd_msdc_set_dma_buffer_size(USBD_MSDC_DMA_BUF_SIZE);
   	/* Init MSDC driver */
    ret = drv_l2_usbd_msdc_init();
    if(ret == STATUS_FAIL)
    {
        /* Init failed, do uninit procedures */
        drv_l2_usbd_msdc_uninit();
        DBG_PRINT("drv_l2_usbd_msdc_uninit failed!\r\n");
        return;
    }
        
    /* Register SCSI inquiry data pointer, it must be done after drv_l2_usbd_msdc_init() */
    drv_l2_usbd_msdc_register_scsi_inquiry_data((INT8U*)Default_scsi_inquirydata, (INT8U*)Default_scsi_inquirydata_CDROM);
    
    /* Init USBD L1 register layer */
    ret = drv_l1_usbd_init();
    if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l1_usbd_init failed!\r\n");
        return;
    }
    
	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(drv_l1_usbd_isr_handle);
    	
    DBG_PRINT("USB MSDC device init completed!\r\n");
}

void usbd_msdc_uninit(void)
{
	drv_l2_usbd_ctl_uninit();
	drv_l2_usbd_msdc_uninit();
	/* Change system clock to 144Mhz */
	_usbd_set_system_clk(INIT_MHZ);

	Debug_UART_Port_Enable();
}	

USBD_CONTROL_BLK usbd_msdc_ctl_blk = 
{
	USBD_MSDC_MODE,
	usbd_msdc_init,
	usbd_msdc_uninit
};	

