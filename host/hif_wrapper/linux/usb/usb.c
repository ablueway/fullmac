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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/skbuff.h>



#include <ssv6200.h>

#include "usb.h"


struct ssv6xxx_usb_glue *usb_glue;


/* TODO(aaron): to intergrated with redbull,put here temply */
typedef enum{
    SSV6XXX_HWM_STA		,
    SSV6XXX_HWM_AP		,
    SSV6XXX_HWM_IBSS	,
    SSV6XXX_HWM_WDS	    ,
    SSV6XXX_HWM_SCONFIG ,
    SSV6XXX_HWM_INVALID	
} ssv6xxx_hw_mode;

extern int ssv6xxx_dev_init(ssv6xxx_hw_mode hmode);
extern void OS_StopScheduler(void);

void ssv_dev_init(struct work_struct *work)
{
	ssv6xxx_dev_init(SSV6XXX_HWM_STA);
}

  
static u16 ssv6xxx_get_cmd_sequence(struct ssv6xxx_usb_glue *usb_glue)
{
	usb_glue->sequence = usb_glue->sequence % USB_CMD_SEQUENCE;
	(usb_glue->sequence)++;

	return usb_glue->sequence;
}
	
static void ssv6xxx_usb_delete(struct kref *kref)
{
	struct ssv6xxx_usb_glue *usb_glue = to_ssv6xxx_usb_dev(kref);
	
	/* cancel urb */
	usb_kill_urb(usb_glue->rx_endpoint.rx_urb);

	/* free buffer */
	if (usb_glue->cmd_endpoint.buff) 
		kfree(usb_glue->cmd_endpoint.buff);
	if (usb_glue->rsp_endpoint.buff) 
		kfree(usb_glue->rsp_endpoint.buff);
	if (usb_glue->rx_endpoint.rx_buf)
		usb_free_coherent(usb_glue->udev, MAX_HCI_RX_AGGR_SIZE, 
				usb_glue->rx_endpoint.rx_buf, 
				usb_glue->rx_endpoint.rx_urb->transfer_dma);

	/* free urb */
	usb_free_urb(usb_glue->rx_endpoint.rx_urb);
	
	usb_put_dev(usb_glue->udev);
	kfree(usb_glue);
}

static int ssv6xxx_usb_recv_rsp(struct ssv6xxx_usb_glue *usb_glue, int size, int *rsp_len)
{
	int retval = 0, foolen = 0;

	if (!usb_glue || !usb_glue->interface) {
		retval = -ENODEV;
		return retval;
	}

	retval = usb_bulk_msg(usb_glue->udev, 
				usb_rcvbulkpipe(usb_glue->udev, usb_glue->rsp_endpoint.address),
				usb_glue->rsp_endpoint.buff, size,
				&foolen, TRANSACTION_TIMEOUT);
	if (retval) 
	{
		*rsp_len = 0;
		USB_DBG_PRINT(&usb_glue->hif_data, "Cannot receive response, error=%d\n", retval);
	} 
	else 
	{ 
		*rsp_len = foolen;
		usb_glue->err_cnt = 0;
	}	
	return retval;
}

