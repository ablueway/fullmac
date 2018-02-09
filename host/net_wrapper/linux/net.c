/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include "os.h"

#ifdef __linux__
#include <linux/byteorder/generic.h>
#include <net/cfg80211.h>
#endif
#include <linux/byteorder/generic.h>


#include <host_apis.h>
#include <net_mgr.h>
#include <net_wrapper.h>
#include "ssv_frame.h"
#include "dev.h"

void netdev_status_change_cb(void *netif)
{
}

/* Transfer L2 packet to netstack
 * [in] data: pointer of incoming data
 * [in] len: length of real 802.3 packet
 * Transfer incoming wifi data to be processed by netstack 
 */
int netstack_input(void *data, u32 len, u8 vif_idx)
{
	return 0;
}

/* Transfer L3 packet to netstack
 * [in] data: pointer of incoming L3 data frame
 * [in] len: length of real 802.3 packet
 * Transfer incoming wifi data to be processed by netstack 
 */
int netstack_output(void *net_if, void *data, u32 len)
{    
    u8 *frame;
    unsigned char *pucBuffer;
    s8 retry_cnt = 5;
    struct sk_buff *skb = (struct sk_buff *)data;

retry:
	
    frame = os_frame_alloc((skb->len + 80) * sizeof(unsigned char ), FALSE);
    if (NULL == frame)
    {
        retry_cnt--;

        //don't xmit this frame. no enough frame.
        if (0 == retry_cnt)
		{
            return -1;
        }
        OS_TickDelay(1);
        goto retry;
    }
        
	pucBuffer = &((unsigned char *)skb->data)[0];
	
	/* Copy L3 frame data(i.e ethernet hdr + payload) to L2 frame pkt buffer */
	MEMCPY(OS_FRAME_GET_DATA(frame), pucBuffer, (skb->len) * sizeof(unsigned char));
    
	kfree_skb(skb);
	
	ssv6xxx_wifi_send_ethernet(0, frame, OS_FRAME_GET_DATA_LEN(frame), 0);

	return 0;
}



/*Init device with setting
 * [in] pdev: pointer of config
 * Init netstack with specific config
 * You may 
 * init netdev
 */

int netdev_init(NET_DEV *pdev, bool dft_dev, bool init_up)
{      
    return NS_OK;
}

int netdev_set_default(const char *ifname)
{
    return NS_OK;
}

//get hw mac
int netdev_getmacaddr(const char *ifname, u8 *macaddr)
{
    return NS_OK;
}

//get ipinfo
int netdev_getipv4info(const char *ifname, u32 *ip, u32 *gw, u32 *netmask)
{
    return NS_OK;
}

//set ipinfo
int netdev_setipv4info(const char *ifname, u32 ip, u32 gw, u32 netmask)
{
    return NS_OK;
}

//get dns server
//int netdev_get_ipv4dnsaddr(const char *ifname, u32 *dnsserver);
//set dns server
//int netdev_set_ipv4dnsaddr(const char *ifname, const u32 *dnsserver);

//get interface status
int netdev_check_ifup(const char *ifname)
{
    return NS_OK;
}

//set interface up
int netdev_l3_if_up(const char *ifname)
{
	printk("L3 if_up(%s)\n", ifname);
    return NS_OK;
}

//set interface down
int netdev_l3_if_down(const char *ifname)
{   
	printk("L3 if_down(%s)\n", ifname);
    return NS_OK;
}

//interface link up cb
void netdev_link_up_cb(void *ifname)
{   
	printk("L2 netdev_link_up(%s)\n", (char *)ifname);
}

//interface link down cb
void netdev_link_down_cb(void *ifname)
{   
	printk("L2 netdev_link_down(%s)\n", (char *)ifname);
}

//set dhcp client on dev
int dhcpc_wrapper_set(const char *ifname, const bool enable)
{
    return NS_OK;
}

