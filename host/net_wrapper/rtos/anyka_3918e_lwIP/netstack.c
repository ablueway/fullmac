/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "netif/etharp.h"

#include "rtos.h"
#include <host_apis.h>
#include <net_mgr.h>
#include <netstack.h>
#include <dhcpserver.h>



#if CONFIG_STATUS_CHECK
extern u32 g_l2_tx_packets;
extern u32 g_l2_tx_copy;
extern u32 g_notpool;
extern u32 g_dump_tx;
extern u32 g_tx_allocate_fail;
#endif

//#include <pbuf.h>

#include <os_wrapper.h>
#include <log.h>

void netdev_status_change_cb(struct netif *netif)
{
}

int calc_bandwidth(int len)
{ 
    static int tick1 = 0;
    static int totallen = 0;
    int tick2 = 0;
    if (tick1 ==0 ) tick1 = OS_GetSysTick();
    totallen += len;
    tick2 = OS_GetSysTick();

    if ((tick2 - tick1) * TICK_RATE_MS > 1000)
    {
        printf("bandwitdh = %d Mbps\r\n", totallen*8/((tick2 - tick1) * TICK_RATE_MS)/1000);
        totallen = 0;
        tick1 = OS_GetSysTick();
    }
}

int g_debug_bandwidth = 0;
extern struct netif *p_netif;

/* Transfer L2 packet to netstack
 * [in] data: pointer of incoming data
 * [in] len: length of real 802.3 packet
 * Transfer incoming wifi data to be processed by netstack 
 */
int netstack_input(void *data, u32 len)
{   
    if ((g_debug_bandwidth) && (len == 1066))
    {
        calc_bandwidth(len);
    	os_frame_free(data);
        return 0;
    }
    
    if (p_netif)
    {
        ethernetif_input(p_netif,OS_FRAME_GET_DATA(data),len);
    }
    
    os_frame_free(data);
	return 0;
}

/* Transfer netstack packet to L2
 * [in] data: pointer of output data
 * [in] len: length of real 802.3 packet
 * Transfer output data to be sent 
 */

void print_data(char *func, void *data, int len)
{
    int i = 0;
    LOG_DEBUGF(LOG_TXRX, ("\r\n[%s]\r\n", func));
    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0) LOG_DEBUGF(LOG_TXRX, ("\r\n"));
        LOG_DEBUGF(LOG_TXRX, ("%02X ", *((char *)data + i)));
    }
    LOG_DEBUGF(LOG_TXRX, ("\r\n"));
}

