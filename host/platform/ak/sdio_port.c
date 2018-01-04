/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <config.h>
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
#include "drv_share_pin.h"


//#define SDIO_PORT_DEBUG

#define SDIO_DEFAULT_BLOCK_SIZE  64

#define SDIO_ISR_GPIO_PIN 71 

static unsigned int sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

static void sdio_gpio_init();

bool sdio_readb_cmd52(u32 addr, u8 *data)
{
	u8   val = 0x0;
    bool ret = 0;
    
	ret = sdio_read_byte(1, addr, &val);
    if (ret != AK_TRUE)
    {
        SDIO_ERROR("sdio_readb_cmd52 ret = %d, addr = %x\r\n", ret, addr);
        *data=0;
    }
    else
    {
        *data=val;
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_readb_cmd52", addr, val);
    
	return (ret != AK_TRUE)?false:true;
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

bool sdio_host_init(void (*sdio_isr)(void))
{
    bool ret = 0;
    
    sdio_gpio_init();

    ret  = sdio_initial(USE_FOUR_BUS, SDIO_DEFAULT_BLOCK_SIZE);
    //ret  = sdio_initial(USE_ONE_BUS, SDIO_DEFAULT_BLOCK_SIZE);
    if (ret == AK_FALSE)
    {
        SDIO_ERROR("sdio_init fail, ret = %d\r\n", ret);
        return false;
    }

    //sdio_set_clock((50*1000*1000));
    
    sdio_set_block_size(SDIO_DEFAULT_BLOCK_SIZE);
    
    /* enable isr on cis for my ic */
    ret = sdio_write_byte(0, 0x4, 0x3);
    if (ret == AK_FALSE)
    {
        SDIO_ERROR("sdio_write_byte, ret = %d\r\n", ret);
        return false;
    }

    /* install isr here */
    g_sdio_isr_func = sdio_isr;
    
    //sdio_host_enable_isr(true);
    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "sdio_host_init", "ok");
    return true;
}

#define  SDIO_HISR_STACK_SIZE 1024
static T_hHisr hHisr;
static unsigned char hHisr_stack[SDIO_HISR_STACK_SIZE];

void sdio_gpio_host_isr( u32 pin, u8 polarity )
{
    if (pin == SDIO_ISR_GPIO_PIN)
    {
        //
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
    //gpio_int_control(SDIO_ISR_GPIO_PIN, AK_FALSE);
    //putch('T');
    if (g_sdio_isr_func)
    {
        g_sdio_isr_func();
    }
}

bool sdio_host_detect_card(void)
{
    return TRUE;
}

static void sdio_gpio_init()
{   
    T_SHARE_SDMMC m_share_sdio = {

        /*.clk = */34,
        /*.cmd = */35,
        /*.dat = */{36, 37, 38, 17, INVALID_GPIO, INVALID_GPIO, INVALID_GPIO, INVALID_GPIO},

    };
    
    gpio_init();
    gpio_share_pin_set(ePIN_AS_SDMMC2, (T_U8 *)&m_share_sdio, sizeof(m_share_sdio));    
    
    OS_MsDelay(500);
    
    // pin 33 for wake, it is isr pin. 
    gpio_set_pin_as_gpio(SDIO_ISR_GPIO_PIN);
    gpio_set_pin_dir(SDIO_ISR_GPIO_PIN, GPIO_DIR_INPUT); 
    gpio_set_int_mode(SDIO_ISR_GPIO_PIN, 0);
    //create hisr
    hHisr = AK_Create_HISR(HISR_SDIO, 
                            "sdio_hisr", 
                            0, 
                            (T_VOID *)hHisr_stack, 
                            SDIO_HISR_STACK_SIZE);

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

