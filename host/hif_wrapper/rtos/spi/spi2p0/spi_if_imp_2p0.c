/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include<host_config.h>
#if((CONFIG_CHIP_ID==SSV6006B)||(CONFIG_CHIP_ID==SSV6006C))
#include <rtos.h>
#include <log.h>
#include <msgevt.h>
#include <dev.h>
#include <ssv_drv_if.h>
#include <ssv_drv.h>
#include <ssv_hal.h>
#include <ssv_drv_config.h>
#include "spi_2p0.h"
#include "spi_if_imp_2p0.h"
#include "../spi_def.h"
#include "../../core/txrx_task.h"


/* -------------- h/w register ------------------- */
#define BASE_SDIO	0

/* Note :
	For now, the reg of SDIO Host & Card Controller are the same.
If it changes in the future, you should define again.
*/
#define REG_DATA_IO_PORT_0		(BASE_SDIO + 0x00)		// 0
#define REG_DATA_IO_PORT_1	    (BASE_SDIO + 0x01)		// 0
#define REG_DATA_IO_PORT_2		(BASE_SDIO + 0x02)		// 0
#define REG_INT_MASK			(BASE_SDIO + 0x04)		// 0
#define REG_INT_STATUS			(BASE_SDIO + 0x08)		// 0
#define REG_INT_TRIGGER			(BASE_SDIO + 0x09)		// 0
#define REG_Fn1_STATUS			(BASE_SDIO + 0x0c)		// 0
#define REG_CARD_PKT_LEN_0		(BASE_SDIO + 0x10)		// 0
#define REG_CARD_PKT_LEN_1		(BASE_SDIO + 0x11)		// 0
#define REG_CARD_FW_DL_STATUS	(BASE_SDIO + 0x12)		// 0
#define REG_CARD_SELF_TEST		(BASE_SDIO + 0x13)		// 0
#define REG_CARD_RCA_0			(BASE_SDIO + 0x20)		// 0
#define REG_CARD_RCA_1			(BASE_SDIO + 0x21)		// 0
#define REG_SDIO_FIFO_WR_THLD_0	(BASE_SDIO + 0x24)		// 80
#define REG_SDIO_FIFO_WR_THLD_1	(BASE_SDIO + 0x25)		// 0
#define REG_OUTPUT_TIMING_REG	(BASE_SDIO + 0x55)		// 0
//#define REG_PMU_WAKEUP			(BASE_SDIO + 0x64)		// 0 FPGA
#define REG_PMU_WAKEUP			(BASE_SDIO + 0x67)		// 0
#define REG_REG_IO_PORT_0		(BASE_SDIO + 0x70)		// 0
#define REG_REG_IO_PORT_1	    (BASE_SDIO + 0x71)		// 0
#define REG_REG_IO_PORT_2		(BASE_SDIO + 0x72)		// 0

//SDIO TX ALLOCATE FUNCTION
#define REG_SDIO_TX_ALLOC_SIZE	(BASE_SDIO + 0x98)		// 0
#define REG_SDIO_TX_ALLOC_SHIFT	(BASE_SDIO + 0x99)		// 0
#define REG_SDIO_TX_ALLOC_STATE	(BASE_SDIO + 0x9a)		// 0

#define REG_SDIO_TX_INFORM_0	(BASE_SDIO + 0x9c)		// 0
#define REG_SDIO_TX_INFORM_1	(BASE_SDIO + 0x9d)		// 0
#define REG_SDIO_TX_INFORM_2	(BASE_SDIO + 0x9e)		// 0

#define REG_SDIO_CCCR06	        (BASE_SDIO + 0xc6)		// 0


#define TURISMO_SPI_VERIFICATION    0


#define USE_DYNAMIC_BUF 0
u32 gSpiZeroLenCount=0;
u32 gSpiZeroLenRetryCount=0;
u32 gSpiReadCount=0;
extern DeviceInfo_st *gDeviceInfo;
extern struct Host_cfg g_host_cfg;

