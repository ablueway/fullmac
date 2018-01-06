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
//ToDo Liam this is temp for build.
// it should re-consider for multiple chip use.
#include <hwif/hwif.h>
#include "usb.h"

#include <hif_wrapper.h>

//#include "ssv_common.h"
/* TODO:aaron */
typedef enum{
    SSV6XXX_HWM_STA		,
    SSV6XXX_HWM_AP		,
    SSV6XXX_HWM_IBSS	,
    SSV6XXX_HWM_WDS	    ,
    SSV6XXX_HWM_SCONFIG ,
    SSV6XXX_HWM_INVALID	
}ssv6xxx_hw_mode;
extern int ssv6xxx_dev_init(ssv6xxx_hw_mode hmode);

/* Define these values to match devices */
#define USB_SSV_VENDOR_ID  0x8065
#define USB_SSV_PRODUCT_ID 0x6000

#define TRANSACTION_TIMEOUT			(3000) /* ms */
#define SSV6XXX_MAX_TXCMDSZ			(sizeof(struct ssv6xxx_cmd_hdr))
#define SSV6XXX_MAX_RXCMDSZ			(sizeof(struct ssv6xxx_cmd_hdr))
#define SSV6XXX_CMD_HEADER_SIZE		(sizeof(struct ssv6xxx_cmd_hdr) - sizeof(union ssv6xxx_payload))
#define USB_CMD_SEQUENCE			255
#define MAX_RETRY_SSV6XXX_ALLOC_BUF	3


#define IS_GLUE_INVALID(usb_glue)  \
      ((usb_glue == NULL) \
       || (usb_glue->dev_ready == false) \
       || (usb_glue->wlan_data.is_enabled == false) \
      )


/* table of devices that work with this driver */
static const struct usb_device_id ssv_usb_table[] = {
	{ USB_DEVICE(USB_SSV_VENDOR_ID, USB_SSV_PRODUCT_ID) },
	{ }					
};
MODULE_DEVICE_TABLE(usb, ssv_usb_table);

/* Structure to hold all of our device specific stuff */
struct ssv6xxx_usb_glue {
	struct device                   *dev;			/* driver model's view of this device of usb interface */
	struct usb_device               *udev;			/* the usb device for this device */
	struct usb_interface            *interface;		/* the usb interface device for this device */
	struct ssv6xxx_platform_data     wlan_data;
	struct ssv6xxx_cmd_endpoint      cmd_endpoint;	/* command endpoint */
	struct ssv6xxx_cmd_endpoint      rsp_endpoint;	/* response endpoint */
	struct ssv6xxx_tx_endpoint       tx_endpoint;	/* tx endpoint */
	struct ssv6xxx_rx_endpoint		rx_endpoint;	/* rx endpoint */
	struct kref		kref;
	struct mutex	io_mutex;						/* synchronize I/O with disconnect */
	struct mutex	cmd_mutex;						/* blocking cmd/rsp */
	u16	   sequence;
	u16	   err_cnt;									/*  TODO(aaron): need SW handle retry ?? */
	bool   dev_ready;
	struct workqueue_struct *wq;					/* receive rx workqueue */
	struct work_struct rx_work;
	u32 *rx_pkt;
	void *rx_cb_args;
	bool rx_wq_running;
    int (*rx_cb)(struct sk_buff *rx_skb, void *args);
};
#define to_ssv6xxx_usb_dev(d) container_of(d, struct ssv6xxx_usb_glue, kref)

static struct usb_driver ssv_usb_driver;
  
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
	
	//flush_workqueue(usb_glue->wq);
	destroy_workqueue(usb_glue->wq);
	usb_glue->rx_wq_running = false;
	
	usb_put_dev(usb_glue->udev);
	kfree(usb_glue);
}

