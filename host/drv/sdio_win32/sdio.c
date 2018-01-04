/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <hctrl.h>
#include <ssv_hal.h>

#include "sdio.h"

static u32	s_hSDIO				= 0;		// where is ONLY one SDIO handle in this module!
static bool	s_bCurBlockMode		= true;
static bool s_bForceBlockMode	= false;
static u16	s_cur_block_size	= SDIO_DEF_BLOCK_SIZE;
static OsMutex	s_SDIOMutex;
static u32	s_write_data_cnt	= 1;
static bool s_sdio_data_mode = true; // Power on status is data mode.
const char*	_err_str(u32 err);

const char* _err_str(u32 err)
{
    switch (err)
    {
        case 0: return "The operation completed successfully.";
        case 1: return "Incorrect function.";
        case 2: return "The system cannot find the file specified.";
        case 3: return "The system cannot find the path specified.";
		case 4: return "The system cannot open the file.";
		case 5: return "Access is denied.";
        case 6: return "The handle is invalid.";
        default: return "Other. Plz visit MSDN website.";
    }
}

u32	sdio_handle(void)
{
	return s_hSDIO;
}

#define	SDIO_CHECK_OPENED()		{ if (s_hSDIO == 0)	SDIO_FAIL_RET(0, "SDIO device is NOT opened!\n");	}


bool	sdio_get_function_number(u8 *num)
{
	u8		in_buf[4], out_buf[1];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 1;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32) IOCTL_SSV6XXX_SDIO_GET_FUNCTION_NUMBER,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_FUNCTION_NUMBER\n");
		return false;
	}

	*num = out_buf[0];
	SDIO_TRACE("%-20s : %d (0x%02x)\n", "GET_FUNCTION_NUMBER", *num, *num);
	return true;
}

bool	sdio_get_driver_version(u16 *version)
{
	u8		in_buf[4], out_buf[2];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	*version = 0;
	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 2;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32) IOCTL_SSV6XXX_SDIO_GET_DRIVER_VERSION,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_DRIVER_VERSION\n");
		return false;
	}

	*version = (((u16)out_buf[0]) | ((u16)out_buf[1] << 8));
	SDIO_TRACE("%-20s : %d (0x%04x)\n", "GET_DRIVER_VERSION", *version, *version);
	return true;
}

u8	sdio_get_bus_width(void)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;
	u32		r;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 1;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_GET_BUS_WIDTH,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_BUS_WIDTH\n");
		return 0;
	}

	r = ((u32)out_buf[0]);
	SDIO_TRACE("%-20s : %d (0x%02x)\n", "GET_BUS_WIDTH", r, r);
	return r;
}

bool	sdio_set_bus_width(u8 busWidth)
{
	u8		in_buf[1], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 1;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = (u8)((busWidth >> 0) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
		(u32)IOCTL_SSV6XXX_SDIO_SET_BUS_WIDTH,
		in_buf,  in_len,
		out_buf, out_len,
		&dwRet,
		NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_BUS_WIDTH\n");
		return false;
	}

	SDIO_TRACE("%-20s : %d\n", "SET_BUS_WIDTH", busWidth);
	return true;
}

u32		sdio_get_bus_clock(void)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;
	u32		r;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_GET_BUS_CLOCK,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_BUS_CLOCK\n");
		return 0;
	}

	r = ((u32)out_buf[0]       | (u32)out_buf[1] <<  8 |
		 (u32)out_buf[2] << 16 | (u32)out_buf[3] << 24);
	SDIO_TRACE("%-20s : %d (0x%08x)\n", "GET_BUS_CLOCK", r, r);
	return r;
}

bool	sdio_set_bus_clock(u32 clock_rate)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = (u8)((clock_rate >> 0) & 0xff);
	in_buf[1] = (u8)((clock_rate >> 8) & 0xff);
	in_buf[2] = (u8)((clock_rate >> 16) & 0xff);
	in_buf[3] = (u8)((clock_rate >> 24) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_BUS_CLOCK,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_BUS_CLOCK\n");
		return false;
	}

	SDIO_TRACE("%-20s : %d\n", "SET_BUS_CLOCK", clock_rate);
	return true;
}

