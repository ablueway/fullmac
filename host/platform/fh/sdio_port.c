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

#include "sdio.h"
#include "sdio_port.h"
#include "sdio_def.h"

volatile buf_chain_t sdio_dma_desc;   // should map to DmaDesc

static HSDC sdio_handle;

//#define SDIO_PORT_DEBUG

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static unsigned int sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

u8	sdio_readb_cmd52(u32 addr)
{
	unsigned int  resp;
	u8   data;
    int ret;
    if (sdio_handle == NULL)
    {
        SDIO_ERROR("sdio_handle = %s\r\n", "NULL");
        return 0;
    }
    
	ret = sdio_drv_creg_read(sdio_handle, (int)addr, 1, &resp);
    if (ret != 0)
    {
        SDIO_ERROR("_sdio_drv_creg_read ret = %d\r\n", ret);
    }
    data = (resp & 0xff);
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, data);
    
	return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
	unsigned int  resp;
    int ret;
    
    if (sdio_handle == NULL)
    {
        SDIO_ERROR("sdio_handle = %s\r\n", "NULL");
        return false;
    }

    ret = sdio_drv_creg_write(sdio_handle, (int)addr, 1, data, &resp);
    if (ret != 0)
    {
        SDIO_ERROR("sdio_drv_creg_write ret = %d\r\n", ret);
    }
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "WRITE_BYTE", addr, data);
	return true;
}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 rx_blocks = 0, blksize = 0;
    int ret = 0;
    
    if (sdio_handle == NULL)
    {
        SDIO_ERROR("sdio_handle = %s\r\n", "NULL");
        return false;
    }

    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }

    if (size > sdio_block_size) {
        rx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    } else {
        blksize = size;
        rx_blocks = 1;
    }

    if (blksize > 0)
    {
        if(0 != (ret = sdio_drv_read(sdio_handle, dataPort, 1, rx_blocks, blksize, dat)))
        {
            SDIO_ERROR("sdio_drv_read, ret = %d\r\n", ret);
            return false;
        }
    }

    
	SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "READ_BLOCK", dataPort, dat, size);
	return true;
}

bool sdio_write_cmd53(u32 dataPort, void *dat, size_t size)
{
	unsigned int tx_blocks = 0, buflen = 0;
    int ret = 0;
    
    if (sdio_handle == NULL)
    {
        SDIO_ERROR("sdio_handle = %s\r\n", "NULL");
        return false;
    }

    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }

    if (size > sdio_block_size) {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        buflen = sdio_block_size;
    } else {
        buflen = size;
        tx_blocks = 1;
    }

    if (0 != (ret = sdio_drv_write(sdio_handle, dataPort, 1, tx_blocks, buflen, dat)))
    {        
        SDIO_ERROR("sdio_drv_write, ret = %d\r\n", ret);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "WRITE_BLOCK", dataPort, dat, size);

	return true;
}

bool sdio_host_enable_isr(bool enable)
{    
    int ret = 0;
    
    if (sdio_handle == NULL)
    {
        SDIO_ERROR("sdio_handle = %s\r\n", "NULL");
        return false;
    }
    
    if (0 != (ret = sdio_enable_card_int(sdio_handle, enable ? 1 : 0)))
    {
        SDIO_ERROR("sdio_enable_card_int, ret = %d\r\n", ret);
        return false;
    }
    
	SDIO_TRACE("%-20s : %d\n", "enable_isr", enable);
    
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
    
    ret = sdio_drv_creg_write(sdio_handle, (int)0x110, 0, blk[0], &resp);
    if (!ret)
    {
        ret = sdio_drv_creg_write(sdio_handle, (int)0x111, 0, blk[1], &resp);
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

static void sdio_host_isr (void)
{
    if (g_sdio_isr_func)
    {
        g_sdio_isr_func();
    }
}

bool is_truly_isr()
{
    return TRUE;
}
bool sdio_host_detect_card(void)
{
	sdio_host_enable_isr(false);
    return TRUE;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    int ret = 0;
    
    ret  = sdio_init(0, SDC_WKMOD_4WIRE|SDC_WKMOD_25M_STAND_SPEED, (unsigned int*)((void *)&sdio_dma_desc) /*4Byte aligned*/, &sdio_handle);
    
    if(sdio_handle == NULL)
    {
        SDIO_ERROR("sdio_init fail, ret = %d\n", ret);
        return false;
    }

    sdio_set_block_size(SDIO_DEFAULT_BLOCK_SIZE);
    
    //install isr here
    sdio_host_enable_isr(false);

    g_sdio_isr_func = sdio_isr;
    
	sdio_set_card_int_cb(sdio_handle, sdio_host_isr);

    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "host_int", "ok");
    
    return true;
}


#ifdef SDIO_PORT_DEBUG
unsigned char sdio_data[1024];

void sdio_test_cmd53(int size)
{
    int i = 0;

    for (i = 0; i < 1024; i++)
    {
        sdio_data[i] = i & 0x3f;
    }
    
    sdio_data[60] = 0x00;
    sdio_data[61] = 0x00;
    sdio_data[62] = 0x55;
    sdio_data[63] = 0x00;
    
    sdio_write_cmd53(0x10000,sdio_data, size);
    
}
#endif

