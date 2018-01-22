/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include "os.h"
#include <host_apis.h>
#include <net_mgr.h>
#include <net_wrapper.h>


char *brcmf_ifname(struct brcmf_if *ifp)
{
	if (!ifp)
		return "<if_null>";

	if (ifp->ndev)
		return ifp->ndev->name;

	return "<if_none>";
}

struct brcmf_if *brcmf_get_ifp(struct brcmf_pub *drvr, int ifidx)
{
	struct brcmf_if *ifp;
	s32 bsscfgidx;

	if (ifidx < 0 || ifidx >= BRCMF_MAX_IFS) {
		brcmf_err("ifidx %d out of range\n", ifidx);
		return NULL;
	}

	ifp = NULL;
	bsscfgidx = drvr->if2bss[ifidx];
	if (bsscfgidx >= 0)
		ifp = drvr->iflist[bsscfgidx];

	return ifp;
}

static void _brcmf_set_multicast_list(struct work_struct *work)
{
	struct brcmf_if *ifp;
	struct net_device *ndev;
	struct netdev_hw_addr *ha;
	u32 cmd_value, cnt;
	__le32 cnt_le;
	char *buf, *bufp;
	u32 buflen;
	s32 err;

	ifp = container_of(work, struct brcmf_if, multicast_work);

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	ndev = ifp->ndev;

	/* Determine initial value of allmulti flag */
	cmd_value = (ndev->flags & IFF_ALLMULTI) ? true : false;

	/* Send down the multicast list first. */
	cnt = netdev_mc_count(ndev);
	buflen = sizeof(cnt) + (cnt * ETH_ALEN);
	buf = kmalloc(buflen, GFP_ATOMIC);
	if (!buf)
		return;
	bufp = buf;

	cnt_le = cpu_to_le32(cnt);
	memcpy(bufp, &cnt_le, sizeof(cnt_le));
	bufp += sizeof(cnt_le);

	netdev_for_each_mc_addr(ha, ndev) {
		if (!cnt)
			break;
		memcpy(bufp, ha->addr, ETH_ALEN);
		bufp += ETH_ALEN;
		cnt--;
	}

	err = brcmf_fil_iovar_data_set(ifp, "mcast_list", buf, buflen);
	if (err < 0) {
		brcmf_err("Setting mcast_list failed, %d\n", err);
		cmd_value = cnt ? true : cmd_value;
	}

	kfree(buf);

	/*
	 * Now send the allmulti setting.  This is based on the setting in the
	 * net_device flags, but might be modified above to be turned on if we
	 * were trying to set some addresses and dongle rejected it...
	 */
	err = brcmf_fil_iovar_int_set(ifp, "allmulti", cmd_value);
	if (err < 0)
		brcmf_err("Setting allmulti failed, %d\n", err);

	/*Finally, pick up the PROMISC flag */
	cmd_value = (ndev->flags & IFF_PROMISC) ? true : false;
	err = brcmf_fil_cmd_int_set(ifp, BRCMF_C_SET_PROMISC, cmd_value);
	if (err < 0)
		brcmf_err("Setting BRCMF_C_SET_PROMISC failed, %d\n",
			  err);
}

#if IS_ENABLED(CONFIG_IPV6)
static void _brcmf_update_ndtable(struct work_struct *work)
{
	struct brcmf_if *ifp;
	int i, ret;

	ifp = container_of(work, struct brcmf_if, ndoffload_work);

	/* clear the table in firmware */
	ret = brcmf_fil_iovar_data_set(ifp, "nd_hostip_clear", NULL, 0);
	if (ret) {
		brcmf_dbg(TRACE, "fail to clear nd ip table err:%d\n", ret);
		return;
	}

	for (i = 0; i < ifp->ipv6addr_idx; i++) {
		ret = brcmf_fil_iovar_data_set(ifp, "nd_hostip",
					       &ifp->ipv6_addr_tbl[i],
					       sizeof(struct in6_addr));
		if (ret)
			brcmf_err("add nd ip err %d\n", ret);
	}
}
#else
static void _brcmf_update_ndtable(struct work_struct *work)
{
}
#endif

