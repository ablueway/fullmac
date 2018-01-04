/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "drv_l1_spi.h"
#include "drv_l1_gpio.h"
#include "drv_l1_timer.h"
#include "drv_l2_spi_flash.h"
#if _OPERATING_SYSTEM != _OS_NONE 
#include "os.h"
#endif

#if (defined _DRV_L2_SPI) && (_DRV_L2_SPI == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_SPI_NUM			SPI_0			// use spi 0
#define C_SPI_CS_PIN		IO_D6			// IOF1
#define C_SPI_TIME_OUT		3000			// time out count

// spi flash command
#define CMD_WRITE_EN		0x06
#define CMD_WRITE_DIS		0x04
#define CMD_READ_ID			0x9F
#define CMD_READ_STATUS		0x05
#define CMD_WRITE_STATUS	0x01
#define CMD_SECTOR_ERASE	0x20
#define CMD_BLOCK_ERASE		0xD8	//0x52
#define CMD_CHIP_ERASE		0x60	//0xC7		
#define CMD_PAGE_READ		0x03
#define CMD_PAGE_PROGRAM	0x02

// spi flash status bit 
#define SPI_FLASH_SR_WIP	(1 << 0)

// spi flash type 
#define MX					0x00
#define SST					0x01
 
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		gp_memset((INT8S *)ptr, 0x00, cblen)
 
#if 1
#define SPI_CS_INIT	\
{\
	gpio_init_io(C_SPI_CS_PIN, GPIO_OUTPUT); 	\
	gpio_set_port_attribute(C_SPI_CS_PIN, 1); 	\
	gpio_write_io(C_SPI_CS_PIN, 1);				\
} 

#define SPI_CS_HIGH		gpio_write_io(C_SPI_CS_PIN, 1);
#define SPI_CS_LOW		gpio_write_io(C_SPI_CS_PIN, 0);

#else
#define SPI_CS_INIT		DBG_PRINT("CS Init\r\n");
#define SPI_CS_HIGH		DBG_PRINT("CS High\r\n");
#define SPI_CS_LOW		DBG_PRINT("CS Low\r\n");
#endif
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void SPI_Flash_Lock(void);
static void SPI_Flash_Unlock(void);
static INT32S SPI_Flash_Send_Command(INT8U *cmd, INT8U cblen);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const INT8U SST_ManID[] = {0xBF,0x8C};
static INT8U SPI_Man_Type =NULL;
#if _OPERATING_SYSTEM != _OS_NONE 
OS_EVENT *spi_sem;
#endif

static void SPI_Flash_Lock(void)
{
#if _OPERATING_SYSTEM != _OS_NONE
	INT8U err;
	OSSemPend(spi_sem, 0, &err);
#endif
}

static void SPI_Flash_Unlock(void)
{
#if _OPERATING_SYSTEM != _OS_NONE
	OSSemPost(spi_sem);
#endif
}

static INT32S SPI_Flash_Send_Command(INT8U *cmd, INT8U cblen)
{
	INT32S ret;
	
	SPI_Flash_Lock();
	SPI_CS_LOW;
	ret = drv_l1_spi_transceive_cpu(C_SPI_NUM, cmd, cblen, cmd, cblen);
	SPI_CS_HIGH;
	SPI_Flash_Unlock();
	return ret;
} 