u32 netdev_getallnetdev(NET_DEV *pdev, u32 num_of_pdev)
{
    return 0;
}

int netstack_udp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport, s16 rptsndtimes)
{
    return 0;
}

int netstack_tcp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport)
{
    return 0;
}

int netstack_find_ip_in_arp_table(u8 *mac,netstack_ip_addr_t *ipaddr)
{
    return 0;
}

int netstack_etharp_unicast(u8 *dst_mac, netstack_ip_addr_t *ipaddr)
{
    return 0;
}


int netstack_dhcps_info_set(dhcps_info *if_dhcps, dhcps_info *des_if_dhcps, u8 vif_idx)
{
#if DHCPD_SUPPORT    
    return 0;
#else
    return -1;
#endif
}

int netstack_udhcpd_start(void)
{
#if DHCPD_SUPPORT    
    return 0;
#else
    return -1;
#endif
}

int netstack_udhcpd_stop(void)
{
#if DHCPD_SUPPORT    
    return 0;
#else
    return -1;
#endif
}

int netstack_dhcp_ipmac_get(dhcpdipmac *ipmac, int *size_count)
{
#if DHCPD_SUPPORT    
    return 0;
#else
    return -1;
#endif
}

#define ssv_in_range(c, lo, up)  ((u8)c >= lo && (u8)c <= up)
#define ssv_isprint(c)           ssv_in_range(c, 0x20, 0x7f)
#define ssv_isdigit(c)           ssv_in_range(c, '0', '9')
#define ssv_isxdigit(c)          (ssv_isdigit(c) || ssv_in_range(c, 'a', 'f') || ssv_in_range(c, 'A', 'F'))
#define ssv_islower(c)           ssv_in_range(c, 'a', 'z')
#define ssv_isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#define ssv_ip4_addr_set_u32(dest_ipaddr, src_u32) ((dest_ipaddr)->addr = (src_u32))
#define ssv_ip4_addr_get_u32(src_ipaddr) ((src_ipaddr)->addr)

/**
 * Convert an u16_t from host- to network byte order.
 *
 * @param n u16_t in host byte order
 * @return n in network byte order
 */
