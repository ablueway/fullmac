/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <ssv_lib.h>
#include <log.h>
#include <rtos.h>

#include <stdlib.h>
#include <string.h>

extern bool sdio_if_load_fw(u8* fw_bin, u32 fw_bin_len);

extern void *pvPortMalloc( size_t size );
extern void vPortFree( void *pv );

#ifndef __MTRACE_CHECK__

OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite 
        *  this piece of code for different system.
        */
    return pvPortMalloc(size);
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
    __HAL_CTL_APB0_CLK_ENABLE();
    __HAL_CTL_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.Pin = GPIO_PIN_10;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStructure.Data = GPIO_PIN_RESET;
    HAL_GPIO_Init(GPIO_PORT_A, &GPIO_InitStructure);
}

void platform_ldo_en(bool en)
{
    if(en == 1)    
        HAL_GPIO_WritePin(GPIO_PORT_A, GPIO_PIN_10, GPIO_PIN_SET);
    else 
        HAL_GPIO_WritePin(GPIO_PORT_A, GPIO_PIN_10, GPIO_PIN_RESET);

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
    unsigned int fw_size, i=0;
 
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
