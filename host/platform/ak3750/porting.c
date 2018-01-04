/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#define _PORTING_C_

#include "rtos.h"
#include "porting.h"
#include "mem_api.h"
#include "os_wrapper.h"
#include "hal_gpio.h"
#include "drv_gpio.h"
#include "drv_api.h"
#include "drv_share_pin.h"
#include "fwl_osmalloc.h"

#define APP_INCLUDE_WIFI_FW

#if 1
u8 hal_getchar()
{
    u8 data = 0;

    #if 1
    while (uart_read(0, &data, 1) == 0)
    {
        OS_MsDelay(os_tick2ms(1));
    }
    #else

    //getnstr(&data, 1);
    data = getch();
    //hal_print("DATA=%d\r\n",data);
    #endif

    return data;
}
#endif

/*=============Memory==================*/
OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite
        *  this piece of code for different system.
        */
    void * ptr = NULL;

    ptr = (void *)Fwl_Malloc(size);

	OS_MemSET(ptr,0,size);
    
    return ptr;
}

OS_APIs void __OS_MemFree( void *m )
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
        Fwl_Free(m);
}

void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    //OS_MemCopy((void*)pdest,(void*)psrc,size);

    if ( NULL != pdest ) {
		u8 * ps = (u8 *)pdest;
		u8 * pes = (u8 *)psrc;
        while( size--)
			*(ps++) = *(pes++);
    }

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
//#define SSV_LDO_EN_PIN              66


void platform_ldo_en_pin_init(void)
{
#ifdef SSV_LDO_EN_PIN
    gpio_set_pin_as_gpio(SSV_LDO_EN_PIN);
    gpio_set_pin_dir(SSV_LDO_EN_PIN, GPIO_DIR_OUTPUT);
#endif
}

void platform_ldo_en(bool en)
{
#ifdef SSV_LDO_EN_PIN
    gpio_set_pin_level(SSV_LDO_EN_PIN, en & 0x1);
#endif
}

//=====================find fw to download=======================
#if 1
#include <ssv_hal.h>
bool platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(SSV_HAL_FW_BIN));
    return ssv6xxx_download_fw((u8 *)SSV_HAL_FW_BIN,sizeof(SSV_HAL_FW_BIN));
}
#else  //this option is to set fw.bin as resource to open. please check resource_wifi_fw.s
void platform_download_firmware(void)
{
    u32 fw_size;
    extern u8 *RES_WIFI_FW_START;
    extern u8 *RES_WIFI_END;

    fw_size = ((u32)&RES_WIFI_END) - ((u32)&RES_WIFI_FW_START);
    LOG_PRINTF("fw_size=%d\r\n",fw_size);
    ssv6xxx_download_fw((u8 *)&RES_WIFI_FW_START,fw_size);//??? u8* bin
}
#endif

void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,s,len);
}

void platform_udelay(u32 us)
{
    
    if(us<150000) //150ms
    {
        us_delay(us);
    }
    else
    {
        OS_MsDelay(us/1000);
    }
}   

