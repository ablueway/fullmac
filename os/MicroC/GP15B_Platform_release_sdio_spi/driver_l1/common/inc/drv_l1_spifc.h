#ifndef __drv_l1_SPIFC_H__
#define __drv_l1_SPIFC_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

typedef enum
{
	COMMAND_ONLY,
	COMMAND_ADDR,
	COMMAND_ADDR_TX,
	COMMAND_ADDR_RX,
	COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX,
	COMMAND_TX,
	COMMAND_RX,
	COMMAND_MAX
} COMMAND_MODE;

typedef enum
{
	IO_1BIT,
	IO_2BIT,
	IO_4BIT,
	IO_MAX
} IO_NUMBER;

typedef struct spifcCtrl_s
{
	INT8U set_ahb_en;		/* only change ahb access mode enable */
	INT8U command_mode;		/* command mode as COMMAND_MODE */
	INT8U command;			/* spi flash command */
	INT8U enhance_data;		/* enhance mode data, 0x5A, 0xA5, 0xF0, 0x0F, 0x55, 0xAA, 0xFF */
	INT8U dummy_clk;		/* dummy clock cycle number */
	INT8U cmd_mio;			/* multi io number for command state, IO_NUMBER */
	INT8U addr_mio;			/* multi io number for address state, IO_NUMBER */
	INT8U data_mio;			/* multi io number for data state, IO_NUMBER */
	
	INT32U spi_addr;		/* spi flash address */
	INT8U *tx_buf;			/* transfer buffer */
	INT32U tx_cblen;		/* transfer buffer length in byte */
	INT32U *rx_buf;			/* receive buffer */
	INT32U rx_cblen;		/* receive buffer length in byte */
} spifcCtrl_t;

// spifc controller
extern void drv_l1_spifc_init(INT32U speed);
extern void drv_l1_spifc_set_timing(INT8U ignore_last_clk, INT8U clk_idle_level, INT8U sample_delay);

// spi flash
extern INT32S drv_l1_spifcflash_write_enable(void);
extern INT32S drv_l1_spifcflash_write_disable(void);
extern INT32S drv_l1_spifcflash_read_id(INT8U *ID);
extern INT32U drv_l1_spifcflash_read_status(void);
extern INT32S drv_l1_spifcflash_write_status(INT32U enable, INT32U bit);
extern INT32S drv_l1_spifcflash_set_quad_enable(INT32U enable);
extern INT32S drv_l1_spifcflash_wait_status(INT32U msec);
extern INT32S drv_l1_spifcflash_chip_erase(void);
extern INT32S drv_l1_spifcflash_sector_erase(INT32U addr);
extern INT32S drv_l1_spifcflash_block_erase(INT32U addr);
extern INT32S drv_l1_spifcflash_read_page(INT8U mio, INT32U addr, INT8U *rx_buf, INT32U cblen);
extern INT32S drv_l1_spifcflash_write_page(INT8U mio, INT32U addr, INT8U *tx_buf, INT32U cblen);

// spifc set ahb mode
extern INT32S drv_l1_spifc_set_read_mode(INT8U mio, INT8U enhance_en);
#endif 		// __drv_l1_SPIFC_H__