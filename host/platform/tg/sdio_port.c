#include <errno.h>
#include <string.h>
#include <log.h>
#include "../ssv_drv_if.h"
#include "sdio_port.h"
#include "sdio_def.h"
#include <host_apis.h>

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static unsigned int sdio_block_size;
void (*sdio_int_isr)(void);

bool sdio_host_enable_isr(bool enable)
{  
    tg4xx_mmc_enable_sdio_irq(enable);
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
    if (!ret)
    {
        ret = SDIO_Write_Byte(0, (int)0x111, blk[1]);
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
    return TRUE;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    int ret = 0;
    
    SDIO_Init();
   
    sdio_int_isr = sdio_isr;

    sdio_set_block_size(64);
    ret = SDIO_Write_Byte(0, 0x4, 0x3);
    
    if (0 != ret)
    {
        SDIO_ERROR("mmc_io_rw_direct_host ret = %d\r\n", ret);
        return false;
    }
    
    sdio_host_enable_isr(false);

    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, REG_OUTPUT_TIMING_REG);

	SDIO_TRACE("%-20s : %s\n", "host_int", "ok");

    return true;
}

u8	sdio_readb_cmd52(u32 addr)
{
    u8 data;
    int ret;
    ret = SDIO_Read_Byte(1,addr,&data);
    if (ret != 0)
    {
        SDIO_ERROR("sdio_readb_cmd52 ret = %d, addr = %x\r\n", ret, addr);
        return 0;
    }
    
//	printf("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, data);
    
    return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    int ret;
    ret = SDIO_Write_Byte(1, addr,data);
    if (ret != 0)
    {
        printf("sdio_drv_creg_write ret = %d\r\n", ret);
        return false;
    }
//	printf("%-20s : 0x%08x, 0x%02x\n", "WRITE_BYTE", addr, data);
    
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
        blksize = size;
        rx_blocks = 1;
    }

    ret = SDIO_Read_Multi_Byte(1, dataPort, size, 0, dat);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_drv_read error ret = % d \r\n", ret);
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
        buflen = size;
        tx_blocks = 1;
    }

    ret = SDIO_Write_Multi_Byte(1, dataPort, size, 0, dat);
    if (0 != ret)
    {        
            SDIO_ERROR("sdio_drv_write error \r\n");
            return false;
    }

    SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "WRITE_BLOCK", dataPort, dat, size);

    return true;
}