static int ssv6xxx_usb_send_cmd(struct ssv6xxx_usb_glue *usb_glue, 
			u8 cmd, u16 seq, const void *data, u32 data_len)
{
	int retval = 0, foolen = 0;
	struct ssv6xxx_cmd_hdr *hdr;

	if (!usb_glue || !usb_glue->interface) {
		retval = -ENODEV;
		return retval;
	}
	
	/* fill the cmd context 
	 * packet format
	 * =============================================
	 * |   plen   |   cmd    |    seq   | payload  |
	 * |  1 byte  |  1 byte  |  2bytes  |          |
	 * =============================================   
	 * */
	hdr = (struct ssv6xxx_cmd_hdr *)usb_glue->cmd_endpoint.buff;
	memset(hdr, 0, sizeof(struct ssv6xxx_cmd_hdr));

	hdr->plen = (data_len >> (0))& 0xff;
	hdr->cmd = cmd;
	hdr->seq = cpu_to_le16(seq);
	memcpy(&hdr->payload, data, data_len);

	retval = usb_bulk_msg(usb_glue->udev, 
					usb_sndbulkpipe(usb_glue->udev, usb_glue->cmd_endpoint.address),
					usb_glue->cmd_endpoint.buff, (data_len+SSV6XXX_CMD_HEADER_SIZE),
					&foolen, TRANSACTION_TIMEOUT);

	/* TODO(aaron): should we use the foolen to protect the bug-free ? */
	if (retval) { 
		USB_DBG_PRINT(&usb_glue->hif_data, 
			"Cannot send cmd data, error=%d\n", retval);
	} else {
		usb_glue->err_cnt = 0;
	}
	return retval;
}

static int ssv6xxx_usb_cmd(struct ssv6xxx_usb_glue *usb_glue, u8 cmd, 
							void *data, u32 data_len, void *result)
{

	int retval = (-1), rsp_len = 0, i = 0;
	struct ssv6xxx_cmd_hdr *rsphdr;
	u16 sequence;

	mutex_lock(&usb_glue->cmd_mutex);
	sequence = ssv6xxx_get_cmd_sequence(usb_glue);
	retval = ssv6xxx_usb_send_cmd(usb_glue, cmd, sequence, data, data_len);

	if (retval) {
		USB_DBG_PRINT(&usb_glue->hif_data, 
			"%s: Fail to send cmd, sequence=%d, retval=%d\n", 
				__FUNCTION__, sequence, retval);
		goto exit;
	}

	/* TODO(aaron): why we need to read all previous event status ?? 
					should we take the sequence number for reference ? */
	/* If cmd error(result is device buff), we should read previous result. */
	for (i = 0; i < USB_CMD_SEQUENCE; i++) {
		retval = ssv6xxx_usb_recv_rsp(usb_glue, SSV6XXX_MAX_RXCMDSZ, &rsp_len);
		if (retval) {
			USB_DBG_PRINT(&usb_glue->hif_data, 
				"%s: Fail to receive response, sequence=%d, retval=%d\n", 
				__FUNCTION__, sequence, retval);			
			goto exit;
		}
		/* parse the response context */
		if (rsp_len < SSV6XXX_CMD_HEADER_SIZE) {
			USB_DBG_PRINT(&usb_glue->hif_data, 
				"Receviced abnormal response length[%d]\n", rsp_len);
			goto exit; 
		}
		rsphdr = (struct ssv6xxx_cmd_hdr *)usb_glue->rsp_endpoint.buff;
		if (sequence == rsphdr->seq) {
			break;
		} else {
			USB_DBG_PRINT(&usb_glue->hif_data, 
				"received incorrect sequence=%d[%d]\n", sequence, rsphdr->seq);
		}
	}
	/* TODO(aaron): should this be packed in one function call ? */
	switch (rsphdr->cmd) {
		case SSV6200_CMD_WRITE_REG:
			break;
		case SSV6200_CMD_READ_REG:
			if (result)
				memcpy(result, &rsphdr->payload, sizeof(struct ssv6xxx_read_reg_result));
			break;
		default:
			retval = -1;
			USB_DBG_PRINT(&usb_glue->hif_data, 
				"%s: unknown response cmd[%d]\n", __FUNCTION__, rsphdr->cmd);
			break;
	}

exit:
	mutex_unlock(&usb_glue->cmd_mutex);
	return retval;
}


