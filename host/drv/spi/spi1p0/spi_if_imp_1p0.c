/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <host_config.h>
#if((CONFIG_CHIP_ID==SSV6051Q)||(CONFIG_CHIP_ID==SSV6051Z)||(CONFIG_CHIP_ID==SSV6030P))
#include "msgevt.h"
11

#include <ssv_drv_if.h>
#include <ssv_drv.h>
#include <ssv_hal.h>
#include <ssv_drv_config.h>
#include "spi_1p0.h"
#include "spi_if_imp_1p0.h"
#include "../spi_def.h"
#include "rtos.h"
#include "../../core/txrx_task.h"
#include "dev.h"
#define USE_DYNAMIC_BUF 0
u32 gSpiZeroLenCount=0;
u32 gSpiZeroLenRetryCount=0;
u32 gSpiReadCount=0;
extern DeviceInfo_st *gDeviceInfo;

static void _spi_host_isr(void);
static u32 _ssvcabrio_spi_read_status(void);
static bool _ssvcabrio_spi_read_reg (u32 addr, u32 *data);
static bool _ssvcabrio_spi_write_reg (u32 addr, u32 data);
static bool _ssvcabrio_spi_init (void);
static u32 _ssvcabrio_write_fw_to_sram(u8* fw_bin, u32 fw_bin_len);
static bool _ssvcabrio_do_firmware_checksum(u32 origin);
static bool _check_rx_rdy(u32 *rx_len);
static void _spi_host_isr(void)
{
    TXRXTask_Isr(INT_RX,spi_is_truly_isr());
}
static u32 _ssvcabrio_spi_read_status(void)
{
    u32 sizeToTransfer;
    u32 sizeTransfered;
    bool status=TRUE;
    u8     cmd[5];
    u32    data32[2]={0};
    u8    *data = (u8 *)&data32[0];

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    // Send read register address
    sizeToTransfer = 1;
    data += 4;
    cmd[0] = SPI_READ_STS_CMD;
    status = spi_host_write(cmd, sizeToTransfer,&sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                        );
    if(FALSE==status) goto ERROR;

    MEMSET((u8 *)data32,0,sizeof(data32));
    sizeToTransfer = 4;
    status = spi_host_read(data, sizeToTransfer, &sizeTransfered,
                    SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                    | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                    | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                    );

    if(FALSE==status) goto ERROR;
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return data32[1];
ERROR:
    SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return 0xFFFFFFFF;
}

static bool _ssvcabrio_spi_read_reg (u32 addr, u32 *data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    u32 retry=0;
    bool status=TRUE;
    u8  cmd[9]; // 1B CMD + 4B ADDR + 4B DATA
    u8  temp[4]={0};

    // Send read register address
    sizeToTransfer = 5;
    cmd[0] = SPI_READ_REG_CMD;
    cmd[1]= (u8)(addr >> 24);
    cmd[2]= (u8)(addr >> 16);
    cmd[3]= (u8)(addr >> 8);
    cmd[4]= (u8)(addr);

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    status = spi_host_read((u8 *)temp, 4, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    *data = (  ((u32)temp[0]) | ((u32)temp[1] << 8)
            | ((u32)temp[2] << 16) | ((u32)temp[3] << 24));

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
ERROR:
    SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return FALSE;
}

static bool _ssvcabrio_spi_write_reg (u32 addr, u32 data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    u32 retry=0;
    bool status=TRUE;
    u8  cmd[9]; // 1B CMD + 4B ADDR + 4B DATA

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    //BEGIN_SPI_TRANS();

    cmd[0] = SPI_WRITE_REG_CMD;
    cmd[1]= (u8)(addr >> 24);
    cmd[2]= (u8)(addr >> 16);
    cmd[3]= (u8)(addr >> 8);
    cmd[4]= (u8)(addr);
    cmd[5]= (u8)(data);
    cmd[6]= (u8)(data >> 8);
    cmd[7]= (u8)(data >> 16);
    cmd[8]= (u8)(data >> 24); // MSB last
    sizeToTransfer = 5;
    sizeTransfered = 0;
    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                    | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                    | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                    | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                    );
    if(FALSE==status) goto ERROR;

    sizeToTransfer = 4;
    sizeTransfered = 0;
    status = spi_host_write(&cmd[5], sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                    | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                    | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                    | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                    );
    if(FALSE==status) goto ERROR;

    //Ian ToDo: check status
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);

    return TRUE;
ERROR:
    SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return FALSE;
}


