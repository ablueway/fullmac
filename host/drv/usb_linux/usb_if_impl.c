/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <log.h>
#include "ssv_drv_config.h"
#include "ssv_drv_if.h"
#include "usb_linux.h"

bool usb_open(void)
{
	return true;
}

bool usb_close(void)
{
	return true;
}

bool usb_init(void)
{
	return true;
}

bool usb_recv(void)
{
	return true;
}

bool usb_send(void)
{
	return true;
}

bool usb_get_name(void)
{
	return true;
}

bool usb_ioctl(u32 ctl_code, void *in_buf, size_t in_size,
		void *out_buf, size_t out_size, size_t *bytes_ret)
{
	return true;
}

bool usb_handle(void)
{
	return true;
}

bool usb_ack_int(void)
{
	return true;
}

bool usb_write_sram(u32 addr, u8 *data, u32 size)
{
	return true;
}

bool usb_write_reg(u32 addr, u8 *data, u32 size)
{
	return true;
}

bool usb_read_reg(u32 addr, u8 *data, u32 size)
{
	return true;
}

bool usb_write_byte(u8 func,u32 addr, u8 data)
{
	return true;
}

u32 usb_read_byte(u8 func, u32 addr)
{
	return 0;
}

u32 usb_write_fw_to_sram(u8 *bin, u32 bin_len, u32 block_size)
{
	return 0;
}

s32 usb_start(void)
{
	return 0;
}


s32 usb_stop(void)
{
	return 0;
}

void usb_ssv_irq_enable(void)
{

}

void usb_ssv_irq_disable(void)
{

}

bool usb_wakeup_wifi(bool sw)
{
	return true;
}
	
bool usb_detect_card(void)
{
	return true;
}

const struct ssv6xxx_drv_ops g_drv_usb_linux =
{
	DRV_NAME_USB,
			NULL,
			NULL,
			NULL,			
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL			
};
#if 0
const struct ssv6xxx_drv_ops g_drv_usb_linux =
{
	.name = DRV_NAME_USB,
	.open = usb_open,
	.close = usb_close,
	.init = usb_init,
	.recv = usb_recv,
	.send = usb_send,
	.get_name = usb_get_name,
	usb_ioctl,
	usb_handle,
	usb_ack_int,
	usb_write_sram,
	usb_read_reg,
	usb_write_byte,
	.read_byte = NULL,
	usb_write_fw_to_sram,
	usb_start,
	usb_stop,
	usb_ssv_irq_enable = NULL,
	usb_ssv_irq_disable = NULL,
	.wakeup_wifi = NULL,
	usb_detect_card = NULL
};
#endif
