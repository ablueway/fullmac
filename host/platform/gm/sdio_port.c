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

#include "sdio_port.h"
#include "sdio_def.h"

#include "ftsdc021.h"
#include "ftsdc021_sdio.h"
#include "lib_sdc.h"

//#define SDIO_PORT_DEBUG

#define SDIO_INDEX 0

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static u32 sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

bool sdio_set_clk(u32 clk)
{
    return true;
}

bool sdio_set_bwidth(u8 bwidth)
{
    u32 bwith;
    if(SDIO_BUS_1_BIT == bwidth)
        bwith = 1;
    if(SDIO_BUS_4_BIT == bwidth)
        bwith = 4;
    gm_sdc_api_action(SDIO_INDEX, GM_SDC_ACTION_SET_BUS_WIDTH, (void *)&bwith, NULL);
    return true;
}


u8	sdio_readb_cmd52(u32 addr,u8 *data)
{
    u8 in, out;
    u32 ret;
    
    in  = 0;
    out = 0;
    ret = gm_sdc_api_sdio_cmd52(SDIO_INDEX, 0, 1, addr, in, &out);
    if (0 != ret)
    {
        SDIO_ERROR("drvl2_sdio_rw_direct(read), ret = %d\r\n", ret);
        *data=0;
        return false;
    }
    else
    {
        *data=out;
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_readb_cmd52", addr, out);

    return true;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    
    u8 in, out;
    u32 ret;
    
    in  = data;
    out = 0;
    ret = gm_sdc_api_sdio_cmd52(SDIO_INDEX, 1, 1, addr, in, &out);
    if (0 != ret)
    {
        SDIO_ERROR("gm_sdc_api_sdio_cmd52(write), ret = %d\r\n", ret);
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

    ret = gm_sdc_api_sdio_cmd53(SDIO_INDEX, 0, 1, dataPort, 0, (u32 *)dat, rx_blocks, blksize);
	if (0 != ret)
	{
        SDIO_ERROR("gm_sdc_api_sdio_cmd53(read), ret = %d\r\n", ret);
	    return false;
	}
    
	SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "sdio_read_cmd53", dataPort, dat, size);
    
	return true;

}

bool sdio_write_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 tx_blocks = 0, blksize = 0;
    u32 ret = 0;
    
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

    ret = gm_sdc_api_sdio_cmd53(SDIO_INDEX, 1, 1, dataPort, 0, (u32 *)dat, tx_blocks, blksize);
    if (0 != ret)
    {        
        SDIO_ERROR("gm_sdc_api_sdio_cmd53(write), ret = %d\r\n", ret);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "sdio_write_cmd53", dataPort, dat, size);

	return true;
}

bool sdio_host_enable_isr(bool enable)
{    
    int ret = 0;
    u32 in;
    u32 type;
    
    type = enable ? GM_SDC_ACTION_ENABLE_IRQ : GM_SDC_ACTION_DISABLE_IRQ;
    in = SDHCI_INTR_STS_CARD_INTR;

    ret = gm_sdc_api_action(SDIO_INDEX, type, &in, NULL);
    if (0 != ret)
    {
        SDIO_ERROR("gm_sdc_api_action, ret = %d enable = %d\r\n", ret, enable);
        return false;
    }
    
	SDIO_TRACE("%-20s : %d\n", "sdio_host_enable_isr", enable);
    
    return true;
}

