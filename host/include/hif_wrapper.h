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


#define DRV_INFO_FLAG_REGISTER_TYPE_SHIFT		0x8
#define DRV_INFO_FLAG_REGISTER_TYPE_MASK		0x0000FF00
#define DRV_INFO_FLAG_REGISTER_TYPE_PASSIVE		0x0
#define DRV_INFO_FLAG_REGISTER_TYPE_ACTIVE		0x1

#define SET_DRV_INFO_FLAG_REGISTER_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_REGISTER_TYPE_SHIFT) & DRV_INFO_FLAG_REGISTER_TYPE_MASK))


#define DRV_INFO_FLAG_HW_TYPE_SHIFT				0x10
#define DRV_INFO_FLAG_HW_TYPE_MASK				0x00FF0000
#define DRV_INFO_FLAG_HW_TYPE_USB				0x0
#define DRV_INFO_FLAG_HW_TYPE_SDIO				0x1
#define DRV_INFO_FLAG_HW_TYPE_SPI				0x2

#define SET_DRV_INFO_FLAG_HW_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_HW_TYPE_SHIFT) & DRV_INFO_FLAG_HW_TYPE_MASK))


#define UNIFY_DRV_NAME_MAX_LEN	(32)	


union unify_drv_info
{
	struct 
	{
		u32 os_type:8;
		u32 register_type:8;
		u32 hw_type:8;
		u32 rsvd:8;
	} fields;
	u32	flags;
};

struct unify_drv_ops {
	/***************************************************************************/
	/*							Linux Driver								   */
	/***************************************************************************/	
	int __must_check (*read)(void *dev, void *rx_data_buf, size_t expec_rx_len);
	int __must_check (*write)(void *dev, void *buf, size_t len);
    int __must_check (*readreg)(void *dev, u32 addr, u32 *buf);
    int __must_check (*writereg)(void *dev, u32 addr, u32 buf);
    int __must_check (*safe_readreg)(void *dev, u32 addr, u32 *buf);
    int __must_check (*safe_writereg)(void *dev, u32 addr, u32 buf);
    int __must_check (*burst_readreg)(void *dev, u32 *addr, u32 *buf, u8 reg_amount);
    int __must_check (*burst_writereg)(void *dev, u32 *addr, u32 *buf, u8 reg_amount);    
    int __must_check (*burst_safe_readreg)(void *dev, u32 *addr, u32 *buf, u8 reg_amount);
    int __must_check (*burst_safe_writereg)(void *dev, u32 *addr, u32 *buf, u8 reg_amount);    

	int (*trigger_tx_rx)(void *dev);
    int (*irq_getmask)(void *dev, u32 *mask);
    void (*irq_setmask)(void *dev,int mask);
    void (*irq_enable)(void *dev);
    void (*irq_disable)(void *dev,bool iswaitirq);
    int (*irq_getstatus)(void *dev,int *status);
    void (*irq_request)(void *dev, s32 (*irq_handler)(int, void *),void *irq_dev);
    void (*irq_trigger)(void *dev);

	void (*pmu_wakeup)(void *dev);
    int __must_check (*load_fw)(void *dev, u32 start_addr, u8 *data, int data_length);
    void (*load_fw_pre_config_device)(void *dev);
    void (*load_fw_post_config_device)(void *dev);
    int (*cmd52_read)(void *dev, u32 addr, u32 *value);
    int (*cmd52_write)(void *dev, u32 addr, u32 value);
    bool (*support_scatter)(void *dev);    
    int (*rw_scatter)(void *dev, void *scat_req);
    bool (*is_ready)(void *dev);
    int (*dev_write_sram)(void *dev, u32 addr, u8 *data, u32 size);
    void (*interface_reset)(void *dev);    
    int (*start_usb_acc)(void *dev, u8 epnum);
    int (*stop_usb_acc)(void *dev, u8 epnum);
    int (*jump_to_rom)(void *dev);
    int (*property)(void *dev);
    void (*sysplf_reset)(void *dev, u32 addr, u32 value);
   	void (*hwif_rx_task)(void *dev, int (*rx_cb)(void *rx_skb, void *args), void *args); 
	/***************************************************************************/
	/*								RTOS Driver								   */
	/***************************************************************************/
	// SSV6XXX_DRV_TYPE		type;
	bool	(*open)(void);
	bool	(*close)(void);
	bool	(*init)(void);
	// return
	//  < 0 : fail
	// >= 0 : # of bytes recieve
	s32 	(*recv)(u8 *dat, size_t len);
    // return
	//  < 0 : fail
	// >= 0 : # of bytes send
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

struct ssv_unify_drv {
	
	char name[UNIFY_DRV_NAME_MAX_LEN];
	union unify_drv_info drv_info;
	struct unify_drv_ops drv_ops;
};

#endif /* __HIF_WRAPPER_H__ */
