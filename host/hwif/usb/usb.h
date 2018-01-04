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
 
#endif /* _USB_DEF_H_ */