static int brcmf_netdev_set_mac_address(struct net_device *ndev, void *addr)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct sockaddr *sa = (struct sockaddr *)addr;
	int err;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	err = brcmf_fil_iovar_data_set(ifp, "cur_etheraddr", sa->sa_data,
				       ETH_ALEN);
	if (err < 0) {
		brcmf_err("Setting cur_etheraddr failed, %d\n", err);
	} else {
		brcmf_dbg(TRACE, "updated to %pM\n", sa->sa_data);
		memcpy(ifp->mac_addr, sa->sa_data, ETH_ALEN);
		memcpy(ifp->ndev->dev_addr, ifp->mac_addr, ETH_ALEN);
	}
	return err;
}

static void brcmf_netdev_set_multicast_list(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);

	schedule_work(&ifp->multicast_work);
}

static netdev_tx_t brcmf_netdev_start_xmit(struct sk_buff *skb,
					   struct net_device *ndev)
{
	int ret;
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	struct ethhdr *eh;

	brcmf_dbg(DATA, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	/* Can the device send data? */
	if (drvr->bus_if->state != BRCMF_BUS_UP) {
		brcmf_err("xmit rejected state=%d\n", drvr->bus_if->state);
		netif_stop_queue(ndev);
		dev_kfree_skb(skb);
		ret = -ENODEV;
		goto done;
	}

	/* Make sure there's enough writable headroom*/
	ret = skb_cow_head(skb, drvr->hdrlen);
	if (ret < 0) {
		brcmf_err("%s: skb_cow_head failed\n",
			  brcmf_ifname(ifp));
		dev_kfree_skb(skb);
		goto done;
	}

	/* validate length for ether packet */
	if (skb->len < sizeof(*eh)) {
		ret = -EINVAL;
		dev_kfree_skb(skb);
		goto done;
	}

	eh = (struct ethhdr *)(skb->data);

	if (eh->h_proto == htons(ETH_P_PAE))
		atomic_inc(&ifp->pend_8021x_cnt);

	ret = brcmf_fws_process_skb(ifp, skb);

done:
	if (ret) {
		ifp->stats.tx_dropped++;
	} else {
		ifp->stats.tx_packets++;
		ifp->stats.tx_bytes += skb->len;
	}

	/* Return ok: we always eat the packet */
	return NETDEV_TX_OK;
}

void brcmf_txflowblock_if(struct brcmf_if *ifp,
			  enum brcmf_netif_stop_reason reason, bool state)
{
	unsigned long flags;

	if (!ifp || !ifp->ndev)
		return;

	brcmf_dbg(TRACE, "enter: bsscfgidx=%d stop=0x%X reason=%d state=%d\n",
		  ifp->bsscfgidx, ifp->netif_stop, reason, state);

	spin_lock_irqsave(&ifp->netif_stop_lock, flags);
	if (state) {
		if (!ifp->netif_stop)
			netif_stop_queue(ifp->ndev);
		ifp->netif_stop |= reason;
	} else {
		ifp->netif_stop &= ~reason;
		if (!ifp->netif_stop)
			netif_wake_queue(ifp->ndev);
	}
	spin_unlock_irqrestore(&ifp->netif_stop_lock, flags);
}

void brcmf_txflowblock(struct device *dev, bool state)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	brcmf_dbg(TRACE, "Enter\n");

	brcmf_fws_bus_blocked(drvr, state);
}

void brcmf_netif_rx(struct brcmf_if *ifp, struct sk_buff *skb)
{
	if (skb->pkt_type == PACKET_MULTICAST)
		ifp->stats.multicast++;

	if (!(ifp->ndev->flags & IFF_UP)) {
		brcmu_pkt_buf_free_skb(skb);
		return;
	}

	ifp->stats.rx_bytes += skb->len;
	ifp->stats.rx_packets++;

	brcmf_dbg(DATA, "rx proto=0x%X\n", ntohs(skb->protocol));
	if (in_interrupt())
		netif_rx(skb);
	else
		/* If the receive is not processed inside an ISR,
		 * the softirqd must be woken explicitly to service
		 * the NET_RX_SOFTIRQ.  This is handled by netif_rx_ni().
		 */
		netif_rx_ni(skb);
}

static int brcmf_rx_hdrpull(struct brcmf_pub *drvr, struct sk_buff *skb,
			    struct brcmf_if **ifp)
{
	int ret;

