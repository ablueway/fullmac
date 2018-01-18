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


#ifndef __HWIF_H__
#define __HWIF_H__


#include <linux/mmc/host.h>
//#include <ssv6xxx_common.h> 
#include <linux/skbuff.h>

// general adress for chip id
#define SYS_REG_BASE           0xc0000000

#define ADR_CHIP_ID_0                          (SYS_REG_BASE+0x00000008)          
#define ADR_CHIP_ID_1                          (SYS_REG_BASE+0x0000000c)          
#define ADR_CHIP_ID_2                          (SYS_REG_BASE+0x00000010)          
#define ADR_CHIP_ID_3                          (SYS_REG_BASE+0x00000014) 

#define SSVCABRIO_PLAT_EEP_MAX_WORDS	2048

/* Hardware Interface Property */
//bit0: hwif capability. (0: interrupt, 1:polling)
//bit1: interface.       (0: SDIO, 1:USB)
//mask define
#define SSV_HWIF_CAPABILITY_MASK       0x00000001
#define SSV_HWIF_INTERFACE_MASK        0x00000002
//shift define
#define SSV_HWIF_CAPABILITY_SFT        0
#define SSV_HWIF_INTERFACE_SFT         1
//capability
#define SSV_HWIF_CAPABILITY_INTERRUPT  (0 << SSV_HWIF_CAPABILITY_SFT)
#define SSV_HWIF_CAPABILITY_POLLING    (1 << SSV_HWIF_CAPABILITY_SFT)
//interface
#define SSV_HWIF_INTERFACE_SDIO        (0 << SSV_HWIF_INTERFACE_SFT)
#define SSV_HWIF_INTERFACE_USB         (1 << SSV_HWIF_INTERFACE_SFT)


/**
* Macros for ssv6200 register read/write access on Linux platform.
* @ SSV_REG_WRITE() : write 4-byte value into hardware register.
* @ SSV_REG_READ()  : read 4-byte value from hardware register.
*             
*/
#define SSV_REG_WRITE(dev, reg, val) \
        (sh)->priv->ops->writereg((sh)->sc->dev, (reg), (val))
#define SSV_REG_READ(dev, reg, buf)  \
        (sh)->priv->ops->readreg((sh)->sc->dev, (reg), (buf))

#define HWIF_DBG_PRINT(_pdata, format, args...)  \
    do { \
		if ((_pdata != NULL) && ((_pdata)->dbg_control)) \
			printk(format, ##args); \
    } while (0)

 
struct sdio_scatter_req;
/**
* Hardware Interface (SDIO/SPI) APIs for ssv6200 on Linux platform.
*/
struct ssv6xxx_hwif_ops {
    int __must_check (*read)(void *dev, void *buf,size_t *size, int mode);
    int __must_check (*write)(void *dev, void *buf, size_t len,u8 queue_num);
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
    void (*irq_request)(void *dev,irq_handler_t irq_handler,void *irq_dev);
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
    int (*write_sram)(void *dev, u32 addr, u8 *data, u32 size);
    void (*interface_reset)(void *dev);    
    int (*start_usb_acc)(void *dev, u8 epnum);
    int (*stop_usb_acc)(void *dev, u8 epnum);
    int (*jump_to_rom)(void *dev);
    int (*property)(void *dev);
    void (*sysplf_reset)(void *dev, u32 addr, u32 value);
    void (*hwif_rx_task)(void *dev, int (*rx_cb)(struct sk_buff *rx_skb, void *args), void *args, u32 *pkt); 
};


struct ssv6xxx_platform_data {
    //use to avoid remove mmc cause dead lock.
    atomic_t                   irq_handling;
    bool                        is_enabled;
    u8                          chip_id[32];
    u8                          short_chip_id[33];
    unsigned short              vendor;		/* vendor id */
    unsigned short              device;		/* device id */
	struct ssv_unify_drv		*unify_drv; 			
	
	
	bool						dbg_control;

    // Application defined socket buffer allocation/free functions.
    struct sk_buff *(*skb_alloc) (void *param, s32 len, gfp_t gfp_mask);
    void (*skb_free) (void *param, struct sk_buff *skb);
    void *skb_param;

/* in */
#ifdef CONFIG_PM	
	// Application defined suspend/resume functions
	void (*suspend)(void *param);
	void (*resume)(void *param);
	void *pm_param;
#endif
    void (*enable_usb_acc)(void *param, u8 epnum);
    void (*disable_usb_acc)(void *param, u8 epnum);
    void (*jump_to_rom)(void *param);
    void *usb_param;

    int (*rx_burstread_size)(void *param);
    void *rx_burstread_param;
};

#endif /* __HWIF_H__ */