int netstack_output(void *net_interface, void *data, u32 len)
{
	struct pbuf *q;
	static unsigned char *ucBuffer;  
	unsigned char *pucBuffer = ucBuffer;
	unsigned char *pucChar;

	u8 *frame;
	s8 retry_cnt = 5;

	struct pbuf *p = (struct pbuf *)data;

	retry:
	frame = os_frame_alloc(p->tot_len*sizeof(unsigned char ), FALSE);
	 if(NULL == frame){
		 retry_cnt--;
		 
#if CONFIG_STATUS_CHECK
		 g_tx_allocate_fail++;
#endif

		 //don't xmit this frame. no enough frame.
		 if(0 == retry_cnt){
			return -1;
		 }
		 OS_TickDelay(1);
		 goto retry;
	 }

	ucBuffer = OS_FRAME_GET_DATA(frame);

	/* Initiate transfer. */

	if( p->len == p->tot_len ) 
	{
		/* No pbuf chain, don't have to copy -> faster. */
		pucBuffer = &( ( unsigned char * ) p->payload )[ ETH_PAD_SIZE ];
		MEMCPY(ucBuffer, pucBuffer, p->tot_len*sizeof(unsigned char ));
	} 
	else 
	{
			pucChar = ucBuffer;
		for( q = p; q != NULL; q = q->next ) 
		{
			/* Send the data from the pbuf to the interface, one pbuf at a
			time. The size of the data in each pbuf is kept in the ->len
			variable. */
			/* send data from(q->payload, q->len); */
			LWIP_DEBUGF( NETIF_DEBUG, ("NETIF: send pucChar %p q->payload %p q->len %i q->next %p\r\n", pucChar, q->payload, ( int ) q->len, ( void* ) q->next ) );
			if( q == p ) 
			{
			
				MEMCPY( pucChar, &( ( char * ) q->payload )[ ETH_PAD_SIZE ], q->len - ETH_PAD_SIZE );
				pucChar += q->len - ETH_PAD_SIZE;					
			} 
			else 
			{					
				MEMCPY( pucChar, q->payload, q->len );
				pucChar += q->len;		
			}				
		}		
	}
    print_data(__func__, OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
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
	ssv6xxx_wifi_get_mac(macaddr);
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
    extern struct netif *netif_default;
    struct netif * pwlan = netif_default;
    if (NULL == pwlan)        
        return NS_ERR_ARG;
    
    if (pwlan->flags & NETDEV_IF_UP)
    {
        return NS_OK;
    }
    else
    {
        return NS_NG;
    }
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
	if (netmgr_wifi_check_sta())
    {
        //lwip_wifistation_netif_init();
    }
	else if(netmgr_wifi_check_ap())
	{
		//lwip_wifiap_netif_init();
	}
}

//interface link down cb
void netdev_link_down_cb(void *ifname)
{   
}

//set dhcp client on dev
int dhcpc_wrapper_set(const char *ifname, const bool enable)
{
    return NS_OK;
}

u32 netdev_getallnetdev(struct netdev *pdev, u32 num_of_pdev)
{
    extern struct netif *netif_list;
    struct netif *netif;
    u32 num = 0;
    if ((pdev == NULL) ||(num_of_pdev == 0))
        return 0;
        
    for(netif=netif_list; ((num < num_of_pdev) && (netif!=NULL)); netif=netif->next)        
    {
        MEMCPY((void *)&pdev[num].name, (void *)&netif->name, sizeof(netif->name));
        MEMCPY((void *)&pdev[num].hwmac, (void *)&netif->hwaddr, sizeof(netif->hwaddr));
        pdev[num].flags = netif->flags;
        pdev[num].ipaddr = netif->ip_addr.addr;
        pdev[num].gw = netif->gw.addr;
        pdev[num].netmask = netif->netmask.addr;
        pdev[num].mtu = netif->mtu;
        num++;
    }
    return num;
}


int netstack_udp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport, s16 rptsndtimes)
{
    int err=0;
#if 0
    struct netconn *conn;
    struct netbuf *buf;
    char *netbuf_data;

    ip_addr_t srcaddr, dstaddr;
    
    
    conn = netconn_new(NETCONN_UDP);    
    if(conn==NULL) err++;
    srcaddr.addr = srcip;
    dstaddr.addr = dstip;

    if(err==0){
        if(ERR_OK!=netconn_bind(conn, &srcaddr, srcport)) err++;
    }

    if(err==0){
        if(ERR_OK!=netconn_connect(conn, &dstaddr, dstport)) err++;
    }
    if(err==0){
        do{
            buf=netbuf_new();
            if(buf==NULL) {
                netbuf_delete(buf);
                err++;
                break;
            }

            netbuf_data=netbuf_alloc(buf,len);
            if(netbuf_data==NULL) {
                err++;
                break;
            }
            ssv6xxx_memcpy(netbuf_data,(void *)data,len);
            if(ERR_OK!=netconn_send(conn,buf)){
                netbuf_delete(buf);
                err++;
                break;
            }
            netbuf_delete(buf);
        }while(rptsndtimes--);
    }

    if(conn!=NULL) netconn_delete(conn);

    if(err!=0){
        LOG_PRINTF("AirKiss fail\r\n");
    }else{
        LOG_PRINTF("Airkiss ok\r\n");
    }
#else
    int sockfd;
    u32 nbytes;
    struct sockaddr_in dst;
    socklen_t addrlen;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        //LOG_PRINTF("client socket failure %d\n", errno);
        return -1;
    }

    dst.sin_family             = AF_INET;
    dst.sin_port               = htons(dstport);
    dst.sin_addr.s_addr        = htonl(dstip); //broad cast address
    addrlen                       = sizeof(struct sockaddr_in);
    
    do{
        nbytes = sendto(sockfd, data, len, 0,(struct sockaddr *)&dst, addrlen);
        if (nbytes == len)
        {
            break;
        }
        rptsndtimes--;
    }while(rptsndtimes > 0);

    close(sockfd);

    if (rptsndtimes > 0)
    {
        LOG_PRINTF("Airkiss ok\r\n");
    }
    else
    {
        LOG_PRINTF("AirKiss fail\r\n");
    }