	/* process and remove protocol-specific header */
	ret = brcmf_proto_hdrpull(drvr, true, skb, ifp);

	if (ret || !(*ifp) || !(*ifp)->ndev) {
		if (ret != -ENODATA && *ifp)
			(*ifp)->stats.rx_errors++;
		brcmu_pkt_buf_free_skb(skb);
		return -ENODATA;
	}

	skb->protocol = eth_type_trans(skb, (*ifp)->ndev);
	return 0;
}

void brcmf_rx_frame(struct device *dev, struct sk_buff *skb, bool handle_event)
{
	struct brcmf_if *ifp;
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	brcmf_dbg(DATA, "Enter: %s: rxp=%p\n", dev_name(dev), skb);

	if (brcmf_rx_hdrpull(drvr, skb, &ifp))
		return;

	if (brcmf_proto_is_reorder_skb(skb)) {
		brcmf_proto_rxreorder(ifp, skb);
	} else {
		/* Process special event packets */
		if (handle_event)
			brcmf_fweh_process_skb(ifp->drvr, skb);

		brcmf_netif_rx(ifp, skb);
	}
}

void brcmf_rx_event(struct device *dev, struct sk_buff *skb)
{
	struct brcmf_if *ifp;
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	brcmf_dbg(EVENT, "Enter: %s: rxp=%p\n", dev_name(dev), skb);

	if (brcmf_rx_hdrpull(drvr, skb, &ifp))
		return;

	brcmf_fweh_process_skb(ifp->drvr, skb);
	brcmu_pkt_buf_free_skb(skb);
}

void brcmf_txfinalize(struct brcmf_if *ifp, struct sk_buff *txp, bool success)
{
	struct ethhdr *eh;
	u16 type;

	eh = (struct ethhdr *)(txp->data);
	type = ntohs(eh->h_proto);

	if (type == ETH_P_PAE) {
		atomic_dec(&ifp->pend_8021x_cnt);
		if (waitqueue_active(&ifp->pend_8021x_wait))
			wake_up(&ifp->pend_8021x_wait);
	}

	if (!success)
		ifp->stats.tx_errors++;

	brcmu_pkt_buf_free_skb(txp);
}

void brcmf_txcomplete(struct device *dev, struct sk_buff *txp, bool success)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;
	struct brcmf_if *ifp;

	/* await txstatus signal for firmware if active */
	if (brcmf_fws_fc_active(drvr->fws)) {
		if (!success)
			brcmf_fws_bustxfail(drvr->fws, txp);
	} else {
		if (brcmf_proto_hdrpull(drvr, false, txp, &ifp))
			brcmu_pkt_buf_free_skb(txp);
		else
			brcmf_txfinalize(ifp, txp, success);
	}
}

static struct net_device_stats *brcmf_netdev_get_stats(struct net_device *ndev)
{
	struct DeviceInfo *g_dev_info; = netdev_priv(ndev);

	printk("Enter, bsscfgidx=%d\n", 
		g_dev_info->vif[g_dev_info->used_vif].bsscfgidx);

	return &ifp->stats;
}

static void net_ethtool_get_drvinfo(struct net_device *ndev,
				    struct ethtool_drvinfo *info)
{
//	struct DeviceInfo *g_dev_info; = netdev_priv(ndev);
	char *drv_name = "RebBull Fullmac Host";
	char *drv_info = "Linux";
	char *fw_info = "Trumiso";
	char *bus_info = "SSV_USB";

	strlcpy(info->driver, drv_name, strlen(drv_name));
	strlcpy(info->version, drv_info, strlen(drv_info));
	strlcpy(info->fw_version, fw_info, strlen(fw_info));
	strlcpy(info->bus_info, bus_info, strlen(bus_info));
}

static const struct ethtool_ops net_ethtool_ops = {
	.get_drvinfo = net_ethtool_get_drvinfo,
};

static int brcmf_netdev_stop(struct net_device *ndev)
{
	struct DeviceInfo *g_dev_info; = netdev_priv(ndev);

//	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

//	brcmf_cfg80211_down(ndev);

//	brcmf_fil_iovar_data_set(ifp, "arp_hostip_clear", NULL, 0);

//	brcmf_net_setcarrier(ifp, false);

	return 0;
}

