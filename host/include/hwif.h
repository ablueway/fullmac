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

//#include <ssv6xxx_common.h> 

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