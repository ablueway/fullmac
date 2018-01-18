/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include "rtos.h"
#include <host_apis.h>
#include <net_mgr.h>
#include <netstack.h>
#include "ssv_frame.h"

#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>

#if CONFIG_STATUS_CHECK
extern u32 g_l2_tx_packets;
extern u32 g_l2_tx_copy;
extern u32 g_notpool;
extern u32 g_dump_tx;
extern u32 g_tx_allocate_fail;
#endif

void netdev_status_change_cb(void *netif)
{
    netmgr_l3_status_cb();
}

/* Transfer L2 packet to netstack
 * [in] data: pointer of incoming data
 * [in] len: length of real 802.3 packet
 * Transfer incoming wifi data to be processed by netstack 
 */

int netstack_input(void *data, u32 len)
{
	if (ssv6030p_recv_cb(OS_FRAME_GET_DATA(data), len, OS_FRAME_GET_HDR_OFFSET(data)) < 0)
	{
        os_frame_free(data);
	}

	return SSV6XXX_DATA_ACPT;
}

/* Transfer netstack packet to L2
 * [in] data: pointer of output data
 * [in] len: length of real 802.3 packet
 * Transfer output data to be sent 
 */

int netstack_output(void* net_interface, void *data, u32 len)
{
	int i;
	u8 *frame;
	u16 resHead;
	struct net_buf *buf = (struct net_buf *)data;
	struct net_buf *frag;
	struct net_context *context;

	if (len > 0)
	{
		context = net_nbuf_context(buf);
		if ((NULL != context) && (net_context_get_ip_proto(context) == IPPROTO_TCP)
			&& (net_nbuf_appdatalen(buf) > 0)) {
			/* Send tcp data, net_context will keep buf until receive ack */
			net_buf_set_owner(buf, NBUF_OWNER_TCPIP);
			frame = os_frame_alloc(len, true);
			memcpy(frame, net_nbuf_ll(buf), len);
			resHead = ETHERNET_WIFI_RESERVE;
		} else {
			/* just let wifi keep frags, wifi will free it */
			/* frame must point to frag->__buf, or os_frame_free can't free it */
			frag = buf->frags;
			net_buf_set_owner(buf, NBUF_OWNER_WIFI);
			frame = (uint8_t *)frag + sizeof(struct net_buf);

			resHead =  (u16)(frag->data - frame - (net_nbuf_ll_reserve(buf) - ETHERNET_WIFI_RESERVE));
			buf->frags = NULL;
		}

		net_nbuf_unref(buf);

		memset(frame, 0, resHead);
		OS_FRAME_SET_DATA_LEN(frame, (len - ETHERNET_WIFI_RESERVE));
		OS_FRAME_SET_HDR_OFFSET(frame, resHead);
	    ssv6xxx_wifi_send_ethernet(frame, OS_FRAME_GET_DATA_LEN(frame), 0);
	}

    return NS_OK;
}


/* Init netstack
 * [in] config: pointer of config
 * Init netstack with specific config
 * You may 
 * 1)init netstack
 * 2)add default net interface
 * 3)connect io of netstack and 
 */
int netstack_init(void *config)
{
    return NS_OK; 
}

/*Init device with setting
 * [in] pdev: pointer of config
 * Init netstack with specific config
 * You may 
 * init netdev
 */

int netdev_init(struct netdev *pdev, bool dft_dev, bool init_up)
{      
    return NS_OK;
}

//get hw mac
int netdev_getmacaddr(const char *ifname, u8 *macaddr)
{
	ssv6xxx_wifi_get_mac(macaddr);
    return NS_OK;
}

//get ipinfo
int netdev_getipv4info(const char *ifname, u32 *ip, u32 *gw, u32 *netmask)
{
    extern void ssv6030p_iface_get_ipaddr(u32 *ipaddr, u32 *mask, u32 *gw, u32 *dns);
    ssv6030p_iface_get_ipaddr(ip, netmask, gw, NULL);
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
    return NS_OK;
}

//set interface down
int netdev_l3_if_down(const char *ifname)
{   
    return NS_OK;
}

//interface link up cb
void netdev_link_up_cb(void *ifname)
{   
	ssv6030p_iface_link_up();
}

//interface link down cb
void netdev_link_down_cb(void *ifname)
{   
	ssv6030p_iface_link_down();
}

