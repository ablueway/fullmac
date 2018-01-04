/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


/******************************************************************************/
/* Include files */
/******************************************************************************/
/* Standard C libraries */
#include<stdio.h>
#include<stdlib.h>
/* OS specific libraries */
#if (_WIN32 == 1)
#include<windows.h>
#endif
/* Include libMPSSE header */
#include "libMPSSE_spi.h"
#include "ftdi_spi.h"
#include "SPI_defs.h"
#include <log.h>

/******************************************************************************/
/* Macro and type defines */
/******************************************************************************/
#define CHECK_SPI_MUTEX

#define SPI_WRITE_REG_CMD       (0x07)
#define SPI_READ_REG_CMD        (0x05)
#define SPI_READ_STS_CMD        (0x09)
#define SPI_READ_REG_DATA_CMD   (0x2C)
#define SPI_TX_DATA_CMD         (0x01)
#define SPI_RX_DATA_CMD         (0x0C)

#define SPI_TX_BLOCK_SHIFT      (4)
#define SPI_TX_BLOCK_SIZE       (1 << SPI_TX_BLOCK_SHIFT)

/* Helper macros */
#define APP_CHECK_STATUS(exp) \
    do { \
        if (exp!=FT_OK) \
        { \
            LOG_PRINTF("%s:%d:%s(): status(0x%x) != FT_OK\n", \
                       __FILE__, __LINE__, __FUNCTION__,exp); \
            /* exit(1); */ \
        } \
    } while (0)

#define CHECK_NULL(exp) \
    do { \
        if (exp==NULL) \
        { \
            LOG_PRINTF("%s:%d:%s(): NULL expression encountered \n", \
                       __FILE__, __LINE__, __FUNCTION__); \
            /* exit(1); */ \
        } \
    } while (0)


#ifdef CHECK_SPI_MUTEX
static u32 cur_mutex_owner = 0;

#define BEGIN_SPI_TRANS()   \
	do { \
		if (cur_mutex_owner) LOG_PRINTF("XX %d - %d XX\n", cur_mutex_owner, __LINE__); \
        OS_MutexLock(s_SPIMutex); \
		cur_mutex_owner = __LINE__; \
	} while (0)

#define END_SPI_TRANS()     \
	do { \
		cur_mutex_owner = 0; \
		OS_MutexUnLock(s_SPIMutex); \
	} while (0)
#else
#define BEGIN_SPI_TRANS()   \
	do { \
        OS_MutexLock(s_SPIMutex); \
	} while (0)


#define END_SPI_TRANS()     \
	do { \
		OS_MutexUnLock(s_SPIMutex); \
	} while (0)
#endif

/* Application specific macro definations */
#define SPI_DEVICE_BUFFER_SIZE 256

#define SPI_WRITE_COMPLETION_RETRY 10
#define START_ADDRESS_EEPROM 0x00 /*read/write start address inside the EEPROM*/
#define END_ADDRESS_EEPROM 0x10
#define RETRY_COUNT_EEPROM 10 /* number of retries if read/write fails */
#define CHANNEL_TO_OPEN 0 /*0 for first available channel, 1 for next... */
#define SPI_SLAVE_0 0
#define SPI_SLAVE_1 1
#define SPI_SLAVE_2 2
#define DATA_OFFSET 3
/******************************************************************************/
/* Global variables */
/******************************************************************************/
static OsMutex  s_SPIMutex;

uint32 channels;
FT_HANDLE ftHandle = NULL;
ChannelConfig channelConf;
uint8 buffer[SPI_DEVICE_BUFFER_SIZE];

u32	ftdi_spi_get_bus_clock (void)
{
    return 0;
} // end of - ftdi_spi_get_bus_clock -

bool ftdi_spi_set_bus_clock (u32 clock_rate)
{
    return true;
} // end of - ftdi_spi_set_bus_clock -


