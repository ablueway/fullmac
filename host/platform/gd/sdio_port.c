/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <log.h>

#include "sdio_port.h"
#include "sdio_def.h"

//#include "GD25Qxx.h"
#include "sdcard.h"

//#define SDIO_PORT_DEBUG

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static unsigned int sdio_block_size;
void (*gpio_sdio_isr)(void);

/* GD blocksize must be 2 power */
static u32 convert_blocksize(u32 size)
{
    u32 i = 0;
    
    while (i < 16)
    {
        if ((2 << i) >=  size)
        {
            return (2 << i);
        }
        i ++;
    }
    
    return(size);
}

bool sdio_host_enable_isr(bool enable)
{  
    if(enable)
    {        
        SDIO_INTConfig(SDIO_INT_SDIOINT,ENABLE);
    }
    else
    {
        SDIO_ClearIntBitState(SDIO_INT_SDIOINT);
        SDIO_INTConfig(SDIO_INT_SDIOINT,DISABLE);
    }
    
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
    
	ret = SDIO_Write_Byte(0, (int)0x110, blk[0]);
    if (ret == SD_OK)
    {
        ret = SDIO_Write_Byte(0, (int)111, blk[1]);
    }

	ret = SDIO_Write_Byte(0, (int)0x10, blk[0]);
    if (ret == SD_OK)
    {
        ret = SDIO_Write_Byte(0, (int)0x11, blk[1]);
    }

    if (ret == SD_OK)
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
    //SD_Init();
    return TRUE;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    int ret = SD_OK;
    
    ret = SD_Init();
    if (SD_OK != ret)
    {
        SDIO_ERROR("SD_Init faild. ret = %d\r\n", ret);
    }

    sdio_set_block_size(SDIO_DEFAULT_BLOCK_SIZE);

    gpio_sdio_isr = sdio_isr;

	SDIO_Write_Byte(0, 0x7, 0x2);

    ret = SDIO_Write_Byte(0, 0x4, 0x3);
    if (SD_OK != ret)
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
    
    ret = SDIO_Read_Byte(1, addr, &data);
    if (ret != SD_OK)
        SDIO_ERROR("_sdio_drv_creg_read ret = %d\r\n", ret);
    
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, data);
    
    return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    int ret;

    ret = SDIO_Write_Byte(1, addr, data);
    if (ret != SD_OK)
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
         size = convert_blocksize(size);
         blksize = size;
         rx_blocks = 1;
    }

    ret = SDIO_Read_Multi_Byte(1, dataPort, size, 0, dat);
    if (SD_OK != ret)
    {
        SDIO_ERROR("SDIO_Read_Multi_Byte error ret = % d \r\n", ret);
        return false;
    }
    
    SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "READ_BLOCK", dataPort, dat, size);
    return true;    
}

bool sdio_write_cmd53(u32 dataPort, void *dat, size_t size)
{
    unsigned int tx_blocks = 0, buflen = 0;
    int ret = 0;
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
        size = convert_blocksize(size);
        buflen = size;
        tx_blocks = 1;
    }

    ret = SDIO_Write_Multi_Byte(1, dataPort, size, 0, dat);
    if (SD_OK != ret)
    {        
        SDIO_ERROR("SDIO_Write_Multi_Byte error ret = %d len = %d\r\n", ret, size);
        return false;
    }

    SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "WRITE_BLOCK", dataPort, dat, size);

    return true;
}

#define SDIO_PORT_DEBUG

#ifdef SDIO_PORT_DEBUG
unsigned char sdio_data[1024];
void sdio_test_cmd53(int size)
{
    int i = 0;
    u32 tick1 = 0;
    for (i = 0; i < 1024; i++)
    {
        sdio_data[i] = i & 0x3f;
    }
    
    sdio_data[60] = 0x00;
    sdio_data[61] = 0x00;
    sdio_data[62] = 0x55;
    sdio_data[63] = 0x00;

    sdio_data[0] = 0xb0;
    sdio_data[1] = 0x02;
    sdio_data[2] = 0x00;
    sdio_data[3] = 0xca;
    tick1 = OS_GetSysTick();
    for (i = 0; i < 10000; i++)
    {
        //sdio_write_cmd53(0x10000, sdio_data, size);
        ssv6xxx_drv_send(sdio_data, size);
    }
    LOG_PRINTF("tick=%u\r\n",OS_GetSysTick()-tick1);
    
}
void sdio_test_read(int addr)
{
    printf("addr=%x, value=%x\r\n", addr, *((volatile u32 *)(0x40000000 + 0x18000 + addr)));
}




#endif

