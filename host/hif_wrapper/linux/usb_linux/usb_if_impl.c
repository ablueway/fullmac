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

#if 0
struct unified_drv_ops g_drv_usb_linux = 
{
	.name = "SSV6XXX_USB";
	.drv_info.flags = UNIFY_RTOS_USB_DRV_INFO_FLAGS,
	.drv_ops = 
	{
	    .read            		= NULL,
	    .write           		= NULL,
	    .readreg	     		= NULL,
	    .writereg        		= NULL,
	    .safe_readreg    		= NULL,
	    .safe_writereg   		= NULL,
	    .burst_readreg   		= NULL,
	    .burst_writereg  		= NULL,    
	    .burst_safe_readreg   	= NULL,
	    .burst_safe_writereg  	= NULL,    
	    .load_fw         		= NULL,
	    .property        		= NULL,
	    .hwif_rx_task    		= NULL,
	    .start_usb_acc   		= NULL,
	    .stop_usb_acc    		= NULL,
	    .jump_to_rom     		= NULL,
		.sysplf_reset    		= NULL, 
	};
};


const struct unified_drv_ops g_drv_usb_linux = 
{
	.name = "SSV6XXX_USB";
	.drv_info.flags = UNIFY_USB_DRV_INFO_FLAGS,
	.drv_ops = 
	{
	    .read            		= ssv6xxx_usb_read,
	    .write           		= ssv6xxx_usb_write,
	    .readreg	     		= ssv6xxx_usb_read_reg,
	    .writereg        		= ssv6xxx_usb_write_reg,
	    .safe_readreg    		= ssv6xxx_usb_read_reg,
	    .safe_writereg   		= ssv6xxx_usb_write_reg,
	    .burst_readreg   		= ssv6xxx_usb_burst_read_reg,
	    .burst_writereg  		= ssv6xxx_usb_burst_write_reg,    
	    .burst_safe_readreg   	= ssv6xxx_usb_burst_read_reg,
	    .burst_safe_writereg  	= ssv6xxx_usb_burst_write_reg,    
	    .load_fw         		= ssv6xxx_usb_load_firmware,
	    .property        		= ssv6xxx_usb_property,
	    .hwif_rx_task    		= ssv6xxx_usb_rx_task,
	    .start_usb_acc   		= ssv6xxx_usb_start_acc,
	    .stop_usb_acc    		= ssv6xxx_usb_stop_acc,
	    .jump_to_rom     		= ssv6xxx_usb_jump_to_rom,
		.sysplf_reset    		= ssv6xxx_usb_sysplf_reset, 
	}
};
#endif
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