bool	sdio_set_force_block_mode(bool mode)
{
	SDIO_CHECK_OPENED();

	s_bForceBlockMode = mode;
	SDIO_TRACE("%-20s : %d\n", __FUNCTION__, mode);
	return true;
}

bool	sdio_set_block_mode(bool mode)
{
	u8		in_buf[1], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	if (mode == s_bCurBlockMode)
	{
		SDIO_TRACE("%s() : mode = s_bCurBlockMode = %d, NO need to set, return!\n", __FUNCTION__, s_bCurBlockMode);
		return true;
	}

	in_len	= 1;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	s_bCurBlockMode = mode;
	in_buf[0] = s_bCurBlockMode ? 1 : 0;
	//OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_BLOCK_MODE,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	//OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_BLOCK_MODE\n");
		return false;
	}
	SDIO_TRACE("%-20s : %d (0x%02x)\n", "SET_BLOCK_MODE", s_bCurBlockMode, s_bCurBlockMode);
	return true;
}

inline u16	sdio_get_cur_block_size(void)
{
	return s_cur_block_size;
}

u16		sdio_get_block_size(void)
{
	u8		in_buf[4], out_buf[2];
	u32		in_len, out_len, dwRet;
	bool	status;
	u16		r = 0;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 2;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_GET_BLOCKLEN,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_BLOCKLEN\n");
		return false;
	}
	r = (out_buf[0] | (out_buf[1] << 8));
	SDIO_TRACE("%-20s : %d\n", "GET_BLOCKLEN", r);
	return true;
}

// note : this func will automatically update 's_cur_block_size' value after success
bool	sdio_set_block_size(u16 size)
{
	u8		in_buf[2], out_buf[1];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 2;
	//memset(in_buf, 0x00, in_len);
	out_len	= 1;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(size >> 0) & 0xff);
	in_buf[1] = ((u8)(size >> 8) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_BLOCKLEN,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_BLOCKLEN\n");
		return false;
	}
	SDIO_TRACE("%-20s : %d\n", "SET_BLOCKLEN", size);
	s_cur_block_size = size;
	return true;
}

bool	sdio_set_function_focus(u8 func)
{
	u8		in_buf[1], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 1;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = func;
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_FUNCTION_FOCUS,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_FUNCTION_FOCUS\n");
		return false;
	}
	SDIO_TRACE("%-20s : %d\n", "SET_FUNCTION_FOCUS", func);
	return true;
}

bool	sdio_get_function_focus(u8 out_buf[1])
{
	u8		in_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 1;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_GET_FUNCTION_FOCUS,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_FUNCTION_FOCUS\n");
		return false;
	}
	SDIO_TRACE("%-20s : %d (0x%02x)\n", "GET_FUNCTION_FOCUS", out_buf[0], out_buf[0]);
	return true;
}

bool		sdio_read_reg(u32 addr , u32 *data)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;
	u32		r;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(addr >> 0) & 0xff);
	in_buf[1] = ((u8)(addr >> 8) & 0xff);
	in_buf[2] = ((u8)(addr >> 16) & 0xff);
	in_buf[3] = ((u8)(addr >> 24) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_READ_REG,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if ((status == FALSE) || (dwRet < 0))
	{
		SDIO_ERROR("READ_REG, status = %d, dwRet = %d\n", status, dwRet);
		return FALSE;
	}

	r = (((u32)(out_buf[0]) << 0) |
		 ((u32)(out_buf[1]) << 8 ) |
		 ((u32)(out_buf[2]) << 16) |
		 ((u32)(out_buf[3]) << 24));
	SDIO_TRACE("%-20s() : 0x%08x, 0x%08x\n", "READ_REG", addr, r);

    *data = r;
	return TRUE;
}

bool	sdio_write_reg(u32 addr, u32 data)
{
	u8		in_buf[8], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 8;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(addr >> 0) & 0xff);
	in_buf[1] = ((u8)(addr >> 8) & 0xff);
	in_buf[2] = ((u8)(addr >> 16) & 0xff);
	in_buf[3] = ((u8)(addr >> 24) & 0xff);
	in_buf[4] = ((u8)(data >> 0) & 0xff);
	in_buf[5] = ((u8)(data >> 8) & 0xff);
	in_buf[6] = ((u8)(data >> 16) & 0xff);
	in_buf[7] = ((u8)(data >> 24) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_WRITE_REG,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("WRITE_REG\n");
		return false;
	}

	SDIO_TRACE("%-20s : 0x%08x, 0x%08x\n", "WRITE_REG", addr, data);
	return true;
}


