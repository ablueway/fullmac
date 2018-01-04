/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <log.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <hctrl.h>

#include "drv_api.h"
#include "sdio_port.h"
#include "sdio_def.h"

#include "anyka_types.h"
#include "hal_gpio.h"
#include "drv_api.h"
#include "arch_mmc_sd.h"
#include "arch_sdio.h"


//#define SDIO_PORT_DEBUG

#define SDIO_DEFAULT_BLOCK_SIZE  64

//#define SDIO_ISR_GPIO_PIN 10 //for lewei board 

#define SDIO_ISR_GPIO_PIN 15 //for anyka dev board

static unsigned int sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

static void sdio_gpio_init();
extern void sdio_gpio_host_isr( u32 pin, u8 polarity );

u8	sdio_readb_cmd52(u32 addr)
{
	u8   data = 0x0;
    bool ret = 0;
    
	ret = sdio_read_byte(1, addr, &data);
    if (ret != AK_TRUE)
    {
        SDIO_ERROR("sdio_readb_cmd52 ret = %d, addr = %x\r\n", ret, addr);
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_readb_cmd52", addr, data);
    
	return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    bool ret = 0;

    ret = sdio_write_byte(1, addr, data);
    if (ret != AK_TRUE)
    {
        SDIO_ERROR("sdio_writeb_cmd52 ret = %d, addr = %x\r\n", ret, addr);
        return false;
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_writeb_cmd52", addr, data);
    
	return true;
}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 rx_blocks = 0, blksize = 0;
    bool ret = 0;
    int size_1 = size;
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
        
    if (size > sdio_block_size) {
        rx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
        size = sdio_block_size * rx_blocks;
    } else {
        blksize = size;
        rx_blocks = 1;
    }
    
    ret = sdio_read_multi(1, dataPort, size, 0, dat);
    
	if (ret != AK_TRUE)
	{
        SDIO_ERROR("sdio_read_multi, ret = %d size = %d\r\n", ret, size);
	    return false;
	}

	SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "sdio_read_cmd53", dataPort, dat, size);
	return true;
}

bool sdio_write_cmd53(u32 dataPort, void *dat, size_t size)
{
    bool ret = 0;
	u32 tx_blocks = 0, buflen = 0;
    
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }

    if (size > sdio_block_size) {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        buflen = sdio_block_size;
        size = sdio_block_size * tx_blocks;
    } else {
        buflen = size;
        tx_blocks = 1;
    }

    ret = sdio_write_multi(1, dataPort, size, 0, dat);
    if (ret != AK_TRUE)
    {        
        SDIO_ERROR("sdio_write_multi, ret = %d size = %d\r\n", ret, size);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "sdio_write_cmd53", dataPort, dat, size);

	return true;
}

