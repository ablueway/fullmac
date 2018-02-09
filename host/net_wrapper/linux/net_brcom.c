/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include "os.h"
#include <host_apis.h>
#include <net_mgr.h>
#include <net_wrapper.h>

/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include "os.h"
#include <host_apis.h>
#include <net_mgr.h>
#include <net_wrapper.h>
#include "dev.h"


static netdev_tx_t ssv_netdev_start_xmit(struct sk_buff *skb,
					   NET_DEV *ndev)
{
	struct DeviceInfo *g_dev_info = netdev_priv(ndev);
	struct ssv_vif_st *curr_vif = &g_dev_info->vif[0];

 	netstack_output(curr_vif, skb, skb->len);
	/* Return ok: we always eat the packet */
	return NETDEV_TX_OK;
}



static void net_ethtool_get_drvinfo(struct net_device *ndev,
				    struct ethtool_drvinfo *info)
{
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

static int ssv_netdev_stop(struct net_device *ndev)
{
	unsigned long flags;
	struct DeviceInfo *g_dev_info = netdev_priv(ndev);
	ssv_vif *curr_if = &g_dev_info->vif[0];
	spin_lock_irqsave(&curr_if->netif_stop_lock, flags);

	netif_stop_queue(ndev);

	/* TODO(aaron): should we record reason code ? */
	curr_if->netif_stop_reason = 0xff;
	spin_unlock_irqrestore(&curr_if->netif_stop_lock, flags);

	return 0;
}

static int ssv_netdev_open(NET_DEV *ndev)
{
	struct DeviceInfo *g_dev_info = netdev_priv(ndev);
	ssv_vif *curr_if = &g_dev_info->vif[0];
	/* if reference count */
	if (curr_if->vif_cnt != 0)
	{
		return -1;
	}
	curr_if->vif_cnt++;

	/* TODO(aaron): should we need hook this here ? */
//	net_dev->wireless_handlers = ??

	/* TODO(aaron): should we need do chip & other init here ? */

	/* notify kernel, our hw not support checksum offload */	
	ndev->features &= ~NETIF_F_IP_CSUM;

	/* Clear, carrier, set when connected or AP mode. */
	netif_carrier_off(ndev);

	return 0;
}

static const struct net_device_ops net_netdev_ops = {
	.ndo_open = ssv_netdev_open,
	.ndo_stop = ssv_netdev_stop,
//	.ndo_get_stats = ssv_netdev_get_stats,
	.ndo_start_xmit = ssv_netdev_start_xmit,	
};

int net_netdev_ops_attach(ssv_vif *ifp, bool rtnl_locked)
{
	s32 err;
	NET_DEV *ndev = ifp->net_dev;;

	printk("Enter, bsscfgidx=%d mac=%pM\n", ifp->bsscfgidx, ifp->self_mac);

	/* set appropriate operations */
	ndev->netdev_ops = &net_netdev_ops;

	ndev->needed_headroom += 80;
	ndev->ethtool_ops = &net_ethtool_ops;

	/* TODO(aaron): should we hook iw handler here ? does cfg80211 need this ? */
//	ndev->wireless_handlers = 

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

void net_free_netdev(NET_DEV *ndev)
{
	struct DeviceInfo *g_dev_info;
	ssv_vif *curr_if;

	g_dev_info = netdev_priv(ndev);

	/* TODO(aaron): temply hard code if 0, need to fix */
	curr_if = &g_dev_info->vif[0];

	OS_MemSET(curr_if, 0x0, sizeof(struct ssv_vif_st));

	free_netdev(ndev);
}

int net_creat_if(s32 bsscfgidx, s32 ifidx, const char *name, u8 *mac_addr)
{
	NET_DEV *ndev = NULL;
	struct DeviceInfo *g_dev_info;

	printk("allocate netdev interface\n");
	/* Allocate netdev, including space for private structure */
	ndev = alloc_netdev(sizeof(struct DeviceInfo *), name, ether_setup);
	if (!ndev)
		return -1;

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

	printk("create netdev(%s), bsscfgidx(%d), ifidx(%d)\n", name, bsscfgidx, ifidx);

	return 0;
}


int net_init(void *config)
{
	net_creat_if(0, 0, "wlan%d", NULL);	
	return 0;
}