/*
static struct sk_buff *ssv_skb_alloc(s32 len)
{
	struct sk_buff *skb;

	skb = __dev_alloc_skb(len + SSV6200_ALLOC_RSVD ,GFP_ATOMIC);
	if (skb != NULL) {
		skb_reserve(skb, SSV_SKB_info_size);
	}

	return skb;
}
*/
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
	
	if (retval) {
		*rsp_len = 0;
		HWIF_DBG_PRINT(&usb_glue->wlan_data, "Cannot receive response, error=%d\n", retval);
		//if (((usb_glue->err_cnt)++) > 5)
		//	WARN_ON(1);
	} else { 
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
		HWIF_DBG_PRINT(&usb_glue->wlan_data, "Cannot send cmd data, error=%d\n", retval);
		//if (((usb_glue->err_cnt)++) > 5)
		//	WARN_ON(1);
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
		HWIF_DBG_PRINT(&usb_glue->wlan_data, 
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
			HWIF_DBG_PRINT(&usb_glue->wlan_data, 
				"%s: Fail to receive response, sequence=%d, retval=%d\n", 
				__FUNCTION__, sequence, retval);
			goto exit;
		}
		/* parse the response context */
		if (rsp_len < SSV6XXX_CMD_HEADER_SIZE) {
			HWIF_DBG_PRINT(&usb_glue->wlan_data, 
				"Receviced abnormal response length[%d]\n", rsp_len);
			goto exit; 
		}
		rsphdr = (struct ssv6xxx_cmd_hdr *)usb_glue->rsp_endpoint.buff;
		if (sequence == rsphdr->seq) 
			break;
		else 
			HWIF_DBG_PRINT(&usb_glue->wlan_data, 
				"received incorrect sequence=%d[%d]\n", sequence, rsphdr->seq);
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
			HWIF_DBG_PRINT(&usb_glue->wlan_data, 
				"%s: unknown response cmd[%d]\n", 
				__FUNCTION__, rsphdr->cmd);
			break;
	}

exit:
	mutex_unlock(&usb_glue->cmd_mutex);
	return retval;
}

static void ssv6xxx_usb_recv_rx_complete(struct urb *urb)
{
	struct ssv6xxx_usb_glue *usb_glue = urb->context;
 
	struct sk_buff *rx_mpdu = NULL;
	unsigned char *data;
    int i;

	usb_glue->rx_endpoint.rx_res = urb->status;

	if (urb->status) {
		HWIF_DBG_PRINT(&usb_glue->wlan_data, 
			"fail rx status received:%d\n", urb->status);

		usb_glue->rx_endpoint.rx_filled = 0;
		usb_glue->rx_endpoint.rx_res = urb->status;
		//if (((usb_glue->err_cnt)++) > 5)
		//	WARN_ON(1);
		goto skip;
	}

	usb_glue->err_cnt = 0;
	usb_glue->rx_endpoint.rx_filled = urb->actual_length;
	
	/* for debug */
	//ssv6xxx_dump_rx_desc(usb_glue->rx_endpoint.rx_buf);
	
	/* recevied invalid frame, MAX_HCI_RX_AGGR_SIZE(9344) */
	if (usb_glue->rx_endpoint.rx_filled > MAX_HCI_RX_AGGR_SIZE) {
		HWIF_DBG_PRINT(&usb_glue->wlan_data, 
			"recv invalid data length %d\n", 
				usb_glue->rx_endpoint.rx_filled);
		goto skip;
	}

	if (usb_glue->rx_endpoint.rx_filled) {

		/* record the rx pkt cnt in hci_hw_ctrl object's rx_pkt filed */
		(*usb_glue->rx_pkt)++;
 
		/* MAX_RETRY_SSV6XXX_ALLOC_BUF(3) */
        for (i = 0; i < MAX_RETRY_SSV6XXX_ALLOC_BUF; i++) {
		    rx_mpdu = usb_glue->wlan_data.skb_alloc(usb_glue->wlan_data.skb_param, 
											MAX_HCI_RX_AGGR_SIZE, GFP_ATOMIC);
		    if (rx_mpdu != NULL)
                break;
			
            mdelay(1);
        }
		
        if (i == MAX_RETRY_SSV6XXX_ALLOC_BUF)
            goto skip;

		/* get skb_tail pointer as our data ptr to prepare putting new data */
		data = skb_put(rx_mpdu, usb_glue->rx_endpoint.rx_filled);
		/* put the received data */
		memcpy(data, usb_glue->rx_endpoint.rx_buf, usb_glue->rx_endpoint.rx_filled);	

		usb_glue->rx_cb(rx_mpdu, usb_glue->rx_cb_args); 
	}
	
skip:
	queue_work(usb_glue->wq, &usb_glue->rx_work);
}