static int brcmf_netdev_open(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	struct brcmf_bus *bus_if = drvr->bus_if;
	u32 toe_ol;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	/* If bus is not ready, can't continue */
	if (bus_if->state != BRCMF_BUS_UP) {
		brcmf_err("failed bus is not ready\n");
		return -EAGAIN;
	}

	atomic_set(&ifp->pend_8021x_cnt, 0);

	/* Get current TOE mode from dongle */
	if (brcmf_fil_iovar_int_get(ifp, "toe_ol", &toe_ol) >= 0
	    && (toe_ol & TOE_TX_CSUM_OL) != 0)
		ndev->features |= NETIF_F_IP_CSUM;
	else
		ndev->features &= ~NETIF_F_IP_CSUM;

	if (brcmf_cfg80211_up(ndev)) {
		brcmf_err("failed to bring up cfg80211\n");
		return -EIO;
	}

	/* Clear, carrier, set when connected or AP mode. */
	netif_carrier_off(ndev);
	return 0;
}

static const struct net_device_ops net_netdev_ops = {
	.ndo_open = ssv_netdev_open,
	.ndo_stop = ssv_netdev_stop,
	.ndo_get_stats = ssv_netdev_get_stats,
	.ndo_start_xmit = ssv_netdev_start_xmit,
	.ndo_set_mac_address = ssv_netdev_set_mac_address,
	.ndo_set_rx_mode = ssv_netdev_set_multicast_list
};

int net_netdev_ops_attach(ssv_vif *ifp, bool rtnl_locked)
{
	s32 err;
	NET_DEV ndev = ifp->ndev;;

	printk("Enter, bsscfgidx=%d mac=%pM\n", 
		ifp->bsscfgidx, ifp->mac_addr);

	/* set appropriate operations */
	ndev->netdev_ops = &net_netdev_ops;

	ndev->needed_headroom += drvr->hdrlen;
	ndev->ethtool_ops = &net_ethtool_ops;

	/* set the mac address */
	memcpy(ndev->dev_addr, ifp->self_mac, ETH_ALEN);

	if (rtnl_locked)
		err = register_netdevice(ndev);
	else
		err = register_netdev(ndev);
	if (err != 0) {
		printk("couldn't register the net device\n");
		goto fail;
	}

	printk("Redbull Host Driver\n");
	return 0;

fail:
	ndev->netdev_ops = NULL;
	free_netdev(ndev);
	return -EBADE;
}

void net_free_netdev(NET_DEV ndev)
{
	struct DeviceInfo *g_dev_info;
	ssv_vif *curr_if;

	g_dev_info = netdev_priv(ndev);

	curr_if = &g_dev_info->vif[g_dev_info->used_vif];

	OS_MemSET(curr_if, 0x0, sizeof(struct ssv_vif));

	free_netdev(ndev);
}

void net_creat_if(s32 bsscfgidx, s32 ifidx, const char *name, u8 *mac_addr)
{
	NET_DEV ndev = NULL;
	struct DeviceInfo *g_dev_info;

	printk("allocate netdev interface\n");
	/* Allocate netdev, including space for private structure */
	ndev = alloc_netdev(sizeof(struct DeviceInfo *), name, NET_NAME_UNKNOWN, ether_setup);
	if (!ndev)
		return ERR_PTR(-ENOMEM);

	ndev->destructor = net_free_netdev;
	g_dev_info = netdev_priv(ndev);	
	/* record the global device info object pointer*/
	g_dev_info = gDeviceInfo;

	g_dev_info->vif[ifidx].net_dev = ndev;
	g_dev_info->vif[ifidx].ifidx = ifidx;	
	g_dev_info->vif[ifidx].bsscfgidx = bsscfgidx;

	spin_lock_init(&g_dev_info->vif[ifidx].netif_stop_lock);

	if (mac_addr != NULL)
		memcpy(g_dev_info->vif[ifidx].self_mac, mac_addr, ETH_ALEN);

	prinfk("create netdev(%s), bsscfgidx(%d), ifidx(%d)\n", name, bsscfgidx, ifidx);
}


int net_init(void)
{
	net_creat_if(0, 0, "wlan%d", NULL);	
	return 0;
}