#endif
    return err;
}


int netstack_tcp_send(void* data, u32 len, u32 srcip, u16 srcport, u32 dstip, u16 dstport)
{
    return 0;
}

int netstack_dhcps_info_set(dhcps_info *if_dhcps, dhcps_info *des_if_dhcps)
{
 
}

int netstack_udhcpd_start(void)
{
	return NS_OK;
}

int netstack_udhcpd_stop(void)
{

}

int netstack_dhcp_ipmac_get(dhcpdipmac *ipmac, int *size_count)
{
    int i = 0;
    struct dhcps_pool *lease = NULL;
    int ret = 0;

    if (!ipmac || !size_count || (*size_count <= 0))
    {
        return -1;
    }

    lease = (struct dhcps_pool *)MALLOC(sizeof(struct dhcps_pool) * (*size_count));
    if (!lease)
    {
        return -1;
    }

    ret = dhcpd_lease_get_api(lease, size_count);

    if (ret == 0)
    {
        for (i = 0; i < *size_count; i++)
        {
           ((dhcpdipmac *) (ipmac + i))->ip = lease[i].ip.addr;
            MEMCPY((void*)(((dhcpdipmac *) (ipmac + i))->mac), (void*)(lease[i].mac), 6);
        }
    }

    FREE((void*)lease);
    return 0;
}

int netstack_find_ip_in_arp_table(u8 * mac,netstack_ip_addr_t *ipaddr)
{
    struct eth_addr* ethaddr_ret;
    ip_addr_t* ipaddr_ret;
    struct netif *netif = NULL;

    if((mac==NULL) || (ipaddr==NULL))
    {
        return 0;
    }

    if (etharp_find_addr_by_mac(netif, mac, &ethaddr_ret, &ipaddr_ret) > -1)
    {
        ipaddr->addr = ipaddr_ret->addr;
        return 1;
    }
    
    return 0;
}

int netstack_dhcp_ip_get_by_mac(u8* Mac, u32* ip)
{
#if DHCPD_SUPPORT    
    int i = 0;
    struct dyn_lease *lease = NULL;
    int ret = 0;
    int size_count=10;

    //LOG_PRINTF("Input [%02x:%02x:%02x:%02x:%02x:%02x]\r\n", 
    //    Mac[0], Mac[1], Mac[2],Mac[3], Mac[4], Mac[5]);
    
    lease = (struct dyn_lease *)MALLOC(sizeof(struct dyn_lease) * 10);
    if (!lease)
    {
        return -1;
    }

    ret = dhcpd_lease_get_api(lease, &size_count);

    if (ret == 0)
    {
        for (i = 0; i < size_count; i++)
        {            
            if(!MEMCMP((void*)Mac, (void*)(lease[i].lease_mac), 6))
            {
                *ip = lease[i].lease_nip;
            }
                
        }
    }

    FREE((void*)lease);

    return 0;
#else
    return -1;
#endif //#if DHCPD_SUPPORT
}

