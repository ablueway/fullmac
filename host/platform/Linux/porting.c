/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>

#define _PORTING_C_
#include <os.h>
#include <porting.h>
#include <ssv_lib.h>


#define APP_INCLUDE_WIFI_FW

/*=============console==================*/
u8 hal_getchar(void)
{
	return 0;
}

/*=============Memory==================*/

/* 
 * The platform memory allocate is different by case, 
 * Please implement this function for memory operation.  
 */
OS_APIs void *OS_MemAlloc(u32 size)
{
	void *ptr = NULL;
	ptr = kzalloc(size, GFP_KERNEL);
	return ptr;
}

OS_APIs void __OS_MemFree(void *m)
{
    kfree(m);
}


void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
	memcpy(pdest, psrc, size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    memset(pdest, byte, size);
}


/* 
 * If our dev in platform need do something init first(ex: hw reset), 
 * Please implement in this function, the iinit flow will use it.  
 */
void platform_dev_init(void)
{
	/* default do nothing */
	printk("%s()at line(%d)\n",__FUNCTION__,__LINE__);	
}

//=====================find fw to download=======================
#include <ssv_hal.h>
bool platform_download_firmware(void)
{
	/* TODO(aaron): to do the FW DL for USB in redbull */
    return ssv6xxx_download_fw((u8 *)SSV_HAL_FW_BIN, sizeof(SSV_HAL_FW_BIN));
//	return TRUE;
}
void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,s,len);
}

void platform_udelay(u32 us_delay)
{
    udelay(us_delay/1000);
}   