static void ssv6xxx_usb_recv_rx(struct work_struct *work)
{
	struct ssv6xxx_usb_glue *usb_glue = 
		container_of(work, struct ssv6xxx_usb_glue, rx_work);
	int size = MAX_HCI_RX_AGGR_SIZE; 
	int retval;

	/* prepare a read */
	usb_fill_bulk_urb(usb_glue->rx_endpoint.rx_urb, usb_glue->udev, 
		usb_rcvbulkpipe(usb_glue->udev, usb_glue->rx_endpoint.address),
		usb_glue->rx_endpoint.rx_buf, size, ssv6xxx_usb_recv_rx_complete, usb_glue);

	usb_glue->rx_endpoint.rx_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* submit bulk in urb, which means no data to deliver */
	usb_glue->rx_endpoint.rx_filled = 0;

	/* do it */
	retval = usb_submit_urb(usb_glue->rx_endpoint.rx_urb, GFP_ATOMIC);
	if (retval) {
		HWIF_DBG_PRINT(&usb_glue->wlan_data, 
			"Fail to submit rx urb, error=%d\n", retval);
		usb_glue->rx_wq_running = false;
	}
}

/* 
 * For usb interface,  we make use of work queue to receive the rx frame.
 */
static int __must_check ssv6xxx_usb_read(struct device *child,
        void *buf, size_t *size, int mode)
{
    //printk(KERN_INFO "==>%s()\n", __func__);
	*size = 0;
    //printk(KERN_INFO "<==%s()\n", __func__);
	return 0;
}

static int ssv6xxx_usb_send_tx(struct ssv6xxx_usb_glue *usb_glue, struct sk_buff *skb, size_t size)
{
	int foolen = 0, retval = 0;
	int tx_len = size;	

    //printk(KERN_INFO "==>%s()\n", __func__);
	
	/* for USB 3.0 port, add padding byte and let host driver send short packet */
	if ((tx_len % usb_glue->tx_endpoint.packet_size) == 0) {
		skb_put(skb, 1);
		tx_len++;
	}

	retval = usb_bulk_msg(usb_glue->udev, 
					usb_sndbulkpipe(usb_glue->udev, usb_glue->tx_endpoint.address),
					skb->data, tx_len, &foolen, TRANSACTION_TIMEOUT);
	if (retval) 
		HWIF_DBG_PRINT(&usb_glue->wlan_data, "Cannot send tx data, retval=%d\n", retval);
	

    //printk(KERN_INFO "<==%s()\n", __func__);
	return retval;
}

static int __must_check ssv6xxx_usb_write(struct device *child,
        void *buf, size_t len, u8 queue_num)
{
    int retval = (-1);	
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);

    //printk(KERN_INFO "==>%s()\n", __func__);
    if (IS_GLUE_INVALID(usb_glue))
        return retval;

	/* for debug */
	//ssv6xxx_dump_tx_desc(buf);
	/* use urb to send tx data */
    if ((retval = ssv6xxx_usb_send_tx(usb_glue, (struct sk_buff *)buf, len)) < 0) {
		HWIF_DBG_PRINT(&usb_glue->wlan_data, "%s: Fail to send tx data\n", __FUNCTION__);
		//if (((usb_glue->err_cnt)++) > 5)
		//	WARN_ON(1);
	} else {
		usb_glue->err_cnt = 0;
	}

    //printk(KERN_INFO "<==%s()\n", __func__);
	return retval;
}


static int __must_check __ssv6xxx_usb_read_reg(struct ssv6xxx_usb_glue *usb_glue, 
														u32 addr, u32 *buf)
{
    int retval = (-1);
	struct ssv6xxx_read_reg read_reg;
	struct ssv6xxx_read_reg_result result;
    //printk(KERN_INFO "==>%s()\n", __func__);
	
	if (IS_GLUE_INVALID(usb_glue)) {
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
		HWIF_DBG_PRINT(&usb_glue->wlan_data, 
			"%s: Fail to read register address %x\n", __FUNCTION__, addr);
	}

    //printk(KERN_INFO "<==%s()\n", __func__);
	return retval;
}

static int __must_check ssv6xxx_usb_read_reg(struct device *child, u32 addr,
                                             u32 *buf)
{
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);
    //printk(KERN_INFO "==>%s()\n", __func__);
	return __ssv6xxx_usb_read_reg(usb_glue, addr, buf);
}


