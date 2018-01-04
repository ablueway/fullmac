/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#define _PORTING_C_

#include <ssv_lib.h>
#include <log.h>
#include <rtos.h>
#include "gpio.h"
#include "porting.h"

extern bool sdio_if_load_fw(u8* fw_bin, u32 fw_bin_len);

extern void *pvPortMalloc( size_t size );
extern void vPortFree( void *pv );

static unsigned char IsSwUartActedAsFuartFlag = 0;
extern void EnableSwUartAsFuart(unsigned char EnableFlag);

int hal_putchar(int c)
{
    if(IsSwUartActedAsFuartFlag)
    {
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            SwUartSend((unsigned char*)lfca, 2);
        }
        else
        {
            SwUartSend((unsigned char*)&c, 1);
        }
    }
    else
    {
#ifdef MV_AP80A0
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            FuartSend((unsigned char*)lfca, 2);
        }
        else
        {
            FuartSend((unsigned char*)&c, 1);
        }

#else
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            BuartSend((unsigned char*)lfca, 2);
        }
        else
        {
            BuartSend((unsigned char*)&c, 1);
        }
#endif        
    }
    
	return c;
}

u8 hal_getchar(void)
{
    char data = 0;
#ifdef MV_AP80A0
    FuartRecv(&data, 1, 0);  
#else    
    BuartRecv(&data, 1, 0);   
#endif
    return (u8)data;
}

#ifndef __MTRACE_CHECK__

OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite 
        *  this piece of code for different system.
        */

    void *ptr = NULL;
	
    ptr = pvPortMalloc(size);
    OS_MemSET(ptr, 0, size);
	
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
#ifdef MV_AP80A0
void platform_ldo_en_pin_init(void)
{
    GpioClrRegOneBit(GPIO_C_IE,GPIOC2);
    GpioSetRegOneBit(GPIO_C_OE,GPIOC2);
                          
    GpioClrRegOneBit(GPIO_C_PU,GPIOC2);
    GpioClrRegOneBit(GPIO_C_PD,GPIOC2);
}
void platform_ldo_en(bool en)
{
    if ( 1 == en)
    {
        GpioSetRegBits( GPIO_C_OUT, GPIOC2);
    }
    else
    {
        GpioClrRegBits(GPIO_C_OUT, GPIOC2);
    }
}

#else
void platform_ldo_en_pin_init(void)
{
    GpioClrRegOneBit(GPIO_B_IE,GPIOB22);
    GpioSetRegOneBit(GPIO_B_OE,GPIOB22);

    GpioSetRegOneBit(GPIO_B_PU,GPIOB22);
    GpioClrRegOneBit(GPIO_B_PD,GPIOB22);
}

void platform_ldo_en(bool en)
{   
    if ( 1 == en)
    {
        GpioClrRegOneBit(GPIO_B_PU, GPIOB22);
        GpioClrRegOneBit(GPIO_B_PD, GPIOB22);
        GpioSetRegBits(GPIO_B_OUT, GPIOB22);
    }
    else
    {
        GpioSetRegOneBit(GPIO_B_PU,GPIOB22);
        GpioSetRegOneBit(GPIO_B_PD,GPIOB22);
        GpioSetRegBits(GPIO_B_OUT, GPIOB22);
    }
}
#endif
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
    printf("fw_size=%d,%x, fw=%x,%x\r\n",fw_size,*fw ,(unsigned int)fw,(unsigned int)&RES_WIFI_FW_START);
    return ssv6xxx_download_fw((u8 *)&RES_WIFI_FW_START-1,fw_size);//??? u8* bin

#endif
}
void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,(void *)s,len);
}