//Only for SDIO register access
u8	sdio_read_byte(u8 func,u32 addr)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	sdio_set_function_focus(func);

	in_len	= 4;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(addr >> 0) & 0xff);
	in_buf[1] = ((u8)(addr >> 8) & 0xff);
	in_buf[2] = ((u8)(addr >> 16) & 0xff);
	in_buf[3] = ((u8)(addr >> 24) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_READ_BYTE,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);

	if ((status == FALSE) || (dwRet < 0))
	{
		SDIO_ERROR("READ_BYTE, status = %d, dwRet = %d [%d]\n", status, dwRet,GetLastError());
		return 0x00;
	}

	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, out_buf[0]);
	return out_buf[0];
}


void    sdio_set_data_mode(bool use_data_mode)
{
	if (use_data_mode)
	{
		sdio_write_byte(1,0x0c,0);
		s_sdio_data_mode = true;
	}
	else
	{
        sdio_write_byte(1,0x0c,0x2);
		s_sdio_data_mode = false;
	}
}

//Only for SDIO register access
bool	sdio_write_byte(u8 func,u32 addr, u8 data)
{
	u8		in_buf[5], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	sdio_set_function_focus(func);

	in_len	= 5;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(addr >> 0) & 0xff);
	in_buf[1] = ((u8)(addr >> 8) & 0xff);
	in_buf[2] = ((u8)(addr >> 16) & 0xff);
	in_buf[3] = ((u8)(addr >> 24) & 0xff);
	in_buf[4] = data;
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_WRITE_BYTE,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("WRITE_BYTE [$d]\n",GetLastError());
        //DebugBreak();
		return false;
	}

	SDIO_TRACE("%-20s : 0x%08x, 0x%02x\n", "WRITE_BYTE", addr, data);
	return true;
}

s32	sdio_ask_rx_data_len(void)
{
	u8		int_status;
	u32		r = 0;

#if (CONFIG_RX_POLL == 1)
	do
	{
		int_status = sdio_read_byte(1,REG_INT_STATUS);
		if (int_status & 0x01)
		{
			r = sdio_read_byte(1,REG_CARD_PKT_LEN_0) & 0xff;
			r = r | ((sdio_read_byte(1,REG_CARD_PKT_LEN_1) & 0xff) << 8);
			return r;
		}
	}while(r==0);
#else
	{
		int_status = sdio_read_byte(1,REG_INT_STATUS);
		if (int_status & 0x01)
		{
			r = sdio_read_byte(1,REG_CARD_PKT_LEN_0) & 0xff;
			r = r | ((sdio_read_byte(1,REG_CARD_PKT_LEN_1) & 0xff) << 8);
			return r;
		}
	}
#endif
	return r;
}

s32		sdio_read_data(u8 *dat, size_t size)
{
	u8		in_buf[4], *out_buf = NULL;
	u32		in_len, out_len, dwRet;
	bool	status;

	u32		block_size;
	u32		block_count;

	SDIO_CHECK_OPENED();
	SDIO_TRACE("%s() <= : size = %d\n", __FUNCTION__, size);
    OS_MutexLock(s_SDIOMutex);
	// cal 'block_size'
	if (s_bForceBlockMode || size > SDIO_DEF_BLOCK_MODE_THRD)
	{
		if (!sdio_set_block_mode(true))
			SDIO_FAIL_RET(-1, "sdio_set_block_mode(true)\n");

		block_size = sdio_get_cur_block_size();
	}
	else
	{
		if (!sdio_set_block_mode(false))
			SDIO_FAIL_RET(-1, "sdio_set_block_mode(false)\n");
		block_size = 8;
	}

	// malloc() for out_buf with 8 bytes multiple, for alignment
	block_count = ((size % block_size) > 0) ? ((size/block_size) + 1) : (size/block_size);
	out_len = block_count * block_size;		// force to 8 bytes alignment

	//SDIO_TRACE("out_buf = 0x%08x, out_len = %d\n", out_buf, out_len);
	//SDIO_TRACE("block_count = %d, block_size = %d\n", block_count, block_size);

	// Note :
	//	- the in_len & in_buf is 'fake'
	//	- the underlining IOCTL_SSV6XXX_SDIO_WRITE_MULTI_BYTE opeation will use 'out_len' to decide how many data should be read
	//
	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);

	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_READ_MULTI_BYTE,
							in_buf,  in_len,
							dat, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		// SDIO_FAIL_RET(-1, "READ_MULTI_BYTE, status = FALSE\n");
        //SDIO_TRACE("READ_MULTI_BYTE, status = FALSE [%08X]  [%08X]\n",status,GetLastError());
        //DebugBreak();
		return -1;
	}

	// SDIO_TRACE("%-20s : len = %d (0x%08x)\n", "READ_MULTI_BYTE", out_len, out_len);
	return size;
}