/**
 * @brief   Initialize SPI flash interface
 * @param   none
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l2_spiflash_init(INT32U speed)
{
#if _OPERATING_SYSTEM != _OS_NONE
	if(spi_sem == 0) { 
		spi_sem = OSSemCreate(1);
		if(spi_sem == 0) {
			return STATUS_FAIL;
		}
	}
#endif
	
	SPI_CS_INIT;		
	drv_l1_spi_init(C_SPI_NUM);
	drv_l1_spi_clk_set(C_SPI_NUM, speed);
	return STATUS_OK;
}

/**
 * @brief   Un-initialize SPI flash interface
 * @param   none
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l2_spiflash_uninit(void)
{
#if _OPERATING_SYSTEM != _OS_NONE 	
	if(spi_sem) {
		INT8U err;
		
		OSSemDel(spi_sem, OS_DEL_ALWAYS, &err);
		spi_sem = 0;
	}
#endif
	
	drv_l1_spi_uninit(C_SPI_NUM);
	return STATUS_OK;
}

/**
 * @brief   spi flash write enable
 * @param   none
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l2_spiflash_write_enable(void)
{
	INT8U cmd[1];
	INT32S ret;
	
	// send command
	cmd[0] = CMD_WRITE_EN;
	ret = SPI_Flash_Send_Command(cmd, 1);	
	if (ret < 0) {		
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

/**
 * @brief   spi flash write disable
 * @param   none
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l2_spiflash_write_disable(void)
{
	INT8U cmd[1];
	INT32S ret;
	
	// send command
 	cmd[0] = CMD_WRITE_DIS;
	ret = SPI_Flash_Send_Command(cmd, 1);	
	if (ret < 0) {		
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

/**
 * @brief   read spi flash ID
 * @param   Id: byte 1 is manufacturer ID(0xc2), byte 2 is memory type ID, byte 3 is device ID
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l2_spiflash_read_id(INT8U* ID)
{
	INT8U cmd[4];
	INT8U i;
	INT32S ret;

	// send command
	CLEAR(cmd, 4);
	cmd[0] = CMD_READ_ID;
	
	ret = SPI_Flash_Send_Command(cmd, 4);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	ID[0] = cmd[1];
	ID[1] = cmd[2];
	ID[2] = cmd[3];
	
	SPI_Man_Type = MX; // default MX
	for(i=0; i<sizeof(SST_ManID); i++){ // SST SPI write comand dif from MX
		if(ID[0] == SST_ManID[i]){
			drv_l2_spiflash_write_status(0); 	// SST SPI need it 
			SPI_Man_Type = SST;
		}
	}
	
	return STATUS_OK;
}

/**
 * @brief   spi flash read status
 * @param   none
 * @return 	result; STATUS_FAIL/status
 */
INT32S drv_l2_spiflash_read_status(void)
{
	INT8U cmd[2];
	INT32S ret;
	
	// send command
	CLEAR(cmd, 2);
	cmd[0] = CMD_READ_STATUS;
	ret = SPI_Flash_Send_Command(cmd, 2);
	if (ret < 0) {
		return STATUS_FAIL;
	}
	
	ret = (INT32S)cmd[1];
	return ret;
}

/**
 * @brief   spi flash write status
 * @param   status: write status
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l2_spiflash_write_status(INT8U status)
{
	INT32S	ret = STATUS_OK;
	INT8U	cmd[2];
	
	// write enable
	drv_l2_spiflash_write_enable();
	
	// write status	
	CLEAR(cmd, 2);
	cmd[0] = CMD_WRITE_STATUS;
	cmd[1] = status;
	ret = SPI_Flash_Send_Command(cmd, 2);
	if (ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

/**
 * @brief   spi flash wait status
 * @param   none
 * @return 	result; STATUS_FAIL/status
 */
INT32S drv_l2_spiflash_wait_status(void)
{
	INT32U	i = 0;
	
	while((drv_l2_spiflash_read_status() & SPI_FLASH_SR_WIP) != 0) {
	#if _OPERATING_SYSTEM != _OS_NONE
		OSTimeDly(1);
	#else
		drv_msec_wait(1);
	#endif
		if(++i >= C_SPI_TIME_OUT) {
			return SPI_FLASH_TIMEOUT;
		}
	}
	return STATUS_OK;
}

