#include <log.h>

#include "sdio_port.h"
#include "sdio_def.h"
#include "nuttx/wqueue.h"
#include "../../drivers/mmcsd/mmcsd_wifi.h"

#define SDIO_PORT_DEBUG

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static unsigned int sdio_block_size;

typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

int test_sdio_isr;


u8	sdio_readb_cmd52(u32 addr)
{
	u8   data;
    wwd_result_t ret;

    ret = wwd_bus_cmd52(BUS_READ, 1, addr, 0, RESPONSE_NEEDED, &data);
    if (ret != SDIO_SUCCESS)
    {
        SDIO_ERROR("wwd_bus_cmd52 read, ret = %d\r\n", ret);
        return 0;
    }
        
	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, data);
    
	return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
	unsigned int  resp;
    int ret;
    
    ret = wwd_bus_cmd52(BUS_WRITE, 1, addr, data, NO_RESPONSE, NULL);
    if (ret != SDIO_SUCCESS)
    {
        SDIO_ERROR("wwd_bus_cmd52 write, ret = %d\r\n", ret);
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
    } else {
        blksize = size;
        rx_blocks = 1;
    }

    if (blksize > 0)
    {
        ret = wwd_bus_cmd53(BUS_READ, 1, dataPort, rx_blocks*blksize, dat,
                NO_RESPONSE, 0);
        if (ret != SDIO_SUCCESS)
        {
            SDIO_ERROR("wwd_bus_cmd53 read, ret = %d\r\n", ret);
            return false;
        }
    }
    
	SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "READ_BLOCK", dataPort, dat, size);
	return true;
}

bool sdio_write_cmd53(u32 dataPort, void *dat, size_t size)
{
	unsigned int tx_blocks = 0, blksize = 0;
    int ret = 0;
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }

    if (size > sdio_block_size) {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    } else {
        blksize = size;
        tx_blocks = 1;
    }
    
    if (blksize > 0)
    {
        ret = wwd_bus_cmd53(BUS_WRITE, 1, dataPort, tx_blocks*blksize, dat,
                NO_RESPONSE, 0);
        if (ret != SDIO_SUCCESS)
        {
            SDIO_ERROR("wwd_bus_cmd53 write, ret = %d len = %d\r\n", ret, tx_blocks*blksize);
            return false;
        }
    }
    
    if (ret != SDIO_SUCCESS)
    {
        SDIO_ERROR("sdio_io_rw_extended write, ret = %d\r\n", ret);
        return 0;
    }

	SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "WRITE_BLOCK", dataPort, dat, size);

	return true;
}

bool sdio_host_enable_isr(bool enable)
{    
    sdio_interupt_enable(enable);
    
	SDIO_TRACE("%-20s : %d\n", "enable_isr", enable);
    
    return true;
}

bool sdio_set_blk_size(unsigned int blksize)
{
    unsigned char blk[2];
    unsigned int resp;
    int ret;
    
    if (blksize == 0)
    {
        blksize = SDIO_DEFAULT_BLOCK_SIZE;
    }
    
    ret = sdio_set_block_size(1, blksize);
    
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

unsigned int sdio_get_blk_size()
{
    return sdio_block_size;
}

static void sdio_host_isr (void *arg)
{
	if(test_sdio_isr)
	{
    	printf("sdio_host_isr\r\n");
	}
    //LOG_PRINTF("sdio_host_isr\r\n");
    sdio_host_enable_isr(true);
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
    unsigned char response;
    
    sdio_intf_initialize();
    if (1)
    {
        wwd_bus_cmd52(BUS_READ, 0, 0x7, 0x0, RESPONSE_NEEDED, &response);
        response &= 0xfc;
        response |= (1 << 1);
        wwd_bus_cmd52(BUS_WRITE, 0, 0x7, response, NO_RESPONSE, 0);
        
        sdio_speed_config(25000000, 4);
    }
	
		
    sdio_set_blk_size(SDIO_DEFAULT_BLOCK_SIZE);

    wwd_bus_cmd52(BUS_WRITE, 0, 0x4, 0x3, NO_RESPONSE, NULL);

    sdio_interupt_register(sdio_host_isr, NULL);

    //install isr here
    sdio_host_enable_isr(false);

    g_sdio_isr_func = sdio_isr;
    
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