/* TODO(aaron): change the usb rx mechanism for redbull host linux drv archecture */
static int __must_check ssv6xxx_usb_read(void *dev, void *rx_data_buf, size_t expec_rx_len)
{
	int retval = 0;
	int rx_actul_len = 0;
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;

	retval = usb_bulk_msg(usb_glue->udev,
				usb_rcvbulkpipe(usb_glue->udev, usb_glue->rx_endpoint.address),
				rx_data_buf,
				expec_rx_len,
				&rx_actul_len,
				TRANSACTION_TIMEOUT);
	if (!retval)
	{
		USB_DBG_PRINT(&usb_glue->hif_data,
			"recv rx data len=%d\n", rx_actul_len);
		return rx_actul_len;
	}
	else
	{
		USB_DBG_PRINT(&usb_glue->hif_data, 
			"usb rx fail, error code(%d)\n", retval);
		return 0;
	}	
}

/* TODO(aaron): change the usb tx mechanism for redbull host linux drv archecture */
static int __must_check ssv6xxx_usb_write(void *dev, void *buf, size_t len)
{
	int retval = 0;
	int tx_len = len;
	int tx_actul_len = 0;

	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;

	/* for USB 3.0 port, add padding byte and let host driver send short packet */
	if ((len % usb_glue->tx_endpoint.packet_size) == 0) {
		tx_len++;
	}

	retval = usb_bulk_msg(usb_glue->udev, 
				usb_sndbulkpipe(usb_glue->udev, usb_glue->tx_endpoint.address),
				buf, len, &tx_actul_len, TRANSACTION_TIMEOUT);
	if (!retval) 
	{
		USB_DBG_PRINT(&usb_glue->hif_data,
			"send tx data len=%d\n", tx_actul_len);
		return tx_actul_len;
	}
	else
	{
		USB_DBG_PRINT(&usb_glue->hif_data,
			"usb tx fail, error code(%d)\n", retval);
		return 0;
	}	
}

static int __must_check ssv6xxx_usb_read_reg(void *dev, u32 addr, u32 *buf)
{
    int retval = (-1);
	struct ssv6xxx_read_reg read_reg;
	struct ssv6xxx_read_reg_result result;
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;
	
	if (IS_USB_GLUE_INVALID(usb_glue)) {
		/* TODO(aaron):  should we add debug msg here ? */		
		return retval;
	}    	
	memset(&read_reg, 0, sizeof(struct ssv6xxx_read_reg));	
	memset(&result, 0, sizeof(struct ssv6xxx_read_reg_result));	

	read_reg.addr = cpu_to_le32(addr);
	retval = ssv6xxx_usb_cmd(usb_glue, SSV6200_CMD_READ_REG, &read_reg, 
						sizeof(struct ssv6xxx_read_reg), &result);

	if (!retval) {
		*buf = le32_to_cpu(result.value);
	} else { 
		*buf = 0xffffffff;
		USB_DBG_PRINT(&usb_glue->hif_data, 
			"%s: Fail to read register address %x\n", __FUNCTION__, addr);
	}

	return retval;
}

static int __must_check ssv6xxx_usb_write_reg(void *dev, u32 addr, u32 buf)
{
	int retval = (-1);
	struct ssv6xxx_write_reg write_reg;	
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;

	if (IS_USB_GLUE_INVALID(usb_glue))
		return retval;
	
	memset(&write_reg, 0, sizeof(struct ssv6xxx_write_reg));	
	write_reg.addr = cpu_to_le32(addr);
	write_reg.value = cpu_to_le32(buf);
	retval = ssv6xxx_usb_cmd(usb_glue, SSV6200_CMD_WRITE_REG, &write_reg, 
								sizeof(struct ssv6xxx_write_reg), NULL);
	if (retval) 
	{ 
		USB_DBG_PRINT(&usb_glue->hif_data, 
			"%s: Fail to write register address %x, value %x\n", 
			__FUNCTION__, addr, buf);
	}
	return retval;
}

