/*
 * Copyright (c) 2014 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Semiconductor Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __HIF_WRAPPER_H__
#define __HIF_WRAPPER_H__

//typedef s32 (*irq_handler)(int, void *);



#define DRV_INFO_FLAG_OS_TYPE_SHIFT				0x0
#define DRV_INFO_FLAG_OS_TYPE_MASK				0x000000FF
#define DRV_INFO_FLAG_OS_TYPE_LINUX				0x0
#define DRV_INFO_FLAG_OS_TYPE_RTOS				0x1

#define SET_DRV_INFO_FLAG_OS_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_OS_TYPE_SHIFT) & DRV_INFO_FLAG_OS_TYPE_MASK))


#define DRV_INFO_FLAG_REGISTER_TYPE_SHIFT		8
#define DRV_INFO_FLAG_REGISTER_TYPE_MASK		0x0000FF00
#define DRV_INFO_FLAG_REGISTER_TYPE_PASSIVE		0x0
#define DRV_INFO_FLAG_REGISTER_TYPE_ACTIVE		0x1

#define SET_DRV_INFO_FLAG_REGISTER_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_REGISTER_TYPE_SHIFT) & DRV_INFO_FLAG_REGISTER_TYPE_MASK))


#define DRV_INFO_FLAG_HW_TYPE_SHIFT				16
#define DRV_INFO_FLAG_HW_TYPE_MASK				0x00FF0000
#define DRV_INFO_FLAG_HW_TYPE_USB				0x0
#define DRV_INFO_FLAG_HW_TYPE_SDIO				0x1
#define DRV_INFO_FLAG_HW_TYPE_SPI				0x2

#define SET_DRV_INFO_FLAG_HW_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_HW_TYPE_SHIFT) & DRV_INFO_FLAG_HW_TYPE_MASK))


#define DRV_INFO_FLAG_HW_NOTIFY_TYPE_SHIFT				24
#define DRV_INFO_FLAG_HW_NOTIFY_TYPE_MASK				0xFF000000
#define DRV_INFO_FLAG_HW_NOTIFY_TYPE_POLLING			0x0
#define DRV_INFO_FLAG_HW_NOTIFY_TYPE_INTERRUPT			0x1

#define SET_DRV_INFO_FLAG_HW_NOTIFY_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_HW_NOTIFY_TYPE_SHIFT) & DRV_INFO_FLAG_HW_NOTIFY_TYPE_MASK))

#define SSV_HIF_DRV_NAME_MAX_LEN	(32)	
#define SSV_CHIP_NAME_MAX_LEN		(32)	


#define HIF_DBG_PRINT(_pdata, format, args...)  \
    do { \
		if ((_pdata != NULL) && ((_pdata)->log_level)) \
			printk(format, ##args); \
    } while (0)

union ssv_drv_info
{
	struct 
	{
		u32 os_type:8;
		u32 register_type:8;
		u32 hw_type:8;
		u32 hw_notify_type:8;
	} fields;
	u32	flags;
};

struct ssv_drv_ops {
	/***************************************************************************/
	/*							Linux Driver								   */
	/***************************************************************************/	
	int __must_check (*read)(void *dev, void *rx_data_buf, size_t expec_rx_len);
	int __must_check (*write)(void *dev, void *buf, size_t len);
    int __must_check (*readreg)(void *dev, u32 addr, u32 *buf);
    int __must_check (*writereg)(void *dev, u32 addr, u32 buf);
    int __must_check (*load_fw)(void *dev, u32 start_addr, u8 *data, int data_length);

    void (*irq_enable)(void *dev);
    void (*irq_disable)(void *dev,bool iswaitirq);

	/***************************************************************************/
	/*								RTOS Driver								   */
	/***************************************************************************/
	bool	(*open)(void);
	bool	(*close)(void);
	bool	(*init)(void);
	// return
	//  < 0 : fail
	// >= 0 : # of bytes recieve
	s32 	(*recv)(u8 *dat, size_t len);
	s32 	(*send)(void *dat, size_t len);
	bool	(*get_name)(char name[32]);
	bool	(*ioctl)(u32 ctl_code, void *in_buf, size_t in_size, void *out_buf, size_t out_size, size_t *bytes_ret);
	u32		(*handle)(void);
    bool	(*ack_int)(void);
    bool    (*write_sram)(u32 addr, u8 *data, u32 size);
    bool    (*read_sram)(u32 addr, u8 *data, u32 size);
    bool    (*write_reg)(u32 addr, u32 data);
    u32     (*read_reg)(u32 addr);
    bool    (*write_byte)(u8 func,u32 addr, u8 data);
    u32    	(*read_byte)(u8 func,u32 addr);
	u32    	(*write_fw_to_sram)(u8 *bin, u32 bin_len, u32 block_size);
    s32    	(*start)(void);
    s32    	(*stop)(void);
	void    (*ssv_irq_enable)(void);
    void    (*ssv_irq_disable)(void);
    bool    (*wakeup_wifi)(bool sw);
    bool	(*detect_card)(void);
};

struct ssv_hif_drv {
	char name[SSV_HIF_DRV_NAME_MAX_LEN];
	union ssv_drv_info drv_info;
	struct ssv_drv_ops drv_ops;
};

enum ssv_hif_state {
    SSV6_HIF_POWER_OFF 		= 0,	
    SSV6_HIF_POWER_ON 		,
    SSV6_HIF_INIT			,
    SSV6_HIF_ACTIVE			,
    SSV6_HIF_SUSPEND		,
    SSV6_HIF_RESUME 		,
    SSV6_HIF_NON_SUPPORT 	,
};

struct ssv_hif_data {

	/****************** COMMON ********************/
    u8 chip_id[SSV_CHIP_NAME_MAX_LEN];
    u8 short_chip_id[SSV_CHIP_NAME_MAX_LEN];
	struct ssv_hif_drv *hif_drv;
    enum ssv_hif_state hif_state;
	
	u32 log_level;

	
#ifdef CONFIG_PM	
	void (*hif_suspend)(void *param);
	void (*hif_resume)(void *param);
	void *pm_param;
#endif
    void (*hif_reset)(void *param);    
    void (*platform_reset)(void *dev, u32 addr, u32 value);

	/******************   USB  ********************/
    u16 vendor;					/* vendor id */
    u16 device;					/* device id */
	void *usb_param;
    int (*start_usb_acc)(void *dev, u8 epnum);
    int (*stop_usb_acc)(void *dev, u8 epnum);
    int (*jump_to_rom)(void *dev);

	/******************  SDIO  ********************/
    int (*cmd52_read)(void *dev, u32 addr, u32 *value);
    int (*cmd52_write)(void *dev, u32 addr, u32 value);
	/******************   SPI  ********************/
};


#endif /* __HIF_WRAPPER_H__ */