static int __must_check __ssv6xxx_usb_write_reg(struct ssv6xxx_usb_glue *usb_glue, u32 addr,
 		u32 buf)
{
	int retval = (-1);
	struct ssv6xxx_write_reg write_reg;
    //printk(KERN_INFO "==>%s()\n", __func__);
	if (IS_GLUE_INVALID(usb_glue))
		return retval;
	
	memset(&write_reg, 0, sizeof(struct ssv6xxx_write_reg));	
	write_reg.addr = cpu_to_le32(addr);
	write_reg.value = cpu_to_le32(buf);
	retval = ssv6xxx_usb_cmd(usb_glue, SSV6200_CMD_WRITE_REG, &write_reg, sizeof(struct ssv6xxx_write_reg), NULL);
	if (retval) 
		HWIF_DBG_PRINT(&usb_glue->wlan_data, "%s: Fail to write register address %x, value %x\n", __FUNCTION__, addr, buf);

    //printk(KERN_INFO "<==%s()\n", __func__);	
	return retval;
}

static int __must_check ssv6xxx_usb_write_reg(struct device *child, u32 addr,
		u32 buf)
{
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);
    //printk(KERN_INFO "==>%s()\n", __func__);
	return __ssv6xxx_usb_write_reg(usb_glue, addr, buf);
}

static int __must_check ssv6xxx_usb_burst_read_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    //not support
    //printk(KERN_INFO "==>%s()\n", __func__);    
    printk(KERN_INFO "not support\n");
	//printk(KERN_INFO "<==%s()\n", __func__);
	return -EOPNOTSUPP;
}

static int __must_check ssv6xxx_usb_burst_write_reg(struct device *child, u32 *addr,
        u32 *buf, u8 reg_amount)
{
    //not support
    //printk(KERN_INFO "==>%s()\n", __func__);    
    printk(KERN_INFO "not support\n");
	//printk(KERN_INFO "<==%s()\n", __func__);    
    return -EOPNOTSUPP;
}

static int ssv6xxx_usb_load_firmware(struct device *child, 
				u32 start_addr, u8 *data, int data_length)
{
	struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);
   	u16 laddr, haddr;
	u32 addr;
	int retval = 0, max_usb_block = 512;
	u8 *pdata;
	int res_length, offset, send_length;

    //printk(KERN_INFO "==>%s()\n", __func__);    

	if (IS_GLUE_INVALID(usb_glue))
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
			HWIF_DBG_PRINT(&usb_glue->wlan_data, 
				"Load Firmware Fail, retval=%d, sram=0x%08x\n", retval, (laddr|haddr));
			break;
		}

		addr += transfer;
		pdata += transfer;
		offset += transfer;
		res_length -= transfer;
	}

	//printk(KERN_INFO "<==%s()\n", __func__);
	return retval;
}

static int ssv6xxx_usb_property(struct device *child)
{
    //printk(KERN_INFO "==>%s()\n", __func__);    
    printk(KERN_INFO "SSV_HWIF_CAPABILITY_POLLING | SSV_HWIF_INTERFACE_USB\n");
	//printk(KERN_INFO "<==%s()\n", __func__);
	return SSV_HWIF_CAPABILITY_POLLING | SSV_HWIF_INTERFACE_USB;
}

static int ssv6xxx_chk_usb_speed(struct ssv6xxx_usb_glue *usb_glue)
{
    if (IS_GLUE_INVALID(usb_glue)) {
        return -1;
    }
    
    return usb_glue->udev->speed;
}

 
static void ssv6xxx_usb_rx_task(struct device *child, 
			int (*rx_cb)(struct sk_buff *rx_skb, void *args), 
			void *args, u32 *pkt) 
{
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);

    printk(KERN_INFO "==>%s()\n", __func__);    
	usb_glue->rx_cb = rx_cb;
	usb_glue->rx_cb_args = args;
	usb_glue->rx_pkt = pkt;

	/**
	* schedule_work - put work task in ssv6xxx_usb_wq workqueue
	* @work: job to be done
	*
	* Returns zero if @work was already on the workqueue and non-zero otherwise.
	*
	* This puts a job in the workqueue if it was not already queued and leaves 
	* it in the same position on the kernel-global workqueue otherwise.
	*/
	/* start ssv6xxx_usb_wq rx workq */
	queue_work(usb_glue->wq, &usb_glue->rx_work);
	usb_glue->rx_wq_running = true;

	printk(KERN_INFO "<==%s()\n", __func__);
}