static int ssv6xxx_usb_load_firmware(void *dev, u32 start_addr, u8 *data, int data_length)
{
	//struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;
	u16 laddr, haddr;
	u32 addr;
	int retval = 0, max_usb_block = 512;
	u8 *pdata;
	int res_length, offset, send_length;

    USB_DBG_PRINT(&usb_glue->hif_data,
		"start_addr = 0x%x, data_length = 0x%x\n", 
		start_addr, data_length);    

	if (IS_USB_GLUE_INVALID(usb_glue))
		return -1;

	offset = 0;
	pdata = data;
	addr = start_addr;
	res_length = data_length;
	
	while (offset < data_length) {
		int transfer = min_t(int, res_length, max_usb_block);
		laddr = (addr & 0x0000ffff);
		haddr = (addr >> 16);
		send_length = usb_control_msg(usb_glue->udev, usb_sndctrlpipe(usb_glue->udev, 0),
			FIRMWARE_DOWNLOAD, (USB_DIR_OUT | USB_TYPE_VENDOR),
			laddr, haddr, pdata, transfer, TRANSACTION_TIMEOUT);
	
		if (send_length < 0) {
			retval = send_length;
			USB_DBG_PRINT(&usb_glue->hif_data, 
				"Load Firmware Fail, retval=%d, sram=0x%08x\n", 
				retval, (laddr|haddr));
			break;
		}

		addr += transfer;
		pdata += transfer;
		offset += transfer;
		res_length -= transfer;
	}
	return retval;
}

static void ssv6xxx_usb_get_chip_id(struct ssv6xxx_usb_glue *usb_glue)
{
    u32 regval;
    int ret;
    u8 _chip_id[SSV6XXX_CHIP_ID_LENGTH];
    u8 *c = _chip_id;
    int i = 0;

    //CHIP ID
    // Chip ID registers should be common to all SSV6xxx devices. So these registers 
    // must not come from ssv6xxx_reg.h but defined somewhere else.
	ret = ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_3, &regval);
    *((u32 *)&_chip_id[0]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_2, &regval);
    *((u32 *)&_chip_id[4]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_1, &regval);
    *((u32 *)&_chip_id[8]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_0, &regval);
    *((u32 *)&_chip_id[12]) = __be32_to_cpu(regval);

    _chip_id[12 + sizeof(u32)] = 0;

    // skip null for turimo fpga chip_id bug)
    while (*c == 0) {
        i++;
        c++;
        if (i == 16) { // max string length reached.
            c = _chip_id;
            break;
        }
    }

	/* TODO(aaron): no error handler ? */
    if (*c != 0) {		
		/* TODO(aaron): why we can specific strncopy len 24 not 17 ? */
        strncpy(usb_glue->hif_data.chip_id, c, SSV6XXX_CHIP_ID_LENGTH);
		/* SSV6XXX_USB 1-2:1.0: CHIP ID: SSV6006C0 */
		dev_info(usb_glue->dev, "CHIP ID: %s \n", usb_glue->hif_data.chip_id);
		printk("CHIP ID: %s \n", usb_glue->hif_data.chip_id);		
        strncpy(usb_glue->hif_data.short_chip_id, c, SSV6XXX_CHIP_ID_SHORT_LENGTH);
		/* why need do this ?? maybe set 0 in SSV6XXX_CHIP_ID_SHORT_LENGTH+1 ?? */
        usb_glue->hif_data.short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH] = 0;
    } else {
        dev_err(usb_glue->dev, "Failed to read chip ID");
        usb_glue->hif_data.chip_id[0] = 0;
    }
}

#if 0
static int ssv6xxx_usb_jump_to_rom(void *dev)
{
    //struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;
	
    if (IS_USB_GLUE_INVALID(usb_glue)) {
		USB_DBG_PRINT(&usb_glue->hif_data,"failed to jump to ROM\n");
		return -1;
    }
    
    usb_glue->hif_data.jump_to_rom(usb_glue->hif_data.usb_param);

	return 0;
}


