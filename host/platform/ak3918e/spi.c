/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "msgevt.h"

#include <hal_gpio.h>
#include <drv_api.h>
#include <arch_spi.h>

#include "ssv_drv.h"
#include "ssv_drv_config.h"
#include "spi_def.h"
//#include "spi.h"

 #define SPI_SYSCLK_4             20000000
 #define SPI_SYSCLK_8             10000000
 #define SSV_SPI_NUM              SPI_ID1
 //#define SSV_CS_PIN               IO_D6
 #define SSV_IRQ_PIN              23

static void _spi_host_cs_init(void);
static void _spi_host_irq_pin_init(void (*spi_isr)(void));
static void _spi_host_cs_high(void);
static void _spi_host_cs_low(void);

extern unsigned char hHisr_stack[];
extern T_hHisr hHisr;
static void _spi_host_cs_init(void)
{
    //For debug by gpio 5
    gpio_set_pin_as_gpio(5);
    gpio_set_pin_dir(5, GPIO_DIR_OUTPUT); 
}
void spi_gpio_host_isr( u32 pin, u8 polarity )
{
    if (pin == SSV_IRQ_PIN)
    {
        //putch('Q');
        if (!AK_IS_INVALIDHANDLE(hHisr))
        {
            AK_Activate_HISR(hHisr);
        }
        gpio_int_control(SSV_IRQ_PIN, AK_FALSE);
    }
}


static void _spi_host_irq_pin_init(void (*spi_isr)(void))
{

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    _spi_host_cs_init();
    gpio_set_pin_as_gpio(SSV_IRQ_PIN);
    gpio_set_pin_dir(SSV_IRQ_PIN, GPIO_DIR_INPUT); 
    gpio_set_int_p(SSV_IRQ_PIN, 1);
    gpio_int_control(SSV_IRQ_PIN, 1);
    
    //create hisr
    hHisr = AK_Create_HISR(spi_isr, 
                            "sdio_hisr", 
                            0, 
                            (T_VOID *)hHisr_stack, 
                            1024);

    if (AK_IS_INVALIDHANDLE(hHisr))
    {
        LOG_PRINTF("AK_Create_HISR failed\r\n");
        return;
    }

    gpio_register_int_callback(SSV_IRQ_PIN, 1, 
        AK_FALSE, spi_gpio_host_isr);

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
}


static void _spi_host_cs_high(void)
{
    gpio_set_pin_level(5, 1);  //For debug by gpio 5
}

static void _spi_host_cs_low(void)
{
    gpio_set_pin_level(5, 0); ////For debug by gpio 5
}
void (*host_isr)(void);
void platform_spi_isr(void)
{
    if(host_isr)
        host_isr();
}

bool spi_is_truly_isr()
{
    return TRUE;
}

bool spi_host_init(void (*spi_isr)(void))
{
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    if(spi_isr==NULL)
    {
        SDRV_ERROR("%s(): spi_isr is a null pointer\r\n",__FUNCTION__);
        return FALSE;
    }

    if(!spi_init(SSV_SPI_NUM, SPI_MODE0, SPI_MASTER, SPI_SYSCLK_4))
    {
        SDRV_ERROR("spi_init fail\r\n");
        return FALSE;
    }
    host_isr = spi_isr;
    _spi_host_irq_pin_init(platform_spi_isr);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

bool spi_host_readwrite(u8 *buf, u32 sizeToTransfer, u32 *sizeToTransfered, u8 options, bool IsRead)
{
    bool ret=TRUE;
    bool bReleaseCS=0;
    
    if(options & SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE)
        bReleaseCS = 1;
    
    //_spi_host_cs_low();

    spi_set_protect(SSV_SPI_NUM, SPI_BUS1);
    if(TRUE==IsRead){
        ret = spi_master_read(SSV_SPI_NUM, buf, sizeToTransfer, bReleaseCS);            

    }else{

        u32 loop = sizeToTransfer/512;
        u32 i,tsize=sizeToTransfer;
        u8* bptr = buf;
        //LOG_PRINTF("spi lp=%d\r\n",loop);
        for(i=0;i<loop; i++)
        {
            ret = spi_master_write(SSV_SPI_NUM, bptr, 512, 0);
            bptr+=512;
            tsize-=512;
        }
        ret = spi_master_write(SSV_SPI_NUM, bptr, tsize, bReleaseCS);
        
    }
    spi_set_unprotect(SSV_SPI_NUM, SPI_BUS1);

    //if(bReleaseCS)
    //    _spi_host_cs_high();
        
    return ret;
}
#if 0
bool spi_host_write(u8 *buf, u32 sizeToTransfer, u32 *sizeToTransfered, u32 options)
{
    return spi_host_readwrite(buf, sizeToTransfer, sizeToTransfered, options, FALSE);
}

bool spi_host_read(u8 *buf, u32 sizeToTransfer, u32 *sizeToTransfered, u32 options)
{
    return spi_host_readwrite(buf, sizeToTransfer, sizeToTransfered, options, TRUE);
}
#endif
void spi_host_irq_enable(bool enable)
{
    gpio_int_control(SSV_IRQ_PIN, enable);
}