bool ftdi_spi_ack_int (void)
{
    return true;
} // end of - ftdi_spi_ack_int


bool ftdi_spi_get_driver_version (u16 *version)
{
    return true;
} // end of - ftdi_spi_get_driver_version -

static u32 _ftdi_spi_read_status (void)
{
    uint32 sizeToTransfer;
    uint32 sizeTransfered;
    FT_STATUS status;
    u8     cmd[5];
    u32    data32[2];
    u8    *data = (u8 *)&data32[0];

    // Send read register address
    sizeToTransfer = 1;
    data += 4;
    cmd[0] = SPI_READ_STS_CMD;
    status = SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered,
                             SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                           | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	sizeToTransfer = 4;
    status = SPI_Read(ftHandle, data, sizeToTransfer, &sizeTransfered,
                             SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                           | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                           | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);

    return data32[1];
}


static void _SPI_send_cmd (u32 cmd, u32 addr)
{
    uint32 sizeToTransfer;
    uint32 sizeTransfered;
    FT_STATUS status;
    
    sizeToTransfer=2;
    sizeTransfered=0;
    buffer[0] = cmd;

    status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
                         SPI_TRANSFER_OPTIONS_SIZE_IN_BITS
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    APP_CHECK_STATUS(status);
    
    sizeToTransfer=4;
    sizeTransfered=0;
    buffer[0]= (u8)(addr >> 24);
    buffer[1]= (u8)(addr >> 16);
    buffer[2]= (u8)(addr >> 8);
    buffer[3]= (u8)(addr);
            
    status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
                         SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);
}

    
bool ftdi_spi_read_sram (u32 addr, u8 *data, u32 size)
{
    uint32 sizeToTransfer = 0;
    uint32 sizeTransfered=0;
    bool writeComplete=0;
    uint32 retry=0;
    FT_STATUS status;
    
    // Enable burst mode for SRAM access
	ftdi_spi_write_reg(ADR_DEBUG_BURST_MODE, 1);

    BEGIN_SPI_TRANS();
    _SPI_send_cmd(0x80, addr);
    
    /*
        buffer[0]= (u8)(data >> 24);
        buffer[1]= (u8)(data >> 16);
        buffer[2]= (u8)(data >> 8);
        buffer[3]= (u8)(data);
    */    
    sizeToTransfer = size; // Must be multiple of 4-byte
    sizeTransfered = 0;
    status = SPI_Read(ftHandle, data, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);
    END_SPI_TRANS();

    // Disable burst mode for SRAM access
	ftdi_spi_write_reg(ADR_DEBUG_BURST_MODE, 0);

    return true;
} // end of - ftdi_spi_read_sram -


