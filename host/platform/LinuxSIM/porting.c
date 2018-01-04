/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#define _PORTING_C_

#include <ssv_lib.h>
#include <log.h>
#include <rtos.h>

#define APP_INCLUDE_WIFI_FW

extern void *pvPortMalloc( size_t size );
extern void vPortFree( void *pv );

#ifndef __MTRACE_CHECK__

OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite
        *  this piece of code for different system.
        */
    void *ptr = NULL;

    ptr = pvPortMalloc(size);
    if(ptr)
    {
        OS_MemSET(ptr, 0, size);
    }
    return ptr;
}


/**
 *  We do not recommend using OS_MemFree() API
 *  because we do not want to support memory
 *  management mechanism in embedded system.
 */
OS_APIs void __OS_MemFree( void *m )
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
    vPortFree(m);
}
#endif//#ifdef __MTRACE_CHECK__

void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    memcpy(pdest,psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    memset(pdest,byte,size);
}

void platform_ldo_en_pin_init(void)
{

}

void platform_ldo_en(bool en)
{

}
//=====================find fw to download=======================
#include <ssv_hal.h>
bool platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin));
    return ssv6xxx_download_fw((u8 *)SSV_HAL_FW_BIN,sizeof(SSV_HAL_FW_BIN));
}

void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,(void *)s,len);
}