bool sdio_set_block_size(unsigned int blksize)
{
    unsigned char blk[2];
    u8 in, out;
    int ret;
    
    if ((blksize == 0) || (blksize > 512))
    {
        blksize = SDIO_DEFAULT_BLOCK_SIZE;
    }
    
    blk[0] = (blksize >> 0) & 0xff;
    blk[1] = (blksize >> 8) & 0xff;
    in = blk[0];
    
    ret = gm_sdc_api_sdio_cmd52(SDIO_INDEX, 1, 0, (int)0x110, in, &out);
    if (!ret)
    {
        in = blk[1];
        ret = gm_sdc_api_sdio_cmd52(SDIO_INDEX, 1, 0, (int)0x111, in, &out);
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

u32 sdio_get_block_size()
{
    return sdio_block_size;
}

static void sdio_host_isr (u16 arg)
{
    if (SDHCI_INTR_STS_CARD_INTR & arg)
    {
        sdio_host_enable_isr(false);
        if (g_sdio_isr_func)
        {
            g_sdio_isr_func();
        }
    }
}

bool is_truly_isr()
{
    return TRUE;
}
#define DISPLAY_SRC_WIDTH_MAX    848 //1920
#define DISPLAY_SRC_HEIGHT_MAX   480 //1080

#define AVAILABLE_MEM_START                  (0x1600000ul)
#define CAP0_PATH0_YUV422_BUF0               ALIGN(AVAILABLE_MEM_START, 8)
#define CAP0_PATH0_YUV422_BUF1               ALIGN((CAP0_PATH0_YUV422_BUF0 +  (DISPLAY_SRC_WIDTH_MAX * DISPLAY_SRC_HEIGHT_MAX) * 2), 256)
#define CAP0_PATH0_YUV422_BUF2               ALIGN((CAP0_PATH0_YUV422_BUF1 +  (DISPLAY_SRC_WIDTH_MAX * DISPLAY_SRC_HEIGHT_MAX) * 2), 256)
#define FT3DNR_YUV422_BUF0                   ALIGN((CAP0_PATH0_YUV422_BUF2 +  (DISPLAY_SRC_WIDTH_MAX * DISPLAY_SRC_HEIGHT_MAX) * 2), 256)
#define FT3DNR_YUV422_BUF1                   ALIGN((FT3DNR_YUV422_BUF0 +      (DISPLAY_SRC_WIDTH_MAX * DISPLAY_SRC_HEIGHT_MAX) * 2), 256)
#define FT3DNR_YUV422_REFBUF                 ALIGN((FT3DNR_YUV422_BUF1 +      (DISPLAY_SRC_WIDTH_MAX * DISPLAY_SRC_HEIGHT_MAX) * 2), 256)
#define H264_OUT_BUF_START                   ALIGN((FT3DNR_YUV422_REFBUF +    (DISPLAY_SRC_WIDTH_MAX * DISPLAY_SRC_HEIGHT_MAX) * 2), 8)
#define WWD_BUF                              ALIGN((H264_OUT_BUF_START + SZ_8M) , 64)
#define FTSDC021_READ_BUF               ALIGN((WWD_BUF + SZ_64K), 4)
#define FTSDC021_SD_CARD_BUF                 ALIGN((FTSDC021_READ_BUF + SZ_1K),4)
#define FTSDC021_SD_1_ADMA_BUF               ALIGN((FTSDC021_SD_CARD_BUF + SZ_512),4)


extern void sdc_platform_setting();

bool sdio_host_detect_card(void)
{
    u32 sd1_option = 0, sd0_option = 0;
    u32 addr; 
    
    sd0_option = SDC_OPTION_ENABLE | SDC_OPTION_FIXED;
	gm_intc_close_irq(SDC_FTSDC021_0_IRQ);

	gm_api_sdc_platform_init(sd0_option,0,sdc_platform_setting,FTSDC021_SD_CARD_BUF);
	addr = FTSDC021_SD_1_ADMA_BUF;
	gm_sdc_api_action(SD_0,GM_SDC_ACTION_SET_ADMA_BUFER,&addr,NULL);
	gm_sdc_api_action(SD_0,GM_SDC_ACTION_REG_SDIO_APP_INIT,(void*)NULL,NULL);
	gm_sdc_api_action(SD_0,GM_SDC_ACTION_CARD_DETECTION,NULL,NULL);


    return TRUE;

}

bool sdio_host_init(void (*sdio_isr)(void))
{
    u32 ret = 0;
    u8 in, out;

    /* sdio control init, enum sdio card already fh's main.c implement */
    
    /* enable isr on cis for my ic */
    in  = 0x3;
    out = 0;
    ret = gm_sdc_api_sdio_cmd52(SDIO_INDEX, 1, 0, 0x4, in, &out);
    if (0 != ret)
    {
        SDIO_ERROR("gm_sdc_api_sdio_cmd52 ret = %d\r\n", ret);
        return false;
    }

    /* set block size */
    sdio_set_block_size(SDIO_DEFAULT_BLOCK_SIZE);
    
    /* enable sdio control interupt */
    sdio_host_enable_isr(true);

    /* install isr here */ 
	ret = gm_sdc_api_action(SDIO_INDEX, GM_SDC_ACTION_SDIO_REG_IRQ, (void *)sdio_host_isr, NULL);
    if (!ret)
    {
        g_sdio_isr_func = sdio_isr;
    }
    else
    {
        SDIO_ERROR("gm_sdc_api_action, ret = %d\r\n", ret);
        return false;
    }
    
    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "host_int", "ok");
    
    return true;
}

#ifdef SDIO_PORT_DEBUG
#define test_read_register(reg) (*((volatile UINT32*)(reg)))
static volatile ftsdc021_reg *gpRegSD0 = (ftsdc021_reg*) SDC_FTSDC021_0_PA_BASE;

void sdio_in_read_reg(u32 addr)
{
    u8 value;
    #if 0
    unsigned long base = 0x9A200000 + addr;
    unsigned long value = 0;

    value = test_read_register(base);
    #endif
    value = sdio_readb_cmd52(addr);
	printf("0x%08x, 0x%02x\n",  addr, value);
}

void sdio_read_sdio_reg()
{    
	printf("IntrEn = 0x%04x\n",  gpRegSD0->IntrEn);
	printf("IntrSigEn = 0x%04x\n",  gpRegSD0->IntrSigEn);
	printf("IntrSts = 0x%04x\n",  gpRegSD0->IntrSts);
	printf("ErrSts = 0x%04x\n",  gpRegSD0->ErrSts);
    
}

#endif