/**
 * @brief   spi flash sector earse, a sector size is 4k Byte(0x1000)
 * @param   addr: input the start address of sector
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_sector_erase(INT32U addr)
{
	INT8U cmd[4];
	INT32S ret;
	
	// check status
	if(drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}
	
	// write enable
	drv_l2_spiflash_write_enable();

	// send command
	cmd[0] = CMD_SECTOR_ERASE;
	cmd[1] = ((addr>>16) & 0xFF);
	cmd[2] = ((addr>>8) & 0xFF);
	cmd[3] = (addr & 0xFF);   
	ret = SPI_Flash_Send_Command(cmd, 4); 
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	// wait status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}

	return STATUS_OK;
}

/**
 * @brief   spi flash block earse, a block size is 64k Byte(0x10000)
 * @param   addr: input the start address of block
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_block_erase(INT32U addr)
{
	INT8U cmd[4];
	INT32S ret;

	// check status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}

	// write enable
	drv_l2_spiflash_write_enable();

	// send command
	cmd[0] = CMD_BLOCK_ERASE;
	cmd[1] = ((addr>>16) & 0xFF);
	cmd[2] = ((addr>>8) & 0xFF);
	cmd[3] = (addr & 0xFF);
	ret = SPI_Flash_Send_Command(cmd, 4); 
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	// wait status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}
	
	return STATUS_OK;
}

/**
 * @brief   spi flash whole chip earse
 * @param   none
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_chip_erase(void)
{
	INT8U cmd[1];
	INT32S ret;

	// check status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}

	// write wnable
	drv_l2_spiflash_write_enable();
	
	// send command
	cmd[0] = CMD_CHIP_ERASE;
	ret = SPI_Flash_Send_Command(cmd, 1); 
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	// wait status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}
		
	return STATUS_OK;
}

/**
 * @brief   spi flash read a page data(256byte)
 * @param   addr: spi flash start address, the addr must be 256 byte allign
 * @param   buf: output buffer address
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_read_page(INT32U addr, INT8U *buf)
{
	INT8U cmd[4];
	INT32S ret;
	
	// check status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}
	
	SPI_Flash_Lock();
	SPI_CS_LOW;

	// send command
	cmd[0] = CMD_PAGE_READ;
	cmd[1] = ((addr>>16) & 0xFF);
	cmd[2] = ((addr>>8) & 0xFF);
	cmd[3] = (addr & 0xFF);    
	ret = drv_l1_spi_transceive_cpu(C_SPI_NUM, cmd, 4, cmd, 4);
	if (ret < 0) {
		goto __exit;
	}
	
	// read page
	ret = drv_l1_spi_transceive(C_SPI_NUM, buf, 256, buf, 256);
	if (ret < 0) {
		goto __exit;
	}
	
	ret = STATUS_OK; 
__exit:
	SPI_CS_HIGH;	
	SPI_Flash_Unlock();
	return ret;
}

/**
 * @brief   spi flash write a page data(256byte)
 * @param   addr: spi flash start address, the addr must be 256 byte allign
 * @param   buf: input buffer address
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_write_page(INT32U addr, INT8U *buf)
{
	INT8U cmd[6];
	INT32S i, ret;
	
	// check status	
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		ret = SPI_FLASH_TIMEOUT;
		goto __exit;
	}
	
	// write enable
	drv_l2_spiflash_write_enable();
		
	if(SPI_Man_Type==SST) {
		INT8U  *ptr = buf;
		
		// SST_Type, send command	
		cmd[0] = 0xAD;
		cmd[1] = ((addr>>16) & 0xFF);
		cmd[2] = ((addr>>8) & 0xFF);
		cmd[3] = (addr & 0xFF);
		cmd[4] = *ptr++;
		cmd[5] = *ptr++;
		
		SPI_Flash_Lock();
		SPI_CS_LOW;
		ret = drv_l1_spi_transceive_cpu(C_SPI_NUM,cmd, 6, cmd, 6);
		SPI_CS_HIGH;	
		SPI_Flash_Unlock();
		if(ret < 0) {
			goto __exit;
		}
		
		// wait status
		if (drv_l2_spiflash_wait_status() != STATUS_OK) {
			ret = SPI_FLASH_TIMEOUT;
			goto __exit;
		}
	
		for (i=0;i<127;i++) {
			cmd[0] = 0xAD;
			cmd[1] = *ptr++;
			cmd[2] = *ptr++;
			
			SPI_Flash_Lock();
			SPI_CS_LOW;
			ret = drv_l1_spi_transceive_cpu(C_SPI_NUM,cmd, 3, cmd, 3);
			SPI_CS_HIGH;	
			SPI_Flash_Unlock();
			if(ret < 0) {
				goto __exit;
			}
			
			// wait status
			if (drv_l2_spiflash_wait_status() != STATUS_OK) {
				ret = SPI_FLASH_TIMEOUT;
				goto __exit;
			}
		}
		
		// write disable 	
		drv_l2_spiflash_write_disable();
	} else {
		// MX_Type, send command
		cmd[0] = CMD_PAGE_PROGRAM;
		cmd[1] = ((addr>>16) & 0xFF);
		cmd[2] = ((addr>>8) & 0xFF);
		cmd[3] = (addr & 0xFF);
		
		SPI_Flash_Lock();
		SPI_CS_LOW;
		ret = drv_l1_spi_transceive_cpu(C_SPI_NUM, cmd, 4, cmd, 4);
		if (ret < 0) {
			SPI_CS_HIGH;	
			SPI_Flash_Unlock();
			goto __exit;
		}
		
		// write data
		ret = drv_l1_spi_transceive(C_SPI_NUM, buf, 256, buf, 0);
		SPI_CS_HIGH;	
		SPI_Flash_Unlock();
		if (ret < 0) {
			goto __exit;
		}
		
		// wait status
		if (drv_l2_spiflash_wait_status() != STATUS_OK) {
			ret = SPI_FLASH_TIMEOUT;
			goto __exit;
		}
	}
	
	ret = STATUS_OK;
__exit:
	return ret;
}

/**
 * @brief   spi flash read nByte data
 * @param   addr: spi flash start address, the address can be any byte address
 * @param   buf: output buffer address
 * @param   nByte: read length
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_read_nbyte(INT32U addr, INT8U *buf, INT32U nByte)
{
	INT8U cmd[4];
	INT32S ret;
	
	// check status
	if (drv_l2_spiflash_wait_status() != STATUS_OK) {
		return SPI_FLASH_TIMEOUT;
	}

	// send command
	cmd[0] = CMD_PAGE_READ;
	cmd[1] = ((addr>>16) & 0xFF);
	cmd[2] = ((addr>>8) & 0xFF);
	cmd[3] = (addr & 0xFF);

	SPI_Flash_Lock();
	SPI_CS_LOW;
	ret = drv_l1_spi_transceive_cpu(C_SPI_NUM, cmd, 4, cmd, 4);
	if (ret < 0) {
		goto __exit;
	}
	
	// read page
	ret = drv_l1_spi_transceive(C_SPI_NUM, buf, nByte, buf, nByte);
	if (ret < 0) {
		goto __exit;
	}

__exit:
	SPI_CS_HIGH;	
	SPI_Flash_Unlock();  	
	return ret;
}

/**
 * @brief   spi flash write nByte data
 * @param   addr: spi flash start address
 * @param   buf: input buffer address
 * @param   nByte: write length
 * @return 	result; STATUS_FAIL/STATUS_OK/SPI_FLASH_TIMEOUT
 */