static int ssv6xxx_usb_start_acc(struct device *child, u8 epnum)
{
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);

    printk(KERN_INFO "==>%s()\n", __func__);    
    
    if (IS_GLUE_INVALID(usb_glue)) {
        printk("failed to start usb acc of ep%d\n", epnum);
        return -1;
    }

    //only high speed use USB acc
    if (ssv6xxx_chk_usb_speed(usb_glue) == USB_SPEED_HIGH)
        usb_glue->wlan_data.enable_usb_acc(usb_glue->wlan_data.usb_param, epnum);

	printk(KERN_INFO "<==%s()\n", __func__);
    return 0;
}

static int ssv6xxx_usb_stop_acc(struct device *child, u8 epnum)
{   
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);

    printk(KERN_INFO "==>%s()\n", __func__);    
    if (IS_GLUE_INVALID(usb_glue)) {
        printk("failed to stop usb acc of ep%d\n", epnum);
        return -1;
    }

    //only high speed use USB acc
    if (ssv6xxx_chk_usb_speed(usb_glue) == USB_SPEED_HIGH)
        usb_glue->wlan_data.disable_usb_acc(usb_glue->wlan_data.usb_param, epnum);

	printk(KERN_INFO "<==%s()\n", __func__);
    return 0;
}

static int ssv6xxx_usb_jump_to_rom(struct device *child)
{
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);

    printk(KERN_INFO "==>%s()\n", __func__);    
	
    if (IS_GLUE_INVALID(usb_glue)) {
        printk("failed to jump to ROM\n");
        return -1;
    }
    
    usb_glue->wlan_data.jump_to_rom(usb_glue->wlan_data.usb_param);

	printk(KERN_INFO "<==%s()\n", __func__);
	return 0;
}

static void ssv6xxx_usb_sysplf_reset(struct device *child, u32 addr, u32 value)
{
    struct ssv6xxx_usb_glue *usb_glue = dev_get_drvdata(child->parent);
	int retval = (-1), rsp_len = 0;
	u16 sequence;
	struct ssv6xxx_write_reg write_reg;

    printk(KERN_INFO "==>%s()\n", __func__);    

    if (IS_GLUE_INVALID(usb_glue)) 
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
	retval = ssv6xxx_usb_send_cmd(usb_glue, SSV6200_CMD_WRITE_REG, sequence, &write_reg, sizeof(struct ssv6xxx_write_reg));
	if (retval)  
	    HWIF_DBG_PRINT(&usb_glue->wlan_data, "%s: Fail to reset sysplf\n", __FUNCTION__);
	
    retval = ssv6xxx_usb_recv_rsp(usb_glue, SSV6XXX_MAX_RXCMDSZ, &rsp_len);
	
    mutex_unlock(&usb_glue->cmd_mutex);


	printk(KERN_INFO "<==%s()\n", __func__);	
}

#if 0
static struct ssv6xxx_hwif_ops usb_ops =
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
};
#endif

static struct unified_drv_ops usb_ops = 
{
	.drv_info.flags = 0x0,
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

static void ssv6xxx_usb_power_on(struct ssv6xxx_platform_data * pdata, struct usb_interface *interface)
{
	if (pdata->is_enabled == true)
		return;

	pdata->is_enabled = true;
}

static void ssv6xxx_usb_power_off(struct ssv6xxx_platform_data *pdata, struct usb_interface *interface)
{
	if (pdata->is_enabled == false)
		return;

	pdata->is_enabled = false;
}


static void _read_chip_id (struct ssv6xxx_usb_glue *usb_glue)
{
    u32 regval;
    int ret;
    u8 _chip_id[SSV6XXX_CHIP_ID_LENGTH];
    u8 *c = _chip_id;
    int i = 0;

    //CHIP ID
    // Chip ID registers should be common to all SSV6xxx devices. So these registers 
    // must not come from ssv6xxx_reg.h but defined somewhere else.
    ret = __ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_3, &regval);
    *((u32 *)&_chip_id[0]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = __ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_2, &regval);
    *((u32 *)&_chip_id[4]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = __ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_1, &regval);
    *((u32 *)&_chip_id[8]) = __be32_to_cpu(regval);

