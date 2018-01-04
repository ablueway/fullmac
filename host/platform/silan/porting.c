/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include "rtos.h"
#include "porting.h"

#include "log.h"
#include <nuttx/arch.h>
#include <stdio.h>


#define APP_INCLUDE_WIFI_FW
//#define FW_DEBUG

/*=============console==================*/
u8 hal_getchar(void)
{
    char data = 0;

	data = getchar();
    return (u8)data;
}

/*=============Memory==================*/
OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite
        *  this piece of code for different system.
        */
    void * ptr = NULL;

    ptr = (void *)malloc(size);

    OS_MemSET(ptr, 0, size);
    
    return ptr;
}

OS_APIs void __OS_MemFree( void *m )
{
	/**
	*  Platform depedent code. Please rewrite
	*  this piece of code for different system.
	*/
	if(NULL != m){
		free(m);
	}else{
		return;
	}
}
void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    ssv6xxx_memcpy((void*)pdest,(void*)psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    ssv6xxx_memset(pdest, byte, size);
}

//=====================Platform LDO EN ping setting=======================
//#define SSV_LDO_EN_PIN              HW_CARD_EN_GPIO

void platform_ldo_en_pin_init(void)
{
#ifdef SSV_LDO_EN_PIN
    LOG_PRINTF("to be implement\r\n");
#endif
}

void platform_ldo_en(bool en)
{
#ifdef SSV_LDO_EN_PIN
    LOG_PRINTF("to be implement\r\n");
#endif
}

//=====================find fw to download=======================
#include <firmware/ssv6200_uart_bin.h>
extern unsigned int  ssv6200_uart_bin_len;

bool platform_download_firmware(void)
{
#ifdef APP_INCLUDE_WIFI_FW  //this option is to set fw.bin as resource to open. please check resource_wifi_fw.s
    LOG_PRINTF("download firmware from memory\r\n");
    return ssv6xxx_download_fw((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));
#else 
#if PARTIAL_LOAD_FW_FROM_FLASH
    LOG_PRINTF("download firmware from flash. 0\r\n");
    return ssv6xxx_download_fw((u8 *)0xa000,ssv6200_uart_bin_len);
#else
    LOG_PRINTF("to be implement\r\n");
#ifdef FW_DEBUG
    {
        int i = 0;
        LOG_PRINTF("\r\nssv6200_uart_bin[%d] = ", ssv6200_uart_bin_len);
        for(i = 0; i < ssv6200_uart_bin_len; i++)
        {
            if ((i % 12) == 0)
            {
                LOG_PRINTF("\r\n");
            }
            
            LOG_PRINTF("%02x ", ssv6200_uart_bin[i] & 0xff);
        }
        LOG_PRINTF("\r\n");
    }
#endif
    {
        bool bret;
        bret = ssv6xxx_download_fw((u8 *)ssv6200_uart_bin, ssv6200_uart_bin_len);
        if (!bret)
        {
            LOG_PRINTF("ssv6xxx_download_fw failed\r\n");
            goto load_firmware_fail;
        }

        return bret;

        load_firmware_fail:
            while(1);
    }
#endif /*PARTIAL_LOAD_FW_FROM_FLASH*/
#endif /*APP_INCLUDE_WIFI_FW*/
}
void platform_read_firmware(void *d,void *s,u32 len)
{
#ifdef APP_INCLUDE_WIFI_FW
    OS_MemCPY(d,s,len);
#else
#if PARTIAL_LOAD_FW_FROM_FLASH
    LOG_PRINTF("to be implement\r\n");
#else
    OS_MemCPY(d,s,len);
#endif /*PARTIAL_LOAD_FW_FROM_FLASH*/
#endif /*APP_INCLUDE_WIFI_FW*/
}