s32		sdio_read_data_ex(u8 *dat)
{
	u8		in_buf[4], *out_buf = NULL;
	s32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();
	// Note :
	//	- the in_len & in_buf is 'fake'
	//	- the underlining IOCTL_SSV6XXX_SDIO_WRITE_MULTI_BYTE opeation will use 'out_len' to decide how many data should be read
	//
	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_READ_DATA,
							in_buf,  in_len,
							dat, MAX_RECV_BUF,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		// SDIO_FAIL_RET(-1, "READ_MULTI_BYTE, status = FALSE\n");
		return -1;
	}

    out_len = *((s32 *)dat);

    SDIO_TRACE("%s() <= : size = %d\n", __FUNCTION__, out_len);

	// SDIO_TRACE("%-20s : len = %d (0x%08x)\n", "READ_MULTI_BYTE", out_len, out_len);
	return out_len;
}

s32	sdio_write_data(void *dat, size_t size)
{
	u32		retry_count=0,result32=0;
    struct ssv6xxx_hci_txq_info *txq_info;
	u32 free_tx_page=0, free_tx_id=0;
	u8		*in_buf, out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;
    bool    blockStatus;

	u32		block_size;
	u32		block_count;
	u32		leave_size;

	u32     alloc_size;

	SDIO_CHECK_OPENED();
	SDIO_TRACE("%s() <= : size = %d\n", __FUNCTION__, size);

    in_buf = NULL;
	// cal 'block_size'
	if (s_bForceBlockMode || size > SDIO_DEF_BLOCK_MODE_THRD)
	{
        blockStatus = true;
		block_size = sdio_get_cur_block_size();
	}
	else
	{
        blockStatus = false;
		block_size = 8;
	}

	//Page offset 80  HCI reserve 16 * 3 ( 80 + 48)
	alloc_size = size + 128;

	// malloc() for in_buf & align data for 8 bytes multiple
	leave_size	= (u32)alloc_size % block_size;
	block_count = (leave_size > 0) ? ((alloc_size/block_size) + 1) : (alloc_size/block_size);

	in_len = block_count * block_size;		// force to 8 bytes alignment

	if (s_sdio_data_mode) {
		//MMU 1 page = 256 bytes
		if(in_len % 256)
			alloc_size = (in_len>>8)+1;
		else
			alloc_size = (in_len>>8);

		do{
			result32 = sdio_read_reg(ADR_TX_ID_ALL_INFO);
			txq_info = (struct ssv6xxx_hci_txq_info*)(&result32);

			free_tx_page = SSV6200_PAGE_TX_THRESHOLD - txq_info->tx_use_page;
			free_tx_id	 = SSV6200_ID_TX_THRESHOLD - txq_info->tx_use_id;
			if((free_tx_page >= alloc_size) && free_tx_id)
			{
				if((SSV6200_ID_AC_BK_OUT_QUEUE == txq_info->txq0_size)||
				(SSV6200_ID_AC_BE_OUT_QUEUE == txq_info->txq1_size)||
				(SSV6200_ID_AC_VI_OUT_QUEUE == txq_info->txq2_size)||
				(SSV6200_ID_AC_VO_OUT_QUEUE == txq_info->txq3_size))
					;
				else
					break;
			}
		}while(1);
	}

    OS_MutexLock(s_SDIOMutex);

	if (!sdio_set_block_mode(blockStatus))
			SDIO_FAIL_RET(-1, "sdio_set_block_mode(true)\n");

	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);
    s_write_data_cnt++;
	// LOG_PRINTF("sdio_write_data : # %d\n", s_write_data_cnt);

	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_WRITE_MULTI_BYTE,
							dat,  in_len,
							out_buf, out_len,
							(LPDWORD)&dwRet,
							NULL);

	OS_MutexUnLock(s_SDIOMutex);

	if (status == FALSE){
		//DebugBreak();
        SDIO_FAIL_RET(-1, "WRITE_MULTI_BYTE, status = FALSE [%d]\n",GetLastError());
    }

	SDIO_TRACE("%-20s : len = %d (0x%08x)\n", "WRITE_MULTI_BYTE", in_len, in_len);
	return size;
}

