/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <log.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "gpio.h"
#include "irqs.h"

#include "sdio.h"
#include "sdio_port.h"
#include "sdio_def.h"
#include "porting.h"

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static unsigned int sdio_block_size;
void (*gpio_sdio_isr)(void);

#ifdef MV_AP80A0
__attribute__((section(".driver.isr"))) void GpioInterrupt(void)
{   
    GpioIntClr(GPIO_C_INT, GPIOC3);
    GpioIntDis(GPIO_C_INT, GPIOC3);       
    
    if (gpio_sdio_isr)
        gpio_sdio_isr();
}

void gpio_init() 
{
    GpioClrRegOneBit(GPIO_C_OE,GPIOC3);
    GpioSetRegOneBit(GPIO_C_IE,GPIOC3);

    GpioSetRegOneBit(GPIO_C_PU,GPIOC3);
    GpioSetRegOneBit(GPIO_C_PD,GPIOC3);

    GpioIntEn(GPIO_C_INT, GPIOC3, GPIO_POS_EDGE_TRIGGER);
    NVIC_EnableIRQ(GPIO_IRQn);
}

bool gpio_isr_enable(bool enable)
{
    if (enable)
    {
        GpioIntEn(GPIO_C_INT, GPIOC3, GPIO_POS_EDGE_TRIGGER); 
    }
    else
    {
        GpioIntClr(GPIO_C_INT, GPIOC3);
        GpioIntDis(GPIO_C_INT, GPIOC3);       
    }
}

#else
__attribute__((section(".driver.isr"))) void GpioInterrupt(void)
{   
    GpioIntClr(GPIO_B_INT, GPIOB2);
    GpioIntDis(GPIO_B_INT, GPIOB2);       
    
    if (gpio_sdio_isr)
        gpio_sdio_isr();
}

void gpio_init() 
{
    GpioClrRegOneBit(GPIO_B_OE,GPIOB2);
    GpioSetRegOneBit(GPIO_B_IE,GPIOB2);

    GpioSetRegOneBit(GPIO_B_PU,GPIOB2);
    GpioSetRegOneBit(GPIO_B_PD,GPIOB2);

	GpioIntEn(GPIO_B_INT, GPIOB2, GPIO_POS_EDGE_TRIGGER);
    NVIC_EnableIRQ(GPIO_IRQn);
}

bool gpio_isr_enable(bool enable)
{
    if (enable)
    {
        GpioIntEn(GPIO_B_INT, GPIOB2, GPIO_POS_EDGE_TRIGGER); 
    }
    else
    {
        GpioIntClr(GPIO_B_INT, GPIOB2);
        GpioIntDis(GPIO_B_INT, GPIOB2);       
    }
}
#endif
bool sdio_host_enable_isr(bool enable)
{  
    gpio_isr_enable (enable);
    return true;
}

bool sdio_set_block_size(unsigned int blksize)
{
    unsigned char blk[2];
    unsigned int resp;
    int ret;
    
    if (blksize == 0)
    {
        blksize = SDIO_DEFAULT_BLOCK_SIZE;
    }
    
    blk[0] = (blksize >> 0) & 0xff;
    blk[1] = (blksize >> 8) & 0xff;
    
	ret = mmc_io_rw_direct_host(1, 0, (int)0x110, blk[0], 0);
    if (!ret)
    {
        ret = mmc_io_rw_direct_host(1, 0, (int)0x111, blk[1], 0);
    }

    if (!ret)
    {
        sdio_block_size = blksize;
    }
    else
    {
        SDIO_ERROR("sdio_set_block_size(%x), ret = %d\r\n", blksize, ret);
        return false;
    }
    return true;
}

unsigned int sdio_get_block_size()
{
    return sdio_block_size;
}
bool is_truly_isr()
{
    return TRUE;
}

bool sdio_host_detect_card(void)
{
    printf("sdio_host_detect_card \r\n");
    SD_Init();
    return TRUE;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    int ret = 0;
    
//    SdioControllerInit();
    SD_Init();
    
    gpio_sdio_isr = sdio_isr;
    gpio_init();

    ret = mmc_io_rw_direct_host(1, 0, 0x4, 0x3, 0);
    if (0 != ret)
    {
        SDIO_ERROR("mmc_io_rw_direct_host ret = %d\r\n", ret);
        return false;
    }
    
    sdio_host_enable_isr(true);

    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "host_int", "ok");
  
    return true;
}


u8	sdio_readb_cmd52(u32 addr)
{
    u8 data;
    int ret;
    
    ret = mmc_io_rw_direct_host(0, 1, addr, 0, &data);
    if (ret != 0)
        SDIO_ERROR("_sdio_drv_creg_read ret = %d\r\n", ret);
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, data);
    
    return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    int ret;
	ret = mmc_io_rw_direct_host(1, 1, addr, data, 0);
    if (ret != 0)
    {
        SDIO_ERROR("sdio_drv_creg_write ret = %d\r\n", ret);
        return false;
    }
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "WRITE_BYTE", addr, data);
    
	return true;
}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
    u32 rx_blocks = 0, blksize = 0;
    int ret = 0;
    
#ifndef NO_NEED_ALIGN_BYTE
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
#endif

    if (size > 64) {
        rx_blocks = (size + 64 - 1) / 64;
        blksize = 64;
    } else {
        blksize = size;
        rx_blocks = 1;
    }
    if(0 != mmc_io_rw_extended(0,1,dataPort, 0,dat,rx_blocks,blksize))
    {
        SDIO_ERROR("sdio_drv_read error \r\n");
        return false;
    }
    
    SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "READ_BLOCK", dataPort, dat, size);
    return true;    
}
bool sdio_write_cmd53(u32 dataPort, void *dat, size_t size)
{
  	unsigned int tx_blocks = 0, buflen = 0;
    int ret = 0;

#ifndef NO_NEED_ALIGN_BYTE
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
#endif

    if (size > 64) 
    {
        tx_blocks = (size + 64 - 1) / 64;
        buflen = 64;
    } 
    else 
    {
        buflen = size;
        tx_blocks = 1;
    }
    
    if (0 != mmc_io_rw_extended(1,1,dataPort,0,dat,tx_blocks, buflen))
    {        
            SDIO_ERROR("sdio_drv_write error \r\n");
            return false;
    }

    SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "WRITE_BLOCK", dataPort, dat, size);

    return true;
}


