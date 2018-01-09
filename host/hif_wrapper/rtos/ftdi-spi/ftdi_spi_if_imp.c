/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <log.h>
#include "..\\ssv_drv_config.h"
#include "..\\ssv_drv_if.h"
#include "ftdi_spi.h"
#include "SPI_defs.h"

// return 
//  < 0 : fail
// >= 0 : # of bytes recieve
s32 	recv_ftdi_spi_impl(u8 *dat, size_t len)
{
	u32			rx_ask_len, rx_recv_len;
	const u32	polling_thd = 5000;
	u32			i;
#ifdef PERFORMANCE_MEASURE
	unsigned __int64 freq;
	unsigned __int64 startTime;
	unsigned __int64 endTime;
	double timerFrequency;
#endif
	
	// SPI_TRACE("%s <= \n", __FUNCTION__);
	rx_ask_len = rx_recv_len = i = 0;
	while (1)
	{
#ifdef PERFORMANCE_MEASURE
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		timerFrequency = (1.0/freq);

		QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
#endif
		// polling the INT_STATUS register
		if ((rx_ask_len = ftdi_spi_ask_rx_data_len()) == 0)
		{

			return -1;
			//continue;
		}
#ifdef PERFORMANCE_MEASURE
		QueryPerformanceCounter((LARGE_INTEGER *)&endTime);
		//if(((endTime-startTime) * timerFrequency) > 0.001)
			LOG_PRINTF("time-1 %f\n", ((endTime-startTime) * timerFrequency));
		
		QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
#endif
		rx_recv_len = ftdi_spi_read_data(dat, rx_ask_len);
#ifdef PERFORMANCE_MEASURE
		QueryPerformanceCounter((LARGE_INTEGER *)&endTime);
		//if(((endTime-startTime) * timerFrequency) > 0.001)
			LOG_PRINTF("time-2 %f\n", ((endTime-startTime) * timerFrequency));
#endif
		if (rx_recv_len  == -1)
		{
			// SPI_FAIL_RET(-1, "sdio_read_data(0x%08x, %d)  = -1 !\n", dat, rx_recv_len);
			return -1;
		}
		if (rx_recv_len != rx_ask_len)
		{
			// SDIO_FAIL_RET(-1, "sdio_read_data(0x%08x, %d) != %d !\n", dat, rx_recv_len, rx_ask_len);
			return -1;
		}
		// otherwise, success!, exit the while loop
		break;
	}
	return rx_recv_len;
}

s32 send_ftdi_spi_impl(void *dat, size_t len)
{
	SPI_TRACE("%s <= \n", __FUNCTION__);

	return 0;
}

bool get_name_ftdi_spi_impl(char name[32])
{
	SPI_TRACE("%s <= : name = %s\n", __FUNCTION__, name);

	strcpy(name, DRV_NAME_SPI);
	return true;
}

bool	ioctl_ftdi_spi_impl(u32 ctl_code, 
					void *in_buf, size_t in_size,	
					void *out_buf, size_t out_size, 
					size_t *bytes_ret)
{
	SPI_TRACE("%s <= \n", __FUNCTION__);
	return true;
}


struct ssv6xxx_drv_ops	g_drv_spi =
{
	DRV_NAME_SPI,
    ftdi_spi_open,
    ftdi_spi_close,
	ftdi_spi_init,
	recv_ftdi_spi_impl,
	ftdi_spi_write_data,	// send_ftdi_spi_impl, 
	get_name_ftdi_spi_impl,
	ioctl_ftdi_spi_impl,
	ftdi_spi_handle,
    ftdi_spi_ack_int,
    ftdi_spi_write_sram,
    ftdi_spi_read_sram,
    ftdi_spi_write_reg,
    ftdi_spi_read_reg,
    NULL,
    NULL,
    NULL,
};