bool	sdio_get_multi_byte_io_port(u32 *port)
{
	u8	buf[3];

	SDIO_CHECK_OPENED();

	*port = 0;
	//memset(buf, 0x00, 3);
	buf[0] = sdio_read_byte(1,0x00);
	buf[1] = sdio_read_byte(1,0x01);
	buf[2] = sdio_read_byte(1,0x02);
	*port = (((buf[0] & 0xff) << 0) |
			 ((buf[1] & 0xff) << 8) |
			 ((buf[2] & 0x01) << 16));
	SDIO_TRACE("%-20s : %d (0x%08x)\n", "GET_MULTI_BYTE_IO_PORT", *port, *port);
	return true;
}

bool	sdio_set_multi_byte_io_port(u32 port)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(port >> 0) & 0xff);
	in_buf[1] = ((u8)(port >> 8) & 0xff);
	in_buf[2] = ((u8)(port >> 16) & 0xff);
	in_buf[3] = ((u8)(port >> 24) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_MULTI_BYTE_IO_PORT,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_MULTI_BYTE_IO_PORT\n");
		return false;
	}

	SDIO_TRACE("%-20s : %d (0x%08x)\n", "SET_MULTI_BYTE_IO_PORT", port, port);
	return true;
}

bool	sdio_get_multi_byte_reg_io_port(u32 *port)
{
	u8	buf[3];

	SDIO_CHECK_OPENED();

	*port = 0;
	//memset(buf, 0x00, 3);
	buf[0] = sdio_read_byte(1,0x70);
	buf[1] = sdio_read_byte(1,0x71);
	buf[2] = sdio_read_byte(1,0x72);
	*port = (((buf[0] & 0xff) << 0) |
			 ((buf[1] & 0xff) << 8) |
			 ((buf[2] & 0x01) << 16));
	SDIO_TRACE("%-20s : %d (0x%08x)\n", "GET_MULTI_BYTE_REG_IO_PORT", *port, *port);
	return true;
}

bool	sdio_set_multi_byte_reg_io_port(u32 port)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	//memset(in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = ((u8)(port >> 0) & 0xff);
	in_buf[1] = ((u8)(port >> 8) & 0xff);
	in_buf[2] = ((u8)(port >> 16) & 0xff);
	in_buf[3] = ((u8)(port >> 24) & 0xff);
	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_MULTI_BYTE_REG_IO_PORT,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_MULTI_BYTE_REG_IO_PORT\n");
		return false;
	}

	SDIO_TRACE("%-20s : %d (0x%08x)\n", "SET_MULTI_BYTE_REG_IO_PORT", port, port);
	return true;
}

bool	sdio_ack_int(void)
{
	u8		in_buf[4], out_buf[4];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 4;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 4;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_ACK_INT,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("ACK_INT\n");
		return false;
	}

	SDIO_TRACE("%-20s : \n", "ACK_INT");
	return true;
}

u32		sdio_get_auto_ack(void)
{
	u8		in_buf[1], out_buf[1];
	u32		in_len, out_len, dwRet;
	bool	status;
	u32		r;

	SDIO_CHECK_OPENED();

	in_len	= 1;
	ssv6xxx_memset((void *)in_buf, 0x00, in_len);
	out_len	= 1;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_GET_AUTO_ACK_INT,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("GET_AUTO_ACK\n");
		return 0;
	}

	r = out_buf[0];
	SDIO_TRACE("%-20s : %d (0x%08x)\n", "GET_AUTO_ACK", r, r);
	return r;
}

