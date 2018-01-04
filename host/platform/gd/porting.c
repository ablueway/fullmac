/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <ssv_lib.h>
#include <log.h>
#include <rtos.h>

extern bool sdio_if_load_fw(u8* fw_bin, u32 fw_bin_len);

extern void *pvPortMalloc( size_t size );
extern void vPortFree( void *pv );

int hal_putchar(int c)
{
    putchar(c);
	return c;
}

u8 hal_getchar(void)
{
    char data = 0;
    data = getchar();
    if (data == -1) data = 0;
    return (u8)data;
}

#ifndef __MTRACE_CHECK__

OS_APIs void *OS_MemAlloc( u32 size )
{
    void *ptr;
    /**
        *  Platform dependent code. Please rewrite 
        *  this piece of code for different system.
        */
    ptr = pvPortMalloc(size);
    if (ptr)
    {
        OS_MemSET(ptr, 0, size);
        return ptr;
    }
    
    return NULL;
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
    ssv6xxx_memcpy(pdest,psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    ssv6xxx_memset(pdest,byte,size);
}

void platform_ldo_en_pin_init(void)
{

}

void platform_ldo_en(bool en)
{

}
//=====================find fw to download=======================
#if 0
#include <firmware/ssv6200_uart_bin.h>
void platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin)); 
    ssv6xxx_download_fw((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));
#else
bool platform_download_firmware(void)
{
    unsigned int fw_size, i;
 
    extern const unsigned char RES_WIFI_FW_START[];
    extern const unsigned char RES_WIFI_END[];
    unsigned char* fw = (unsigned char*)&RES_WIFI_FW_START;
    fw_size = ((unsigned int)&RES_WIFI_END) - ((unsigned int)&RES_WIFI_FW_START);
    printf("fw_size=%d,%x,%x fw=%x,%x\r\n",fw_size,*fw , *(fw+2),(unsigned int)fw,(unsigned int)&RES_WIFI_FW_START);
    ssv6xxx_download_fw((u8 *)&RES_WIFI_FW_START-1,fw_size);//??? u8* bin
    return TRUE;

#endif
}
void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,(void *)s,len);
}