u16
ssv_htons(u16 n)
{
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

/**
 * Convert an u16_t from network- to host byte order.
 *
 * @param n u16_t in network byte order
 * @return n in host byte order
 */
u16
ssv_ntohs(u16 n)
{
  return htons(n);
}

/**
 * Convert an u32_t from host- to network byte order.
 *
 * @param n u32_t in host byte order
 * @return n in network byte order
 */
u32
ssv_htonl(u32 n)
{
  return ((n & 0xff) << 24) |
    ((n & 0xff00) << 8) |
    ((n & 0xff0000UL) >> 8) |
    ((n & 0xff000000UL) >> 24);
}

/**
 * Convert an u32_t from network- to host byte order.
 *
 * @param n u32_t in network byte order
 * @return n in host byte order
 */
u32
ssv_ntohl(u32 n)
{
  return htonl(n);
}

/**
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int
ssv_ipaddr_aton(const char *cp, netstack_ip_addr_t *addr)
{
  u32 val;
  u8 base;
  char c;
  u32 parts[4];
  u32 *pp = parts;

  c = *cp;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
    if (!ssv_isdigit(c))
      return (0);
    val = 0;
    base = 10;
    if (c == '0') {
      c = *++cp;
      if (c == 'x' || c == 'X') {
        base = 16;
        c = *++cp;
      } else
        base = 8;
    }
    for (;;) {
      if (ssv_isdigit(c)) {
        val = (val * base) + (int)(c - '0');
        c = *++cp;
      } else if (base == 16 && ssv_isxdigit(c)) {
        val = (val << 4) | (int)(c + 10 - (ssv_islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else
        break;
    }
    if (c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
      if (pp >= parts + 3) {
        return (0);
      }
      *pp++ = val;
      c = *++cp;
    } else
      break;
  }
  /*
   * Check for trailing characters.
   */
  if (c != '\0' && !ssv_isspace(c)) {
    return (0);
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  switch (pp - parts + 1) {

  case 0:
    return (0);       /* initial nondigit */

  case 1:             /* a -- 32 bits */
    break;

  case 2:             /* a.b -- 8.24 bits */
    if (val > 0xffffffUL) {
      return (0);
    }
    val |= parts[0] << 24;
    break;

  case 3:             /* a.b.c -- 8.8.16 bits */
    if (val > 0xffff) {
      return (0);
    }
    val |= (parts[0] << 24) | (parts[1] << 16);
    break;

  case 4:             /* a.b.c.d -- 8.8.8.8 bits */
    if (val > 0xff) {
      return (0);
    }
    val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
    break;
  default:
    //LWIP_ASSERT("unhandled", 0);
    break;
  }
  if (addr) {
    ssv_ip4_addr_set_u32(addr, ssv_htonl(val));
  }
  return (1);
}

/**
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @return ip address in network order
 */
u32 netstack_ipaddr_addr(const char *cp)
{
	netstack_ip_addr_t val;

	if (ssv_ipaddr_aton(cp, &val)) 
	{
		return ssv_ip4_addr_get_u32(&val);
	}
	return (0xffffffffUL);
}

char *netstack_inet_ntoa(netstack_ip_addr_t addr)
{
	return NULL;
}

u16 netstack_ip4_addr1_16(u32 *ipaddr)
{
	return 0;
}
u16 netstack_ip4_addr2_16(u32 *ipaddr)
{
	return 0;
}
u16 netstack_ip4_addr3_16(u32 *ipaddr)
{
	return 0;
}
u16 netstack_ip4_addr4_16(u32 *ipaddr)
{
	return 0;
}


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

	/* TODO(aaron): brcom do this ?? Clear, carrier, set when connected or AP mode. */
//	netif_carrier_off(ndev);

	/* TODO(aaron): MTK do this ?? need check what way we need to do */
	netif_start_queue(ndev);
	netif_carrier_on(ndev);
	netif_wake_queue(ndev);

	return 0;
}

static const struct net_device_ops net_netdev_ops = {
	.ndo_open = ssv_netdev_open,
	.ndo_stop = ssv_netdev_stop,
//	.ndo_get_stats = ssv_netdev_get_stats,
	.ndo_start_xmit = ssv_netdev_start_xmit,	
};

int net_netdev_ops_attach(ssv_vif *vif, bool rtnl_locked)
{
	s32 err;
	NET_DEV *ndev = vif->net_dev;;

	printk("Enter, bsscfgidx=%d mac=%pM\n", vif->bsscfgidx, vif->self_mac);

	/* set appropriate operations */
	ndev->netdev_ops = &net_netdev_ops;

	ndev->needed_headroom += 80;
	ndev->ethtool_ops = &net_ethtool_ops;

	/* TODO(aaron): should we hook iw handler here ? does cfg80211 need this ? */
//	ndev->wireless_handlers = 

	/* set the mac address */
	memcpy(ndev->dev_addr, vif->self_mac, ETH_ALEN);

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
	ndev = alloc_netdev(sizeof(struct DeviceInfo), name, ether_setup);
	if (!ndev)
		return -1;

	ndev->destructor = net_free_netdev;

	/* record the g_dev_info to global device info object pointer */
	gDeviceInfo = netdev_priv(ndev);	
	
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

extern struct wiphy *net_cfg80211_init(size_t priv_data_len);
extern int net_cfg80211_register(struct wiphy *wiphy);

int net_init(void *config)
{
	net_creat_if(0, 0, "wlan%d", NULL);	

	net_cfg80211_register(net_cfg80211_init(sizeof(struct dev_info_wrapper)));

	net_netdev_ops_attach(&gDeviceInfo->vif[0], FALSE);

	return 0;
}