static void _spi_host_isr(void);
static bool _ssvcabrio_spi_read_reg (u32 addr, u32 *data);
static bool _ssvcabrio_spi_write_reg (u32 addr, u32 data);
static bool _ssvcabrio_spi_init (void);
static u32 _ssvcabrio_write_fw_to_sram(u8* fw_bin, u32 fw_bin_len);
static bool _ssvcabrio_do_firmware_checksum(u32 origin);
static bool _check_rx_rdy(u32 *rx_len);
static s32 _status_read_turismo(u8 *value);
static s32 status_read_turismo(void);
static s32 status_write_turismo(u8 *spi_status);
static bool _ssvcabrio_spi_read_local_reg (u32 addr, u32 *data);
static bool _ssvcabrio_spi_write_local_reg (u32 addr, u32 data);
static bool _ssvcabrio_spi_positive_edge_mode(bool en);

#define CONFIG_POSITIVE_EDGE_MODE TRUE
bool g_ConfigPositiveEdgModeDone=FALSE;

#define READ_STATUS_THROUGH_AHB 0
#define LOCAL_REG_BURST_READ 1
#define CHECK_RX_IRQ_RDY_BIT 0

#define DYNAMIC_CHANGE_SPI_READ_DUMMY_BYTES 1
u8  last_hci_dummy_read_cycles=2; //Default value is 2
u32 hci_dummy_read_cycle_change_cnt=0; 

//u8  last_ahb_dummy_read_cycles=0; //Default value is 0
//u32 ahb_dummy_read_cycle_change_cnt=0; 

static void _spi_host_isr(void)
{
    TXRXTask_Isr(INT_RX,spi_is_truly_isr());
}

//for Turismo AHB Read Address and AHB Read Data
static bool _ssvcabrio_spi_read_reg (u32 addr, u32 *data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    u32 retry=0;
    bool status=TRUE;
    u8  cmd[9]; // 1B CMD + 4B ADDR + 4B DATA
    u8  temp[4]={0};
    u8  err=0;
    u8  dummy_cycles=0;


    if(g_ConfigPositiveEdgModeDone==FALSE)
    {
        _ssvcabrio_spi_positive_edge_mode(CONFIG_POSITIVE_EDGE_MODE);
        g_ConfigPositiveEdgModeDone=TRUE;
    }


    // Send read register address
    sizeToTransfer = 5;
    cmd[0] = SPI_READ_REG_ADDR_CMD;
    cmd[1]= (u8)(addr >> 24);
    cmd[2]= (u8)(addr >> 16);
    cmd[3]= (u8)(addr >> 8);
    cmd[4]= (u8)(addr);

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      //| SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    sizeToTransfer = 2;

    // Send read register data cmd
    cmd[0] = SPI_READ_REG_DATA_CMD;


    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
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

    if(0!=status_read_turismo()) goto ERROR;
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
ERROR:
    //SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return FALSE;
}

