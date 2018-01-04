/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


//#include <config.h>
#include <log.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <hctrl.h>
#include "porting.h"

#include "sdio_port.h"
#include "sdio_def.h"
#include "driver_l2_cfg.h"

#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
#include "drv_l2_sdio.h"
#endif

#if (HOST_PLATFORM_SEL == GP_22B)
#include "drv_l1_clock.h"
#endif
#include "drv_l1_gpio.h"

//#define SDIO_PORT_DEBUG

static u32 sdio_block_size;
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
gpSDIOInfo_t* sdio_hd=NULL;
#endif
typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;
#if (HOST_PLATFORM_SEL == GP_15B)
int sdio_host_device_id = 0;	//needs to be determinded in application
#else
extern int sdio_host_device_id;
#endif



bool sdio_readb_cmd52(u32 addr, u8 *data)
{
    u8 in, out;
    s32 ret;
    
    in  = 0;
    out = 0;
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    ret = drvl2_sdio_rw_direct(sdio_hd, 0, 1, addr, in, &out);
#endif
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
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    ret = drvl2_sdio_rw_direct(sdio_hd, 1, 1, addr, in, &out);
#endif
    if (0 != ret)
    {
        SDIO_ERROR("drvl2_sdio_rw_direct(write), ret = %d\r\n", ret);
        return false;
    }
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "sdio_writeb_cmd52", addr, data);

    return true;
    
}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
	u32 rx_blocks = 0, blksize = 0;
    s32 ret = 0;
    
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

#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    ret = drvl2_sdio_rw_extended(sdio_hd, 0, 1, dataPort, 0, (u32 *)dat, rx_blocks, blksize);
#endif
	if (0 != ret)
	{
        SDIO_ERROR("drvl2_sdio_rw_extended(read), ret = %d\r\n", ret);
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

#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    ret = drvl2_sdio_rw_extended(sdio_hd, 1, 1, dataPort, 0, (u32 *)dat, tx_blocks, blksize);
#endif
    if (0 != ret)
    {        
        SDIO_ERROR("drvl2_sdio_rw_extended(write), ret = %d\r\n", ret);
        return false;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "sdio_write_cmd53", dataPort, dat, size);

	return true;
}
#define DRV_Reg32(addr)               (*(volatile INT32U *)(addr))

bool sdio_host_enable_isr(bool enable)
{    
    int ret = 0;
    u32 in;
    u32 type;
    
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    drvl2_sdio_irq_en(sdio_hd,enable);
#endif    
	//LOG_PRINTF("sdio_host_enable_isr,%d\r\n", enable);
    //LOG_PRINTF("%x,%x,%x\r\n",DRV_Reg32(0xC0250014),DRV_Reg32(0xC0250018),DRV_Reg32(0xC025001C));
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
    
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    ret = drvl2_sdio_rw_direct(sdio_hd, 1, 0, (int)0x110, in, &out);
    if (!ret)
    {
        in = blk[1];
        ret = drvl2_sdio_rw_direct(sdio_hd, 1, 0, (int)0x111, in, &out);
    }
#endif

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

bool sdio_set_clk(u32 clk)
{
    return true;
}

bool sdio_set_bwidth(u8 bwidth)
{
    //if(SDIO_BUS_1_BIT == bwidth)
    //    return sdio_set_bus_width(USE_ONE_BUS);

    //if(SDIO_BUS_4_BIT == bwidth)
    //    return sdio_set_bus_width(USE_FOUR_BUS);
    return true;
}

void sdio_host_isr (u16 arg)
{
    //if (SDHCI_INTR_STS_CARD_INTR & arg)
    {
        sdio_host_enable_isr(false);
        //LOG_PRINTF("%s\r\n",__func__);
        if (g_sdio_isr_func)
        {
            g_sdio_isr_func();
        }
    }
}

void SD0_IRQHandler(void)
{
    sdio_host_isr(0);
}

void SD1_IRQHandler(void)
{
    sdio_host_isr(1);
}

bool is_truly_isr()
{
    return TRUE;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    u32 ret = 0;
    u8 in, out;

#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    /* sdio control init*/
    if(sdio_hd == NULL)
    {
        /* sdio control init*/
#if (HOST_PLATFORM_SEL == GP_22B)
        drv_l1_clock_set_system_clk_en(CLK_EN0_SDC, 1);
#elif (HOST_PLATFORM_SEL == GP_15B)
        drvl2_sdio_register_isr_handler(sdio_host_device_id, (void*)((sdio_host_device_id == 0)?SD0_IRQHandler:SD1_IRQHandler));
#endif
        drvl2_sdio_init(sdio_host_device_id);

        sdio_hd = drvl2_sdio_get_info(sdio_host_device_id);
        /* enable isr on cis for my ic */
        in  = 0x3;
        out = 0;
        ret = drvl2_sdio_rw_direct(sdio_hd, 1, 0, 0x4, in, &out);
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
    	//ret = gm_sdc_api_action(sdio_host_device_id, GM_SDC_ACTION_SDIO_REG_IRQ, (void *)sdio_host_isr, NULL);
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
        LOG_PRINTF("output timing to %d blk size=%d\r\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEFAULT_BLOCK_SIZE);

        SDIO_TRACE("%-20s : %s\n", "host_int", "ok");
    }
#endif
    return true;
}

bool sdio_host_detect_card(void)
{
    u32 ret = 0;
    u8 in, out;
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
    drvl2_sdio_detect(sdio_host_device_id);
    sdio_hd = drvl2_sdio_get_info(sdio_host_device_id);
    /* enable isr on cis for my ic */
    in  = 0x3;
    out = 0;
    ret = drvl2_sdio_rw_direct(sdio_hd, 1, 0, 0x4, in, &out);
    if (0 != ret)
    {
        SDIO_ERROR("gm_sdc_api_sdio_cmd52 ret = %d\r\n", ret);
        return false;
    }

    /* set block size */
    sdio_set_block_size(SDIO_DEFAULT_BLOCK_SIZE);

    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);

    /* enable sdio control interupt */
    sdio_host_enable_isr(true);
#endif 
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

