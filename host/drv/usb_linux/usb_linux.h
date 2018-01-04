/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _USB_LINUX_H_
#define _USB_LINUX_H_

bool usb_open(void);
bool usb_close(void);
bool usb_init(void);
bool usb_recv(void);
bool usb_send(void);
bool usb_get_name(void);
bool usb_ioctl(u32 ctl_code, void *in_buf, size_t in_size,
		void *out_buf, size_t out_size, size_t *bytes_ret);
bool usb_handle(void);
bool usb_ack_int(void);
bool usb_write_sram(u32 addr, u8 *data, u32 size);
bool usb_write_sram(u32 addr, u8 *data, u32 size);
bool usb_write_reg(u32 addr, u8 *data, u32 size);
bool usb_read_reg(u32 addr, u8 *data, u32 size);
bool usb_write_byte(u8 func,u32 addr, u8 data);
u32 usb_read_byte(u8 func, u32 addr);
u32 usb_write_fw_to_sram(u8 *bin, u32 bin_len, u32 block_size);
s32 usb_start(void);
s32 usb_stop(void);
void usb_ssv_irq_enable(void);
void usb_ssv_irq_disable(void);
bool usb_wakeup_wifi(bool sw);
bool usb_detect_card(void);

#endif /* _USB_LINUX_H_ */