//set dhcp client on dev
int dhcpc_wrapper_set(const char *ifname, const bool enable)
{
    return NS_OK;
}

u32 netdev_getallnetdev(struct netdev *pdev, u32 num_of_pdev)
{
    return 0;
}

static struct net_buf *prepare_udp(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport)
{
	struct net_if *iface;
	struct net_buf *buf;
	struct net_buf *frag;
	struct net_ipv4_hdr *ipv4;
	struct net_udp_hdr *udp;
	struct in_addr tmpaddr;
	uint16_t tmplen;

	iface = net_if_get_default();

	buf = net_nbuf_get_reserve_tx(0, K_FOREVER);
	if (!buf) {
		return NULL;
	}

	frag = net_nbuf_get_reserve_data(net_if_get_ll_reserve(iface, NULL),
					 K_FOREVER);
	if (!frag) {
		net_nbuf_unref(buf);
		return NULL;
	}

	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));

	net_buf_frag_add(buf, frag);

	/* Leave room for IPv4 + UDP headers */
	net_buf_add(buf->frags, NET_IPV4UDPH_LEN);

	memcpy((frag->data + NET_IPV4UDPH_LEN), data, len);
	net_buf_add(frag, len);

	ipv4 = NET_IPV4_BUF(buf);
	udp = NET_UDP_BUF(buf);

	tmplen = net_buf_frags_len(buf->frags);

	/* Setup IPv4 header */
	memset(ipv4, 0, sizeof(struct net_ipv4_hdr));

	ipv4->vhl = 0x45;
	ipv4->ttl = 0xFF;
	ipv4->proto = IPPROTO_UDP;
	ipv4->len[0] = tmplen >> 8;
	ipv4->len[1] = (uint8_t)tmplen;
	ipv4->chksum = ~net_calc_chksum_ipv4(buf);

	tmpaddr.s_addr[0] = srcip;
	net_ipaddr_copy(&ipv4->src, &tmpaddr);
	tmpaddr.s_addr[0] = dstip;
	net_ipaddr_copy(&ipv4->dst, &tmpaddr);

	tmplen -= NET_IPV4H_LEN;
	/* Setup UDP header */
	udp->src_port = htons(srcport);
	udp->dst_port = htons(dstport);
	udp->len = htons(tmplen);
	udp->chksum = 0;
	udp->chksum = ~net_calc_chksum(buf, IPPROTO_UDP);

	return buf;
}

int netstack_udp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport, s16 rptsndtimes)
{
	struct net_buf *buf;
	int i;

	for (i = 0; i < rptsndtimes; i++) {
		buf = prepare_udp(data, len, srcip, srcport, dstip, dstport);
		if (!buf) {
			continue;
		}

		if (net_send_data(buf) < 0) {
			net_nbuf_unref(buf);
		}
	}

	return 0;
}

int netstack_tcp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport)
{
    return 0;
}

int netstack_dhcps_info_set(dhcps_info *if_dhcps, dhcps_info *des_if_dhcps)
{
    return NS_OK;
}

int netstack_udhcpd_start(void)
{
    return NS_OK;
}

int netstack_udhcpd_stop(void)
{
    return NS_OK;
}

int netstack_dhcp_ipmac_get(dhcpdipmac *ipmac, int *size_count)
{
    return NS_OK;
}

int netstack_dhcp_ip_get_by_mac(u8* Mac, u32* ip)
{
    return NS_OK;
}

int netstack_find_ip_in_arp_table(u8 * mac,netstack_ip_addr_t *ipaddr)
{
    return NS_OK;
}

int netstack_etharp_unicast (u8 *dst_mac, netstack_ip_addr_t *ipaddr)
{
    return NS_OK;
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
  return lwip_htons(n);
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
  return lwip_htonl(n);
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

  if (ssv_ipaddr_aton(cp, &val)) {
    return ssv_ip4_addr_get_u32(&val);
  }
  return (0xffffffffUL);
}

char *netstack_inet_ntoa(netstack_ip_addr_t addr)
{
    return NULL;
}

u16 netstack_ip4_addr1_16(u32* ipaddr)
{
    return 0;
}
u16 netstack_ip4_addr2_16(u32* ipaddr)
{
    return 0;
}
u16 netstack_ip4_addr3_16(u32* ipaddr)
{
    return 0;
}
u16 netstack_ip4_addr4_16(u32* ipaddr)
{
    return 0;
}
