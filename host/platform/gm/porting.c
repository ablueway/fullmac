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
    return malloc(size);/*pvPortMalloc(size);*/
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
    /*vPortFree(m);*/
    free(m);
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
    return ssv6xxx_download_fw((u8 *)SSV_HAL_FW_BIN,sizeof(SSV_HAL_FW_BIN));
}
void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,s,len);
}
void platform_udelay(u32 us_delay)
{
    
    if(us_delay<1000) //1ms
    {
        OS_TickDelay(1); //No udelay
    }
    else
    {
        OS_MsDelay(us_delay/1000);
    }
}   