//for Turismo AHB Write Data
static bool _ssvcabrio_spi_write_reg (u32 addr, u32 data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    u32 retry=0;
    bool status=TRUE;
    u8  cmd[9]; // 1B CMD + 4B ADDR + 4B DATA

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    //BEGIN_SPI_TRANS();
    if(g_ConfigPositiveEdgModeDone==FALSE)
    {
        _ssvcabrio_spi_positive_edge_mode(CONFIG_POSITIVE_EDGE_MODE);
        g_ConfigPositiveEdgModeDone=TRUE;
    }

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

//for Turismo Register Read
static bool _ssvcabrio_spi_read_local_reg (u32 addr, u32 *data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    u32 retry=0;
    bool status=TRUE;
    u8  cmd[4]={0};
    u8  temp[4]={0};
    u8  i=0;

    //cmd[0]= SPI_READ_LOC_REG_CMD_F0;  //0xB0 for SDIO Function 0,0xC4h
    cmd[0]= SPI_READ_LOC_REG_CMD_F1|((addr>>15)&0x03);  //0xB2 for SDIO Function 1,0x04h
    //0xB2 for SDIO Function 1,0x04h. SDIO address[17:16] is included in the last two bits of the REG READ op code.
    //SDIO address[17:16] is included in the last two bits of the REG READ op code.

    //SDIO address[15:8]
    cmd[1]= (u8)(addr >> 8);
    //SDIO address[7:0]
    cmd[2]= (u8)addr;
    //dummy cycle for read
    cmd[3]= SPI_DUMMY_DATA;

    // Send read register op code, address and one dummy cycle
    sizeToTransfer = 4;

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);


    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    #if(LOCAL_REG_BURST_READ==1)
    // recieve 2 byte data
    sizeToTransfer = 2;
    #else
    // recieve 1 byte data
    sizeToTransfer = 1;
    #endif
    status = spi_host_read((u8 *)temp, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    *data = (  ((u32)temp[0]) | ((u32)temp[1] << 8)
            | ((u32)temp[2] << 16) | ((u32)temp[3] << 24));
#if TURISMO_SPI_VERIFICATION
    LOG_PRINTF("%s():*data=0x%x\r\n",__FUNCTION__,*data);
#endif
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
ERROR:
    SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return FALSE;
}


//for Turismo Register Write
static bool _ssvcabrio_spi_write_local_reg (u32 addr, u32 data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    u32 retry=0;
    bool status=TRUE;
    u8  cmd[4]; // 1B CMD + 2B ADDR + 1B DATA
#if TURISMO_SPI_VERIFICATION
    u32 i=0;
#endif
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    //BEGIN_SPI_TRANS();

    //0xB6 for SDIO Function 1,0x04h. SDIO address[17:16] is included in the last two bits of the REG READ op code.
    cmd[0] = (SPI_WRITE_LOC_REG_CMD_F1)|((addr>>15)&0x03);
    //SDIO address[15:8]
    cmd[1]= (u8)(addr >> 8);
    //SDIO address[7:0]
    cmd[2]= (u8)(addr);
    //data
    cmd[3]= (u8)(data);

#if TURISMO_SPI_VERIFICATION
    for( i=0; i<sizeof(cmd); i++ )
    {
        LOG_PRINTF("1. %s():cmd[%d]=0x%x\r\n",__FUNCTION__, i, cmd[i]);
    }
#endif

    sizeToTransfer = 4;
    sizeTransfered = 0;
    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
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


static bool _ssvcabrio_spi_positive_edge_mode(bool en)
{
	u8 cmd;
	u32 sizeToTransfer = 0;
	u32 sizeTransfered=0;

	if(TRUE==en)
		cmd = SPI_POSITIVE_EDGE_CMD;
	else
		cmd = SPI_NEGATIVE_EDGE_CMD;
	sizeToTransfer=1;
	spi_host_write(&cmd, sizeToTransfer, &sizeTransfered,
	                    SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
	                  | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
	                  | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
	                  | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
	                  );
	return TRUE;
}
static bool _ssvcabrio_spi_init (void)
{
    u32 data;
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

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
    //_ssvcabrio_spi_positive_edge_mode(TRUE);
    //_ssvcabrio_spi_positive_edge_mode(TRUE);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

static bool _check_rx_rdy(u32 *rx_len)
{

    u32 val = 0;

#if(READ_STATUS_THROUGH_AHB==1)

    #if(CHECK_RX_IRQ_RDY_BIT==1)
    if(FALSE==_ssvcabrio_spi_read_reg(0xc000080C,&val))  //ADR_FN1_STATUS_REG
        return FALSE;
        

    if(0==(val&0x01)) 
        return FALSE;
        
    #endif

    if(FALSE==_ssvcabrio_spi_read_reg(0xc0000810,&val)) 
        return FALSE;
        

    *rx_len=val&0xFFFF;
    if(*rx_len!=0)
        return TRUE;
    else
        return FALSE;    

#else //#if(READ_STATUS_THROUGH_AHB==1)

    #if(CHECK_RX_IRQ_RDY_BIT==1)
    if(FALSE==_ssvcabrio_spi_read_local_reg(REG_INT_STATUS,&val)) 
        return FALSE;

    if(0==(val&0x01)) 
        return FALSE;

    #endif
    
    #if(LOCAL_REG_BURST_READ==1)
        if(FALSE==_ssvcabrio_spi_read_local_reg(REG_CARD_PKT_LEN_0,&val)) return FALSE;

        *rx_len=((val>>0)&0xFF);
        *rx_len|=(((val>>8)&0xFF)<<8);	

        if(*rx_len!=0)
            return TRUE;
        else
            return FALSE;
    #else
    
        if(FALSE==_ssvcabrio_spi_read_local_reg(REG_CARD_PKT_LEN_0,&val)) return FALSE;;
            *rx_len=(val&0xFF);

        if(FALSE==_ssvcabrio_spi_read_local_reg(REG_CARD_PKT_LEN_1,&val)) return FALSE;
            *rx_len|=((val&0xFF)<<8);
        
        if(*rx_len!=0)
            return TRUE;
        else
            return FALSE;

    #endif  //#if(LOCAL_REG_BURST_READ==1)   

#endif  //#if(READ_STATUS_THROUGH_AHB==1)
}

u8* temp_wbuf;
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
    if(g_host_cfg.hci_aggr_tx)
    {
        OS_MemSET(temp_wbuf,0,MAX_HCI_AGGR_SIZE);
    }
    else
    {
        OS_MemSET(temp_wbuf,0,1600);
    }
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
    u8 status=0;
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

/*
	#define HCI_RX_AGGR    0
	#define HCI_AGGR_TX    0   
*/
	/* not in */
	if (g_host_cfg.hci_aggr_tx)
    {
        temp_wbuf = (u8*)OS_MemAlloc(MAX_HCI_AGGR_SIZE);
        if(!temp_wbuf)
            SDRV_ERROR("%s(): Malloc spi temp_wbuf FAIL\r\n",__FUNCTION__);
        OS_MemSET(temp_wbuf,0,MAX_HCI_AGGR_SIZE);
    }
    else
    {
        temp_wbuf = (u8*)OS_MemAlloc(1600);
        if (!temp_wbuf)
            SDRV_ERROR("%s(): Malloc spi temp_wbuf FAIL\r\n",__FUNCTION__);
        OS_MemSET(temp_wbuf,0,1600);
    }
    _ssvcabrio_spi_init();
    if(0==_status_read_turismo(&status))
    {
        LOG_PRINTF("\33[32mspi status = 0x%x\33[0m\r\n",status);
    }
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

static u32 ssvcabrio_read_reg(u32 addr)
{
#define MAX_RETRY 5
    u32 data;
    u8 i=0;
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

    for(i=0;i<MAX_RETRY;i++)
    {
        if(TRUE==_ssvcabrio_spi_read_reg(addr, &data))
        {
            break;
        }
    }

    if(i==5)
    {
        SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
        return 0;
    }
    else
    {
        SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
        return data;
    }
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
    g_ConfigPositiveEdgModeDone=FALSE;
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}

static bool ssvcabrio_close(void)
{
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return TRUE;
}





//status_read_turismo
//s32 status_read_turismo(u8 *data, size_t len)
static s32 _status_read_turismo(u8 *value)
{
    u32 sizeToTransfer = 0, r_length=0, w_length=0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[2]={0};
    u8  temp[1]={0};

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);


    // Send read register address
    w_length = 2;
    r_length = 1;

    cmd[0] = SPI_READ_STS_CMD;  // 0xAC
    cmd[1] = 0;                 // dummy bytes
    sizeToTransfer = w_length;

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    //read data
    sizeToTransfer = r_length; // 1
    status = spi_host_read((u8 *)temp, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    *value=temp[0];
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return 0;
ERROR:
    //SDRV_ERROR("%s(): fail\r\n",__FUNCTION__);
    return -1;
}

static s32 status_read_turismo(void)
{
#if 1
    u8 temp=0;
    if(-1==_status_read_turismo(&temp))
    {
        goto ERROR;
    }

     if( (temp & (0x1<<2) ) != 0 )
     {
         //rdata fail
         //LOG_PRINTF("\33[31m %s():%d rdata fail \33[0m\r\n",__func__,__LINE__);
         goto ERROR;
     }
     else if( (temp & (0x1<<3) ) != 0 )
     {
         //fail cmd
         //LOG_PRINTF("\33[31m %s():%d fail cmd \33[0m\r\n",__func__,__LINE__);
         goto ERROR;
     }

    return 0;
ERROR:
    //SDRV_ERROR("%s(): fail\r\n",__FUNCTION__);
    return -1;    
#else
    u32 sizeToTransfer = 0, r_length=0, w_length=0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[2]={0};
    u8  temp[1]={0};

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);


    // Send read register address
    w_length = 2;
    r_length = 1;

    cmd[0] = SPI_READ_STS_CMD;  // 0xAC
    sizeToTransfer = w_length;

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    //read data
    sizeToTransfer = r_length; // 1
    status = spi_host_read((u8 *)temp, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    //LOG_PRINTF("status_read_turismo=0x%x\r\n",*temp);

    //analysis *temp
     if( ( *temp & (0x1<<2) ) != 0 )
     {
         //rdata fail
         //LOG_PRINTF("\33[31m %s():%d rdata fail \33[0m\r\n",__func__,__LINE__);
         goto ERROR;
     }
     else if( ( *temp & (0x1<<3) ) != 0 )
     {
         //fail cmd
         //LOG_PRINTF("\33[31m %s():%d fail cmd \33[0m\r\n",__func__,__LINE__);
         goto ERROR;
     }

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return 0;
ERROR:
    //SDRV_ERROR("%s(): fail\r\n",__FUNCTION__);
    return -1;
#endif	
}

//status_write_turismo
static s32 status_write_turismo(u8 *spi_status)
{
    u32 sizeToTransfer = 0, r_length=0, w_length=0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[2]={0};

    w_length = 2;
    r_length = 0;
    {
        //LOG_PRINTF("%s():*spi_status=0x%x\r\n", __FUNCTION__, *spi_status);
        cmd[0] = SPI_WRITE_STS_CMD;  // 0xAE
        cmd[1] = *spi_status;
        sizeToTransfer = w_length;
#if TURISMO_SPI_VERIFICATION
        LOG_PRINTF("%s():cmd[1]=0x%x\r\n", __FUNCTION__, cmd[1]);
#endif
        status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                          | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                          );
        if(FALSE==status) goto ERROR;
    }
    return 0;
ERROR:
    SDRV_ERROR("%s(): fail\r\n",__FUNCTION__);
    return -1;
}

//turismo_spi_ahb_radd
static s32 turismo_spi_ahb_radd(u8 *address)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[1]={0};
    u32 i = 0;

    sizeToTransfer = 1;

    cmd[0] = SPI_READ_REG_ADDR_CMD;  //0xA8

#if TURISMO_SPI_VERIFICATION
            LOG_PRINTF("%s():cmd[%d]=%d \r\n",__FUNCTION__, i, cmd[i]);
#endif

    {
        sizeToTransfer = 1;
        status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                          | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                          );
        if(FALSE==status) goto ERROR;
    }

    {
        sizeToTransfer = 4;
        status = spi_host_write((u8 *)address, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
        if(FALSE==status) goto ERROR;
    }

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return 0;

ERROR:
    SDRV_ERROR("%s():FAIL\r\n",__FUNCTION__);
    return -1;
}

//turismo_spi_ahb_rdata
static s32 turismo_spi_ahb_rdata(u8 *data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[1]={0};
    u32 i = 0;

    sizeToTransfer = 1;

    cmd[0] = SPI_READ_REG_DATA_CMD;  //0xA9

#if TURISMO_SPI_VERIFICATION
            LOG_PRINTF("%s():cmd[%d]=%d \r\n",__FUNCTION__, i, cmd[i]);
#endif

    {
        sizeToTransfer = 2;
        status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                          | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                          );
        if(FALSE==status) goto ERROR;
    }

    {
        sizeToTransfer = 4;
        status = spi_host_write(data, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
        if(FALSE==status) goto ERROR;
    }
#if TURISMO_SPI_VERIFICATION
    LOG_PRINTF("%s():*(data+0)=0x%x \r\n",__FUNCTION__, *(data+0) );
    LOG_PRINTF("%s():*(data+1)=0x%x \r\n",__FUNCTION__, *(data+1) );
    LOG_PRINTF("%s():*(data+2)=0x%x \r\n",__FUNCTION__, *(data+2) );
    LOG_PRINTF("%s():*(data+3)=0x%x \r\n",__FUNCTION__, *(data+3) );
#endif
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return 0;

ERROR:
    SDRV_ERROR("%s():FAIL\r\n",__FUNCTION__);
    return -1;
}



//turismo_spi_ahbw
static s32 turismo_spi_ahbw(u8 *address, u8 *data)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[1]={0};
    u32 i = 0;

    sizeToTransfer = 1;

    cmd[0] = SPI_WRITE_REG_CMD;  //0xAA

#if TURISMO_SPI_VERIFICATION
        LOG_PRINTF("1.%s():cmd[0]=%d \r\n",__FUNCTION__, cmd[0]);
        LOG_PRINTF("1.%s():*(address+0)=0x%x \r\n",__FUNCTION__, *(address+0));
        LOG_PRINTF("1.%s():*(address+1)=0x%x \r\n",__FUNCTION__, *(address+1));
        LOG_PRINTF("1.%s():*(address+2)=0x%x \r\n",__FUNCTION__, *(address+2));
        LOG_PRINTF("1.%s():*(address+3)=0x%x \r\n",__FUNCTION__, *(address+3));
        LOG_PRINTF("1.%s():*(data+0)=0x%x \r\n",__FUNCTION__, *(data+0));
        LOG_PRINTF("1.%s():*(data+1)=0x%x \r\n",__FUNCTION__, *(data+1));
        LOG_PRINTF("1.%s():*(data+2)=0x%x \r\n",__FUNCTION__, *(data+2));
        LOG_PRINTF("1.%s():*(data+3)=0x%x \r\n",__FUNCTION__, *(data+3));
#endif

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );

    if(FALSE==status) goto ERROR;

    {
        sizeToTransfer = 4;
        status = spi_host_write(address, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
        if(FALSE==status) goto ERROR;
    }

    {
        sizeToTransfer = 4;
        status = spi_host_write(data, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                        | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
        if(FALSE==status) goto ERROR;
    }
#if TURISMO_SPI_VERIFICATION
        LOG_PRINTF("2.%s():cmd[0]=0x%x \r\n",__FUNCTION__, cmd[0]);
        LOG_PRINTF("2.%s():*(address+0)=0x%x \r\n",__FUNCTION__, *(address+0));
        LOG_PRINTF("2.%s():*(address+1)=0x%x \r\n",__FUNCTION__, *(address+1));
        LOG_PRINTF("2.%s():*(address+2)=0x%x \r\n",__FUNCTION__, *(address+2));
        LOG_PRINTF("2.%s():*(address+3)=0x%x \r\n",__FUNCTION__, *(address+3));
        LOG_PRINTF("2.%s():*(data+0)=0x%x \r\n",__FUNCTION__, *(data+0));
        LOG_PRINTF("2.%s():*(data+1)=0x%x \r\n",__FUNCTION__, *(data+1));
        LOG_PRINTF("2.%s():*(data+2)=0x%x \r\n",__FUNCTION__, *(data+2));
        LOG_PRINTF("2.%s():*(data+3)=0x%x \r\n",__FUNCTION__, *(data+3));
#endif

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return 0;

ERROR:
    SDRV_ERROR("%s():FAIL\r\n",__FUNCTION__);
    return -1;
}

//for Turismo HCI read
static s32 ssvcabrio_recv(u8 *data, size_t len)
{
    u32 sizeToTransfer = 0;
#if TURISMO_SPI_VERIFICATION
    u32 i = 0;
    u32 *r_lengthSDIO;
#endif
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[5]={0};
    u8  dummy[4]={0};
    u8  align_4_len;
    u32 rlen=0;
    u8  retry=2;
    u8  dummy_cycles=0;
    u8  *pData=NULL, *pTemp=NULL;
    if(g_ConfigPositiveEdgModeDone==FALSE)
    {
        _ssvcabrio_spi_positive_edge_mode(CONFIG_POSITIVE_EDGE_MODE);
        g_ConfigPositiveEdgModeDone=TRUE;
    }

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    if(data==NULL)
    {
        SDRV_ERROR("%s(): data is a null pointer\r\n",__FUNCTION__);
        return -1;
    }

    do{
        if(FALSE==_check_rx_rdy(&rlen))
        {
            goto NOT_READY;
        }
        retry--;
    }while((rlen==0)&&(retry!=0));

    if(retry==0) gSpiZeroLenRetryCount++;

    SDRV_DEBUG("%s() :rlen=%d(0x%x) \r\n",__FUNCTION__,rlen,rlen);
    if(rlen==0){
        gSpiZeroLenCount++;
        goto ERROR;
    }
    gSpiReadCount++;
    
    //rlen=(rlen<len)?rlen:len;
    
    //align_4_len
    align_4_len=(rlen%4)?(4-(rlen%4)):0;
    SDRV_DEBUG("%s() :align_4_len=%d(0x%x) \r\n",__FUNCTION__,align_4_len,align_4_len);

    #if(DYNAMIC_CHANGE_SPI_READ_DUMMY_BYTES==1)
    status = _status_read_turismo(&dummy_cycles);
    if(0!=status) goto ERROR;

    #if(CONFIG_POSITIVE_EDGE_MODE==TRUE)
    if(0x82!=dummy_cycles)
    #else
    if(0x02!=dummy_cycles)
    #endif
    {
        LOG_PRINTF("\33[31mspi local status is changed:0x%x\33[0m\r\n",dummy_cycles);        
    }

    
    dummy_cycles=(dummy_cycles&0x03);
    sizeToTransfer=dummy_cycles+1; //The dummy read is n+1. ex: n=2, we need to read three dummy bytes  
    sizeToTransfer+=1; // This +1 is for SPI_RX_DATA_CMD
    if(last_hci_dummy_read_cycles!=dummy_cycles)
    {
        hci_dummy_read_cycle_change_cnt++;
        LOG_PRINTF("\33[31mspi hci dummy cycles: last(0x%x), now(0x%x),cnt(0x%x) \33[0m\r\n",last_hci_dummy_read_cycles,dummy_cycles,hci_dummy_read_cycle_change_cnt);
        last_hci_dummy_read_cycles=dummy_cycles;
    }

    #else
    sizeToTransfer = 4;
    #endif //#if(DYNAMIC_CHANGE_SPI_READ_DUMMY_BYTES==1)

    
    // Send read register address
    cmd[0] = SPI_RX_DATA_CMD;

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );
    if(FALSE==status) goto ERROR;

    if((rlen>len)&&(!g_host_cfg.hci_rx_aggr))
    {
        LOG_PRINTF("\33[31mspi rlen(%d) is bigger than len(%d)\33[0m\r\n",rlen,len); 
        pData=OS_MemAlloc(rlen);
        if(pData==NULL) goto ERROR;
        pTemp=pData;
    }
    else
    {
        pTemp=data;
    }

    if(align_4_len!=0)
    {
        //LOG_PRINTF("spi read length is not 4 bytes alignment \r\n");
        status = spi_host_read((u8 *)pTemp, rlen, &sizeTransfered,
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
        status = spi_host_read((u8 *)pTemp, rlen, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                        | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                      );

        if(FALSE==status) goto ERROR;
    }

    if((rlen>len)&&(!g_host_cfg.hci_rx_aggr))
    {
        OS_MemCPY(data,pData,len);
        OS_MemFree(pData);
        pData=NULL;
        SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
        return len;
    }
    else
    {
        SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
        return rlen;        
    }
ERROR:    
    if(pData!=NULL)
    {
        OS_MemFree(pData);
        pData=NULL;        
    }
    SDRV_ERROR("%s(): fail\r\n",__FUNCTION__);
    return -1;
NOT_READY:
    SDRV_DEBUG("%s(): NOT_READY\r\n",__FUNCTION__);
    return -1;
}


//for Turismo HCI write
static s32 ssvcabrio_send(void *data, size_t len)
{
    u32 sizeToTransfer = 0;
    u32 sizeTransfered=0;
    bool status=TRUE;
    u8  cmd[1]={0};
    u8  dummy[4]={0};
    u8  align_4_len;
#if TURISMO_SPI_VERIFICATION
    u32 i = 0;
#endif
    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    //alignment 4 bytes
    align_4_len=(len%4)?(4-(len%4)):0;
    SDRV_DEBUG("%s() :align_4_len=%d(0x%x) \r\n",__FUNCTION__,align_4_len,align_4_len);

    if(g_ConfigPositiveEdgModeDone==FALSE)
    {
        _ssvcabrio_spi_positive_edge_mode(CONFIG_POSITIVE_EDGE_MODE);
        g_ConfigPositiveEdgModeDone=TRUE;
    }

    // Send read register address
    sizeToTransfer = 1;

    cmd[0] = SPI_TX_DATA_CMD;  //0xA6

    status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                      );

    if(FALSE==status) goto ERROR;

    //write data
    if(align_4_len!=0)
    {
        //LOG_PRINTF("spi write length is not 4 bytes alignment \r\n");
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
#if TURISMO_SPI_VERIFICATION
    for(i=0;i<len;i++)
    {
        LOG_PRINTF("%s():*(data+%d)=%d \r\n",__FUNCTION__, i, *( (u8 *) data+i));
    }
    LOG_PRINTF("%s():len=%d \r\n",__FUNCTION__, len);
#endif
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
    sizeToTransfer = 1;
    if(sw)
    {
        // Send SPI special pattern to wakeup PMU
        cmd[0] = SPI_DATA_WAKEUP;
        cmd[1] = 0;
        cmd[2] = 0;
        cmd[3] = 0;

        status = spi_host_write(cmd, sizeToTransfer, &sizeTransfered,
                    SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                  | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                  | SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE
                  | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE
                  );

    }
    else
    {
        // Send SPI special pattern to clear previous wakeup signal.
        u32 val=0;
        _ssvcabrio_spi_read_reg(0xc0000100,&val);
        val|=0x01;// write 1 clear
        _ssvcabrio_spi_write_reg(0xc0000100,0x01);

    }
    

    return  status;
}

static bool ssvcabrio_ioctl(u32 ctl_code,
                            void *in_buf, size_t in_size,
                            void *out_buf, size_t out_size,
                            size_t *bytes_ret)
{
    switch(ctl_code)
    {
        case SPI2P0_LOCAL_REG_WRITE:
        {
            struct spi2p0_local_reg *p=(struct spi2p0_local_reg *)in_buf;
            _ssvcabrio_spi_write_local_reg(p->addr,p->data);
            break;
        }
        case SPI2P0_LOCAL_REG_READ:
        {
            struct spi2p0_local_reg *p=(struct spi2p0_local_reg *)in_buf;
            _ssvcabrio_spi_read_local_reg(p->addr,&(p->data));
            break;
        }
        case SPI2P0_STATUS_WRITE:
        {
            status_write_turismo((u8 *)in_buf);
            break;
        }
        case SPI2P0_STATUS_READ:
        {
            status_read_turismo();
            break;
        }
    }

    return TRUE;
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
    ssvcabrio_ioctl, //ioctl
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
#endif //#if((CONFIG_CHIP_ID==SSV6006B)||(CONFIG_CHIP_ID==SSV6006C))


