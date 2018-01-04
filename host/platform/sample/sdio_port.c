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
#include <sizes.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include "sdio_port.h"
#include "sdio_def.h"


struct sdio_func *g_func;
//#define SDIO_PORT_DEBUG

#define SDIO_INDEX 0

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static u32 sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

u8	sdio_readb_cmd52(u32 addr)
{
    u8 out;
    int ret;
    
    sdio_claim_host(g_func);
    *out = sdio_readb(g_func, addr, &ret);
    sdio_release_host(g_func);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_cmd52(read), ret = %d\r\n", ret);
        return 0;
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_readb_cmd52", addr, out);

    return out;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    int ret=0;
    sdio_claim_host(g_func);
    sdio_writeb(g_func,data, addr, &ret);
    sdio_release_host(g_func);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_cmd52(write), ret = %d\r\n", ret);
        return false;
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_writeb_cmd52", addr, data);

    return true;
    
}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 rx_blocks = 0, blksize = 0;
    u32 ret = 0;
    
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte, data = 0x%08x, size = %d\r\n", (unsigned int)dat, size);
        return false;
    }

     if (size > sdio_block_size) 
    {
        rx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    } 
    else 
    {
        blksize = size;
        rx_blocks = 1;
    }

    sdio_claim_host(g_func);
    
    ret = sdio_memcpy_fromio(g_func, (void*)dat, dataPort, blksize*rx_blocks);

    sdio_release_host(func);
	if (0 != ret)
	{
        SDIO_ERROR("sgm_sdc_api_dio_cmd53(read), ret = %d\r\n", ret);
	    return false;
	}
    
	SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "sdio_read_cmd53", dataPort, dat, size);
    
	return true;
}

bool sdio_write_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 tx_blocks = 0, blksize = 0;
    int ret = 0,
    
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte, data = 0x%08x, size = %d\r\n", (unsigned int)dat, size);
        return false;
    }

    if (size > sdio_block_size) 
    {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    }
    else 
    {
        blksize = size;
        tx_blocks = 1;
    }
    
    sdio_claim_host(g_func);
    writesize = blksize*tx_blocks;

    do
    {
        ret = sdio_memcpy_toio(g_func, dataPort, (void*)dat, writesize);
        if ( ret == -EILSEQ || ret == -ETIMEDOUT )
        {
            ret = -1;
            break;
        }
        else
        {
            if(ret)
                dev_err(glue->dev,"Unexpected return value ret=[%d]\n",ret);
        }
    }
    while( ret == -EILSEQ || ret == -ETIMEDOUT);
    sdio_release_host(g_func); 

    if (0 != ret)
    {        
        SDIO_ERROR("sdio_cmd53(write), ret = %d\r\n", ret);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "sdio_write_cmd53", dataPort, dat, size);

	return true;
}

bool sdio_host_enable_isr(bool enable)
{    
    int ret = 0;

    
    sdio_claim_host(g_func);
    
    if(enable)
        ret =  sdio_claim_irq(g_func, sdio_host_isr);
    else
        ret = sdio_release_irq(g_func);
        
    if (0 != ret)
    {
        SDIO_ERROR("gm_sdc_api_action, ret = %d enable = %d\r\n", ret, enable);
        return false;
    }
    
	SDIO_TRACE("%-20s : %d\n", "sdio_host_enable_isr", enable);
            
    sdio_release_host(g_func);

    return true;
}


u32 sdio_get_block_size()
{
    return sdio_block_size;
}

static void sdio_host_isr (struct sdio_func *func)
{

    sdio_host_enable_isr(false); //Disable first
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
    /**/
    return TRUE;

}


bool sdio_host_init(void (*sdio_isr)(void))
{
    u32 ret = 0;
    u8 in, out;

    sdio_claim_host(g_func);

    /* set block size */
    sdio_set_block_size(g_func,SDIO_DEFAULT_BLOCK_SIZE);
    
    /* enable sdio control interupt */
    /* install isr here */
    g_sdio_isr_func = sdio_isr; 
    ret =  sdio_claim_irq(g_func, sdio_host_isr);
    if (ret)
    {
        SDIO_ERROR("sdio_host_init, ret = %d\r\n", ret);
        return false;
    }
    
    sdio_release_host(g_func);
    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "host_int", "ok");
    
    return true;
}