static bool _ssvcabrio_spi_init (void)
{
    u32 data;
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    //OS_MutexInit(&s_SPIMutex);
    // Read status register to ensure CS is up for 1st meaningful command.
    _ssvcabrio_spi_read_status();
    // Switch HCI to SPI
    _ssvcabrio_spi_write_reg(0xc0000a0c, 0);
    // bit 8 to enable TX. Set block size.
    _ssvcabrio_spi_write_reg(0xc0000a24, 0x100 | (SPI_TX_BLOCK_SHIFT));
    #if 0
    //read CHIP_ID_0, 0x31333131
    _ssvcabrio_spi_read_reg(0xc0000008,&data);
    //read CHIP_ID_1, 0x322d3230
    _ssvcabrio_spi_read_reg(0xc000000C,&data);
    //read CHIP_ID_2, 0x32303041 ==> get 0x32303241 ?
    _ssvcabrio_spi_read_reg(0xc0000010,&data);
    //read CHIP_ID_3, 0x53535636 ==> get 0x53535336 ?
    _ssvcabrio_spi_read_reg(0xc0000014,&data);
    #endif
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

static bool _check_rx_rdy(u32 *rx_len)
{
#define CHECK_RDY_RETRY_COUNT 1
    u32 status32 = 0;
    u8  *ptr=NULL;
    u8 retry=0;

    do {
        status32 = _ssvcabrio_spi_read_status();
        ptr= (u8 *)&status32;
        if(status32!=0xFFFFFFFF){
            if (ptr[READ_STATUS_BYTE0] & READ_STATUS_BYTE0_RX_RDY) // rx ready?
            {
                *rx_len= (((u32)(ptr[READ_STATUS_RX_LEN_L]) & (u32)0x000000ffUL)) |
                (((u32)(ptr[READ_STATUS_RX_LEN_H]) & (u32)0x000000ffUL) << 8);
                break;
            }
        }
        retry++;
    } while (retry < CHECK_RDY_RETRY_COUNT);

    if(retry==CHECK_RDY_RETRY_COUNT)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}
u8 temp_wbuf[1600];
static u8 *_spi_alloc_load_fw_buf(u32 len)
{
    #if(USE_DYNAMIC_BUF==1)
    return OS_MemAlloc(len);
    #else
    return temp_wbuf;
    #endif
}

static void _spi_free_load_fw_buf(u8 *buf)
{
    #if(USE_DYNAMIC_BUF==1)
    OS_MemFree(buf);
    #else
    OS_MemSET(temp_wbuf,0,sizeof(temp_wbuf));
    #endif
}

static u32 ssvcabrio_write_fw_to_sram(u8* fw_bin, u32 fw_bin_len, u32 block_size)
{
    u8   *fw_buffer = NULL;
    u8   *fw_ptr = fw_bin;
    u32  sram_addr = 0x00000000;
    u32  bin_len=fw_bin_len;
    u32  i=0,len=0, checksum=0, temp=0;
    u32  j=0;

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    fw_buffer = (u8 *)_spi_alloc_load_fw_buf(block_size);
    if (fw_buffer == NULL)
    {
        SDRV_FAIL("%s(): Failed to allocate buffer for firmware.\r\n",__FUNCTION__);
        return 0;
    }

    while(bin_len > 0){
        len=(bin_len >= block_size)?block_size:bin_len;
        bin_len -= len;
        if(len!=block_size)
        {
            MEMSET((void*)fw_buffer, 0xA5, block_size);
        }
        MEMCPY((void*)fw_buffer,(void*)fw_ptr,len);
        fw_ptr += len;

        SDRV_DEBUG("%s(): read len=0x%x,sram_addr=0x%x\r\n",__FUNCTION__,len,sram_addr);
        for (i = 0; i < (block_size)/4; i++) /* force 1024 bytes a set. */
        {
            temp = *((u32 *)(&fw_buffer[i*4]));
            if(FALSE==_ssvcabrio_spi_write_reg(sram_addr, temp))
            {
                goto ERROR;
            }
            sram_addr += 4;
            checksum += temp;
        }
        j++;
        if(gDeviceInfo->recovering != TRUE)
        {
            SDRV_INFO("* ",__FUNCTION__);
            if(j%16==0)
            {
                SDRV_INFO("\r\n",__FUNCTION__);
            }
        }
    }// while(bin_len > 0){
    SDRV_INFO("\r\n",__FUNCTION__);
    SDRV_DEBUG("%s(): checksum = 0x%x\r\n",__FUNCTION__, checksum);
    _spi_free_load_fw_buf(fw_buffer);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return checksum;
ERROR:
    SDRV_INFO("\r\n",__FUNCTION__);
    _spi_free_load_fw_buf(fw_buffer);
    SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return 0;
}

static bool ssvcabrio_init(void)
{
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    _ssvcabrio_spi_init();
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

static u32 ssvcabrio_read_reg(u32 addr)
{
    u32 data;
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

    if(FALSE==_ssvcabrio_spi_read_reg(addr, &data))
    {
        data=0;
    }

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return data;
}

static bool ssvcabrio_write_reg(u32 addr, u32 data)
{
    bool ret=FALSE;
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

    ret = _ssvcabrio_spi_write_reg(addr, data);

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return ret;
}


static bool ssvcabrio_open(void)
{
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    spi_host_init(_spi_host_isr);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

static bool ssvcabrio_close(void)
{
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}


static s32 ssvcabrio_recv(u8 *data, size_t len)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[4]={0};
    u8  dummy[4]={0};
    u8 align_4_len;
    u32 rlen=0;
    u8 retry=2;

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    if(data==NULL)
    {
        SDRV_ERROR("%s(): data is a null pointer\r\n",__FUNCTION__);
        return -1;
    }
#if 1
    do{
        if(FALSE==_check_rx_rdy(&rlen))
        {
            goto NOT_READY;
        }
        retry--;
    }while((rlen==0)&&(retry!=0));

    if(retry==0) gSpiZeroLenRetryCount++;
#else
    if(FALSE==_check_rx_rdy(&rlen))
    {
        goto NOT_READY;
    }
#endif
    SDRV_DEBUG("%s() :rlen=%d(0x%x) \r\n",__FUNCTION__,rlen,rlen);
    if(rlen==0){
        gSpiZeroLenCount++;
        goto ERROR;
    }
    gSpiReadCount++;
    rlen=(rlen<len)?rlen:len;
    //align_4_len
    align_4_len=(rlen%4)?(4-(rlen%4)):0;
    SDRV_DEBUG("%s() :align_4_len=%d(0x%x) \r\n",__FUNCTION__,align_4_len,align_4_len);

    // Send read register address
    sizeToTransfer = 4;
    cmd[0] = SPI_RX_DATA_CMD;

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    //read data
    if(align_4_len!=0)
    {
        status = spi_host_read((u8 *)data, rlen, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);

        if(FALSE==status) goto ERROR;

        status = spi_host_read((u8 *)&dummy[0], align_4_len, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                            | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                            | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
        if(FALSE==status) goto ERROR;
    }
    else
    {
        status = spi_host_read((u8 *)data, rlen, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      );
        if(FALSE==status) goto ERROR;

    }
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return rlen;
ERROR:
    SDRV_ERROR("%s(): fail\r\n",__FUNCTION__);
    return -1;
NOT_READY:
    SDRV_DEBUG("%s(): NOT_READY\r\n",__FUNCTION__);
    return -1;
}



static s32 ssvcabrio_send(void *data, size_t len)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[4]={0};
    u8  dummy[4]={0};
    u8  align_4_len;

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    //alignment 4 bytes
    align_4_len=(len%4)?(4-(len%4)):0;
    SDRV_DEBUG("%s() :align_4_len=%d(0x%x) \r\n",__FUNCTION__,align_4_len,align_4_len);

    // Send read register address
    sizeToTransfer = 4;
    cmd[0] = SPI_TX_DATA_CMD;

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );

    if(FALSE==status) goto ERROR;

    //write data
    if(align_4_len!=0)
    {
        status = spi_host_write((u8 *)data, len, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      );

        if(FALSE==status) goto ERROR;

        status = spi_host_write((u8 *)&dummy[0], align_4_len, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      );
        if(FALSE==status) goto ERROR;
    }
    else
    {
        status = spi_host_write((u8 *)data, len, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      );
        if(FALSE==status) goto ERROR;


    }
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return len;
ERROR:
    SDRV_ERROR("%s():FAIL\r\n",__FUNCTION__);
    return -1;
}

static void ssvcabrio_irq_enable(void)
{
    spi_host_irq_enable(TRUE);
}

static void ssvcabrio_irq_disable(void)
{
    spi_host_irq_enable(FALSE);
}

static bool ssvcabrio_wakeup_wifi(bool sw)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[4];

    //LOG_PRINTF("%s() => :%d\r\n",__FUNCTION__,sw);
    sizeToTransfer = 4;
    if(sw)
    {
        _ssvcabrio_spi_write_reg(0xC0000A18,0x000EFFFF); //ADR_SPI_TO_PHY_PARAM1
        // Send SPI special pattern to wakeup PMU
        cmd[0] = 0x27;
        cmd[1] = 0XF0;
        cmd[2] = 0;
        cmd[3] = 0;

    }
    else
    {
        // Send SPI special pattern to clear previous wakeup signal.
        cmd[0] = 0x27;
        cmd[1] = 0;
        cmd[2] = 0;
        cmd[3] = 0;

    }
    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                    SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                  | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                  | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                  | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                  );

    return  status;
}


static bool spi_if_detect_card(void)
{
    return TRUE;
}


struct ssv6xxx_drv_ops  g_drv_spi=
{
    DRV_NAME_SPI, //name
    ssvcabrio_open, //open
    ssvcabrio_close, //close
    ssvcabrio_init, //init
    ssvcabrio_recv, //recv
    ssvcabrio_send, //send
    NULL, //get name
    NULL, //ioctl
    NULL, //handle
    NULL, //ack init
    NULL, //write sram
    NULL, //read sram
    ssvcabrio_write_reg, //write reg
    ssvcabrio_read_reg, // read reg
    NULL, //write bytes
    NULL, //read bytes
    ssvcabrio_write_fw_to_sram, //load fw
    NULL, //start
    NULL, //stop
    ssvcabrio_irq_enable, //irq enable
    ssvcabrio_irq_disable, //irq disable
    ssvcabrio_wakeup_wifi, //wakeup_wifi
    spi_if_detect_card,
};


#endif //#if((CONFIG_CHIP_ID==ID_SSV6051Q)||(CONFIG_CHIP_ID==ID_SSV6051Z)||(CONFIG_CHIP_ID==ID_SSV6030P))
