/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#define _PORTING_C_


#include "rtos.h"
#include "porting.h"

#include "base.h"
#include "hardware_config.h"
#include "includes.h"
#include "soc.h"
#include "gpio.h"

#include "log.h"

//#define APP_INCLUDE_WIFI_FW
//#define FW_DEBUG
#define PARTIAL_LOAD_FW_FROM_FLASH 1

/*=============console==================*/
u8 hal_getchar(void)
{
    char data = 0;
    while (fhInChar(&data) ==0)
    {
        OS_TickDelay(1);
    }
    
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

    ptr = (void *)appmem_malloc(size);
    OS_MemSET(ptr, 0, size);
    
    return ptr;
}

OS_APIs void __OS_MemFree( void *m )
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
        appmem_free(m);
}

void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    OS_MemCopy((void*)pdest,(void*)psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    if ( NULL != pdest ) {
		u8 * ps= (u8 *)pdest;
		const u8 * pes= ps+size;
        while( ps != pes )
			*(ps++) = (u8) byte;
    }
    
    return;
}

//=====================Platform LDO EN ping setting=======================
#define SSV_LDO_EN_PIN              HW_CARD_EN_GPIO


void platform_ldo_en_pin_init(void)
{
#ifdef SSV_LDO_EN_PIN
    //LOG_PRINTF("ldoen init: gpio%d\r\n", SSV_LDO_EN_PIN);
    Soc_SetIoMux(IO_GPIOx, SSV_LDO_EN_PIN);
    Gpio_SetDirectionX(SSV_LDO_EN_PIN, GPIO_DIR_OUT);
#endif
}

void platform_ldo_en(bool en)
{
#ifdef SSV_LDO_EN_PIN
    //LOG_PRINTF("ldoen gpio%d %d\r\n", SSV_LDO_EN_PIN, en);
    Gpio_SetPortX(SSV_LDO_EN_PIN, en);
#endif
}

//=====================find fw to download=======================
#include <firmware/ssv6200_uart_bin.h>
extern unsigned int  ssv6200_uart_bin_len;
#if(PARTIAL_LOAD_FW_FROM_FLASH ==0)
#define WIFI_FW_MEM_ADDR  0x9b900000
unsigned char *ssv6200_uart_bin = (unsigned char *)WIFI_FW_MEM_ADDR;
#endif/*APP_INCLUDE_WIFI_FW*/
bool platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin));
    
#ifdef APP_INCLUDE_WIFI_FW  //this option is to set fw.bin as resource to open. please check resource_wifi_fw.s
    LOG_PRINTF("download firmware from memory\r\n");
    return ssv6xxx_download_fw((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));
#else 
#if PARTIAL_LOAD_FW_FROM_FLASH
    LOG_PRINTF("download firmware from flash. 0\r\n");
    return ssv6xxx_download_fw((u8 *)0xa000,ssv6200_uart_bin_len);
#else
    extern void flush_dcache_range(unsigned long start, unsigned long len);
    int ret = 0;
    bool bret = false;
    unsigned int firmware_max_size = 260280;

    LOG_PRINTF("download firmware from flash. 1\r\n");

    ret = iFlash_ReadData(0xa000, ssv6200_uart_bin, firmware_max_size);
    if(0 != ret)
    {
        LOG_PRINTF("iFlash_ReadData firmware failed, addr=%p, len=%d\r\n", ssv6200_uart_bin, firmware_max_size);
        goto load_firmware_fail;
    }

    flush_dcache_range((unsigned long)ssv6200_uart_bin, (unsigned long)ssv6200_uart_bin_len);
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

    bret = ssv6xxx_download_fw((u8 *)ssv6200_uart_bin, ssv6200_uart_bin_len);
    if (!bret)
    {
        LOG_PRINTF("ssv6xxx_download_fw failed\r\n");
        goto load_firmware_fail;
    }

    return bret;

    load_firmware_fail:
        while(1);

#endif /*PARTIAL_LOAD_FW_FROM_FLASH*/
#endif /*APP_INCLUDE_WIFI_FW*/
}
void platform_read_firmware(void *d,void *s,u32 len)
{
#ifdef APP_INCLUDE_WIFI_FW
    OS_MemCPY(d,s,len);
#else
#if PARTIAL_LOAD_FW_FROM_FLASH
    iFlash_ReadData(s, d, len);
#else
    OS_MemCPY(d,s,len);
#endif /*PARTIAL_LOAD_FW_FROM_FLASH*/
#endif /*APP_INCLUDE_WIFI_FW*/
}

//=====================Platform wifi rc setting=======================

void ssv6xxx_wifi_set_rc_mask_api(u32 mask)
{
    if (ssv6xxx_set_rc_value(RC_RATEMASK, mask) == SSV6XXX_SUCCESS)
        LOG_PRINTF("rc mask 0x%x is OK!!\r\n", mask);
}

void ssv6xxx_wifi_set_rc_resent_api(bool on)
{
    ssv6xxx_set_rc_value(RC_RESENT, (u32)on);
    LOG_PRINTF("rc resent is OK!!\r\n");
}

void ssv6xxx_wifi_set_rc_per_api(u32 up_per, u32 down_per)
{
    u32 param;
    param = (((up_per<<16)&0xff0000)|(down_per&0xff));
    
    if (ssv6xxx_set_rc_value(RC_PER, param) == SSV6XXX_SUCCESS)
        LOG_PRINTF("rc simplealg is OK!!\r\n");
}


