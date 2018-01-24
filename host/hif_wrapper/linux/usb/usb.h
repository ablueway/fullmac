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

#ifndef _USB_DEF_H_
#define _USB_DEF_H_

#include "hif_wrapper.h"

/* debug message */
#define USB_DBG(fmt, ...)    pr_debug(fmt "\n", ##__VA_ARGS__)

#define FW_START_ADDR				0x00

#define	FIRMWARE_DOWNLOAD			0xf0

/* endpoint number */
#define SSV_EP_CMD               0x01	//bulk out
#define SSV_EP_RSP               0x02	//bulk in
#define SSV_EP_TX                0x03	//bulk out
#define SSV_EP_RX                0x04	//bulk in

/* Define CMD */
#define SSV6200_CMD_WRITE_REG		0x01
#define SSV6200_CMD_READ_REG		0x02


/* Define these values to match devices */
#define USB_SSV_VENDOR_ID  0x8065
#define USB_SSV_PRODUCT_ID 0x6000

#define TRANSACTION_TIMEOUT			(3000) /* ms */
#define SSV6XXX_MAX_TXCMDSZ			(sizeof(struct ssv6xxx_cmd_hdr))
#define SSV6XXX_MAX_RXCMDSZ			(sizeof(struct ssv6xxx_cmd_hdr))
#define SSV6XXX_CMD_HEADER_SIZE		(sizeof(struct ssv6xxx_cmd_hdr) - sizeof(union ssv6xxx_payload))
#define USB_CMD_SEQUENCE			255
#define MAX_RETRY_SSV6XXX_ALLOC_BUF	3


#define SET_USB_HIF_STATE(p_hif_data, state) (p_hif_data->hif_state = state)

#define IS_USB_GLUE_INVALID(usb_glue)  \
      ((usb_glue == NULL) \
		|| (usb_glue->hif_data.hif_state != SSV6_HIF_ACTIVE) \
      )

#define UNIFY_LINUX_USB_DRV_INFO_FLAGS ((DRV_INFO_FLAG_OS_TYPE_LINUX << DRV_INFO_FLAG_OS_TYPE_SHIFT) || \
					(DRV_INFO_FLAG_REGISTER_TYPE_PASSIVE << DRV_INFO_FLAG_REGISTER_TYPE_SHIFT) || \
					(DRV_INFO_FLAG_HW_TYPE_USB << DRV_INFO_FLAG_HW_TYPE_SHIFT))

#define to_ssv6xxx_usb_dev(d) container_of(d, struct ssv6xxx_usb_glue, kref)

#define GET_SSV6XXX_USB_SPEED(usb_glue) ((usb_glue)->udev->speed)

// general adress for chip id
#define SYS_REG_BASE           0xc0000000

#define ADR_CHIP_ID_0                          (SYS_REG_BASE+0x00000008)          
#define ADR_CHIP_ID_1                          (SYS_REG_BASE+0x0000000c)          
#define ADR_CHIP_ID_2                          (SYS_REG_BASE+0x00000010)          
#define ADR_CHIP_ID_3                          (SYS_REG_BASE+0x00000014) 

#define SSVCABRIO_PLAT_EEP_MAX_WORDS	2048

#define USB_DBG_PRINT 	HIF_DBG_PRINT

struct ssv6xxx_read_reg_result {
	u32		value;	
}__attribute__ ((packed));

struct ssv6xxx_read_reg {
	u32		addr;
	u32		value;
}__attribute__ ((packed));

struct ssv6xxx_write_reg {
	u32		addr;
	u32		value;
}__attribute__ ((packed));

union ssv6xxx_payload {
	struct ssv6xxx_read_reg		rreg;
	struct ssv6xxx_read_reg_result rreg_res;
	struct ssv6xxx_write_reg	wreg;
};

/* TODO(aaron): can we modify the struct name, why hdr include payload ?? */
struct ssv6xxx_cmd_hdr {
	u8		plen;
	u8		cmd;
	u16		seq;
	union ssv6xxx_payload  payload;
}__attribute__ ((packed));

struct ssv6xxx_cmd_endpoint {
	u8	 				address;
	u16					packet_size;
	void 				*buff;
};

struct ssv6xxx_tx_endpoint {
	u8					address;
	u16					packet_size;				
	int					tx_res;
};

struct ssv6xxx_rx_endpoint {
	u8	 				address;
	struct urb			*rx_urb;		/* the urb to read data with */
	u16					packet_size;				
	void			  	*rx_buf;		/* the buffer to receive data */
	unsigned int		rx_filled;		/* number of bytes which we receive in the buffer */
	int					rx_res;
};

/* Structure to hold all of our device specific stuff */
struct ssv6xxx_usb_glue {
	struct device                   *dev;			/* driver model's view of this device of usb interface */
	struct usb_device               *udev;			/* the usb device for this device */
	struct usb_interface            *interface;		/* the usb interface device for this device */
	struct ssv_hif_data     		hif_data;		/* for hif driver common data */
	struct ssv6xxx_cmd_endpoint      cmd_endpoint;	/* command endpoint */
	struct ssv6xxx_cmd_endpoint      rsp_endpoint;	/* response endpoint */
	struct ssv6xxx_tx_endpoint       tx_endpoint;	/* tx endpoint */
	struct ssv6xxx_rx_endpoint		rx_endpoint;	/* rx endpoint */
	struct kref		kref;
	struct mutex	io_mutex;						/* synchronize I/O with disconnect */
	struct mutex	cmd_mutex;						/* blocking cmd/rsp */
	u16	   sequence;								/* keep global usb cmd sequence number to assign */
	u16	   err_cnt;									/* TODO(aaron): need SW handle retry ?? */
	bool   dev_ready;
	struct work_struct dev_init;
};
 
#endif /* _USB_DEF_H_ */