static void ssv6xxx_usb_sysplf_reset(void *dev, u32 addr, u32 value)
{
	u16 sequence;
	struct ssv6xxx_write_reg write_reg;
	int retval = (-1), rsp_len = 0;
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;


    if (IS_USB_GLUE_INVALID(usb_glue)) 
        return;
    
	mutex_lock(&usb_glue->cmd_mutex);
	sequence = ssv6xxx_get_cmd_sequence(usb_glue);
	memset(&write_reg, 0, sizeof(struct ssv6xxx_write_reg));	
	write_reg.addr = cpu_to_le32(addr);
	write_reg.value = cpu_to_le32(value);

    /* 
     * Reset sysplf will causes sequence error.
     * It makes use of the ssv6xxx_usb_send_cmd and ssv6xxx_usb_recv_rsp to complete the command process,
     * instead of common api - ssv6xxx_usb_cmd.
     */
	retval = ssv6xxx_usb_send_cmd(usb_glue, SSV6200_CMD_WRITE_REG, 
			sequence, &write_reg, sizeof(struct ssv6xxx_write_reg));
	if (retval) {  
	    USB_DBG_PRINT(&usb_glue->hif_data, 
	    "%s: Fail to reset sysplf\n", __FUNCTION__);
	}
    retval = ssv6xxx_usb_recv_rsp(usb_glue, SSV6XXX_MAX_RXCMDSZ, &rsp_len);
	
    mutex_unlock(&usb_glue->cmd_mutex);
}
#endif

static void ssv6xxx_usb_irq_enable(void *dev)
{
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;

    if (IS_USB_GLUE_INVALID(usb_glue)) 
        return;

    USB_DBG_PRINT(&usb_glue->hif_data,"not support %s()\n", __func__);
}

static void ssv6xxx_usb_irq_disable(void *dev, bool iswaitirq)
{
	struct ssv6xxx_usb_glue *usb_glue = (struct ssv6xxx_usb_glue *)dev;

    if (IS_USB_GLUE_INVALID(usb_glue)) 
        return;
	
	USB_DBG_PRINT(&usb_glue->hif_data, "not support %s()\n", __func__);	
}

struct ssv_hif_drv usb_ops = 
{
	.name = "SSV6XXX_USB",
	.drv_info.flags = UNIFY_LINUX_USB_DRV_INFO_FLAGS,
	.drv_ops = 
	{
	    .read            		= ssv6xxx_usb_read,
	    .write           		= ssv6xxx_usb_write,
	    .readreg	     		= ssv6xxx_usb_read_reg,
	    .writereg        		= ssv6xxx_usb_write_reg,
	    .load_fw         		= ssv6xxx_usb_load_firmware,
		.irq_enable				= ssv6xxx_usb_irq_enable,
		.irq_disable			= ssv6xxx_usb_irq_disable,		
	}
};

static int ssv_usb_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct ssv_hif_data *p_hif_data;
	struct usb_host_interface *iface_desc;
	int ep_idx = 0 ;
	int retval = -ENOMEM;

    printk(KERN_INFO "==>%s()\n", __func__);
	printk(KERN_INFO "=======================================\n");
	printk(KERN_INFO "== TURISMO With Redbull Project-USB  ==\n");
	printk(KERN_INFO "=======================================\n");

    /* allocate memory for our device state and initialize it */
	usb_glue = kzalloc(sizeof(*usb_glue), GFP_KERNEL);
	if (!usb_glue) {
		dev_err(&interface->dev, "Out of memory\n");
		goto error;
	}
	p_hif_data = &usb_glue->hif_data;

	SET_USB_HIF_STATE(p_hif_data, SSV6_HIF_POWER_ON);
	
	usb_glue->sequence = 0;
	usb_glue->err_cnt = 0;
	kref_init(&usb_glue->kref);
	mutex_init(&usb_glue->io_mutex);
	mutex_init(&usb_glue->cmd_mutex);

	INIT_WORK(&usb_glue->dev_init, ssv_dev_init);

	/* point to the device of usb interface which use in the device tree */
	usb_glue->dev = &interface->dev;
	/* USB core needs to know usb_device, so get usb_device form usb_interface */
	usb_glue->udev = usb_get_dev(interface_to_usbdev(interface));
	/* TODO: aaron: we already store the interface why need keep the dev ?*/
	usb_glue->interface = interface;
	usb_glue->dev_ready = true;

	/* Tell PM core that we don't need the card to be powered now */ 
