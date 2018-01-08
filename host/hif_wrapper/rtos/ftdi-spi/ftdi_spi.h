/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef __FTDI_SPI_H__
#define __FTDI_SPI_H__

#if (_WIN32 == 1)
	#include "Windows.h"
	#include "types.h"
	#include "..\\ssv_drv_config.h"	
	#include <log.h>
//#elif (defined __linux__)
//	#error For now, SPI driver is NOT supported in LINUX platform.
//#else
//	#error Unknown build platform!!!
#endif

u32		ftdi_spi_get_bus_clock(void);
bool	ftdi_spi_set_bus_clock(u32 clock_rate);
bool	ftdi_spi_ack_int(void); 
bool	ftdi_spi_get_driver_version(u16 *version);
/*
	the difference between r/w 'byte' & 'data'
	'byte' -> command mode,    need 'addr' 
	'data' -> data    mode, NO need 'addr'
*/
u32		ftdi_spi_read_reg(u32 addr);
bool	ftdi_spi_write_reg(u32 addr, u32 data);
bool    ftdi_spi_write_sram (u32 addr, const u8 *data, u32 size);
bool    ftdi_spi_read_sram (u32 addr, u8 *data, u32 size);

s32		ftdi_spi_write_data(void *dat, size_t size);
// return
//  < 0 : fail
// >= 0 : # of bytes recieved
s32		ftdi_spi_read_data(u8 *dat, size_t size);
s32		ftdi_spi_ask_rx_data_len(void);

bool	ftdi_spi_open(void);
bool	ftdi_spi_close(void);
bool	ftdi_spi_init(void);

// cheat func
u32		ftdi_spi_handle(void);
u32     ftdi_spi_read_status (void);

#endif // __FTDI_SPI_H__

