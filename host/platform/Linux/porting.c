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
OS_APIs void *OS_MemAlloc(u32 size)
{
	/**
	 *  Platform dependent code. Please rewrite
	 *  this piece of code for different system.
	 */
    void *ptr = NULL;
    ptr = kzalloc(size, GFP_KERNEL);
    return ptr;
}

OS_APIs void __OS_MemFree(void *m)
{
    /**
	 *  Platform depedent code. Please rewrite
	 *  this piece of code for different system.
	 */
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


/* TODO:aaron */
//=====================Platform LDO EN ping setting=======================

void platform_ldo_en_pin_init(void)
{
#ifdef __linux__

#else
/* in */
#ifdef SSV_LDO_EN_PIN
/* not in */
#if (HOST_PLATFORM_SEL == GP_19B)
    gpio_set_memcs(1,0); //cs1 set as IO
#endif
    gpio_init_io(SSV_LDO_EN_PIN,GPIO_OUTPUT);
    gpio_set_port_attribute(SSV_LDO_EN_PIN, 1);
    gpio_write_io(SSV_LDO_EN_PIN, 0);
#endif
#endif
}

void platform_ldo_en(bool en)
{
#ifdef __linux__

#else
/* in */
#ifdef SSV_LDO_EN_PIN
    gpio_write_io(SSV_LDO_EN_PIN, en);
#endif
#endif
}

//=====================find fw to download=======================
#include <ssv_hal.h>
bool platform_download_firmware(void)
{
    return ssv6xxx_download_fw((u8 *)SSV_HAL_FW_BIN, sizeof(SSV_HAL_FW_BIN));
}
void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,s,len);
}

void platform_udelay(u32 us_delay)
{
    udelay(us_delay/1000);
}   