    if (ret == 0)
        ret = __ssv6xxx_usb_read_reg(usb_glue, ADR_CHIP_ID_0, &regval);
    *((u32 *)&_chip_id[12]) = __be32_to_cpu(regval);

    _chip_id[12+sizeof(u32)] = 0;

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
        strncpy(usb_glue->wlan_data.chip_id, c, SSV6XXX_CHIP_ID_LENGTH);

		/* usb_glue->wlan_data.chip_id=SSV6006C0 */
		printk("usb_glue->wlan_data.chip_id=%s\n", usb_glue->wlan_data.chip_id);

		/* SSV6XXX_USB 1-2:1.0: CHIP ID: SSV6006C0 */
		dev_info(usb_glue->dev, "CHIP ID: %s \n", usb_glue->wlan_data.chip_id);

        strncpy(usb_glue->wlan_data.short_chip_id, c, SSV6XXX_CHIP_ID_SHORT_LENGTH);

		/* before: short_chip_id=SSV6006C0 */
		printk("before: short_chip_id=%s\n", usb_glue->wlan_data.short_chip_id);

		/* why need do this ?? maybe set 0 in SSV6XXX_CHIP_ID_SHORT_LENGTH+1 ?? */
        usb_glue->wlan_data.short_chip_id[SSV6XXX_CHIP_ID_SHORT_LENGTH] = 0;

		/* after : short_chip_id=SSV6006C0 */			
		printk("after : short_chip_id=%s\n", usb_glue->wlan_data.short_chip_id);
    } else {
        dev_err(usb_glue->dev, "Failed to read chip ID");
        usb_glue->wlan_data.chip_id[0] = 0;
    }
}


static int ssv_usb_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct ssv6xxx_platform_data *pwlan_data;
	struct ssv6xxx_usb_glue *usb_glue;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	int retval = -ENOMEM;
	unsigned int epnum;


	/* ==>ssv_usb_probe() */
    printk(KERN_INFO "==>%s()\n", __func__);

	printk(KERN_INFO "=======================================\n");
	printk(KERN_INFO "==          TURISMO - USB            ==\n");
	printk(KERN_INFO "=======================================\n");

    /* allocate memory for our device state and initialize it */
	usb_glue = kzalloc(sizeof(*usb_glue), GFP_KERNEL);
	if (!usb_glue) {
		dev_err(&interface->dev, "Out of memory\n");
		goto error;
	}

	usb_glue->sequence = 0;
	usb_glue->err_cnt = 0;
	kref_init(&usb_glue->kref);
	mutex_init(&usb_glue->io_mutex);
	mutex_init(&usb_glue->cmd_mutex);

	/* INIT RX */
	INIT_WORK(&usb_glue->rx_work, ssv6xxx_usb_recv_rx);
	usb_glue->rx_wq_running = false;
	usb_glue->wq = create_singlethread_workqueue("ssv6xxx_usb_wq");
	if (!usb_glue->wq) {
		dev_err(&interface->dev, "Could not allocate Work Queue\n");
		goto error;
	}

	/* point to the device of usb interface which use in the device tree */
	usb_glue->dev = &interface->dev;
	/* USB core needs to know usb_device, so get usb_device form usb_interface */
	usb_glue->udev = usb_get_dev(interface_to_usbdev(interface));
	/* TODO: aaron: we already store the interface why need keep the dev ?*/
	usb_glue->interface = interface;
	usb_glue->dev_ready = true;

	/* Tell PM core that we don't need the card to be powered now */ 
	pwlan_data = &usb_glue->wlan_data;
	memset(pwlan_data, 0, sizeof(struct ssv6xxx_platform_data));
	atomic_set(&pwlan_data->irq_handling, 0);

	/* Set verdor/product id */
	pwlan_data->vendor = id->idVendor;
	pwlan_data->device = id->idProduct;
	
	/* Set hwif operation */
	pwlan_data->unify = &usb_ops;
	
	/* setup the endpoint information */
	iface_desc = interface->cur_altsetting;

	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		
		endpoint = &iface_desc->endpoint[i].desc;	
		epnum = endpoint->bEndpointAddress & 0x0f;
   
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
	ssv6xxx_usb_power_on(pwlan_data, interface);

	_read_chip_id(usb_glue);

	ssv6xxx_dev_init(SSV6XXX_HWM_STA);
	 
    printk(KERN_INFO "<==%s()\n", __func__);
	return 0;