INT32S drv_l2_spiflash_write_nbyte(INT32U addr, INT8U *buf, INT16U BufCnt)
{
	INT32S ret = STATUS_OK;
	INT8U cmd[6];
	INT32S i;
	
	// check status
	if(drv_l2_spiflash_wait_status() != STATUS_OK) {
		ret = SPI_FLASH_TIMEOUT;
		goto __exit;
	}
	
	// write enable
	drv_l2_spiflash_write_enable();
	
	if(SPI_Man_Type == SST){
		INT8U *ptr = buf;
		
		//SST_Type, send command
		cmd[0] = 0xAD;
		cmd[1] = ((addr>>16) & 0xFF);
		cmd[2] = ((addr>>8) & 0xFF);
		cmd[3] = (addr & 0xFF);
		cmd[4] = *ptr++;
		cmd[5] = *ptr++;

		SPI_Flash_Lock();
		SPI_CS_LOW;
		ret = drv_l1_spi_transceive_cpu(C_SPI_NUM,cmd, 6, cmd, 6);
		SPI_CS_HIGH;	
		SPI_Flash_Unlock();
		if(ret < 0) {
			goto __exit;
		}
		
		// check status
		if (drv_l2_spiflash_wait_status() != STATUS_OK) {
			ret = SPI_FLASH_TIMEOUT;
			goto __exit;
		}

		for (i=0;i<127;i++) {
			// send command
			cmd[0] = 0xAD;
			cmd[1] = *ptr++;
			cmd[2] = *ptr++;
			
			SPI_Flash_Lock();
			SPI_CS_LOW;
			ret = drv_l1_spi_transceive_cpu(C_SPI_NUM,cmd, 3, cmd, 3); 
			SPI_CS_HIGH;	
			SPI_Flash_Unlock();
			
			// wait status
			if (drv_l2_spiflash_wait_status() != STATUS_OK) {
				ret = SPI_FLASH_TIMEOUT;
				goto __exit;
			}
		}
		
		// write disable
		drv_l2_spiflash_write_disable();
	} else {
		//MX_Type, send command 
		cmd[0] = CMD_PAGE_PROGRAM;
		cmd[1] = ((addr>>16) & 0xFF);
		cmd[2] = ((addr>>8) & 0xFF);
		cmd[3] = (addr & 0xFF);

		//send command
		SPI_Flash_Lock();
		SPI_CS_LOW;	
		ret = drv_l1_spi_transceive_cpu(C_SPI_NUM, cmd, 4, cmd, 4);
		if (ret < 0) {
			SPI_CS_HIGH;	
			SPI_Flash_Unlock();
			goto __exit;
		}

		// write data
		ret = drv_l1_spi_transceive(C_SPI_NUM, buf, BufCnt, buf, 0); 
		if (ret < 0) {
			SPI_CS_HIGH;	
			SPI_Flash_Unlock();
			goto __exit;
		}
		
		// wait status
		if (drv_l2_spiflash_wait_status() != STATUS_OK) {
			ret = SPI_FLASH_TIMEOUT;
			goto __exit;
		}
	}

__exit:
	return ret;
}
#endif //(defined _DRV_L2_SPI) && (_DRV_L2_SPI == 1)            