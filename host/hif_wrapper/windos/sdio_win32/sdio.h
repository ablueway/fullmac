/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _SDIO_H_
#define _SDIO_H_

#if (_WIN32 == 1)
	#include "Windows.h"
	#include "types.h"
	#include "sdio_def.h"
	#include "sdio_ioctl.h"
	#include "..\\ssv_drv_config.h"	
	#include <log.h>
#elif (defined __linux__)
	#error For now, SDIO driver is NOT supported in LINUX platform.
#else
	#error Unknown build platform!!!
#endif

u8		sdio_get_bus_width(void);
u32		sdio_get_bus_clock(void);
bool	sdio_set_bus_clock(u32 clock_rate);
bool	sdio_set_force_block_mode(bool mode);
bool	sdio_set_block_mode(bool mode);
bool	sdio_set_block_size(u16 size);
u16		sdio_get_block_size(void);				// will really call DeviceIoControl(...)
u16		sdio_get_cur_block_size(void);			// cheat func, will return 's_cur_block_size' value
bool	sdio_set_function_focus(u8 func);
bool	sdio_get_function_focus(u8 out_buf[1]);
bool	sdio_set_multi_byte_io_port(u32 port);
bool	sdio_get_multi_byte_io_port(u32 *port);
bool	sdio_set_multi_byte_reg_io_port(u32 port);
bool	sdio_get_multi_byte_reg_io_port(u32 *port);
bool	sdio_ack_int(void);
bool	sdio_get_driver_version(u16 *version);
bool	sdio_get_function_number(u8 *num);
/*
	the difference between r/w 'byte' & 'data'
	'byte' -> command mode,    need 'addr' 
	'data' -> data    mode, NO need 'addr'
*/
u8		sdio_read_byte(u8 func,u32 addr);
bool	sdio_write_byte(u8 func,u32 addr, u8 data);
void    sdio_set_data_mode(bool use_data_mode);
u32		sdio_read_reg(u32 addr);
bool	sdio_write_reg(u32 addr, u32 data);
bool    sdio_read_sram(u32 addr, u8 *data, u32 size);
bool    sdio_write_sram(u32 addr, u8 *data, u32 size);
s32		sdio_write_data(void *dat, size_t size);

// return
//  < 0 : fail
// >= 0 : # of bytes recieved
s32		sdio_read_data(u8 *dat, size_t size);
s32		sdio_ask_rx_data_len(void);

bool	sdio_open(void);
bool	sdio_close(void);
bool	sdio_init(void);

// cheat func
u32		sdio_handle(void);

#endif /* _SDIO_H_ */