//	p_hif_data = &usb_glue->hif_data;
	memset(p_hif_data, 0, sizeof(struct ssv_hif_data));

	SET_USB_HIF_STATE(p_hif_data, SSV6_HIF_INIT);

	/* Set verdor/product id */
	p_hif_data->vendor = id->idVendor;
	p_hif_data->device = id->idProduct;
	
	/* Set hwif operation */
	p_hif_data->hif_drv = &usb_ops;

	/* setup the endpoint information */
	iface_desc = interface->cur_altsetting;

	for (ep_idx = 0; ep_idx < iface_desc->desc.bNumEndpoints; ++ep_idx) {
		struct usb_endpoint_descriptor *endpoint = &iface_desc->endpoint[ep_idx].desc;	
		unsigned int epnum = endpoint->bEndpointAddress & 0x0f;   

		if (epnum == SSV_EP_CMD) {
			usb_glue->cmd_endpoint.address = endpoint->bEndpointAddress;
			usb_glue->cmd_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
			usb_glue->cmd_endpoint.buff = kmalloc(SSV6XXX_MAX_TXCMDSZ, GFP_ATOMIC);
			if (!usb_glue->cmd_endpoint.buff) {
				dev_err(&interface->dev, "Could not allocate cmd buffer\n");
				goto error;
			}
		} 
		if (epnum == SSV_EP_RSP) {
			usb_glue->rsp_endpoint.address = endpoint->bEndpointAddress;
			usb_glue->rsp_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
			usb_glue->rsp_endpoint.buff = kmalloc(SSV6XXX_MAX_RXCMDSZ, GFP_ATOMIC);
			if (!usb_glue->rsp_endpoint.buff) {
				dev_err(&interface->dev, "Could not allocate rsp buffer\n");
				goto error;
			}
		} 	
		if (epnum == SSV_EP_TX) {
			usb_glue->tx_endpoint.address = endpoint->bEndpointAddress;
			usb_glue->tx_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
        } 		
		if (epnum == SSV_EP_RX) {
			
			usb_glue->rx_endpoint.address = endpoint->bEndpointAddress;
			usb_glue->rx_endpoint.packet_size = le16_to_cpu(endpoint->wMaxPacketSize);
			usb_glue->rx_endpoint.rx_urb = usb_alloc_urb(0, GFP_ATOMIC);
			
			if (!usb_glue->rx_endpoint.rx_urb) {
				dev_err(&interface->dev, "Could not allocate rx urb\n");
				goto error;
			}
			
			usb_glue->rx_endpoint.rx_buf = usb_alloc_coherent(
							usb_glue->udev, MAX_HCI_RX_AGGR_SIZE, 
							GFP_ATOMIC, &usb_glue->rx_endpoint.rx_urb->transfer_dma);

			if (!usb_glue->rx_endpoint.rx_buf) {
				dev_err(&interface->dev, "Could not allocate rx buffer\n");
				goto error;
			}
		}
	}
	if (!(usb_glue->cmd_endpoint.address && 
		  usb_glue->rsp_endpoint.address && 
		  usb_glue->tx_endpoint.address && 
		  usb_glue->rx_endpoint.address)) 
	{
		dev_err(&interface->dev, "Could not find all endpoints\n");
		goto error;
	}

	/* save our data pointer in the driver_data of dev of the interface device */
	usb_set_intfdata(interface, usb_glue);

	SET_USB_HIF_STATE(p_hif_data, SSV6_HIF_ACTIVE);

	ssv6xxx_usb_get_chip_id(usb_glue);

	schedule_work(&usb_glue->dev_init);	 
	
	return 0;