error:
	if (usb_glue)
		/* this frees allocated memory */
		kref_put(&usb_glue->kref, ssv6xxx_usb_delete);

	return retval;
}

static void ssv_usb_disconnect(struct usb_interface *interface)
{
	struct ssv6xxx_usb_glue *usb_glue;

    printk(KERN_INFO "==>%s()\n", __func__);
	
	usb_glue = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);
	if (usb_glue) {
		usb_glue->dev_ready = false;
		ssv6xxx_usb_power_off(&usb_glue->wlan_data, interface);
	}
	/* prevent more I/O from starting */
	mutex_lock(&usb_glue->io_mutex);
	usb_glue->interface = NULL;
	mutex_unlock(&usb_glue->io_mutex);

	/* decrement our usage count */
	kref_put(&usb_glue->kref, ssv6xxx_usb_delete);

	dev_info(&interface->dev, "SSV USB is disconnected");
    printk(KERN_INFO "<==%s()\n", __func__);
}

/* in */
#ifdef CONFIG_PM
static int ssv_usb_suspend(struct usb_interface *interface, pm_message_t message)
{
	struct ssv6xxx_usb_glue *usb_glue = usb_get_intfdata(interface);
    printk(KERN_INFO "==>%s()\n", __func__);

	dev_info(usb_glue->dev, "%s(): suspend.\n", __FUNCTION__);
	if (!usb_glue)
		return 0;

	usb_glue->wlan_data.suspend(usb_glue->wlan_data.pm_param);

	/* cancel urb */
	usb_kill_urb(usb_glue->rx_endpoint.rx_urb);

    printk(KERN_INFO "<==%s()\n", __func__);	
	return 0;
}

static int ssv_usb_resume(struct usb_interface *interface)
{
	struct ssv6xxx_usb_glue *usb_glue = usb_get_intfdata(interface);

    printk(KERN_INFO "==>%s()\n", __func__);

	dev_info(usb_glue->dev, "%s(): resume.\n", __FUNCTION__);

	if (!usb_glue)
		return 0;
	
	usb_glue->wlan_data.resume(usb_glue->wlan_data.pm_param);    

    if (usb_glue->rx_wq_running == false) {
        /* start a new rx workq after resume */
        queue_work(usb_glue->wq, &usb_glue->rx_work);
        usb_glue->rx_wq_running = true;
    }    

    printk(KERN_INFO "<==%s()\n", __func__);
	return 0;
}
#endif

static struct usb_driver ssv_usb_driver = {
	.name 		= "SSV6XXX_USB",
	.probe 		= ssv_usb_probe,
	.disconnect = ssv_usb_disconnect,
/* in */	
#ifdef CONFIG_PM
	.suspend 	= ssv_usb_suspend,
	.resume 	= ssv_usb_resume,
#endif
	.id_table 	= ssv_usb_table,
	.supports_autosuspend = 1,

// in, 	3.12.0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)	
	//disable_hub_initiated_lpm: 
	// if set to 0, the USB core will not allow hubs to initiate lower power link 
	// state transitions when an idle timeout occurs.
    .disable_hub_initiated_lpm = 0,
#endif	
};

static int __init ssv6xxx_usb_init(void) 
{
	printk(KERN_INFO "ssv6xxx_usb_init(register ssv_usb_driver)\n");
	return usb_register(&ssv_usb_driver);
}

static int ssv_usb_do_device_exit(struct device *d, void *arg)
{
	struct usb_interface *intf = to_usb_interface(d);
	struct ssv6xxx_usb_glue *usb_glue = usb_get_intfdata(intf);

	if (usb_glue != NULL) {
		//TODO: replace direct address access
		printk(KERN_INFO "ssv_usb_do_device_exit: JUMP to ROM\n");
	}    
	msleep(50);
	return 0;
}


static void __exit ssv6xxx_usb_exit(void) 
{
	if (driver_for_each_device(&ssv_usb_driver.drvwrap.driver, NULL,
		NULL, ssv_usb_do_device_exit));

    printk(KERN_INFO "ssv6xxx_usb_exit\n");
	usb_deregister(&ssv_usb_driver);
}
 
module_init(ssv6xxx_usb_init);
module_exit(ssv6xxx_usb_exit); 

MODULE_LICENSE("GPL");