bool ftdi_spi_write_sram (u32 addr, const u8 *data, u32 size)
{
    uint32 sizeToTransfer = 0;
    uint32 sizeTransfered=0;
    bool writeComplete=0;
    uint32 retry=0;
    FT_STATUS status;

	// Enable burst mode for SRAM access
	ftdi_spi_write_reg(ADR_DEBUG_BURST_MODE, 1);

    BEGIN_SPI_TRANS();
    _SPI_send_cmd(0xC0, addr);

	  sizeToTransfer = size;
    sizeTransfered = 0;
    status = SPI_Write(ftHandle, (uint8 *)data, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    APP_CHECK_STATUS(status);
    END_SPI_TRANS();

	// Disable burst mode for SRAM access
	ftdi_spi_write_reg(ADR_DEBUG_BURST_MODE, 0);

    return true;
} // end of - ftdi_spi_read_sram -


bool	ftdi_spi_read_reg (u32 addr, u32 *data)
{
    uint32 sizeToTransfer = 0;
    uint32 sizeTransfered=0;
    bool writeComplete=0;
    uint32 retry=0;
    FT_STATUS status;
    u8     cmd[5];
    u8     data[4];

    BEGIN_SPI_TRANS();
    // Send read register address
    sizeToTransfer = 5;
    cmd[0] = SPI_READ_REG_CMD;
    cmd[1]= (u8)(addr >> 24);
    cmd[2]= (u8)(addr >> 16);
    cmd[3]= (u8)(addr >> 8);
    cmd[4]= (u8)(addr);

    status = SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    // Check data ready
    do {
        u32 status32 = _ftdi_spi_read_status();
        u8  *status = (u8 *)&status32;
        if (status[0] & 0x80) // data ready?
            break;
		if (++retry < 50)
			continue;
		END_SPI_TRANS();
		SPI_FAIL_RET(0, "SPI reading register time out.\n");
    } while (1);
    // Read back register value
    cmd[0] = SPI_READ_REG_DATA_CMD;

	status = SPI_Write(ftHandle, cmd, 1, &sizeTransfered,
                         SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
	status = SPI_Read(ftHandle, data, 4, &sizeTransfered,
                        SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                      | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
					  | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    END_SPI_TRANS();
    APP_CHECK_STATUS(status);

    *data = (  ((u32)data[0]) | ((u32)data[1] << 8) 
            | ((u32)data[2] << 16) | ((u32)data[3] << 24));

    return TRUE;
} // end of - ftdi_spi_read_reg -


bool ftdi_spi_write_reg (u32 addr, u32 data)
{
    uint32 sizeToTransfer = 0;
    uint32 sizeTransfered=0;
    bool writeComplete=0;
    uint32 retry=0;
    FT_STATUS status;
    u8      cmd[9]; // 1B CMD + 4B ADDR + 4B DATA
    
    BEGIN_SPI_TRANS();
    
    cmd[0] = SPI_WRITE_REG_CMD;
    cmd[1]= (u8)(addr >> 24);
    cmd[2]= (u8)(addr >> 16);
    cmd[3]= (u8)(addr >> 8);
    cmd[4]= (u8)(addr);
    cmd[5]= (u8)(data);
    cmd[6]= (u8)(data >> 8);
    cmd[7]= (u8)(data >> 16);
    cmd[8]= (u8)(data >> 24); // MSB last
    sizeToTransfer = 9;
    sizeTransfered = 0;
    status = SPI_Write(ftHandle, cmd, sizeToTransfer, &sizeTransfered,
                         SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    END_SPI_TRANS();
    APP_CHECK_STATUS(status);
    
    return true;
} // end of - ftdi_spi_write_reg


s32	ftdi_spi_write_data (void *dat, size_t size)
{
    uint32      sizeToTransfer = 0;
    uint32      sizeTransfered=0;
    uint32      retry=0;
    u8          cmd;
    u32         retry_count = 0;
    u32         block_count;

    // Check HCI ready for new TX.
    retry_count = 0;
    do {
        u32  status32 = ftdi_spi_read_status();
        u8  *status = (u8 *)&status32;
        if ((status[1] & 0x01) == 0)
            break;
        if(++retry_count == 50) 
        {
            SPI_FAIL_RET(0, "SPI precheck exceed limit.\n", retry_count);
        }
    } while (1);

    // Issue new TX request
    block_count = (size + SPI_TX_BLOCK_SIZE - 1) >> SPI_TX_BLOCK_SHIFT;
	if (block_count < 4) block_count = 4;
    ftdi_spi_write_reg(0xc0000a28, block_count);
    do {
        u32  status32 = ftdi_spi_read_status();
        u8  *status = (u8 *)&status32;
        if ((status[1] & 0x01) == 0x01)
            break;
        if(status[0] & 0x40)
            SPI_FAIL_RET(0, "HCI TX NO ALLOC\n");
        if(status[0] & 0x20)
            SPI_FAIL_RET(0, "HCI TX DULPICATE ALLOC\n");
        if (++retry_count == 100) 
            SPI_FAIL_RET(0, "SPI TX allocation polling exceed limit.\n");
    } while (1);

    sizeToTransfer = 1;
    cmd = SPI_TX_DATA_CMD;

    BEGIN_SPI_TRANS();
    SPI_Write(ftHandle, &cmd, sizeToTransfer, &sizeTransfered,
                SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
              | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    // sizeToTransfer = (size + 3) & ~3;
	sizeToTransfer = block_count << SPI_TX_BLOCK_SHIFT;
    SPI_Write(ftHandle, dat, sizeToTransfer, &sizeTransfered,
                SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
              | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
              | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    END_SPI_TRANS();

    return size;
} // end of - ftdi_spi_write_data -


// return
//  < 0 : fail
// >= 0 : # of bytes recieved
s32	ftdi_spi_read_data (u8 *dat, size_t size)
{
    uint32 sizeToTransfer = 1;
    uint32 sizeTransfered = 0;
    FT_STATUS status;
    u8  cmd = SPI_RX_DATA_CMD;
    uint32 residual_size = size % 4;

    BEGIN_SPI_TRANS();
    // Send RX command
    status = SPI_Write(ftHandle, &cmd, sizeToTransfer, &sizeTransfered,
                         SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                       | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    // Read back RX data.
    // size that is not 4-byte aligned must handled separately.
    if (residual_size > 0)
    {
        u8  data[4];
        u32 i;
        sizeToTransfer = size - residual_size;
        status = SPI_Read(ftHandle, dat, sizeToTransfer, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
        status = SPI_Read(ftHandle, data, 4, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
        for (i = 0; i < residual_size; i++) 
           dat[sizeToTransfer + i] = data[i];
    } 
    else 
    {
        sizeToTransfer = size;
        status = SPI_Read(ftHandle, dat, sizeToTransfer, &sizeTransfered,
                            SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE
                          | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    }
    END_SPI_TRANS();

	return size;
} // end of - ftdi_spi_read_data -


s32 ftdi_spi_ask_rx_data_len (void)
{
    u32 status32 = ftdi_spi_read_status();
    u8  *status = (u8 *)&status32;

	if (status[0] & 0x01) // RX ready
		return (s32)((u32)status[3] << 8 | (u32)status[2]);
	return 0;
} // end of - ftdi_spi_ask_rx_data_len -


bool ftdi_spi_open (void)
{
    FT_STATUS status;
    FT_DEVICE_LIST_INFO_NODE devList;
    uint8 address=0;
    uint32 i;
#ifdef _MSC_VER
    {
    HINSTANCE hinstLib = LoadLibrary(TEXT("ftd2xx.dll")); 
    if (hinstLib == NULL)
        return false;
    Init_libMPSSE();
    }
#endif
    
    channelConf.ClockRate = 15000000;
    channelConf.LatencyTimer = 10;
    channelConf.configOptions =   SPI_CONFIG_OPTION_MODE0
                                | SPI_CONFIG_OPTION_CS_DBUS3
                                | SPI_CONFIG_OPTION_CS_ACTIVELOW;
    channelConf.Pin = 0x00000000;/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/
    status = SPI_GetNumChannels(&channels);
    if (status != FT_OK)
        return false;

    LOG_PRINTF("Number of available SPI channels = %d\n",channels);
    if (channels>0)
    {
        for(i=0;i<channels;i++)
        {
            status = SPI_GetChannelInfo(i,&devList);
            APP_CHECK_STATUS(status);
            LOG_PRINTF("Information on channel number %d:\n",i);
            /* print the dev info */
            LOG_PRINTF(" Flags=0x%x\n",devList.Flags);
            LOG_PRINTF(" Type=0x%x\n",devList.Type);
            LOG_PRINTF(" ID=0x%x\n",devList.ID);
            LOG_PRINTF(" LocId=0x%x\n",devList.LocId);
            LOG_PRINTF(" SerialNumber=%s\n",devList.SerialNumber);
            LOG_PRINTF(" Description=%s\n",devList.Description);
            LOG_PRINTF(" ftHandle=0x%x\n",devList.ftHandle);/*is 0 unless open*/
        }
        /* Open the first available channel */
        status = SPI_OpenChannel(CHANNEL_TO_OPEN,&ftHandle);
        APP_CHECK_STATUS(status);
        LOG_PRINTF("\nhandle=0x%x status=0x%x\n",ftHandle,status);
        status = SPI_InitChannel(ftHandle,&channelConf);
        APP_CHECK_STATUS(status);
    } 

    return (channels > 0);
}// end of - ftdi_spi_open -


bool ftdi_spi_close (void)
{
    if (ftHandle == NULL)
        return true;
    
    SPI_CloseChannel(ftHandle);
    #ifdef _MSC_VER
    Cleanup_libMPSSE();
    #endif
    return true;
} // end of - ftdi_spi_close -


bool ftdi_spi_init (void)
{
    OS_MutexInit(&s_SPIMutex);
	// Read status register to ensure CS is up for 1st meaningful command.
	ftdi_spi_read_status();
    // Switch HCI to SPI
    ftdi_spi_write_reg(0xc0000a0c, 0); 
    // bit 8 to enable TX. Set block size.
    ftdi_spi_write_reg(0xc0000a24, 0x100 | (SPI_TX_BLOCK_SHIFT));
    return true;
} // end of - ftdi_spi_init -

// cheat func
u32	ftdi_spi_handle (void)
{
    return (u32)ftHandle;
} // end of - ftdi_spi_handle -

 u32 ftdi_spi_read_status (void)
 {
	 u32	status32;
	 BEGIN_SPI_TRANS();
	 status32 = _ftdi_spi_read_status();
	 END_SPI_TRANS();
	 return status32;
 } // end of - _ftdi_spi_read_status -

#if 0
/*!
* \file sample-static.c
*
* \author FTDI
* \date 20110512
*
* Copyright c 2011 Future Technology Devices International Limited
* Company Confidential
*
* Project: libMPSSE
* Module: SPI Sample Application - Interfacing 94LC56B SPI EEPROM
*
* Rivision History:
* 0.1 - 20110512 - Initial version
* 0.2 - 20110801 - Changed LatencyTimer to 255
* Attempt to open channel only if available
* Added & modified macros
* Included stdlib.h
* 0.3 - 20111212 - Added comments
*/
/******************************************************************************/
/* Public function definitions */
/******************************************************************************/
/*!
* \brief Writes to EEPROM
*
* This function writes a byte to a specified address within the 93LC56B EEPROM
*
* \param[in] slaveAddress Address of the I2C slave (EEPROM)
* \param[in] registerAddress Address of the memory location inside the slave to where the byte
* is to be written
* \param[in] data The byte that is to be written
* \return Returns status code of type FT_STATUS(see D2XX Programmer's Guide)
* \sa Datasheet of 93LC56B http://ww1.microchip.com/downloads/en/DeviceDoc/21794F.pdf
* \note
* \warning
*/
FT_STATUS read_byte(uint8 slaveAddress, uint8 address, uint16 *data)
{
uint32 sizeToTransfer = 0;
uint32 sizeTransfered;
bool writeComplete=0;
uint32 retry=0;
FT_STATUS

SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|
SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
APP_CHECK_STATUS(status);
*data = (uint16)(buffer[1]<<8);
*data = (*data & 0xFF00) | (0x00FF & (uint16)buffer[0]);
return status;
}
/*!
* \brief Reads from EEPROM
*
* This function reads a byte from a specified address within the 93LC56B EEPROM
*
* \param[in] slaveAddress Address of the I2C slave (EEPROM)
* \param[in] registerAddress Address of the memory location inside the slave from where the
* byte is to be read
* \param[in] *data Address to where the byte is to be read
* \return Returns status code of type FT_STATUS(see D2XX Programmer's Guide)
* \sa Datasheet of 93LC56B http://ww1.microchip.com/downloads/en/DeviceDoc/21794F.pdf
* \note
* \warning
*/
FT_STATUS write_byte(uint8 slaveAddress, uint8 address, uint16 data)
{
uint32 sizeToTransfer = 0;
uint32 sizeTransfered=0;
bool writeComplete=0;
uint32 retry=0;
bool state;
FT_STATUS status;
/* Write command EWEN(with CS_High -> CS_Low) */
sizeToTransfer=11;
sizeTransfered=0;
buffer[0]=0x9F;/* SPI_EWEN -> binary 10011xxxxxx (11bits) */
buffer[1]=0xFF;
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
SPI_TRANSFER_OPTIONS_SIZE_IN_BITS|
SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE|
SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
APP_CHECK_STATUS(status);
/* CS_High + Write command + Address */
sizeToTransfer=1;
sizeTransfered=0;
buffer[0] = 0xA0;/* Write command (3bits) */
buffer[0] = buffer[0] | ( ( address >> 3) & 0x0F );/*5 most significant add bits*/
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|
SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
APP_CHECK_STATUS(status);
/*Write 3 least sig address bits */
sizeToTransfer=3;
sizeTransfered=0;
buffer[0] = ( address & 0x07 ) << 5; /* least significant 3 address bits */
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
SPI_TRANSFER_OPTIONS_SIZE_IN_BITS);
APP_CHECK_STATUS(status);
/* Write 2 byte data + CS_Low */
sizeToTransfer=2;
sizeTransfered=0;
buffer[0] = (uint8)(data & 0xFF);
buffer[1] = (uint8)((data & 0xFF00)>>8);
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,

SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES|
SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
APP_CHECK_STATUS(status);
/* Wait until D0 is high */
#if 1
/* Strobe Chip Select */
sizeToTransfer=0;
sizeTransfered=0;
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
SPI_TRANSFER_OPTIONS_SIZE_IN_BITS|
SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
APP_CHECK_STATUS(status);
#ifndef __linux__
Sleep(10);
#endif
sizeToTransfer=0;
sizeTransfered=0;
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
SPI_TRANSFER_OPTIONS_SIZE_IN_BITS|
SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
APP_CHECK_STATUS(status);
#else
retry=0;
state=FALSE;
SPI_IsBusy(ftHandle,&state);
while((FALSE==state) && (retry<SPI_WRITE_COMPLETION_RETRY))
{
LOG_PRINTF("SPI device is busy(%u)\n",(unsigned)retry);
SPI_IsBusy(ftHandle,&state);
retry++;
}
#endif
/* Write command EWEN(with CS_High -> CS_Low) */
sizeToTransfer=11;
sizeTransfered=0;
buffer[0]=0x8F;/* SPI_EWEN -> binary 10011xxxxxx (11bits) */
buffer[1]=0xFF;
status = SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
SPI_TRANSFER_OPTIONS_SIZE_IN_BITS|
SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE|
SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
APP_CHECK_STATUS(status);
return status;
}
/*!
* \brief Main function / Entry point to the sample application
*
* This function is the entry point to the sample application. It opens the channel, writes to the
* EEPROM and reads back.
*
* \param[in] none
* \return Returns 0 for success
* \sa
* \note
* \warning
*/
int main()
{
for(address=START_ADDRESS_EEPROM;address<END_ADDRESS_EEPROM;address++)
{
LOG_PRINTF("writing address = %d data = %d\n", address, \
address+DATA_OFFSET);
write_byte(SPI_SLAVE_0, address, (uint16)address+DATA_OFFSET);
}
for(address=START_ADDRESS_EEPROM;address<END_ADDRESS_EEPROM;address++)
{
read_byte(SPI_SLAVE_0, address,&data);
read_byte(SPI_SLAVE_0, address,&data);
LOG_PRINTF("reading address=%d data=%d\n",address,data);
}
}
#endif