error:
	if (usb_glue)
		/* this frees allocated memory */
		kref_put(&usb_glue->kref, ssv6xxx_usb_delete);

	return retval;
}

static void ssv_usb_disconnect(struct usb_interface *interface)
{
	struct ssv6xxx_usb_glue *usb_glue = usb_get_intfdata(interface);
	struct ssv_hif_data *p_hif_data = &usb_glue->hif_data;
	
	usb_set_intfdata(interface, NULL);

	if (usb_glue) {
		usb_glue->dev_ready = false;
		SET_USB_HIF_STATE(p_hif_data, SSV6_HIF_POWER_OFF);
	}

	/* prevent more I/O from starting */
	mutex_lock(&usb_glue->io_mutex);
	usb_glue->interface = NULL;
	mutex_unlock(&usb_glue->io_mutex);

	/* decrement our usage count */
	kref_put(&usb_glue->kref, ssv6xxx_usb_delete);

	dev_info(&interface->dev, "SSV USB is disconnected");
}

/* in */
#ifdef CONFIG_PM
static int ssv_usb_suspend(struct usb_interface *interface, pm_message_t message)
{
	struct ssv6xxx_usb_glue *usb_glue = usb_get_intfdata(interface);

	dev_info(usb_glue->dev, "%s(): suspend.\n", __FUNCTION__);

	if (!usb_glue) 
	{
		return -1;
	}
	if (usb_glue->hif_data.hif_suspend != NULL)
	{
		usb_glue->hif_data.hif_suspend(usb_glue->hif_data.pm_param);
		/* cancel urb */
		usb_kill_urb(usb_glue->rx_endpoint.rx_urb);
	}
	return 0;
}

static int ssv_usb_resume(struct usb_interface *interface)
{
	struct ssv6xxx_usb_glue *usb_glue = usb_get_intfdata(interface);

	dev_info(usb_glue->dev, "%s(): resume.\n", __FUNCTION__);

	if (!usb_glue)
	{
		return -1;
	}

	if (usb_glue->hif_data.hif_resume != NULL)
	{
		usb_glue->hif_data.hif_resume(usb_glue->hif_data.pm_param);
	}
	return 0;
}
#endif

/* table of devices that work with this driver */
static const struct usb_device_id ssv_usb_table[] = 
{
	{USB_DEVICE(USB_SSV_VENDOR_ID, USB_SSV_PRODUCT_ID)},
	{}					
};
MODULE_DEVICE_TABLE(usb, ssv_usb_table);

static struct usb_driver ssv_usb_driver = {
	.name 						= "SSV6XXX_USB",
	.probe 						= ssv_usb_probe,
	.disconnect 				= ssv_usb_disconnect,
	
#ifdef CONFIG_PM
	.suspend 					= ssv_usb_suspend,
	.resume 					= ssv_usb_resume,
#endif

	.id_table 					= ssv_usb_table,
	.supports_autosuspend 		= 1,
    .disable_hub_initiated_lpm 	= 0,
};

static int __init ssv6xxx_usb_init(void) 
{
	printk(KERN_INFO "ssv6xxx_usb_init(register ssv_usb_driver)\n");
	return usb_register(&ssv_usb_driver);
}

static void __exit ssv6xxx_usb_exit(void) 
{
    printk(KERN_INFO "ssv6xxx_usb_exit\n");
    printk(KERN_INFO "stop all kernel tasks\n");	
	OS_StopScheduler();
    printk(KERN_INFO "deregister usb driver\n");
	usb_deregister(&ssv_usb_driver);
}
 
module_init(ssv6xxx_usb_init);
module_exit(ssv6xxx_usb_exit); 

MODULE_LICENSE("GPL");