bool	sdio_set_auto_ack(bool auto_ack)
{
	u8		in_buf[1], out_buf[1];
	u32		in_len, out_len, dwRet;
	bool	status;

	SDIO_CHECK_OPENED();

	in_len	= 1;
	//memset(in_buf, 0x00, in_len);
	out_len	= 1;
	ssv6xxx_memset((void *)out_buf,0x00, out_len);

	in_buf[0] = (u8)auto_ack;

	OS_MutexLock(s_SDIOMutex);
	status = DeviceIoControl((HANDLE)s_hSDIO,
							(u32)IOCTL_SSV6XXX_SDIO_SET_AUTO_ACK_INT,
							in_buf,  in_len,
							out_buf, out_len,
							&dwRet,
							NULL);
	OS_MutexUnLock(s_SDIOMutex);
	if (status == FALSE)
	{
		SDIO_ERROR("SET_AUTO_ACK_INT\n");
		return false;
	}

	SDIO_TRACE("%-20s : %d\n", "SET_AUTO_ACK_INT", auto_ack);
	return true;
}

bool	sdio_open(void)
{
	u32	err;

	SDIO_TRACE("%-20s : 0x%08x\n", __FUNCTION__ " <= ", s_hSDIO);

	if (s_hSDIO != 0)
	{
		SDIO_WARN("Try to open a already opened SDIO device!\n");
		return false;
	}

	s_hSDIO = (u32)CreateFile(SDIO_DEVICE_NAME,
							GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_DEVICE,
							0);
	if ((HANDLE)s_hSDIO == INVALID_HANDLE_VALUE)
	{
		s_hSDIO = 0;
		err = GetLastError();
		SDIO_FAIL_RET(0, "CreateFile(), GetLastError() = 0x%04x\n(%s)\n", err, _err_str(err));
	}

	SDIO_TRACE("%-20s : 0x%08x\n", __FUNCTION__ " => ", s_hSDIO);
	return (s_hSDIO != 0);
}

bool	sdio_close(void)
{
	SDIO_CHECK_OPENED();

	if (CloseHandle((HANDLE)s_hSDIO))
	{
		s_hSDIO = 0;
		SDIO_TRACE("%-20s :\n", __FUNCTION__);
		return true;
	}

	SDIO_WARN("CloseHandle(0x%08x) fail!\n", s_hSDIO);
	return false;
}

