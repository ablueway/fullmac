#ifndef __DRV_l2_SPI_FLASH_H__
#define __DRV_l2_SPI_FLASH_H__

#include "driver_l2.h"

#define SPI_FLASH_BUSY		(-1)
#define SPI_FLASH_TIMEOUT	(-2)

extern INT32S drv_l2_spiflash_init(INT32U speed);
extern INT32S drv_l2_spiflash_uninit(void);
extern INT32S drv_l2_spiflash_write_enable(void);
extern INT32S drv_l2_spiflash_write_disable(void);
extern INT32S drv_l2_spiflash_read_id(INT8U* ID);
extern INT32S drv_l2_spiflash_read_status(void);
extern INT32S drv_l2_spiflash_write_status(INT8U status);
extern INT32S drv_l2_spiflash_wait_status(void);
extern INT32S drv_l2_spiflash_sector_erase(INT32U addr);
extern INT32S drv_l2_spiflash_block_erase(INT32U addr);
extern INT32S drv_l2_spiflash_chip_erase(void);
extern INT32S drv_l2_spiflash_read_page(INT32U addr, INT8U *buf);
extern INT32S drv_l2_spiflash_write_page(INT32U addr, INT8U *buf);
extern INT32S drv_l2_spiflash_read_nbyte(INT32U addr, INT8U *buf, INT32U nByte);
extern INT32S drv_l2_spiflash_write_nbyte(INT32U addr, INT8U *buf, INT16U BufCnt);
#endif	/*__drv_l1_SENSOR_H__*/