int netstack_etharp_unicast (u8 *dst_mac, netstack_ip_addr_t *ipaddr)
{
    ip_addr_t tmp_ipaddr;
    tmp_ipaddr.addr = ipaddr->addr;
    return etharp_unicast(dst_mac, &tmp_ipaddr);
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
ssv_htons(u16_t n)
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
ssv_ntohs(u16_t n)
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
ssv_htonl(u32_t n)
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
ssv_ntohl(u32_t n)
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

#if 1
int wwd_softap_start( char *ssid, char *password, unsigned char security, unsigned char channel )
{
    wifi_mode mode;
    wifi_ap_cfg ap_cfg;
    int ret = 0;
    u32 start_tick = 0;
    bool timeout = false;

    MEMSET((void * )&ap_cfg, 0, sizeof(ap_cfg));
    mode = SSV6XXX_HWM_AP;
    ap_cfg.status = TRUE;
    MEMCPY((void *)ap_cfg.ssid.ssid,ssid,STRLEN(ssid));
    ap_cfg.ssid.ssid_len = STRLEN(ssid);
    STRCPY((void *)(ap_cfg.password),password);
    if (security == WIFI_APMODE_SEC_OPEN)
    {
        ap_cfg.security =  SSV6XXX_SEC_NONE;
    }
	else if (security == WIFI_APMODE_SEC_WEP)
    {
		ap_cfg.status = TRUE;
		if(STRLEN(password) == 5)
			ap_cfg.security =	SSV6XXX_SEC_WEP_40;
		else if (STRLEN(password) == 13)
			ap_cfg.security =	SSV6XXX_SEC_WEP_104;
		else
		{
			LOG_PRINTF("ERR:WEP key length must be 5 or 13 character. \r\n");
			
		}

    }
#ifdef SSV_SUPPORT_WPA
    else if (security == WIFI_APMODE_SEC_WPA)
    {
        ap_cfg.security =   SSV6XXX_SEC_WPA_PSK;
        ap_cfg.proto = WPA_PROTO_RSN;
        ap_cfg.key_mgmt = WPA_KEY_MGMT_PSK ;
        ap_cfg.group_cipher=WPA_CIPHER_CCMP;
        ap_cfg.pairwise_cipher = WPA_CIPHER_CCMP;
    }
#endif 
    else if (security == WIFI_APMODE_SEC_WPA2)
    {
        ap_cfg.security =   SSV6XXX_SEC_WPA2_PSK;
        ap_cfg.proto = WPA_PROTO_RSN;
        ap_cfg.key_mgmt = WPA_KEY_MGMT_PSK ;
        ap_cfg.group_cipher=WPA_CIPHER_CCMP;
        ap_cfg.pairwise_cipher = WPA_CIPHER_CCMP;
    }
    
    ap_cfg.channel = (channel == 0) ? EN_CHANNEL_AUTO_SELECT : channel;

    ret = netmgr_wifi_control_async(mode , &ap_cfg, NULL);
    if (ret != 0)
    {
        LOG_PRINTF("AP mode error\r\n");
        return -1;
    }
    
    start_tick = OS_GetSysTick();
    timeout = false;
    while (!netmgr_wifi_check_ap())
    {
        // wait 3 second timeout
        if (((OS_GetSysTick() - start_tick) * OS_TICK_RATE_MS) >= 3000)
        {
            timeout = true;
            break;
        }
        OS_TickDelay(1);
    }
    
    if (timeout)
    {
        LOG_PRINTF("AP mode timeout\r\n");
        return -1;
    }
    
    LOG_PRINTF("AP mode success\r\n");
    
    return 0;
}
#endif
int wwd_softap_stop(unsigned char force)
{
    wifi_mode mode;
    wifi_ap_cfg ap_cfg;
    int ret = 0;
    MEMSET((void * )&ap_cfg, 0, sizeof(ap_cfg));
    mode = SSV6XXX_HWM_AP;
    ap_cfg.status = FALSE;

    ret = netmgr_wifi_control_async(mode, &ap_cfg, NULL);
    if (ret != 0)
    {
        return -1;
    }
    
    return 0;
	
}

void netstack_wifi_link_register_cb(netdev_link_callback_t link_up_cb, netdev_link_callback_t link_down_cb)
{
    netmgr_wifi_link_register_cb(link_up_cb, link_down_cb);
}

