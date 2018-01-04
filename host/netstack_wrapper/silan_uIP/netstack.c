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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <nuttx/config.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <apps/netutils/netlib.h>

#include "yunos_bsp_wifi.h"
#include "../../../common/wireless_ssv/wwd/libwwd.h"


#if CONFIG_STATUS_CHECK
extern u32 g_l2_tx_packets;
extern u32 g_l2_tx_copy;
extern u32 g_notpool;
extern u32 g_dump_tx;
extern u32 g_tx_allocate_fail;
#endif

extern void netmgr_ifup_cb(void);
void netdev_status_change_cb(struct netif *netif)
{
    //if (netif->flags & NETIF_FLAG_UP)
    //    netmgr_ifup_cb();
}

/* Transfer L2 packet to netstack
 * [in] data: pointer of incoming data
 * [in] len: length of real 802.3 packet
 * Transfer incoming wifi data to be processed by netstack 
 */

extern wifi_recv_eth_cb_t g_wifi_recv_eth_cb;

int netstack_input(void *data, u32 len)
{
    if (g_wifi_recv_eth_cb)
        g_wifi_recv_eth_cb(data, len);
    return SSV6XXX_DATA_ACPT;
}

/* Transfer netstack packet to L2
 * [in] data: pointer of output data
 * [in] len: length of real 802.3 packet
 * Transfer output data to be sent 
 */
int netstack_output(void* net_interface, void *data, u32 len)
{   
    u8 *frame;
    s8 retry_cnt = 5;

retry:           
    frame = os_frame_alloc(len*sizeof(unsigned char));
    
    if(NULL == frame){
        retry_cnt--;
        
#if CONFIG_STATUS_CHECK
        g_tx_allocate_fail++;
#endif

        //don't xmit this frame. no enough frame.
        if(0 == retry_cnt){
           return 0;
        }
        OS_TickDelay(1);
        goto retry;
    }
    //print_hex("wifidrv_output",p,length);
    MEMCPY(OS_FRAME_GET_DATA(frame), data, len);
    ssv6xxx_wifi_send_ethernet(frame, OS_FRAME_GET_DATA_LEN(frame), 0);
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
    bcm_link_up_cb();
}

//interface link down cb
void netdev_link_down_cb(void *ifname)
{   
    bcm_link_down_cb();
}

//set dhcp client on dev
int dhcpc_wrapper_set(const char *ifname, const bool enable)
{    
    return NS_OK;        
}

u32 netdev_getallnetdev(struct netdev *pdev, u32 num_of_pdev)
{
    return NS_OK; 
}


int netstack_udp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport, s16 rptsndtimes)
{
    return NS_OK;
}

int netstack_tcp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport)
{
    return NS_OK;
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
  return ssv_htons(n);
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
  return ssv_htonl(n);
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