bool sdio_host_enable_isr(bool enable)
{    
    int ret = 0;
    
    gpio_int_control(SDIO_ISR_GPIO_PIN, enable);
    
	SDIO_TRACE("%-20s : %d\n", "sdio_host_enable_isr", enable);

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
    
    ret = sdio_set_block_len(1, blksize);
    
    if (ret)
    {
        sdio_block_size = blksize;
    }
    else
    {
        SDIO_ERROR("sdio_set_block_size,blksize = 0x%x, ret = %d\r\n", blksize, ret);
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

#define SD_IDENTIFICATION_MODE_CLK  (350*1000)              ///<350k
#define SD_TRANSFER_MODE_CLK        (20*1000*1000)          ///<20M
#define HS_TRANSFER_MODE_CLK1       (30*1000*1000)          ///<30M
#define HS_TRANSFER_MODE_CLK2       (26*1000*1000)          ///<26M
#define MMC_DEFAULT_MODE_20M        (20*1000*1000)
#define SD_DEFAULT_MODE_25M         (25*1000*1000)
#define MMC_HS_MODE_26M             (26*1000*1000)
#define MMC_HS_MODE_52M             (52*1000*1000)
#define SD_HS_MODE_50M              (50*1000*1000)

#define SD_POWER_SAVE_ENABLE            1
#define SD_POWER_SAVE_DISABLE           0 

extern void sdio_set_clock(unsigned long clk, unsigned long asic, bool pwr_save);

bool sdio_host_init(void (*sdio_isr)(void))
{
    bool ret = 0;

    sdio_gpio_init();

    //ret  = sdio_initial(INTERFACE_SDIO, USE_ONE_BUS, SDIO_DEFAULT_BLOCK_SIZE);
    ret  = sdio_initial(INTERFACE_SDIO, USE_FOUR_BUS, SDIO_DEFAULT_BLOCK_SIZE);
    if (ret == AK_FALSE)
    {
        SDIO_ERROR("sdio_init fail, ret = %d\r\n", ret);
        return false;
    }

    sdio_set_clock(SD_HS_MODE_50M, get_asic_freq(), SD_POWER_SAVE_DISABLE); 	

    sdio_set_block_size(SDIO_DEFAULT_BLOCK_SIZE);
    
    /* enable isr on cis for my ic */
    ret = sdio_write_byte(0, 0x4, 0x3);
    if (ret == AK_FALSE)
    {
        SDIO_ERROR("sdio_write_byte, ret = %d\r\n", ret);
        return false;
    }
    #if 0 // ak3918e only support 1 bit D1 pin interupt.
    ret = sdio_set_int_callback((T_SDIO_INT_HANDLER)sdio_gpio_host_isr);
    if (ret == AK_FALSE)
    {
        SDIO_ERROR("sdio_write_byte, ret = %d\r\n", ret);
        return false;
    }
    #else // gpio int
    
    #endif
    
    /* install isr here */
    g_sdio_isr_func = sdio_isr;
    
    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "sdio_host_init", "ok");
    return true;
}

#define  HISR_STACK_SIZE 1024
T_hHisr hHisr;
unsigned char hHisr_stack[HISR_STACK_SIZE];
void sdio_gpio_host_isr( u32 pin, u8 polarity )
{
    if (pin == SDIO_ISR_GPIO_PIN)
    {
        //putch('Q');
        if (!AK_IS_INVALIDHANDLE(hHisr))
        {
            AK_Activate_HISR(hHisr);
        }
        gpio_int_control(SDIO_ISR_GPIO_PIN, AK_FALSE);
    }
}

static void HISR_SDIO(void)
{
    //putch('T');
    if (g_sdio_isr_func)
    {
        g_sdio_isr_func();
    }
}

bool sdio_host_detect_card(void)
{
    sdio_host_enable_isr(false);
    return TRUE;
}

static void sdio_gpio_init()
{   
    // pin 10 for wake, it is isr pin. 
    gpio_set_pin_as_gpio(SDIO_ISR_GPIO_PIN);
    gpio_set_pin_dir(SDIO_ISR_GPIO_PIN, GPIO_DIR_INPUT); 
    gpio_set_int_p(SDIO_ISR_GPIO_PIN, 1);
    gpio_int_control(SDIO_ISR_GPIO_PIN, 1);
    
    //create hisr
    hHisr = AK_Create_HISR(HISR_SDIO, 
                            "sdio_hisr", 
                            0, 
                            (T_VOID *)hHisr_stack, 
                            HISR_STACK_SIZE);

    if (AK_IS_INVALIDHANDLE(hHisr))
    {
        LOG_PRINTF("AK_Create_HISR failed\r\n");
        return;
    }

    gpio_register_int_callback(SDIO_ISR_GPIO_PIN, 1, 
        AK_FALSE, sdio_gpio_host_isr);
}

#ifdef SDIO_PORT_DEBUG
#define test_read_register(reg) (*((volatile unsigned int*)(reg)))

void sdio_trigger(int enable)
{
    gpio_set_pin_as_gpio(83);
    gpio_set_pin_dir(83, GPIO_DIR_OUTPUT); 
    gpio_set_pin_level(83, enable & 0x1); 
}

void sdio_trigger1(int pin, int enable)
{
    gpio_set_pin_as_gpio(pin);
    gpio_set_pin_dir(pin, GPIO_DIR_OUTPUT); 
    gpio_set_pin_level(pin, enable & 0x1); 
    
    LOG_PRINTF("sdio_trigger1 pin=%d, %d\r\n",pin, enable);
}

void sdio_in_read_reg(u32 addr)
{
    unsigned long base = addr;
    unsigned long value = 0;

    value = test_read_register(base);

	LOG_PRINTF("0x%08x, 0x%08x\n",base, value);
}

#endif