bool	sdio_init(void)
{
	s32		current_sdio_clock_rate = 0;
//	u32		n32;
	u32		port_data, port_reg;
	u8		busWidth;

	SDIO_CHECK_OPENED();

	if (s_hSDIO == 0)
	{
		SDIO_ERROR("SDIO is NOT opened!\n", s_hSDIO);
		return false;
	}
	// mutex init
	OS_MutexInit(&s_SDIOMutex);
	LOG_PRINTF("<sdio init> : init sdio mutex ...\n");

	// set clock
	LOG_PRINTF("<sdio init> : current sdio clock rate 1 : %d\n", sdio_get_bus_clock());
	if (!sdio_set_bus_clock(SDIO_DEF_CLOCK_RATE))
		SDIO_FAIL_RET(0, "sdio_set_bus_clock(%d)\n", SDIO_DEF_CLOCK_RATE);
	LOG_PRINTF("<sdio init> : set bus clock (%d) -> after (%d)\n", SDIO_DEF_CLOCK_RATE, sdio_get_bus_clock());

	// set off block mode
	if (!sdio_set_block_mode(false))
		SDIO_FAIL_RET(0, "sdio_set_block_mode(false)\n");
	LOG_PRINTF("<sdio init> : set off block mode\n");

	// set block size
	if (!sdio_set_block_size(SDIO_DEF_BLOCK_SIZE))
		SDIO_FAIL_RET(0, "sdio_set_block_size(%d)\n", SDIO_DEF_BLOCK_SIZE);
	LOG_PRINTF("<sdio init> : set block size to %d\n", SDIO_DEF_BLOCK_SIZE);

	busWidth = sdio_read_byte(0,REG_CCCR_07H_REG);
	if ( busWidth == 2 ) // 4bit
	{
		busWidth = 4;
	}
	else // 1bit
	{
		busWidth = 1;
	}
	if (!sdio_set_bus_width(busWidth))
		SDIO_FAIL_RET(0, "sdio_set_bus_width(%d)\n",busWidth);
	LOG_PRINTF("<sdio init> : sdio_set_bus_width(%d)\n",busWidth);

	// get dataIOPort
	if (!sdio_get_multi_byte_io_port(&port_data))
		SDIO_FAIL_RET(0, "sdio_get_multi_byte_io_port()\n");
	LOG_PRINTF("<sdio init> : get data i/o port %d (0x%08x)\n", port_data, port_data);

	// get regIOPort
	if (!sdio_get_multi_byte_reg_io_port(&port_reg))
		SDIO_FAIL_RET(0, "sdio_get_multi_byte_reg_io_port()\n");
	LOG_PRINTF("<sdio init> : get  reg i/o port %d (0x%08x)\n", port_reg, port_reg);

	// set dataIOPort
	if (!sdio_set_multi_byte_io_port(port_data))
		SDIO_FAIL_RET(0, "sdio_set_multi_byte_io_port(%d)\n", port_data);
	LOG_PRINTF("<sdio init> : set data i/o port %d (0x%08x)\n", port_data, port_data);

	// set regIOPort
	if (!sdio_set_multi_byte_reg_io_port(port_reg))
		SDIO_FAIL_RET(0, "sdio_set_multi_byte_reg_io_port(%d)\n", port_reg);
	LOG_PRINTF("<sdio init> : set reg  i/o port %d (0x%08x)\n", port_reg, port_reg);

	// mask rx/tx complete int
	// 0: rx int
	// 1: tx complete int
#if (CONFIG_RX_POLL == 0)
	if (!sdio_write_byte(1,REG_INT_MASK, 0x02))
		SDIO_FAIL_RET(0, "sdio_write_byte(0x04, 0x02)\n");
	LOG_PRINTF("<sdio init> : mask rx/tx complete int (0x04, 0x02)\n");
#if (CONFIG_RX_AUTO_ACK_INT == 0)
    sdio_set_auto_ack(false);
#else
    sdio_set_auto_ack(true);
#endif
#else
	if (!sdio_write_byte(1,REG_INT_MASK, 0x0B))
		SDIO_FAIL_RET(0, "sdio_write_byte(0x04, 0x03)\n");
	LOG_PRINTF("<sdio init> : mask rx/tx complete int (0x04, 0x03)\n");
#endif

	//sdio_write_byte(1,REG_SDIO_TX_ALLOC_SHIFT,(SDIO_TX_ALLOC_ENABLE | SDIO_TX_ALLOC_SIZE_SHIFT));
	//LOG_PRINTF("<sdio init> : REG_SDIO_TX_ALLOC_SHIFT=%x\n",sdio_read_byte(1,REG_SDIO_TX_ALLOC_SHIFT));

	// output timing
	if (!sdio_write_byte(1,0x55, SDIO_DEF_OUTPUT_TIMING))
		SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
	LOG_PRINTF("<sdio init> : output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);
	// switch to normal mode
	// bit[1] , 0:normal mode, 1: Download mode
	if (!sdio_write_byte(1,0x0c, 0x00))
		SDIO_FAIL_RET(0, "sdio_write_byte(0x0c, 0x00)\n");
	LOG_PRINTF("<sdio init> : switch to normal mode (0x0c, 0x00)\n");

	LOG_PRINTF("<sdio init> : success!! \n");
	return true;
}

bool sdio_read_sram(u32 addr, u8 *data, u32 size)
{
    return false;
} // end of - sdio_read_sram -


bool sdio_write_sram(u32 addr, u8 *data, u32 size)
{
	u16   bsize = sdio_get_cur_block_size();

    sdio_set_block_size(SDIO_DEF_BLOCK_SIZE);

    #if 0
    {
    u32 i;
    size = (size + (sizeof(u32) - 1)) / sizeof(u32);
    for (i = 0; i< size; i++, sram_addr += sizeof(u32))
    {
        sdio_write_reg(u32 addr,u32 data)(sram_addr, buffer[i]);
	}
    #endif
	// Set SDIO DMA start address
	sdio_write_reg(0xc0000860, addr);

    sdio_set_data_mode(false);

	sdio_write_data(data, size);

    sdio_set_data_mode(true);

    sdio_set_block_size(bsize);

    return true;
}// end of - sdio_write_sram -


