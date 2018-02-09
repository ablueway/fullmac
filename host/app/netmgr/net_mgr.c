/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#include <linux/kthread.h>
#endif

#include <host_config.h>
#include "net_wrapper.h"
#include <ssv_hal.h>

#include <ssv_drv.h>

#include <dev.h>
//#include <netstack.h>
#include <os_wrapper.h>
#include "net_mgr.h"


#include <log.h>
#include <os.h>

#include <ssv_common.h>



extern u16 g_sta_channel_mask;
#if(ENABLE_SMART_CONFIG==1)
extern SSV6XXX_USER_SCONFIG_OP ssv6xxx_user_sconfig_op;
#endif
extern struct Host_cfg g_host_cfg;

static bool g_switch_join_cfg_b = false;
static wifi_sta_join_cfg g_join_cfg_data;
//static struct netdev g_netifdev[MAX_VIF_NUM];
static NET_DEV g_netifdev[MAX_VIF_NUM];

typedef struct st_netmgr_sconfig_result
{
    //wifi_sec_type    sec_type;
    struct cfg_80211_ssid ssid;
    u8                  password[MAX_PASSWD_LEN+1];
    u8                  channel;
    u8                  dat;
    u8                  valid;
    u8                  vif_idx;
} netmgr_sconfig_result;

static netmgr_sconfig_result sconfig_result;

extern u32 g_sconfig_solution;
#if(ENABLE_SMART_CONFIG==1)
extern u16 g_SconfigChannelMask;
extern u32 g_Sconfig5gChannelMask;
extern u8 g_sconfig_auto_join;
#endif
bool g_sconfig_user_mode;
bool g_sconfig_running=FALSE;


extern struct ssv6xxx_ieee80211_bss *ssv6xxx_wifi_find_ap_ssid(struct cfg_80211_ssid *ssid);

//static void netmgr_task(void *arg);
static void netmgr_wifi_reg_event(void);
static void netmgr_wifi_event_cb(u32 evt_id, void *data, s32 len);
int netmgr_wifi_switch_to_sta(wifi_sta_join_cfg *join_cfg, u8 join_channel);
static int _netmgr_wifi_switch(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_join_cfg *join_cfg, u16 scanning_channel_mask, u32 scanning_5g_channel_mask, bool sta_reset);
static void _netmgr_wifi_recovery_cb(void);

#ifdef __linux__
static int netmgr_task(void *arg);
#else
static void netmgr_task(void *arg);
#endif

struct task_info_st st_netmgr_task[] = 
{
	{
		.task_name 		= "netmgr_task", 
		.qevt			= NULL,
		.qlength		= 4,
		.prio 			= OS_NETMGR_TASK_PRIO, 
		.stack_size 	= NETMGR_STACK_SIZE, 
		.args 			= NULL, 
		.task_func 		= netmgr_task,
	}
};

static bool g_wifi_is_joining_b = false; //
//struct resp_evt_result *sconfig_done_cpy=NULL;

#ifdef  NET_MGR_AUTO_JOIN
#define NET_MGR_USER_AP_COUNT     10

typedef struct st_user_ap_info
{
    bool valid;
    struct cfg_80211_ssid ssid;
    char password[MAX_PASSWD_LEN+1];
    int join_times;
}user_ap_info;

static user_ap_info g_user_ap_info[NET_MGR_USER_AP_COUNT];

static struct ssv6xxx_ieee80211_bss *g_ap_list_p = NULL;

void netmgr_apinfo_clear();
void netmgr_apinfo_remove(char *ssid);
user_ap_info * netmgr_apinfo_find(char *ssid);
bool netmgr_apinfo_find_in_aplist(struct ssv6xxx_ieee80211_bss * ap_list_p, int count, char * ssid);
user_ap_info * netmgr_apinfo_find_best(struct ssv6xxx_ieee80211_bss * ap_list_p, int count);
void netmgr_apinfo_save();
void netmgr_apinfo_set(user_ap_info *ap_info, bool valid);
static int netmgr_apinfo_autojoin(user_ap_info *ap_info);
void netmgr_apinfo_show();
#endif

#ifdef NET_MGR_AUTO_RETRY

typedef struct st_auto_retry_ap_info
{
    struct cfg_80211_ssid ssid;
    char password[MAX_PASSWD_LEN+1];    
    u8                  vif_idx;
}auto_retry_ap_info;

typedef enum {
    S_TRY_INIT,
    S_TRY_RUN,
    S_TRY_STOP,
    S_TRY_INVALID = 0XFF,
}E_AUTO_TRY_STATE;

static E_AUTO_TRY_STATE g_auto_retry_status;
extern int  g_auto_retry_times_delay;
extern int  g_auto_retry_times_max;
static int  g_auto_retry_times;
static u32  g_auto_retry_start_time;
static auto_retry_ap_info g_auto_retry_ap;
extern void netmgr_auto_retry_update(void);
extern void netmgr_auto_retry_show(void);
#endif

static netmgr_cfg g_netmgr_config[MAX_VIF_NUM];

static int netmgr_dhcpd_start(bool enable ,u8 vif_idx);
static int netmgr_dhcpc_start(bool enable, u8 vif_idx);

int netmgr_wifi_sconfig_done(u8 *resp_data, u32 len, bool IsUDP,u32 port);

typedef struct st_wifi_sec_info
{
    char *sec_name;
    char dfl_password[MAX_PASSWD_LEN+1];
}wifi_sec_info;

const wifi_sec_info g_sec_info[SSV6XXX_SEC_MAX] =
{
    {"open",   ""},                              // WIFI_SEC_NONE
    {"wep40",  {0x31,0x32,0x33,0x34,0x35,0x00,}}, // WIFI_SEC_WEP_40
    {"wep104", "0123456789012"},                 // WIFI_SEC_WEP_104
    {"wpa",    "secret00"},                      // WIFI_SEC_WPA_PSK
    {"wpa2",   "secret00"},                      // WIFI_SEC_WPA2_PSK
    {"wps",    ""}, // WIFI_SEC_WPS

};
char *if_name[] = {"wlan0", "wlan1"};


static void netmgr_cfg_default(netmgr_cfg *p_cfg)
{
    if (p_cfg)
    {
        p_cfg->ipaddr  = netstack_inet_addr(DEFAULT_IPADDR);
        p_cfg->netmask = netstack_inet_addr(DEFAULT_SUBNET);
        p_cfg->gw      = netstack_inet_addr(DEFAULT_GATEWAY);
        p_cfg->dns     = netstack_inet_addr(DEFAULT_DNS);

        p_cfg->dhcps.start_ip     = netstack_inet_addr(DEFAULT_DHCP_START_IP);
        p_cfg->dhcps.end_ip     = netstack_inet_addr(DEFAULT_DHCP_END_IP);

        p_cfg->dhcps.max_leases     = DEFAULT_DHCP_MAX_LEASES;
        p_cfg->dhcps.subnet     = netstack_inet_addr(DEFAULT_SUBNET);
        p_cfg->dhcps.gw     = netstack_inet_addr(DEFAULT_GATEWAY);
        p_cfg->dhcps.dns     = netstack_inet_addr(DEFAULT_DNS);

        p_cfg->dhcps.auto_time     = DEFAULT_DHCP_AUTO_TIME;
        p_cfg->dhcps.decline_time     = DEFAULT_DHCP_DECLINE_TIME;
        p_cfg->dhcps.conflict_time     = DEFAULT_DHCP_CONFLICT_TIME;
        p_cfg->dhcps.offer_time     = DEFAULT_DHCP_OFFER_TIME;
        p_cfg->dhcps.max_lease_sec     = DEFAULT_DHCP_MAX_LEASE_SEC;
        p_cfg->dhcps.min_lease_sec     = DEFAULT_DHCP_MIN_LEASE_SEC;

        p_cfg->s_dhcpc_enable = 0;
        p_cfg->s_dhcpd_enable = 0;
        p_cfg->s_dhcpc_status = 0;
        p_cfg->s_dhcpd_status = 0;
    }
}

netdev_link_callback_t g_wifi_link_up_cb = NULL;
netdev_link_callback_t g_wifi_link_down_cb = NULL;

void netmgr_wifi_link_register_cb(netdev_link_callback_t link_up_cb, netdev_link_callback_t link_down_cb)
{
    if (link_up_cb)
    {
        g_wifi_link_up_cb = link_up_cb;
    }

    if (link_down_cb)
    {
        g_wifi_link_down_cb = link_down_cb;
    }
}

void ssv_netmgr_add_netdev(u8 vif_idx, bool init_up)
{
    if (g_netifdev[vif_idx].add_state == FALSE)
    {
		LOG_PRINTF("ssv_netmgr_add_netdev vif=%d,init_up=%d\r\n", vif_idx,init_up);
		ssv6xxx_wifi_get_mac((u8 *)g_netifdev[vif_idx].hwmac, vif_idx);
		MEMCPY((void *)(g_netifdev[vif_idx].name),if_name[vif_idx], sizeof(WLAN_IFNAME));
		g_netifdev[vif_idx].ipaddr = g_netmgr_config[vif_idx].ipaddr;
		g_netifdev[vif_idx].netmask = g_netmgr_config[vif_idx].netmask;
		g_netifdev[vif_idx].gw =  g_netmgr_config[vif_idx].gw;
		netdev_init(&g_netifdev[vif_idx], TRUE, init_up);
		g_netifdev[vif_idx].add_state = TRUE;
    }
}

void ssv_netmgr_init_netdev(bool default_cfg)
{
    u8 vif_idx = 0;

    LOG_PRINTF("g_netmgr_config[0].s_dhcpd_enable=%d\r\n", 
						g_netmgr_config[0].s_dhcpd_enable);
	/* MAX_VIF_NUM(2) */
    for (vif_idx = 0; vif_idx < MAX_VIF_NUM; vif_idx++)
    {
        if (default_cfg || (g_netmgr_config[vif_idx].ipaddr == 0))
        {
            netmgr_cfg_default(&g_netmgr_config[vif_idx]);
        }
        MEMSET((void *)&g_netifdev[vif_idx], 0, sizeof(struct netdev));
    }
    ssv6xxx_wifi_reg_rx_cb((data_handler)netstack_input);

    netmgr_wifi_link_register_cb(netdev_link_up_cb, netdev_link_down_cb);
}

extern u8 g_total_task_cnt;
extern struct task_struct *g_os_task_tbl[10];

void netmgr_task_init(void)
{

    if (OS_MsgQCreate(&st_netmgr_task[0].qevt, 
		(s32)st_netmgr_task[0].qlength) != OS_SUCCESS)
    {
        LOG_PRINTF("OS_MsgQCreate faild\r\n");
        return;
    }
    if (OS_TaskCreate(st_netmgr_task[0].task_func,
                  st_netmgr_task[0].task_name,
                  st_netmgr_task[0].stack_size<<4,
                  NULL,
                  st_netmgr_task[0].prio,
                  NULL) != OS_SUCCESS)
    {
        LOG_PRINTF("OS_TaskCreate faild\r\n");
        return;
    }
	
	wake_up_process(g_os_task_tbl[g_total_task_cnt-1]);
}

void netmgr_wifi_reg_cbs(void)
{
#if(ENABLE_SMART_CONFIG==1)
	/* in, ssv6xxx_user_sconfig_op.UserSconfigPaserData = SmartConfigPaserData; */
    if (ssv6xxx_user_sconfig_op.UserSconfigPaserData != NULL) 
	{
		/* SmartConfigPaserData */
        ssv6xxx_wifi_reg_promiscuous_rx_cb(
			(promiscuous_data_handler)ssv6xxx_user_sconfig_op.UserSconfigPaserData);
    }
#endif

    ssv6xxx_wifi_reg_recovery_cb(_netmgr_wifi_recovery_cb);

    netmgr_wifi_reg_event();
}

void netmgr_init_global_parms(void)
{
/* not in */
#ifdef NET_MGR_AUTO_JOIN
    netmgr_apinfo_clear();
#endif
/* in */
#ifdef NET_MGR_AUTO_RETRY
    g_auto_retry_status = S_TRY_INVALID;
    if (g_auto_retry_times_delay == 0)
    {    
		g_auto_retry_times_delay = NET_MGR_AUTO_RETRY_DELAY;
	}
	if (g_auto_retry_times_max == 0)
    {
		g_auto_retry_times_max = NET_MGR_AUTO_RETRY_TIMES;
	}
	g_auto_retry_times = 0;
    g_auto_retry_start_time = 0;
    MEMSET((void *)&g_auto_retry_ap, 0, sizeof(g_auto_retry_ap));
#endif

    g_wifi_is_joining_b = false;

#if(ENABLE_SMART_CONFIG==1)
    g_SconfigChannelMask = DEFAULT_SCONFIG_CHANNEL_MASK; 		// 0x7FFE(ch 1-14)
    g_Sconfig5gChannelMask = DEFAULT_SCONFIG_5G_CHANNEL_MASK;	// 0x00FFFFF(ch 1-14)
#endif

    MEMSET(&sconfig_result,0,sizeof(sconfig_result));
}

void ssv_netmgr_init(bool default_cfg)
{
	netmgr_init_global_parms();

	netmgr_wifi_reg_cbs();

	netmgr_task_init();
}

void netmgr_cfg_get(netmgr_cfg *p_cfg, u8 vif_idx)
{
    if (p_cfg)
    {
        MEMCPY(p_cfg, &g_netmgr_config[vif_idx], sizeof(netmgr_cfg));
    }
}

void netmgr_cfg_set(netmgr_cfg *p_cfg, u8 vif_idx)
{
    if (p_cfg)
    {
        MEMCPY(&g_netmgr_config[vif_idx], p_cfg, sizeof(netmgr_cfg));
    }
}


void netmgr_netif_status_set(bool on,u8 vif_idx)
{
    LOG_DEBUGF(LOG_L4_NETMGR, ("L3 Link %s\r\n",(on==true?"ON":"OFF")));
	printk("vif(%d), L3 Link %s\r\n", vif_idx, (on==true?"ON":"OFF"));
    if (on)
        netdev_l3_if_up(g_netifdev[vif_idx].name);
    else
        netdev_l3_if_down(g_netifdev[vif_idx].name);
}

bool netmgr_check_netif_up(u8 vif_idx)
{
    int res;
    res = netdev_check_ifup(g_netifdev[vif_idx].name);

    if (res == NS_OK)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void netmgr_netif_link_set(bool on,u8 vif_idx)
{
	LOG_DEBUGF(LOG_L4_NETMGR, ("L2 Link %s\r\n",(on==true?"ON":"OFF")));
	LOG_PRINTF("L2 Link %s\r\n",(on==true?"ON":"OFF"));
    if (on)
    {
        if(g_wifi_link_up_cb != NULL)
            g_wifi_link_up_cb(g_netifdev[vif_idx].name);
    }
    else
    {
        if(g_wifi_link_down_cb != NULL)
            g_wifi_link_down_cb(g_netifdev[vif_idx].name);
    }
}

bool netmgr_wifi_is_connected(ssv6xxx_hw_mode hmode)
{
    u8 vif_idx;
    switch (hmode)
    {
        case SSV6XXX_HWM_STA:
            if (netmgr_wifi_check_sta(&vif_idx))
            {
                if (netmgr_check_netif_up(vif_idx))
                {
                    return true;
                }
            }
            break;
        case SSV6XXX_HWM_AP:
            if (netmgr_wifi_check_ap(&vif_idx))
            {
                Ap_sta_status info;
                int ret = 0;
                ret = netmgr_wifi_info_get(&info,vif_idx);
                if ((ret == 0) && (info.vif_u[vif_idx].ap_status.stanum > 0))
                {
                    return true;
                }
            }
            break;
		default:
			if (netmgr_wifi_check_sta(&vif_idx))
			{
                if (netmgr_check_netif_up(vif_idx))
                {
                    return true;
                }
            }			
    }
    return false;
}

int netmgr_ipinfo_get(char *ifname, ipinfo *info)
{
    int ret = NS_OK;
    u32 ipaddr, gw, netmask;
    ret = netdev_getipv4info(ifname, &ipaddr, &gw, &netmask);

    if (NS_OK == ret)
    {
        info->ipv4 = ipaddr;
        info->netmask = gw;
        info->gateway = netmask;
        info->dns = info->gateway;
    
        return 0;
    }
    return -1;
}

int netmgr_hwmac_get(char *ifname, void *mac)
{    
    return (netdev_getmacaddr(ifname, (u8 *)mac));
}

int netmgr_dhcpd_auto_set(u8 vif_idx)
{
    /* dhcps info auto set */
    if ((((g_netmgr_config[vif_idx].ipaddr >> 24) & 0xff) + 1) < 0xff)
    {
        g_netmgr_config[vif_idx].dhcps.start_ip = g_netmgr_config[vif_idx].ipaddr + (1 << 24);
    }
    else
    {
        g_netmgr_config[vif_idx].dhcps.start_ip = (g_netmgr_config[vif_idx].ipaddr & 0x00ffffff) + (1 << 24);
    }

    if ((((g_netmgr_config[vif_idx].ipaddr >> 24) & 0xff) + DEFAULT_DHCP_MAX_LEASES) < 0xff)
    {
        g_netmgr_config[vif_idx].dhcps.end_ip = g_netmgr_config[vif_idx].dhcps.start_ip + ((DEFAULT_DHCP_MAX_LEASES - 1) << 24);
    }
    else
    {
        g_netmgr_config[vif_idx].dhcps.end_ip = (g_netmgr_config[vif_idx].dhcps.start_ip & 0x00ffffff) + (0xfe << 24);
    }

    g_netmgr_config[vif_idx].dhcps.max_leases = ((g_netmgr_config[vif_idx].dhcps.end_ip >> 24) & 0xff) - ((g_netmgr_config[vif_idx].dhcps.start_ip >> 24) & 0xff) + 1;

    g_netmgr_config[vif_idx].dhcps.gw = g_netmgr_config[vif_idx].ipaddr;

    g_netmgr_config[vif_idx].dhcps.dns = g_netmgr_config[vif_idx].dhcps.gw;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_dhcpd_auto_set: ipaddr=%08x start_ip=%08x end_ip=%08x\r\n",g_netmgr_config[vif_idx].ipaddr,\
        g_netmgr_config[vif_idx].dhcps.start_ip, g_netmgr_config[vif_idx].dhcps.end_ip));

    return 0;
}

int netmgr_ipinfo_set(u8 vif_idx, ipinfo *info, bool auto_dhcpd_info)
{
    int ret = NS_OK;
    //u8 i;
    char *ifname =if_name[vif_idx];
    u32 ip = info->ipv4;
    LOG_PRINTF("netmgr_ipinfo_set,ip=%x,if=%s,vif=%d\r\n",info->ipv4,ifname,vif_idx);
    LOG_PRINTF("netmgr_ipinfo_set_ip =|  %3d  |  %3d  |  %3d  |  %3d \r\n",
                      netstack_ip4_addr4_16((u32*)&ip),
                      netstack_ip4_addr3_16((u32*)&ip),
                      netstack_ip4_addr2_16((u32*)&ip),
                      netstack_ip4_addr1_16((u32*)&ip));
    ret = netdev_setipv4info(ifname, info->ipv4, info->gateway, info->netmask);
    
    //if (ret != NS_OK)
    //    return -1;

    //Update default device here, but better method is needed for multiple device
    //for(i=0;i<MAX_VIF_NUM;i++)
    {
        //if(MEMCMP(ifname, if_name[vif_idx], sizeof(WLAN_IFNAME)) == 0)
        {
            LOG_PRINTF("update default config\r\n");
            g_netifdev[vif_idx].ipaddr = g_netmgr_config[vif_idx].ipaddr = info->ipv4;
            g_netifdev[vif_idx].netmask = g_netmgr_config[vif_idx].netmask = info->netmask;
            g_netifdev[vif_idx].gw = g_netmgr_config[vif_idx].gw = info->gateway;
            //break;
        }    
    }
    /* auto dhcpd set*/
    if (auto_dhcpd_info)
    {
        bool dhcpd_status = false;
        bool dhcpc_status = false;
        netmgr_dhcpd_auto_set(vif_idx);
        netmgr_dhcp_status_get(&dhcpd_status, &dhcpc_status,vif_idx);
        if (dhcpd_status)
        {
            netmgr_dhcpd_start(false,vif_idx);
            netmgr_dhcpd_start(true,vif_idx);
        }
    }

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_ipinfo_set\r\n"));
    return 0;
}

int netmgr_dhcps_info_set(dhcps_info *if_dhcps, u8 vif_idx)
{
    if (if_dhcps)
    {
        return netstack_dhcps_info_set(if_dhcps, &(g_netmgr_config[vif_idx].dhcps),vif_idx);
    }
    
    return 0;
}

static int netmgr_dhcpd_start(bool enable, u8 vif_idx)
{
    int ret = NS_OK;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_dhcpd_start %d vif=%d!!\r\n",enable,vif_idx));

    if (enable)
    {        
        ret = netdev_setipv4info(g_netifdev[vif_idx].name, 
                                 g_netifdev[vif_idx].ipaddr, 
                                 g_netifdev[vif_idx].gw, 
                                 g_netifdev[vif_idx].netmask);

        netmgr_netif_link_set(enable,vif_idx);
        netdev_l3_if_up(g_netifdev[vif_idx].name);
        netstack_dhcps_info_set(&(g_netmgr_config[vif_idx].dhcps),&(g_netmgr_config[vif_idx].dhcps),vif_idx);
        ret = netstack_udhcpd_start();
        if(ret)
        {
            LOG_PRINTF("netmgr start dhcpd fail\r\n");
            g_netmgr_config[vif_idx].s_dhcpd_status = false;
            return -1;
        }
        g_netmgr_config[vif_idx].s_dhcpd_status = true;
    }
    else
    {
        ret = netstack_udhcpd_stop();
        g_netmgr_config[vif_idx].s_dhcpd_status = false;
    }

    return ret;
}

static int netmgr_dhcpc_start(bool enable, u8 vif_idx)
{
    int ret = NS_OK;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_dhcpc_start %d !!\r\n",enable));

    if (enable)
    {
        ret = netdev_setipv4info(g_netifdev[vif_idx].name, 0, 0, 0);
        dhcpc_wrapper_set(g_netifdev[vif_idx].name, true);
        g_netmgr_config[vif_idx].s_dhcpc_status = true;
    }
    else
    {        
        dhcpc_wrapper_set(g_netifdev[vif_idx].name, false);
        g_netifdev[vif_idx].ipaddr = g_netmgr_config[vif_idx].ipaddr;
        g_netifdev[vif_idx].netmask = g_netmgr_config[vif_idx].netmask;
        g_netifdev[vif_idx].gw =  g_netmgr_config[vif_idx].gw;
        ret = netdev_setipv4info(g_netifdev[vif_idx].name, 
                                 g_netifdev[vif_idx].ipaddr , 
                                 g_netifdev[vif_idx].gw , 
                                 g_netifdev[vif_idx].netmask);
        g_netmgr_config[vif_idx].s_dhcpc_status = false;
    }

    return 0;
}


int netmgr_dhcpc_set(bool dhcpc_enable,u8 vif_idx)
{
    if (g_netmgr_config[vif_idx].s_dhcpc_status && !dhcpc_enable)
    {
        netmgr_dhcpc_start(false, vif_idx);
    }

    g_netmgr_config[vif_idx].s_dhcpc_enable = dhcpc_enable;

    return 0;
}

int netmgr_dhcpd_set(bool dhcpd_enable,u8 vif_idx)
{
    if (g_netmgr_config[vif_idx].s_dhcpd_status && !dhcpd_enable)
    {
        netmgr_dhcpd_start(false,vif_idx);
    }

    LOG_PRINTF("netmgr_dhcpd_set=%d,vif=%d\r\n",dhcpd_enable,vif_idx);
    g_netmgr_config[vif_idx].s_dhcpd_enable = dhcpd_enable;

    return 0;
}


int netmgr_dhcp_status_get(bool *dhcpd_status, bool *dhcpc_status, u8 vif_idx)
{
    if (!dhcpd_status || !dhcpc_status)
    {
        return -1;
    }

    *dhcpc_status = g_netmgr_config[vif_idx].s_dhcpc_status;
    *dhcpd_status = g_netmgr_config[vif_idx].s_dhcpd_status;

    return 0;
}

int netmgr_dhcp_getip_bymac(u8 *mac, u32 *ipaddr)
{
    dhcpdipmac *ipmac = NULL;
    int size_count = DEFAULT_DHCP_MAX_LEASES;
    int i;

    //PRINTF("netmgr_dhcd_getip_bymac MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
    //    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );

    ipmac = (dhcpdipmac *)MALLOC(DEFAULT_DHCP_MAX_LEASES * sizeof(dhcpdipmac));
    if (ipmac == NULL)
    {
        return -1;
    }
    MEMSET((void *)ipmac, 0, DEFAULT_DHCP_MAX_LEASES * sizeof(dhcpdipmac));

    if (netstack_dhcp_ipmac_get(ipmac, &size_count)){
    //    PRINTF("netmgr_dhcd_ipmac_get return failure \r\n");
        FREE(ipmac);
        return -1;
    }

    for (i=0; i<size_count; i++){
        if (!MEMCMP(ipmac[i].mac, mac, 6)){
            *ipaddr = ipmac[i].ip;
   //         PRINTF("netmgr_dhcd_getip_bymac shot ipaddr:0x%X \r\n", ntohl(*ipaddr));
            FREE(ipmac);
            return 0;
        }
    }

   // PRINTF("netmgr_dhcd_getip_bymac get ipaddr failure\r\n");
    FREE(ipmac);
    return -1;
}

int netmgr_send_arp_unicast (u8 *dst_mac)
{
    netstack_ip_addr_t ipaddr;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_send_arp_unicast %02X:%02X:%02X:%02X:%02X:%02X !!\r\n",dst_mac[0],
        dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5]));

    if (!netmgr_dhcp_getip_bymac(dst_mac, (u32 *)&ipaddr))
    {
        return netstack_etharp_unicast(dst_mac, &ipaddr);
    }
    else if(netstack_find_ip_in_arp_table(dst_mac,&ipaddr))
    {
        return netstack_etharp_unicast(dst_mac, &ipaddr);
    }

    return -1;
}

int netmgr_wifi_mode_get(wifi_mode *mode, bool *status, u8 vif_idx)
{
    Ap_sta_status *info = NULL;

    if (!mode || !status)
    {
        return -1;
    }

    if(vif_idx >= MAX_VIF_NUM)
        return -1;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if(NULL==info)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return -1;
    }
    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    *mode = info->vif_operate[vif_idx];
    *status = info->status ? true : false;

    FREE(info);

    return 0;
}

int netmgr_wifi_info_get(Ap_sta_status *info, u8 vif_idx)
{
    if (info == NULL)
    {
        return -1;
    }
    if(vif_idx >= MAX_VIF_NUM)
        return -1;

    MEMSET(info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    return 0;
}

bool netmgr_wifi_check_mac(unsigned char * mac)
{
    Ap_sta_status *info = NULL;
    bool bRet = false;
    unsigned int i = 0,j;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    for(j=0;j<MAX_VIF_NUM;j++)
    {
        if (info->vif_operate[j] == SSV6XXX_HWM_AP)
        {
             bRet = false;

             for(i = 0; i < info->vif_u[j].ap_status.stanum; i++)
             {
                if (MEMCMP(mac, info->vif_u[j].ap_status.stainfo[i].Mac, 6) == 0)
                {
                    bRet = true;
                    break;
                }
                /*
                 LOG_PRINTF("station Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                     info->u.ap.stainfo[statemp].Mac[0],
                     info->u.ap.stainfo[statemp].Mac[1],
                     info->u.ap.stainfo[statemp].Mac[2],
                     info->u.ap.stainfo[statemp].Mac[3],
                     info->u.ap.stainfo[statemp].Mac[4],
                     info->u.ap.stainfo[statemp].Mac[5]);
                */
             }
        }
    }

    FREE(info);

    return bRet;
}

u8 netmgr_wifi_get_ava_vif(void)
{
    Ap_sta_status *info = NULL; 
    u8 i;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if(info->vif_operate[i]==SSV6XXX_HWM_INVALID)
        {         
            LOG_PRINTF("Netmgr get ava vif=%d\r\n",i);
            break;
        }
    }
    return i;
}

bool netmgr_wifi_check_sconfig(u8* vif_idx)
{
    Ap_sta_status *info = NULL;
    bool bRet = false;
    int i;

    info = (Ap_sta_status *)OS_MemAlloc(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    OS_MemSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if ((info->vif_operate[i] == SSV6XXX_HWM_SCONFIG) && (info->status))
        {
             bRet = true;
             //LOG_PRINTF("netmgr check sconfig vif=%d\r\n",i);
             break;
        }
    }

    *vif_idx = i;
    OS_MemFree(info);

    return bRet;
}
//Seek for a STA VIF
bool netmgr_wifi_check_sta(u8 *vif_idx)
{
    Ap_sta_status *info = NULL;
    bool bRet = false;
    u8 i;
	printk("%s() at line(%d)\n",__FUNCTION__,__LINE__);

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);
   
    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if ((info->vif_operate[i]== SSV6XXX_HWM_STA) && (info->status))
        {
            bRet = true;
            //LOG_PRINTF("netmgr check sta vif=%d\r\n",i);
            break;
        }
    }

    if (vif_idx)
        *vif_idx = i;
	
    OS_MemFree(info);

	printk("%s()at(%d),bRet(%d)\n",__FUNCTION__,__LINE__,bRet);
    return bRet;
}

bool netmgr_wifi_check_ap(u8* vif_idex)
{
    Ap_sta_status *info = NULL;
    bool bRet = false;
    int i=0;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if ((info->vif_operate[i] == SSV6XXX_HWM_AP) && (info->status))
        {
             bRet = true;
             LOG_PRINTF("netmgr check ap vif=%d\r\n",i);
             break;
        }
    }
    *vif_idex = i;
    OS_MemFree(info);

    return bRet;
}

bool netmgr_wifi_check_ap_ready(void)
{
    Ap_sta_status *info = NULL;
    bool bRet = false;
    int i=0;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);
    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if ((info->vif_operate[i] == SSV6XXX_HWM_AP) && (info->status))
        {
            if(info->vif_u[i].ap_status.current_st == AP_STATE_READY)
                bRet = true;
        }
    }
    OS_MemFree(info);

    return bRet;
}

ssv6xxx_hw_mode netmgr_wifi_get_vif_mode(u8 vif_idex)
{
    Ap_sta_status *info = NULL;
    ssv6xxx_hw_mode hmode = SSV6XXX_HWM_INVALID;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return SSV6XXX_HWM_INVALID;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);
    hmode = info->vif_operate[vif_idex];
    OS_MemFree(info);
    return hmode;
}

bool netmgr_wifi_check_sta_connected(u8* vif_idx)
{
    Ap_sta_status *info = NULL;
    bool bRet = false;
    //int i;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    ASSERT((*vif_idx)<MAX_VIF_NUM);

    //for(i=0;i<MAX_VIF_NUM;i++)
    {
        if ((info->vif_operate[*vif_idx] == SSV6XXX_HWM_STA) && (info->status))
        {
            if(info->vif_u[*vif_idx].station_status.apinfo.status == DISCONNECT)
            {
                bRet = false;
                //*vif_idx = i;
                //break;
            }
            else if(info->vif_u[*vif_idx].station_status.apinfo.status == CONNECT)
            {
                bRet = true;
                //*vif_idx = i;
            }
        }
    }    
    
    OS_MemFree(info);

    return bRet;
}

static int _netmgr_wifi_sconfig_async(u16 channel_mask, u32 channel_5g_mask)
{
    MsgEvent *msg_evt = NULL;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_sconfig_async %x \r\n",channel_mask));

    msg_evt = msg_evt_alloc();

    if(NULL==msg_evt)
    {
        LOG_PRINTF("%s:msg evt alloc fail",__FUNCTION__);
        return -1;
    }

    #ifdef NET_MGR_AUTO_RETRY
    if (g_auto_retry_status != S_TRY_INVALID)
    {
       g_auto_retry_status = S_TRY_INVALID;
       LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY INVLAID\r\n"));
    }
    #endif

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_SCONFIG_REQ;
    msg_evt->MsgData1 = (u32)(channel_mask);
    msg_evt->MsgData2 = (u32)(channel_5g_mask);
    msg_evt->MsgData3 = 0;

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);


    return 0;
}

int netmgr_wifi_sconfig_async(u16 channel_mask)
{
    return _netmgr_wifi_sconfig_async(channel_mask, 0);
}

int netmgr_wifi_sconfig_ex_async(u16 channel_mask,u32 channel_5g_mask)
{
    return _netmgr_wifi_sconfig_async(channel_mask, channel_5g_mask);
}


static int _netmgr_wifi_scan_async(u16 channel_mask, u32 channel_5g_mask, char *ssids[], int ssids_count)
{
    MsgEvent *msg_evt = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_scan_async %x \r\n",channel_mask));

    if (!netmgr_wifi_check_sta(NULL))
    {
        LOG_PRINTF("netmgr_wifi_scan_async: mode error.\r\n");
        return -1;
    }

    msg_evt = msg_evt_alloc();

    if(NULL==msg_evt)
    {
        LOG_PRINTF("%s: msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_SCAN_REQ;
    msg_evt->MsgData1 = (u32)(channel_mask);
    msg_evt->MsgData2 = (u32)(channel_5g_mask);
    msg_evt->MsgData3 = 0;

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    return 0;
}

int netmgr_wifi_scan_async(u16 channel_mask, char *ssids[], int ssids_count)
{
    return _netmgr_wifi_scan_async(channel_mask, 0, ssids, ssids_count);
}

int netmgr_wifi_scan_ex_async(u16 channel_mask, u32 channel_5g_mask, char *ssids[], int ssids_count)
{
    return _netmgr_wifi_scan_async(channel_mask, channel_5g_mask, ssids, ssids_count);
}

int netmgr_wifi_join_async(wifi_sta_join_cfg *join_cfg)
{
    MsgEvent *msg_evt = NULL;
    wifi_sta_join_cfg *join_cfg_msg = NULL;
    u8 vif_idx = 0;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join_async \r\n"));

    msg_evt = msg_evt_alloc();
    if (NULL == msg_evt)
    {
        LOG_PRINTF("%s: msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }

#ifdef NET_MGR_AUTO_RETRY
    if (g_auto_retry_status != S_TRY_INVALID)
    {
       g_auto_retry_status = S_TRY_INVALID;
       LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY INVLAID\r\n"));
    }
#endif

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_JOIN_REQ;

    if (join_cfg)
    {
        join_cfg_msg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));
        if(join_cfg_msg==NULL)
        {
            msg_evt_free(msg_evt);
            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
            return -1;
        }
        MEMCPY((void * )join_cfg_msg, (void * )join_cfg, sizeof(wifi_sta_join_cfg));
		/* TODO(aaron): bug ?? */
		join_cfg_msg->vif_idx = vif_idx;
    }

    msg_evt->MsgData1 = (u32)(join_cfg_msg);
    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    return 0;
}

int netmgr_wifi_join_other_async(wifi_sta_join_cfg *join_cfg)
{
    MsgEvent *msg_evt = NULL;
    wifi_sta_join_cfg *join_cfg_msg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join_other_async \r\n"));

    msg_evt = msg_evt_alloc();
    if(NULL==msg_evt)
    {
        LOG_PRINTF("%s: msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }

    #ifdef NET_MGR_AUTO_RETRY
    if (g_auto_retry_status != S_TRY_INVALID)
    {
       g_auto_retry_status = S_TRY_INVALID;
       LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY INVLAID\r\n"));
    }
    #endif

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_JOIN_OTHER_REQ;

    if (join_cfg)
    {
        join_cfg_msg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));
        if(NULL==join_cfg_msg)
        {
            msg_evt_free(msg_evt);
            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
            return -1;
        }
        MEMCPY((void * )join_cfg_msg, (void * )join_cfg, sizeof(wifi_sta_join_cfg));
    }

    msg_evt->MsgData1 = (u32)(join_cfg_msg);
    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    return 0;
}

int netmgr_wifi_leave_async(u8 vif_idx)
{
    MsgEvent *msg_evt = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_leave_async \r\n"));

    msg_evt = msg_evt_alloc();

    if(NULL==msg_evt)
    {
        LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }

    #ifdef NET_MGR_AUTO_RETRY
    if (g_auto_retry_status != S_TRY_INVALID)
    {
       g_auto_retry_status = S_TRY_INVALID;
       LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY INVLAID\r\n"));
    }
    #endif

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_LEAVE_REQ;
    msg_evt->MsgData1 = vif_idx;
    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    return 0;
}

int netmgr_wifi_vif_off_async(u8 vif_idx)
{
    Ap_sta_status *info = NULL;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return -1;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);
    switch (info->vif_operate[vif_idx])
    {
        case SSV6XXX_HWM_STA:            
            if (info->vif_u[vif_idx].station_status.apinfo.status > DISCONNECT)
            {
                netmgr_wifi_leave_async(vif_idx);
            }
            break;
		default:	
    		LOG_PRINTF("non support mode(%d)\n", info->vif_operate[vif_idx]);
	        break;
    }    
    OS_MemFree(info);

    {
        MsgEvent *msg_evt = NULL;

        LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_vif_off_async \r\n"));

        msg_evt = msg_evt_alloc();

        if (NULL == msg_evt)
        {
            LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
            return -1;
        }
        msg_evt->MsgType = MEVT_NET_MGR_EVENT;
        msg_evt->MsgData = MSG_VIF_OFF_REQ;
        msg_evt->MsgData1 = vif_idx;
        msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    }

    return 0;
}

int netmgr_wifi_control_async(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg)
{
    int ret = 0;
	MsgEvent *msg_evt = NULL;
    wifi_ap_cfg *ap_cfg_msg = NULL;
    wifi_sta_cfg *sta_cfg_msg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_control_async \r\n"));

	printk("netmgr_wifi_control_async\n");
	
    g_wifi_is_joining_b = false;

    msg_evt = msg_evt_alloc();
    if(NULL==msg_evt)
    {
        LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }

#ifdef NET_MGR_AUTO_RETRY
	if (g_auto_retry_status != S_TRY_INVALID)
	{
		g_auto_retry_status = S_TRY_INVALID;
		LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY INVLAID\r\n"));
	}
#endif

    g_sconfig_running = FALSE;

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_CONTROL_REQ;
    
#if (AP_MODE_ENABLE == 1)
	if (ap_cfg)
	{
	    if(ap_cfg->channel!=EN_CHANNEL_AUTO_SELECT)
	    {
	        if(FALSE==ssv6xxx_wifi_is_available_channel(SSV6XXX_HWM_AP,ap_cfg->channel))
	        {
	            LOG_PRINTF("%d not available_channel\r\n",ap_cfg->channel);
	            return -1;
	        }
	    }
	    ap_cfg_msg = (wifi_ap_cfg *)MALLOC(sizeof(wifi_ap_cfg));
	    if(NULL==ap_cfg_msg)
	    {
	        msg_evt_free(msg_evt);
	        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
	        return -1;
	    }
	    MEMCPY((void * )ap_cfg_msg, (void * )ap_cfg, sizeof(wifi_ap_cfg));
	}
#endif
    if (sta_cfg)
    {
        sta_cfg_msg = (wifi_sta_cfg *)MALLOC(sizeof(wifi_sta_cfg));
        if(NULL==sta_cfg_msg)
        {
            msg_evt_free(msg_evt);
            FREE(ap_cfg_msg);
            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
            return -1;
        }
        MEMCPY((void * )sta_cfg_msg, (void * )sta_cfg, sizeof(wifi_sta_cfg));
    }

    msg_evt->MsgData1 = (u32)(mode);
    msg_evt->MsgData2 = (u32)(ap_cfg_msg);
    msg_evt->MsgData3 = (u32)(sta_cfg_msg);
    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);

	return ret;
}


int netmgr_wifi_switch_async(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_join_cfg *join_cfg)
{

    int ret = 0;
    MsgEvent *msg_evt = NULL;
    wifi_ap_cfg *ap_cfg_msg = NULL;
    wifi_sta_join_cfg *join_cfg_msg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_switch_async \r\n"));
    
    g_wifi_is_joining_b = false;

    msg_evt = msg_evt_alloc();
    if(NULL==msg_evt)
    {
        LOG_PRINTF("%s: msg allocate fail\r\n",__FUNCTION__);
        return -1;
    }

    #ifdef NET_MGR_AUTO_RETRY
    if (g_auto_retry_status != S_TRY_INVALID)
    {
       g_auto_retry_status = S_TRY_INVALID;
       LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY INVLAID\r\n"));
    }
    #endif

    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_SWITCH_REQ;
    
#if (AP_MODE_ENABLE == 1)
    if (ap_cfg)
    {
        ap_cfg_msg = (wifi_ap_cfg *)MALLOC(sizeof(wifi_ap_cfg));
        if(NULL==ap_cfg_msg)
        {
            msg_evt_free(msg_evt);
            LOG_PRINTF("%s:malloc fail(%d)\r\n",__FUNCTION__,__LINE__);
            return -1;
        }
        MEMCPY((void * )ap_cfg_msg, (void * )ap_cfg, sizeof(wifi_ap_cfg));
    }
#endif

    if (join_cfg)
    {
        join_cfg_msg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));
        if(NULL==join_cfg_msg)
        {
            msg_evt_free(msg_evt);
            FREE(ap_cfg_msg);
            LOG_PRINTF("%s:malloc fail(%d)\r\n",__FUNCTION__,__LINE__);
            return -1;
        }
        MEMCPY((void * )join_cfg_msg, (void * )join_cfg, sizeof(wifi_sta_join_cfg));
    }

    msg_evt->MsgData1 = (u32)(mode);
    msg_evt->MsgData2 = (u32)(ap_cfg_msg);
    msg_evt->MsgData3 = (u32)(join_cfg_msg);
    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    return ret;
}


void netmgr_wifi_station_off(u8 vif_idx)
{
    wifi_mode mode;
    wifi_sta_cfg *sta_cfg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_station_off \r\n"));

    sta_cfg = (wifi_sta_cfg *)MALLOC(sizeof(wifi_sta_cfg));
    if (sta_cfg == NULL)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return;
    }

    MEMSET((void *)sta_cfg, 0, sizeof(wifi_sta_cfg));

    mode = SSV6XXX_HWM_STA;
    sta_cfg->status = false;
    sta_cfg->vif_idx = vif_idx;

    netmgr_wifi_control(mode, NULL, sta_cfg);

    FREE(sta_cfg);
}

void netmgr_wifi_sconfig_off(u8 vif_idx)
{
#if(ENABLE_SMART_CONFIG==1)
    wifi_mode mode;
    wifi_sta_cfg *sta_cfg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_sconfig_off \r\n"));

    sta_cfg = (wifi_sta_cfg *)MALLOC(sizeof(wifi_sta_cfg));
    if (sta_cfg == NULL)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return;
    }

    MEMSET((void *)sta_cfg, 0, sizeof(wifi_sta_cfg));

    if ((ssv6xxx_user_sconfig_op.UserSconfigDeinit != NULL) 
							&& (g_sconfig_user_mode==TRUE)) 
	{
        ssv6xxx_user_sconfig_op.UserSconfigDeinit();
    }

    g_sconfig_running=FALSE;

    mode = SSV6XXX_HWM_SCONFIG;
    sta_cfg->status = false;
    sta_cfg->vif_idx = vif_idx;
    netmgr_wifi_control(mode, NULL, sta_cfg);

    FREE(sta_cfg);
#endif
}


void netmgr_wifi_ap_off(u8 vif_idx)
{
    wifi_mode mode;
    wifi_ap_cfg *ap_cfg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_ap_off \r\n"));

    ap_cfg = (wifi_ap_cfg *)MALLOC(sizeof(wifi_ap_cfg));
    if (ap_cfg == NULL)
    {
        return;
    }

    MEMSET((void *)ap_cfg, 0, sizeof(wifi_ap_cfg));

    mode = SSV6XXX_HWM_AP;
    ap_cfg->status = false;
    ap_cfg->vif_idx = vif_idx;
    netmgr_wifi_control(mode, ap_cfg, NULL);

    FREE(ap_cfg);
}

void netmgr_wifi_vif_off(u8 vif_idx)
{
    Ap_sta_status *info = NULL;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);
    switch(info->vif_operate[vif_idx])
    {
        case SSV6XXX_HWM_STA:
            netmgr_wifi_station_off(vif_idx);
            break;
        case SSV6XXX_HWM_AP:
            netmgr_wifi_ap_off(vif_idx);
            break;
        case SSV6XXX_HWM_SCONFIG:
            netmgr_wifi_sconfig_off(vif_idx);
            break;
		default:
    		LOG_PRINTF("non support mode(%d)\n", info->vif_operate[vif_idx]);
			break;
    }
    
    OS_MemFree(info);
}

int netmgr_wifi_sconfig_done(u8 *resp_data, u32 len, bool IsUDP,u32 port)
{
    if(IsUDP==FALSE)
    {
        return -1;
    }
    
    LOG_PRINTF("IP address is ready!!\r\n");
    return (netstack_udp_send(resp_data, len, 0, ((u16)port-1), 0xffffffff, port, 50));
}

static int _netmgr_wifi_sconfig(u16 channel_mask, u32 channel_5g_mask)
{
#if(ENABLE_SMART_CONFIG == 1)		
    struct cfg_sconfig_request *SconfigReq = NULL;
    u8 vif_idx;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_sconfig \r\n"));

    if (!netmgr_wifi_check_sconfig(&vif_idx))
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

    if(g_sconfig_running==TRUE){
        LOG_PRINTF("SmartConfig is running. Please restart the SCONFIG mode\r\n");
        return -1;
    }
    g_sconfig_running=TRUE;

    if(g_sconfig_solution==WECHAT_AIRKISS_IN_FW){
        g_sconfig_user_mode=FALSE;
    }
    else{
        g_sconfig_user_mode=TRUE;
    }

    ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_SCONFIG, &channel_mask,&channel_5g_mask);


    if (g_sconfig_user_mode==TRUE)
    {
        ssv6xxx_promiscuous_enable();
        if (ssv6xxx_user_sconfig_op.UserSconfigInit!=NULL)
        {
            ssv6xxx_user_sconfig_op.UserSconfigInit();
        }
    }
    else
    {
        ssv6xxx_promiscuous_disable();
    }

    SconfigReq = (void *)OS_MemAlloc(sizeof(*SconfigReq));
    if (!SconfigReq)
    {
        g_sconfig_running=FALSE;
        return -1;
    }
    SconfigReq->channel_mask = channel_mask;
    SconfigReq->channel_5g_mask = channel_5g_mask;    
    SconfigReq->dwell_time = 10;
    SconfigReq->user_mode=g_sconfig_user_mode;

    if (ssv6xxx_wifi_sconfig(SconfigReq) < 0)
    {
       	LOG_PRINTF("Command failed !!\r\n");
        OS_MemFree(SconfigReq);
        g_sconfig_running=FALSE;
        return -1;
    }

    OS_MemFree(SconfigReq);
#endif
    return 0;
}

int netmgr_wifi_sconfig_ex(u16 channel_mask, u32 channel_5g_mask)
{
    if(channel_mask==SCAN_ALL_2G_CHANNEL){
        channel_mask=g_sta_channel_mask;
    }

    if(channel_5g_mask==SCAN_ALL_5G_CHANNEL){
        channel_5g_mask=g_sta_5g_channel_mask;
    }

    return _netmgr_wifi_sconfig(channel_mask, channel_5g_mask);
}

int netmgr_wifi_sconfig(u16 channel_mask)
{
    if(channel_mask==SCAN_ALL_2G_CHANNEL){
        channel_mask=g_sta_channel_mask;
    }
    
    return _netmgr_wifi_sconfig(channel_mask,0);
}



static int _netmgr_wifi_scan(u16 channel_mask, u32 channel_5g_mask, char *ssids[], int ssids_count)
{

    struct cfg_scan_request *ScanReq = NULL;
    int                      i = 0;
    u8                      vif_idx;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_scan \r\n"));

    //Find a sta vif and non-connected
    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if (netmgr_wifi_get_vif_mode(i)==SSV6XXX_HWM_STA)
        {
            vif_idx = i;
            if(!netmgr_wifi_check_sta_connected(&vif_idx))
            {
                break;
            }            
        }
    }
    if(vif_idx >= MAX_VIF_NUM)
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }
    LOG_PRINTF("scan use sta vif =%d\r\n",vif_idx);

    //Here, if channel_mask is 0, it means that user prefer to use the default mask
    #if 0
    if(channel_mask==0){
        channel_mask=g_sta_channel_mask;
    }

    if(channel_5g_mask==0){
        channel_5g_mask=g_sta_5g_channel_mask;
    }
    #endif
    
    //channel_mask&=~(0x8001); //unmask ch0 and ch15;

    //Here, if channel_mask is 0, it means we don't need to scan any channel

    #if 0
    if(channel_mask==0){
        LOG_PRINTF("channel_mask is zero\r\n");
        return 0;
    }
    #endif
    ssv6xxx_wifi_align_available_channel_mask(SSV6XXX_HWM_STA, &channel_mask,&channel_5g_mask);
    ScanReq = (void *)MALLOC(sizeof(*ScanReq) + ssids_count*sizeof(struct cfg_80211_ssid));
    if (!ScanReq)
    {
        return -1;
    }
    ScanReq->is_active      = true;
    ScanReq->n_ssids        = ssids_count;
    ScanReq->channel_mask   = channel_mask;
    ScanReq->vif_idx        = vif_idx;
    if(TRUE==ssv6xxx_wifi_support_5g_band())
    {
        ScanReq->channel_5g_mask   = channel_5g_mask;
    }
    else
    {
        ScanReq->channel_5g_mask   = 0;    
    }
    ScanReq->dwell_time = 0;

    for (i = 0; i < ssids_count; i++)
    {
        MEMCPY((void*)(ScanReq->ssids[i].ssid), (void*)ssids[i], sizeof(struct cfg_80211_ssid));
    }

    if (ssv6xxx_wifi_scan(ScanReq) < 0)
    {
       	LOG_PRINTF("Command failed !!\r\n");
        FREE(ScanReq);
        return -1;
    }

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_scan done g_switch_join_cfg_b=%d\r\n", g_switch_join_cfg_b));

    FREE(ScanReq);

    return 0;
}

int netmgr_wifi_scan_ex(u16 channel_mask, u32 channel_5g_mask, char *ssids[], int ssids_count)
{
    if(channel_mask==SCAN_ALL_2G_CHANNEL){
        channel_mask=g_sta_channel_mask;
    }

    if(channel_5g_mask==SCAN_ALL_5G_CHANNEL){
        channel_5g_mask=g_sta_5g_channel_mask;
    }

    return _netmgr_wifi_scan(channel_mask, channel_5g_mask, ssids, ssids_count);
}

int netmgr_wifi_scan(u16 channel_mask, char *ssids[], int ssids_count)
{
    if(channel_mask==SCAN_ALL_2G_CHANNEL){
        channel_mask=g_sta_channel_mask;
    }
    
    return _netmgr_wifi_scan(channel_mask, 0, ssids, ssids_count);
}

char CharToHex(char bChar)  
{  
    if((bChar>=0x30)&&(bChar<=0x39))  
	{  
        bChar -= 0x30;  
	}  
    else if((bChar>=0x41)&&(bChar<=0x46))  
	{  
        bChar -= 0x37;  
	}  
    else if((bChar>=0x61)&&(bChar<=0x66))  
	{  
        bChar -= 0x57;  
	}
    else   
	{  
        bChar = 0xff;  
	}  
    return bChar;  
} 

int netmgr_wifi_join(wifi_sta_join_cfg *join_cfg)
{
    s32    size = 0;
    struct ssv6xxx_ieee80211_bss       *ap_info_bss = NULL;
    struct cfg_join_request *JoinReq = NULL;

    wifi_sec_type    sec_type = WIFI_SEC_NONE;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};
	
    u8 tmp, vif_idx=MAX_VIF_NUM;
	u8 i = 0;
	int outi = 0;
	int length = STRLEN((char *)join_cfg->password);
	char output[MAX_PASSWD_LEN+1];
    
	
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join \r\n"));

    if (g_wifi_is_joining_b)
    {
        return -1;
    }

    //Find a sta vif and non-connected
    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if (netmgr_wifi_get_vif_mode(i)==SSV6XXX_HWM_STA)
        {
            vif_idx = i;
            if(!netmgr_wifi_check_sta_connected(&vif_idx))
            {
                break;
            }            
        }
    }
    if(vif_idx >= MAX_VIF_NUM)
    {
        LOG_PRINTF("join mode error.\r\n");
        return -1;
    }
    LOG_PRINTF("use sta vif =%d\r\n",vif_idx);
    if(netmgr_wifi_check_sta_connected(&vif_idx))
    {
        int time = 3;
        LOG_PRINTF("leave old ap,vif_idx=%d\r\n",vif_idx);
        netmgr_wifi_leave(vif_idx);
        while (netmgr_wifi_check_sta_connected(&vif_idx) && (time > 0))
        {
            time--;
            OS_MsDelay(1000);
        }

        if (netmgr_wifi_check_sta_connected(&vif_idx))
        {
            LOG_PRINTF("leave old ap timeout\r\n");
            return -1;
        }
        else
        {
            LOG_PRINTF("leave old ap success\r\n");
        }
    }
    
    if ((join_cfg->ssid.ssid_len == 0) || (STRLEN((char *)join_cfg->password) > MAX_PASSWD_LEN))
    {
        LOG_PRINTF("netmgr_wifi_join parameter error.\r\n");
        return -1;
    }

    ap_info_bss = ssv6xxx_wifi_find_ap_ssid(&join_cfg->ssid);

    if (ap_info_bss == NULL)
    {
        MEMCPY((void*)ssid_buf,(void*)join_cfg->ssid.ssid,join_cfg->ssid.ssid_len);
        LOG_PRINTF("No AP \"%s\" was found.\r\n", ssid_buf);
        return -1;
    }

    if((!ssv6xxx_wifi_support_5g_band())&&IS_5G_BAND(ap_info_bss->channel_id))
    {
        LOG_PRINTF("Don't support 5G AP now\r\n");
        return -1;
    }

    ssv6xxx_wifi_set_channel(ap_info_bss->channel_id,SSV6XXX_HWM_STA);
    size = sizeof(struct cfg_join_request) + sizeof(struct ssv6xxx_ieee80211_bss);
    JoinReq = (struct cfg_join_request *)MALLOC(size);
    if(NULL==JoinReq)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return -1;
    }
    MEMSET((void *)JoinReq, 0, size);
    JoinReq->vif_idx = vif_idx;
    
    if (ap_info_bss->capab_info&BIT(4))
    {
        if (ap_info_bss->proto&WPA_PROTO_WPA)
        {
            sec_type = WIFI_SEC_WPA_PSK;

        }
        else if (ap_info_bss->proto&WPA_PROTO_RSN)
        {
            sec_type = WIFI_SEC_WPA2_PSK;
        }
        else
        {
            sec_type = WIFI_SEC_WEP;
        }
    }
    else
    {
        sec_type = WIFI_SEC_NONE;
    }

    if (sec_type == WIFI_SEC_NONE)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_NONE;
    }
    else if (sec_type == WIFI_SEC_WEP)
    {
        #if defined(SEC_USE_WEP40_OPEN) || defined(SEC_USE_WEP104_OPEN)
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        #else
        JoinReq->auth_alg = WPA_AUTH_ALG_SHARED;
        #endif
        JoinReq->wep_keyidx = 0;
        if (STRLEN((char *)join_cfg->password) == 5)
        {
            JoinReq->sec_type = SSV6XXX_SEC_WEP_40;
        }
        else if (STRLEN((char *)join_cfg->password) == 13)
        {
            JoinReq->sec_type = SSV6XXX_SEC_WEP_104;
        }
		
        else if ((length == 10) || (length == 26))
        {
			if(length == 10)
				JoinReq->sec_type = SSV6XXX_SEC_WEP_40;
			else
				JoinReq->sec_type = SSV6XXX_SEC_WEP_104;
			
            for (i = 0; i < length; ++i)
			{
				tmp = CharToHex(join_cfg->password[i]);
				if (tmp  == 0xff)
					return -1;

				if(i%2 == 0)
				{
					output[outi] = (tmp << 4) & 0xf0;
				}
				else
				{
					output[outi] |= tmp;
					++outi;
				}
			}
			MEMCPY((void *)(join_cfg->password), (char *)output, length+ 1);
        }
        else
        {
            LOG_PRINTF("wrong password failed !!\r\n");
            OS_MemFree(JoinReq);
            return -1;
        }
    }
    else if (sec_type == WIFI_SEC_WPA_PSK)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_WPA_PSK;
    }
    else if (sec_type == WIFI_SEC_WPA2_PSK)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_WPA2_PSK;
    }
    else
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_NONE;
        LOG_PRINTF("ERROR: unkown security type: %d\r\n", sec_type);
    }

    if((sec_type == WIFI_SEC_NONE) && (STRLEN((char *)join_cfg->password)!=0))
    {
        MEMCPY((void*)ssid_buf,(void*)join_cfg->ssid.ssid,join_cfg->ssid.ssid_len);
        LOG_PRINTF("The password of AP \"%s\" is error\r\n", ssid_buf);
        FREE(JoinReq);
        return -1;
    }

    if (STRLEN((char *)join_cfg->password) == 0)
    {
        MEMCPY((void *)(JoinReq->password), g_sec_info[JoinReq->sec_type].dfl_password, STRLEN(g_sec_info[JoinReq->sec_type].dfl_password) + 1);
    }
    else
    {
        MEMCPY((void *)(JoinReq->password), (char *)join_cfg->password, STRLEN((char *)join_cfg->password) + 1);
    }

    MEMCPY((void*)&JoinReq->bss, (void*)ap_info_bss, sizeof(struct ssv6xxx_ieee80211_bss));

    LOG_PRINTF("dtim_period = %d, vif=%d\r\n",JoinReq->bss.dtim_period,JoinReq->vif_idx);
    LOG_PRINTF("wmm_used    = %d\r\n",JoinReq->bss.wmm_used);

    MEMCPY((void*)ssid_buf,(void*)JoinReq->bss.ssid.ssid,JoinReq->bss.ssid.ssid_len);
    LOG_PRINTF("Joining \"%s\" using security type \"%s\".\r\n", ssid_buf, g_sec_info[JoinReq->sec_type].sec_name);
    JoinReq->no_bcn_timeout = g_host_cfg.sta_no_bcn_timeout;

    netdev_set_default(if_name[JoinReq->vif_idx]);
    if (ssv6xxx_wifi_join(JoinReq) < 0)
    {
        LOG_PRINTF("ssv6xxx_wifi_join failed !!\r\n");
        FREE(JoinReq);
        return -1;
    }
    g_wifi_is_joining_b = true;


#ifdef NET_MGR_AUTO_JOIN
    {
        user_ap_info ap_item;
        MEMSET(&ap_item, 0, sizeof(ap_item));
        ap_item.valid = false;
        ap_item.join_times = 0;
        //STRCPY((char *)(ap_item.ssid.ssid), (char *)join_cfg->ssid.ssid);
        MEMCPY((void *)(ap_item.ssid), (void *)join_cfg->ssid, sizeof(struct cfg_80211_ssid));
        STRCPY((char *)ap_item.password, (char *)JoinReq->password);
        netmgr_apinfo_save(&ap_item);
        MEMCPY((void*)ssid_buf,(void*)ap_item.ssid.ssid,ap_item.ssid.ssid_len);
        LOG_DEBUGF(LOG_L4_NETMGR, ("AutoJoin: SAVE SSID[%s] info, waiting join success \r\n", (char *)ssid_buf));
    }
#endif

#ifdef NET_MGR_AUTO_RETRY
     if (g_auto_retry_status == S_TRY_INVALID)
     {
          g_auto_retry_status = S_TRY_INIT;
          //STRCPY((char *)(g_auto_retry_ap.ssid.ssid), (char *)join_cfg->ssid.ssid);
          MEMCPY((void *)&(g_auto_retry_ap.ssid), (void *)&join_cfg->ssid, sizeof(struct cfg_80211_ssid));
          STRCPY((char *)g_auto_retry_ap.password, (char *)JoinReq->password);
          MEMCPY((void*)ssid_buf,(void*)g_auto_retry_ap.ssid.ssid,g_auto_retry_ap.ssid.ssid_len);
          g_auto_retry_ap.vif_idx = JoinReq->vif_idx;
          LOG_DEBUGF(LOG_L4_NETMGR, ("AutoTry: netmgr_wifi_join SAVE SSID[%s] info, waiting join success\r\n", (char *)(ssid_buf)));
     }
#endif

    FREE(JoinReq);

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join done\r\n"));

    return 0;
}

int netmgr_wifi_join_other(wifi_sta_join_cfg *join_cfg)
{
    int ret=0;
    u8 vif_idx,i;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join_other \r\n"));

    if (g_wifi_is_joining_b)
    {
        return -1;
    }

    //Find a sta vif and non-connected
    for(i=0;i<MAX_VIF_NUM;i++)
    {
        if (netmgr_wifi_get_vif_mode(i)==SSV6XXX_HWM_STA)
        {
            vif_idx = i;
            if(!netmgr_wifi_check_sta_connected(&vif_idx))
            {
                break;
            }            
        }
    }
    if(vif_idx >= MAX_VIF_NUM)
    {
        LOG_PRINTF("join_other mode error.\r\n");
        return -1;
    }

    if (netmgr_wifi_check_sta_connected(&vif_idx))
    {
        LOG_PRINTF("leave old ap,vif_idx=%d\r\n",vif_idx);
        netmgr_wifi_leave(vif_idx);
        Sleep(1000);
    }
    join_cfg->vif_idx = vif_idx;
    if ((join_cfg->ssid.ssid_len) == 0 || (STRLEN((char *)join_cfg->password) > MAX_PASSWD_LEN))
    {
        LOG_PRINTF("netmgr_wifi_join_other parameter error.\r\n");
        return -1;
    }

    ret=_netmgr_wifi_switch(SSV6XXX_HWM_STA, NULL, join_cfg, g_sta_channel_mask, g_sta_5g_channel_mask,false);
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join_other \r\n"));
    return ret;

}

int netmgr_wifi_leave(u8 vif_idx)
{
    struct cfg_leave_request *LeaveReq = NULL;
    
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_leave \r\n"));
    LOG_PRINTF("netmgr_wifi_leave vif_idx=%d\r\n",vif_idx);

    if (!netmgr_wifi_check_sta(NULL))
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

	LeaveReq = (void *)MALLOC(sizeof(struct cfg_leave_request));
    if(NULL==LeaveReq)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LeaveReq->reason = 1;
    LeaveReq->vif_idx = vif_idx;
    if (ssv6xxx_wifi_leave(LeaveReq) < 0)
    {
        FREE(LeaveReq);
        return -1;
    }
    else
    {
        netmgr_netif_link_set(LINK_DOWN,vif_idx);
    }

    FREE(LeaveReq);
    return 0;
}

extern OsSemaphore ap_sta_on_off_sphr;
extern s32 _ssv6xxx_wifi_send_cmd(void *pCusData, int nCuslen, ssv6xxx_host_cmd_id eCmdID);

int netmgr_wifi_control(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg)
{
    int ret = 0;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_control \r\n"));
	printk("netmgr_wifi_control\n");

    if (mode >= SSV6XXX_HWM_INVALID)
    {
        return -1;
    }

    if ((mode == SSV6XXX_HWM_STA) || (mode == SSV6XXX_HWM_SCONFIG))
    {
        if (sta_cfg)
        {
            struct stamode_setting sta_setting;
            MEMSET(&sta_setting,0,sizeof(struct stamode_setting));
            if (g_netmgr_config[sta_cfg->vif_idx].s_dhcpd_enable && sta_cfg->status)
            {
                netmgr_dhcpd_start(false,sta_cfg->vif_idx);
            }

			sta_setting.mode = mode;
            sta_setting.sta_cfg =sta_cfg;

            LOG_PRINTF("sta_setting.mode=%d\r\n",sta_setting.mode);
            LOG_PRINTF("sta_setting.sta_cfg.status(%d)\r\n", sta_setting.sta_cfg->status);
            LOG_PRINTF("sta_setting.sta_cfg->vif_idx(%d)\r\n", sta_setting.sta_cfg->vif_idx);			
            LOG_PRINTF("Nmgr ctl STA vif=%d\r\n",sta_cfg->vif_idx);

            _ssv6xxx_wifi_send_cmd((void *)&sta_setting, sizeof(struct stamode_setting), SSV6XXX_HOST_CMD_SET_STA_CFG);

            OS_SemWait(ap_sta_on_off_sphr,0);
            
            ret = ssv6xxx_wifi_station(mode,sta_cfg);

            ssv_netmgr_add_netdev(sta_cfg->vif_idx,FALSE);
            if ((ret == (int)SSV6XXX_SUCCESS) && g_netmgr_config[sta_cfg->vif_idx].s_dhcpc_enable)
            {
                netmgr_netif_link_set(LINK_DOWN,sta_cfg->vif_idx);
                netmgr_netif_status_set(false,sta_cfg->vif_idx);
                if (sta_cfg->status)
                {
                    //netmgr_dhcpc_set(true);
					LOG_PRINTF("no dhcpc STA vif=%d\r\n",sta_cfg->vif_idx);
                }
                else
                {
                    netmgr_dhcpc_start(false,sta_cfg->vif_idx);
                }
            }
            else if(!g_netmgr_config[sta_cfg->vif_idx].s_dhcpc_enable)
            {
                netmgr_netif_link_set(LINK_UP,sta_cfg->vif_idx);
                //netmgr_netif_status_set(true,sta_cfg->vif_idx);
            }
        }
    }
    else if(mode == SSV6XXX_HWM_AP)
    {
        if (ap_cfg)
        {
            struct apmode_setting ap_setting;
            MEMSET(&ap_setting,0,sizeof(struct apmode_setting));
			ap_setting.ap_cfg =ap_cfg;
            LOG_PRINTF("Nmgr ctl AP vif=%d\r\n",ap_cfg->vif_idx);
            if(!ap_cfg->static_ip)
                netmgr_dhcpd_set(TRUE,ap_cfg->vif_idx);
            if((ap_cfg->channel!=EN_CHANNEL_AUTO_SELECT)&&(ap_cfg->status))
            {
                if(FALSE==ssv6xxx_wifi_is_available_channel(SSV6XXX_HWM_AP,ap_cfg->channel))
                {
                    return -1;
                }
            }
            
            if (g_netmgr_config[ap_cfg->vif_idx].s_dhcpc_enable && ap_cfg->status)
            {
                netmgr_dhcpc_start(false,ap_cfg->vif_idx);
            }

            ap_setting.step = 0;
            _ssv6xxx_wifi_send_cmd((void *)&ap_setting, sizeof(struct apmode_setting), SSV6XXX_HOST_CMD_SET_AP_CFG);
            OS_SemWait(ap_sta_on_off_sphr,0);
            LOG_PRINTF("netmgr ap step 1\r\n");
            if(ap_cfg->status)
            {
                //---------ACS---------
                ssv6xxx_wifi_ap(ap_setting.ap_cfg,1);
                //---------------------

                ap_setting.step = 2;
                _ssv6xxx_wifi_send_cmd((void *)&ap_setting, sizeof(struct apmode_setting), SSV6XXX_HOST_CMD_SET_AP_CFG);
                OS_SemWait(ap_sta_on_off_sphr,0);
            }
            //ret = ssv6xxx_wifi_ap(ap_cfg);
            LOG_PRINTF("AP vif=%d link up, dhcpd=%d\r\n",ap_cfg->vif_idx,g_netmgr_config[ap_cfg->vif_idx].s_dhcpd_enable);            
            ssv_netmgr_add_netdev(ap_cfg->vif_idx,FALSE);
            if ((ret == (int)SSV6XXX_SUCCESS) && g_netmgr_config[ap_cfg->vif_idx].s_dhcpd_enable)
            {
                LOG_PRINTF("AP link up, ap_cfg->status=%d\r\n",ap_cfg->status);
                netmgr_netif_link_set(LINK_UP,ap_cfg->vif_idx);
                netmgr_netif_status_set(true,ap_cfg->vif_idx);
                if (ap_cfg->status)
                {
                    //if((g_netmgr_config[ap_cfg->vif_idx].dhcps.start_ip)&&(g_netmgr_config[ap_cfg->vif_idx].dhcps.end_ip))
                        netmgr_dhcpd_start(true,ap_cfg->vif_idx);
                }
                else
                {
                    netmgr_dhcpd_start(false,ap_cfg->vif_idx);
                }
            }
            else if (!g_netmgr_config[ap_cfg->vif_idx].s_dhcpd_enable)
            {
                netmgr_netif_link_set(LINK_UP,ap_cfg->vif_idx);
                netmgr_netif_status_set(true,ap_cfg->vif_idx);
            }
        }
    }
    else
    {
        // not support
    }

    return ret;
}

//For switch function, user shall assign an interface to switch
static int _netmgr_wifi_switch(wifi_mode mode, wifi_ap_cfg *ap_cfg, 
	wifi_sta_join_cfg *join_cfg, u16 scanning_channel_mask, 
	u32 scanning_5g_channel_mask, bool sta_reset)
{
    int ret = 0;
    wifi_sta_cfg sta_cfg;
    char *ssid[1];
    ssv6xxx_hw_mode vif_mode;
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_switch \r\n"));

    if (mode >= SSV6XXX_HWM_INVALID)
    {
        return -1;
    }

    if (mode == SSV6XXX_HWM_STA)
    {
        vif_mode = netmgr_wifi_get_vif_mode(join_cfg->vif_idx);
#if (AP_MODE_ENABLE == 1)        
        if(vif_mode == SSV6XXX_HWM_AP)
        {
            netmgr_wifi_ap_off(join_cfg->vif_idx);
        }
#endif
        if(vif_mode == SSV6XXX_HWM_SCONFIG)
        {
            netmgr_wifi_sconfig_off(join_cfg->vif_idx);
        }

        if (g_netmgr_config[join_cfg->vif_idx].s_dhcpd_enable)
        {
            netmgr_dhcpd_start(false,join_cfg->vif_idx);
        }

        netmgr_netif_link_set(LINK_DOWN,join_cfg->vif_idx);
        netmgr_netif_status_set(false,join_cfg->vif_idx);

        if (sta_reset)
        {
            sta_cfg.status = TRUE;
            sta_cfg.vif_idx = join_cfg->vif_idx;
            ret = ssv6xxx_wifi_station(SSV6XXX_HWM_STA, &sta_cfg);
        }

        if (join_cfg)
        {
            // do scan for join.
            g_switch_join_cfg_b = true;
            MEMCPY((void *)&g_join_cfg_data, (void *)join_cfg, sizeof(wifi_sta_join_cfg));
            //ssid[0]=(char *)join_cfg->ssid.ssid;
            ssid[0]=(char *)&join_cfg->ssid;
            netmgr_wifi_scan_ex(scanning_channel_mask, scanning_5g_channel_mask, ssid, 1);
        }

    }
#if (AP_MODE_ENABLE == 1)    
    else if(mode == SSV6XXX_HWM_AP)
    {
        vif_mode = netmgr_wifi_get_vif_mode(ap_cfg->vif_idx);
        if (vif_mode == SSV6XXX_HWM_STA)
        {
            netmgr_wifi_station_off(ap_cfg->vif_idx);
        }

        if (vif_mode == SSV6XXX_HWM_SCONFIG)
        {
            netmgr_wifi_sconfig_off(ap_cfg->vif_idx);
        }

        if (ap_cfg)
        {
            struct apmode_setting ap_setting;
            MEMSET(&ap_setting,0,sizeof(struct apmode_setting));
			ap_setting.ap_cfg =ap_cfg;

            if(ap_cfg->channel!=EN_CHANNEL_AUTO_SELECT)
            {
                if(FALSE==ssv6xxx_wifi_is_available_channel(SSV6XXX_HWM_AP,ap_cfg->channel))
                {
                    return -1;
                }
            }            
            
            if (g_netmgr_config[ap_cfg->vif_idx].s_dhcpc_enable)
            {
                netmgr_dhcpc_start(false,ap_cfg->vif_idx);
            }

            ap_setting.step = 0;
            _ssv6xxx_wifi_send_cmd((void *)&ap_setting, sizeof(struct apmode_setting), SSV6XXX_HOST_CMD_SET_AP_CFG);
            OS_SemWait(ap_sta_on_off_sphr,0);

            //---------ACS---------
            ssv6xxx_wifi_ap(ap_cfg,1);
            //---------------------

            ap_setting.step = 2;
            _ssv6xxx_wifi_send_cmd((void *)&ap_setting, sizeof(struct apmode_setting), SSV6XXX_HOST_CMD_SET_AP_CFG);
            OS_SemWait(ap_sta_on_off_sphr,0);

            if ((ret == (int)SSV6XXX_SUCCESS) && g_netmgr_config[ap_cfg->vif_idx].s_dhcpd_enable)
            {
                netmgr_dhcpd_start(true,ap_cfg->vif_idx);
            }
        }
    }
#endif    
    else
    {
        // not support
    }

    return ret;
}

int netmgr_wifi_switch(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_join_cfg *join_cfg)
{
    return _netmgr_wifi_switch(mode, ap_cfg, join_cfg, g_sta_channel_mask, g_sta_5g_channel_mask,true);
}

int netmgr_wifi_switch_to_sta(wifi_sta_join_cfg *join_cfg, u8 join_channel) //for Sconfig
{
    u8 bit_nm=0;
    bit_nm=ssv6xxx_wifi_ch_to_bitmask(join_channel);
    if(IS_5G_BAND(join_channel))
        return _netmgr_wifi_switch(SSV6XXX_HWM_STA, NULL, join_cfg, 0, (1<<bit_nm), true);
    else
        return _netmgr_wifi_switch(SSV6XXX_HWM_STA, NULL, join_cfg, (1<<bit_nm), 0, true);
}


void netmgr_wifi_reg_event(void)
{
    ssv6xxx_wifi_reg_evt_cb(netmgr_wifi_event_cb);
}

void netmgr_wifi_event_cb(u32 evt_id, void *data, s32 len)
{
    MsgEvent *msg_evt = NULL;

    //LOG_PRINTF("evt_id = %d\r\n", evt_id);

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_event_cb evt_id = %d\r\n", evt_id));

    switch (evt_id)
    {
        case SOC_EVT_SCAN_DONE:
        {
            struct resp_evt_result *scan_done = (struct resp_evt_result *)data;

            if (scan_done)
            {
                msg_evt = msg_evt_alloc();
                if(NULL!=msg_evt)
                {
                    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                    msg_evt->MsgData = MSG_SCAN_DONE;
                    msg_evt->MsgData1 = scan_done->u.scan_done.result_code;
                    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                }
                else
                {
                    LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
                }
            }
            break;
        }

        case SOC_EVT_SCAN_RESULT: // join result
        {
            ap_info_state *scan_res = (ap_info_state *) data;
            if (scan_res)
            {
                msg_evt = msg_evt_alloc();
                if(NULL!=msg_evt)
                {
                    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                    msg_evt->MsgData = MSG_SCAN_RESULT;
                    msg_evt->MsgData1 = (u32)scan_res->apInfo;
                    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                }
                else
                {
                    LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
                }
            }
            break;
        }
        case SOC_EVT_SCONFIG_SCAN_DONE: // join result
        {
            struct resp_evt_result *sconfig_done = (struct resp_evt_result *)data;
            if (sconfig_done)
            {
                msg_evt = msg_evt_alloc();
                if(NULL!=msg_evt)
                {
                    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                    msg_evt->MsgData = MSG_SCONFIG_SCANNING_DONE;

                    sconfig_result.valid=0;
                    MEMCPY((void *)sconfig_result.ssid.ssid, (void *)sconfig_done->u.sconfig_done.ssid,sconfig_done->u.sconfig_done.ssid_len);
                    sconfig_result.ssid.ssid_len=sconfig_done->u.sconfig_done.ssid_len;
                    STRCPY((char *)sconfig_result.password, (char *)sconfig_done->u.sconfig_done.pwd);
                    sconfig_result.channel=sconfig_done->u.sconfig_done.channel;
                    sconfig_result.dat=sconfig_done->u.sconfig_done.rand;
                    sconfig_result.valid=1;
                    sconfig_result.vif_idx = sconfig_done->u.sconfig_done.bssid_idx;
                    
                    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                }
                else
                {
                    LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
                }
            }
            break;
        }
        case SOC_EVT_JOIN_RESULT: // join result
        {
            struct resp_evt_result *join_res = (struct resp_evt_result *)data;
            g_wifi_is_joining_b = false;

            if (join_res)
            {
                msg_evt = msg_evt_alloc();
                if(NULL!=msg_evt)
                {
                    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                    msg_evt->MsgData = MSG_JOIN_RESULT;
                    msg_evt->MsgData1 = (join_res->u.join.status_code != 0) ? DISCONNECT : CONNECT;
                    msg_evt->MsgData2 = join_res->u.join.bssid_idx;
                    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                }
                else
                {
                    LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
                }
            }
            break;
        }
        case SOC_EVT_LEAVE_RESULT: // leave result include disconnnet
        {
            struct resp_evt_result *leave_res = (struct resp_evt_result *)data;
            g_wifi_is_joining_b = false;

            if (leave_res)
            {
                msg_evt = msg_evt_alloc();
                if(NULL!=msg_evt)
                {
                    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                    msg_evt->MsgData = MSG_LEAVE_RESULT;
                    msg_evt->MsgData1 = (u32)(leave_res->u.leave.reason_code);
                    msg_evt->MsgData2 = leave_res->u.leave.bssid_idx;
                    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                }
                else
                {
                    LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
                }
            }
            break;
        }
        case SOC_EVT_POLL_STATION: // ARP request
        {
            if(netmgr_send_arp_unicast(data)== -1)
            {
                u8 * mac = data;
                 LOG_PRINTF("Poll station fail, MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
                   mac[0], mac[1], mac[2],mac[3],mac[4],mac[5] );
            }
            break;
        }
        case SOC_EVT_PS_SETUP_OK:
        case SOC_EVT_PS_WAKENED:
        {
            msg_evt = msg_evt_alloc();
            if(NULL!=msg_evt)
            {
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;

                if(evt_id == SOC_EVT_PS_WAKENED)
                {
                    struct cfg_ps_request wowreq;
                    ssv6xxx_wifi_pwr_saving(&wowreq,FALSE);
                    OS_MsDelay(15);
                    ssv6xxx_set_wakeup_bb_gpio(0); //Wakeup BB low

                    if(g_host_cfg.extRxInt)
                        ssv_hal_set_ext_rx_int(g_host_cfg.rxIntGPIO);

                    msg_evt->MsgData = MSG_PS_WAKENED;
                }
                else if(evt_id == SOC_EVT_PS_SETUP_OK)
                    msg_evt->MsgData = MSG_PS_SETUP_OK;
                                 
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }
            else
            {
                LOG_PRINTF("%s,%d:msg alloc fail\r\n",__FUNCTION__, __LINE__);
            }
            break;
        }
        case SOC_EVT_STA_STATUS: // Station add or remove
        {
            struct cfg_set_sta *cfg_sta =data;

            if(cfg_sta->sta_oper == CFG_STA_ADD)
            {
                LOG_PRINTF("ADD station, MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
                cfg_sta->wsid_info.addr.addr[0],
                cfg_sta->wsid_info.addr.addr[1],
                cfg_sta->wsid_info.addr.addr[2],
                cfg_sta->wsid_info.addr.addr[3],
                cfg_sta->wsid_info.addr.addr[4],
                cfg_sta->wsid_info.addr.addr[5] );                
                netdev_set_default(if_name[cfg_sta->vif_idx]);
                
            }
            else if(cfg_sta->sta_oper == CFG_STA_DEL)
            {
                LOG_PRINTF("DEL station, MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
                cfg_sta->wsid_info.addr.addr[0],
                cfg_sta->wsid_info.addr.addr[1],
                cfg_sta->wsid_info.addr.addr[2],
                cfg_sta->wsid_info.addr.addr[3],
                cfg_sta->wsid_info.addr.addr[4],
                cfg_sta->wsid_info.addr.addr[5] );
            }

            break;
        }
        default:
            // do nothing
            break;
    }

    return;
}

void netmgr_ifup_cb(void)
{
    MsgEvent *msg_evt=NULL;
    if (!((sconfig_result.valid == 0) && (g_sconfig_user_mode == FALSE)))
    {
        msg_evt = msg_evt_alloc();
        if(NULL!=msg_evt)
        {
            LOG_PRINTF("%s:post message\r\n",__FUNCTION__);
            msg_evt->MsgType = MEVT_NET_MGR_EVENT;
            msg_evt->MsgData = MSG_SCONFIG_DONE;
            msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
        }
        else
        {
            LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
        }
    }
}

#ifdef __linux__
int netmgr_task(void *arg)
{
    MsgEvent *msg_evt = NULL;
    OsMsgQ mbox = st_netmgr_task[0].qevt;
    s32 res = 0;
    //u32 lastTRX_time=0;
    int ret = 0;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};

	while (!kthread_should_stop())
    {
        res = msg_evt_fetch_timeout(mbox, &msg_evt, TICK_RATE_MS);
        if (res != OS_SUCCESS)
        {
#ifdef NET_MGR_AUTO_RETRY
            if ((g_auto_retry_status == S_TRY_RUN)&&(true == netmgr_wifi_check_sta(NULL)))
            {
                u32 curr_time = os_tick2ms(OS_GetSysTick()) / 1000;

                if (( g_switch_join_cfg_b)||(curr_time < (g_auto_retry_start_time + g_auto_retry_times_delay)))
                {
                    continue;
                }

                if (++g_auto_retry_times <= g_auto_retry_times_max)
                {
                    if (g_auto_retry_ap.ssid.ssid_len > 0)
                    {
                        wifi_sta_join_cfg *join_cfg = NULL;
                        LOG_DEBUGF(LOG_L4_NETMGR, ("\r\nAUTO RETRY [%d]  %u\r\n", 
							g_auto_retry_times,  curr_time));
                        join_cfg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));
                        if (NULL != join_cfg)
                        {
                            MEMCPY((void *)&join_cfg->ssid, (void *)&g_auto_retry_ap.ssid, sizeof(struct cfg_80211_ssid));
                            STRCPY((char *)join_cfg->password, (char *)g_auto_retry_ap.password);
                            join_cfg->vif_idx = g_auto_retry_ap.vif_idx;
                            ret = _netmgr_wifi_switch(SSV6XXX_HWM_STA, NULL, join_cfg, g_sta_channel_mask, g_sta_5g_channel_mask, false);
                            if (ret != 0)
                            {
                                g_switch_join_cfg_b = false;
                            }

                            FREE(join_cfg);
                        }
                        else
                        {
                            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
                        }
                    }

                    g_auto_retry_start_time = os_tick2ms(OS_GetSysTick()) / 1000;
                }
                else
                {
                    LOG_DEBUGF(LOG_L4_NETMGR, ("\r\nAUTO RETRY [%d], time out\r\n", g_auto_retry_times));
                    g_auto_retry_status = S_TRY_STOP;
                    g_auto_retry_times = 0;
                    g_auto_retry_start_time = 0;

                }
            }

#endif
            continue;
        }
        else if (msg_evt && (msg_evt->MsgType == MEVT_NET_MGR_EVENT))
        {
            LOG_DEBUGF(LOG_L4_NETMGR, ("EVENT [%d]\r\n", msg_evt->MsgData));
            switch (msg_evt->MsgData)
            {
                case MSG_SCAN_DONE:
                {
                    u8 result_code=(u8)msg_evt->MsgData1;
                    bool scan_join_fail = false;
                    if (result_code == 0)
                    {
                        if (g_switch_join_cfg_b)
                        {
                            // do join
                            ret = netmgr_wifi_join(&g_join_cfg_data);
                            if (ret != 0)
                            {
                                g_switch_join_cfg_b = false;
                                scan_join_fail = true;
                            }
                        }
                    }
                    else
                    {
                        if (g_switch_join_cfg_b)
                        {
                            g_switch_join_cfg_b = false;
                            scan_join_fail = true;
                            // can't join
                            MEMCPY((void*)ssid_buf,(void*)g_join_cfg_data.ssid.ssid,g_join_cfg_data.ssid.ssid_len);
                            LOG_PRINTF("Scan FAIL, can't join [%s]\r\n", ssid_buf);
                        }
                    }

#ifdef  NET_MGR_AUTO_JOIN
					netmgr_autojoin_process();
#endif
#ifdef NET_MGR_AUTO_RETRY
                    if (scan_join_fail)
                    {
                        if (g_auto_retry_status == S_TRY_INVALID)
                        {
                            g_auto_retry_status = S_TRY_INIT;
                        }

                        MEMCPY((void *)&(g_auto_retry_ap.ssid), (void *)&(g_join_cfg_data.ssid), sizeof(struct cfg_80211_ssid));
                        STRCPY((char *)g_auto_retry_ap.password, (char *)g_join_cfg_data.password);
                        MEMCPY((void*)ssid_buf,(void*)g_join_cfg_data.ssid.ssid,g_join_cfg_data.ssid.ssid_len);
                        g_auto_retry_ap.vif_idx = g_join_cfg_data.vif_idx;

                        LOG_DEBUGF(LOG_L4_NETMGR, ("AutoTry: scan_join_fail SAVE SSID[%s] info, waiting join success\r\n", ssid_buf));

                        netmgr_auto_retry_update();
                    }
#endif
                    break;
                }
                case MSG_SCONFIG_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    u32 channel_5g_mask = msg_evt->MsgData2;
                    netmgr_wifi_sconfig_ex(channel_mask,channel_5g_mask);
                    if(TRUE==g_sconfig_user_mode){
                        //Reuse the msg_evt
                        msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                        msg_evt->MsgData = MSG_SCONFIG_PROCESS;
                        msg_evt->MsgData1 = 0;
                        msg_evt->MsgData2 = 0;
                        msg_evt->MsgData3 = 0;
                        msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                        msg_evt=NULL;
                    }
                    break;
                }
                case MSG_SCAN_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    u32 channel_5g_mask = msg_evt->MsgData2;

                    netmgr_wifi_scan_ex(channel_mask, channel_5g_mask, 0, 0);
                    break;
                }
                
                case MSG_JOIN_REQ:
                {
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData1;

                    netmgr_wifi_join(join_cfg_msg);

                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }
                case MSG_JOIN_OTHER_REQ:
                {
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData1;

                    netmgr_wifi_join_other(join_cfg_msg);

                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }
                case MSG_LEAVE_REQ:
                {
                    LOG_PRINTF("MSG_LEAVE_REQ vif_idx=%d\r\n",msg_evt->MsgData1);
                    netmgr_wifi_leave(msg_evt->MsgData1);
                    break;
                }
                case MSG_VIF_OFF_REQ:
                {
                    LOG_PRINTF("MSG_VIF_OFF_REQ vif_idx=%d\r\n",msg_evt->MsgData1);
                    netmgr_wifi_vif_off(msg_evt->MsgData1);
                    break;
                }
                case MSG_CONTROL_REQ:
                {
                    wifi_mode mode = (wifi_mode)msg_evt->MsgData1;
                    wifi_ap_cfg *ap_cfg_msg = (wifi_ap_cfg *)msg_evt->MsgData2;
                    wifi_sta_cfg *sta_cfg_msg = (wifi_sta_cfg *)msg_evt->MsgData3;

                    if (ap_cfg_msg)
                    {
                             LOG_PRINTF("use vif = %d for AP\r\n",ap_cfg_msg->vif_idx);
                    }
                    if (sta_cfg_msg)
					{
						LOG_PRINTF("use vif = %d for STA\r\n", sta_cfg_msg->vif_idx);
					}

                    netmgr_wifi_control(mode, ap_cfg_msg, sta_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        FREE(ap_cfg_msg);
                    }
                    if (sta_cfg_msg)
                    {
						LOG_PRINTF("free sta cfg msg\r\n");
                        FREE(sta_cfg_msg);
                    }
                    break;
                }
                case MSG_SWITCH_REQ:
                {
                    wifi_mode mode = (wifi_mode)msg_evt->MsgData1;
                    wifi_ap_cfg *ap_cfg_msg = (wifi_ap_cfg *)msg_evt->MsgData2;
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData3;

                    netmgr_wifi_switch(mode, ap_cfg_msg, join_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        FREE(ap_cfg_msg);
                    }
                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }

                case MSG_SCAN_RESULT:
                {
#ifdef  NET_MGR_AUTO_JOIN
					g_ap_list_p = (struct ssv6xxx_ieee80211_bss *)msg_evt->MsgData1;
#endif
                    break;
                }
                case MSG_SCONFIG_DONE:
                {
                    Ap_sta_status *pinfo=NULL;
                    pinfo=MALLOC(sizeof(Ap_sta_status));
                    if(pinfo==NULL)
                    {
                        break;
                    }
                    
                    if(-1!=netmgr_wifi_info_get(pinfo,sconfig_result.vif_idx))
                    {
                        if(sconfig_result.valid==1)
                        {
                            if(0==MEMCMP((void *)(pinfo->vif_u[sconfig_result.vif_idx].station_status.ssid.ssid),(void *)(sconfig_result.ssid.ssid),sconfig_result.ssid.ssid_len))
                            {
                                if(g_sconfig_user_mode==TRUE)
                                {
                                    if(netmgr_wifi_check_sta(NULL))
                                    {
#if(ENABLE_SMART_CONFIG==1)                                    
                                        if(ssv6xxx_user_sconfig_op.UserSconfigConnect!=NULL)
                                        {
                                            ssv6xxx_user_sconfig_op.UserSconfigConnect();
                                        }
#endif
                                    }
#if(ENABLE_SMART_CONFIG==1)
                                    if(ssv6xxx_user_sconfig_op.UserSconfigDeinit!=NULL)
                                    {
                                        ssv6xxx_user_sconfig_op.UserSconfigDeinit();
                                    }
#endif
                                    g_sconfig_user_mode=FALSE;

								}
                                else
                                {
                                    if(netmgr_wifi_check_sta(NULL))
                                    {
                                        netmgr_wifi_sconfig_done((u8 *)&sconfig_result.dat,1,TRUE,10000);
                                    }
                                }
                                sconfig_result.valid=0;                            
                            }
                        }
                    }

                    FREE(pinfo);
                    break;
                }
                case MSG_SCONFIG_SCANNING_DONE:
                {
#if(ENABLE_SMART_CONFIG == 1)
                    wifi_sta_join_cfg *join_cfg = NULL;
                    if((1==g_sconfig_auto_join)&&(sconfig_result.valid==1))
                    {
                        join_cfg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));                            
                        if(NULL!=join_cfg)
                        {
                            //join_cfg->ssid.ssid_len=STRLEN((const char *)sconfig_done_cpy->u.sconfig_done.ssid);
                            //STRCPY((char *)join_cfg->ssid.ssid, (char *)sconfig_done_cpy->u.sconfig_done.ssid);
                            join_cfg->ssid.ssid_len=sconfig_result.ssid.ssid_len;
                            join_cfg->vif_idx = sconfig_result.vif_idx;
                            MEMCPY((void *)join_cfg->ssid.ssid, (void *)sconfig_result.ssid.ssid,sconfig_result.ssid.ssid_len);
                            STRCPY((char *)join_cfg->password, (char *)sconfig_result.password);
                            netmgr_wifi_switch_to_sta(join_cfg,sconfig_result.channel);
                            FREE(join_cfg);
                        }
                        else
                        {
                            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);                                
                        }
                    }        
#endif
                    break;
                }
                case MSG_JOIN_RESULT:
                {
                    int join_status;
                    u8 vif_idx;
                    join_status = msg_evt->MsgData1;
                    vif_idx = (u8)msg_evt->MsgData2;
                    g_switch_join_cfg_b = false;
                    LOG_PRINTF("MSG_JOIN_RESULT=%d,vif_idx=%d\r\n",join_status,vif_idx);
                    /* join success */
                    if (join_status == CONNECT)
                    {
                        netmgr_netif_link_set(true,vif_idx);

                        if (g_netmgr_config[vif_idx].s_dhcpc_enable)
                        {
                            netmgr_dhcpc_start(true,vif_idx);
                        }
                        else
                        {
                            netmgr_netif_status_set(true,vif_idx);
                        }

                        LOG_DEBUGF(LOG_L4_NETMGR, ("JOIN SUCCESS\r\n"));

#ifdef NET_MGR_AUTO_JOIN
                        {
                            user_ap_info *ap_info;
                            Ap_sta_status *info = NULL;

                            info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
                            if(NULL!=info)
                            {
                                if (netmgr_wifi_info_get(info,vif_idx) == 0)
                                {
                                    MEMCPY((void*)ssid_buf,(void*)info->vif_u[vif_idx].station_status.ssid.ssid,info->vif_u[vif_idx].station_status.ssid.ssid_len);
                                    LOG_DEBUGF(LOG_L4_NETMGR, ("AutoJoin: SSID[%s] connected \r\n", ssid_buf));
                                    ap_info = netmgr_apinfo_find((char *)info->u.station.ssid.ssid);
                                    if (ap_info)
                                    {
                                       netmgr_apinfo_set(ap_info, true);
                                    }
                                }

                                FREE(info);
                            }
                            else
                            {
                                LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
                            }
                        }
#endif
#ifdef NET_MGR_AUTO_RETRY
                        {
                            g_auto_retry_status = S_TRY_STOP;
                            g_auto_retry_times = 0;
                            g_auto_retry_start_time = 0;

                            LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY S_TRY_STOP\r\n"));
						}
#endif
                    }
                    /* join failure */
                    else if (join_status == DISCONNECT)
                    {
                        int ret = NS_OK;
                        LOG_DEBUGF(LOG_L4_NETMGR, ("JOIN FAILED\r\n"));
                        netmgr_netif_link_set(false,vif_idx);
                        netmgr_netif_status_set(false,vif_idx);

                        if (g_netmgr_config[vif_idx].s_dhcpc_enable)
                        {
                            ret = netdev_setipv4info(g_netifdev[vif_idx].name,0,0,0);
                            dhcpc_wrapper_set(g_netifdev[vif_idx].name, false);
                            g_netmgr_config[vif_idx].s_dhcpc_status = false;
                        }
                        else
                        {
	                        netmgr_netif_status_set(false,vif_idx);
                        }
 

#ifdef NET_MGR_AUTO_RETRY
						netmgr_auto_retry_update();
#endif
                    }
                    break;
                }
                case MSG_LEAVE_RESULT:
                {
                    int leave_reason;
                    u8 vif_idx;
                    leave_reason = msg_evt->MsgData1;
                    vif_idx = msg_evt->MsgData2;
                    LOG_DEBUGF(LOG_L4_NETMGR, ("leave result reason = %d,vif_idx=%d\r\n", leave_reason,vif_idx));

                    /* leave success */
                    if (netmgr_wifi_get_vif_mode(vif_idx)==SSV6XXX_HWM_STA)                    
                    {
                        int ret = NS_OK;

                        netmgr_netif_link_set(false,vif_idx);
                        netmgr_netif_status_set(false,vif_idx);

                        if (g_netmgr_config[vif_idx].s_dhcpc_enable)
                        {
                            ret = netdev_setipv4info(g_netifdev[vif_idx].name,0,0,0);
                            dhcpc_wrapper_set(g_netifdev[vif_idx].name, false);
                            g_netmgr_config[vif_idx].s_dhcpc_status = false;
                        }
                        else
                        {
	                        netmgr_netif_status_set(false,vif_idx);
                        }

#ifdef NET_MGR_AUTO_JOIN
						if (leave_reason != 0)
						{
							netmgr_wifi_scan(g_sta_channel_mask, NULL, 0);
						}
#endif

#ifdef NET_MGR_AUTO_RETRY
						if (leave_reason != 0)
						{
							netmgr_auto_retry_update();
						}
#endif
                    }
                    break;
                }
                case MSG_PS_WAKENED:
                {
                    LOG_PRINTF("MSG_PS_WAKENED\r\n");
                    //lastTRX_time = ssv6xxx_drv_get_TRX_time_stamp();
                    break;
                }
                case MSG_PS_SETUP_OK:
                {
                    struct cfg_ps_request wowreq;
                    LOG_PRINTF("MSG_PS_SETUP_OK\r\n");
                    wowreq.host_ps_st = HOST_PS_START;
                    ssv6xxx_wifi_pwr_saving(&wowreq,true);
                    break;
                }
                case MSG_SCONFIG_PROCESS:
                {
                    if(FALSE==netmgr_wifi_check_sconfig(NULL))
                    {
                        g_sconfig_user_mode=FALSE;
                    }
                    else
                    {
#if(ENABLE_SMART_CONFIG==1)                    
                        if((ssv6xxx_user_sconfig_op.UserSconfigSM!=NULL)&&(g_sconfig_user_mode==TRUE)){
                            if(1== ssv6xxx_user_sconfig_op.UserSconfigSM()){
                                OS_MsDelay(50);
                                //Reuse the msg_evt
                                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                                msg_evt->MsgData = MSG_SCONFIG_PROCESS;
                                msg_evt->MsgData1 = 0;
                                msg_evt->MsgData2 = 0;
                                msg_evt->MsgData3 = 0;
                                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                                msg_evt = NULL;
                            }
                        }
#endif						
                    }
                    break;
                }
            }
            if(NULL!=msg_evt)
            {
                msg_evt_free(msg_evt);
            }
        }
    }
	return 0;
}
#else
void netmgr_task(void *arg)
{
    MsgEvent *msg_evt = NULL;
    OsMsgQ mbox = st_netmgr_task[0].qevt;
    s32 res = 0;
    //u32 lastTRX_time=0;
    int ret = 0;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};

    while(1)
    {
        res = msg_evt_fetch_timeout(mbox, &msg_evt, TICK_RATE_MS);
        if (res != OS_SUCCESS)
        {

            #ifdef NET_MGR_AUTO_RETRY
            if ((g_auto_retry_status == S_TRY_RUN)&&(true == netmgr_wifi_check_sta(NULL)))
            {
                u32 curr_time = os_tick2ms(OS_GetSysTick()) / 1000;

                if(( g_switch_join_cfg_b)||(curr_time < (g_auto_retry_start_time + g_auto_retry_times_delay)))
                {
                    continue;
                }

                if (++g_auto_retry_times <= g_auto_retry_times_max)
                {
                    if (g_auto_retry_ap.ssid.ssid_len > 0)
                    {
                        wifi_sta_join_cfg *join_cfg = NULL;
                        LOG_DEBUGF(LOG_L4_NETMGR, ("\r\nAUTO RETRY [%d]  %u\r\n", g_auto_retry_times,  curr_time));
                        join_cfg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));
                        if(NULL!=join_cfg)
                        {
                            //join_cfg->ssid.ssid_len=STRLEN((const char *)g_auto_retry_ap.ssid.ssid);
                            //STRCPY((char *)join_cfg->ssid.ssid, (char *)g_auto_retry_ap.ssid.ssid);
                            MEMCPY((void *)&join_cfg->ssid, (void *)&g_auto_retry_ap.ssid, sizeof(struct cfg_80211_ssid));
                            STRCPY((char *)join_cfg->password, (char *)g_auto_retry_ap.password);
                            join_cfg->vif_idx = g_auto_retry_ap.vif_idx;
                            ret = _netmgr_wifi_switch(SSV6XXX_HWM_STA, NULL, join_cfg, g_sta_channel_mask, g_sta_5g_channel_mask, false);
                            if (ret != 0)
                            {
                                g_switch_join_cfg_b = false;
                            }

                            FREE(join_cfg);
                        }
                        else
                        {
                            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
                        }
                    }

                    g_auto_retry_start_time = os_tick2ms(OS_GetSysTick()) / 1000;
                }
                else
                {
                    LOG_DEBUGF(LOG_L4_NETMGR, ("\r\nAUTO RETRY [%d], time out\r\n", g_auto_retry_times));
                    g_auto_retry_status = S_TRY_STOP;
                    g_auto_retry_times = 0;
                    g_auto_retry_start_time = 0;

                }
            }

            #endif
            continue;
        }
        else if (msg_evt && (msg_evt->MsgType == MEVT_NET_MGR_EVENT))
        {
            LOG_DEBUGF(LOG_L4_NETMGR, ("EVENT [%d]\r\n", msg_evt->MsgData));
            switch (msg_evt->MsgData)
            {
                case MSG_SCAN_DONE:
                {
                    u8 result_code=(u8)msg_evt->MsgData1;
                    bool scan_join_fail = false;
                    if (result_code == 0)
                    {
                        if (g_switch_join_cfg_b)
                        {
                            // do join
                            ret = netmgr_wifi_join(&g_join_cfg_data);
                            if (ret != 0)
                            {
                                g_switch_join_cfg_b = false;
                                scan_join_fail = true;
                            }
                        }
                    }
                    else
                    {
                        if (g_switch_join_cfg_b)
                        {
                            g_switch_join_cfg_b = false;
                            scan_join_fail = true;
                            // can't join
                            MEMCPY((void*)ssid_buf,(void*)g_join_cfg_data.ssid.ssid,g_join_cfg_data.ssid.ssid_len);
                            LOG_PRINTF("Scan FAIL, can't join [%s]\r\n", ssid_buf);
                        }
                    }

                    #ifdef  NET_MGR_AUTO_JOIN
                    netmgr_autojoin_process();
                    #endif

                    #ifdef NET_MGR_AUTO_RETRY
                    if (scan_join_fail)
                    {
                        if (g_auto_retry_status == S_TRY_INVALID)
                        {
                            g_auto_retry_status = S_TRY_INIT;
                        }

                        MEMCPY((void *)&(g_auto_retry_ap.ssid), (void *)&(g_join_cfg_data.ssid), sizeof(struct cfg_80211_ssid));
                        STRCPY((char *)g_auto_retry_ap.password, (char *)g_join_cfg_data.password);
                        MEMCPY((void*)ssid_buf,(void*)g_join_cfg_data.ssid.ssid,g_join_cfg_data.ssid.ssid_len);
                        g_auto_retry_ap.vif_idx = g_join_cfg_data.vif_idx;

                        LOG_DEBUGF(LOG_L4_NETMGR, ("AutoTry: scan_join_fail SAVE SSID[%s] info, waiting join success\r\n", ssid_buf));

                        netmgr_auto_retry_update();
                    }
                    #endif
                    break;
                }
                case MSG_SCONFIG_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    u32 channel_5g_mask = msg_evt->MsgData2;
                    netmgr_wifi_sconfig_ex(channel_mask,channel_5g_mask);
                    if(TRUE==g_sconfig_user_mode){
                        //Reuse the msg_evt
                        msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                        msg_evt->MsgData = MSG_SCONFIG_PROCESS;
                        msg_evt->MsgData1 = 0;
                        msg_evt->MsgData2 = 0;
                        msg_evt->MsgData3 = 0;
                        msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                        msg_evt=NULL;
                    }
                    break;
                }
                case MSG_SCAN_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    u32 channel_5g_mask = msg_evt->MsgData2;

                    netmgr_wifi_scan_ex(channel_mask, channel_5g_mask, 0, 0);
                    break;
                }
                
                case MSG_JOIN_REQ:
                {
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData1;

                    netmgr_wifi_join(join_cfg_msg);

                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }
                case MSG_JOIN_OTHER_REQ:
                {
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData1;

                    netmgr_wifi_join_other(join_cfg_msg);

                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }
                case MSG_LEAVE_REQ:
                {
                    LOG_PRINTF("MSG_LEAVE_REQ vif_idx=%d\r\n",msg_evt->MsgData1);
                    netmgr_wifi_leave(msg_evt->MsgData1);
                    break;
                }
                case MSG_VIF_OFF_REQ:
                {
                    LOG_PRINTF("MSG_VIF_OFF_REQ vif_idx=%d\r\n",msg_evt->MsgData1);
                    netmgr_wifi_vif_off(msg_evt->MsgData1);
                    break;
                }
                case MSG_CONTROL_REQ:
                {
                    wifi_mode mode = (wifi_mode)msg_evt->MsgData1;
                    wifi_ap_cfg *ap_cfg_msg = (wifi_ap_cfg *)msg_evt->MsgData2;
                    wifi_sta_cfg *sta_cfg_msg = (wifi_sta_cfg *)msg_evt->MsgData3;
                    u8 vif_idx;

                    if(ap_cfg_msg)
                    {
                            LOG_PRINTF("use vif=%d for AP=%d\r\n",ap_cfg_msg->vif_idx);
                            //netmgr_wifi_vif_off(ap_cfg_msg->vif_idx);
                    }

                    if(sta_cfg_msg)
                    {
                        LOG_PRINTF("use vif=%d for STA=%d\r\n",vif_idx);
                        //netmgr_wifi_vif_off(sta_cfg_msg->vif_idx);
                    }

                    netmgr_wifi_control(mode, ap_cfg_msg, sta_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        FREE(ap_cfg_msg);
                    }
                    if (sta_cfg_msg)
                    {
                        FREE(sta_cfg_msg);
                    }
                    break;
                }
                case MSG_SWITCH_REQ:
                {
                    wifi_mode mode = (wifi_mode)msg_evt->MsgData1;
                    wifi_ap_cfg *ap_cfg_msg = (wifi_ap_cfg *)msg_evt->MsgData2;
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData3;

                    netmgr_wifi_switch(mode, ap_cfg_msg, join_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        FREE(ap_cfg_msg);
                    }
                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }

                case MSG_SCAN_RESULT:
                {
                    #ifdef  NET_MGR_AUTO_JOIN
                    g_ap_list_p = (struct ssv6xxx_ieee80211_bss *)msg_evt->MsgData1;
                    #endif
                    break;
                }
                case MSG_SCONFIG_DONE:
                {
                    Ap_sta_status *pinfo=NULL;
                    pinfo=MALLOC(sizeof(Ap_sta_status));
                    if(pinfo==NULL)
                    {
                        break;
                    }
                    
                    if(-1!=netmgr_wifi_info_get(pinfo,sconfig_result.vif_idx))
                    {
                        if(sconfig_result.valid==1)
                        {
                            if(0==MEMCMP((void *)(pinfo->vif_u[sconfig_result.vif_idx].station_status.ssid.ssid),(void *)(sconfig_result.ssid.ssid),sconfig_result.ssid.ssid_len))
                            {
                                if(g_sconfig_user_mode==TRUE)
                                {
                                    if(netmgr_wifi_check_sta(NULL))
                                    {
#if(ENABLE_SMART_CONFIG==1)                                    
                                        if(ssv6xxx_user_sconfig_op.UserSconfigConnect!=NULL)
                                        {
                                            ssv6xxx_user_sconfig_op.UserSconfigConnect();
                                        }
#endif
                                    }
#if(ENABLE_SMART_CONFIG==1)
                                    if(ssv6xxx_user_sconfig_op.UserSconfigDeinit!=NULL)
                                    {
                                        ssv6xxx_user_sconfig_op.UserSconfigDeinit();
                                    }
#endif
                                    g_sconfig_user_mode=FALSE;
                                }
                                else
                                {
                                    if(netmgr_wifi_check_sta(NULL))
                                    {
                                        netmgr_wifi_sconfig_done((u8 *)&sconfig_result.dat,1,TRUE,10000);
                                    }
                                }
                                sconfig_result.valid=0;                            
                            }
                        }
                    }

                    FREE(pinfo);
                    break;
                }
                case MSG_SCONFIG_SCANNING_DONE:
                {
                    wifi_sta_join_cfg *join_cfg = NULL;
                    if((1==g_sconfig_auto_join)&&(sconfig_result.valid==1))
                    {
                        join_cfg = (wifi_sta_join_cfg *)MALLOC(sizeof(wifi_sta_join_cfg));                            
                        if(NULL!=join_cfg)
                        {
                            //join_cfg->ssid.ssid_len=STRLEN((const char *)sconfig_done_cpy->u.sconfig_done.ssid);
                            //STRCPY((char *)join_cfg->ssid.ssid, (char *)sconfig_done_cpy->u.sconfig_done.ssid);
                            join_cfg->ssid.ssid_len=sconfig_result.ssid.ssid_len;
                            join_cfg->vif_idx = sconfig_result.vif_idx;
                            MEMCPY((void *)join_cfg->ssid.ssid, (void *)sconfig_result.ssid.ssid,sconfig_result.ssid.ssid_len);
                            STRCPY((char *)join_cfg->password, (char *)sconfig_result.password);
                            netmgr_wifi_switch_to_sta(join_cfg,sconfig_result.channel);
                            FREE(join_cfg);
                        }
                        else
                        {
                            LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);                                
                        }
                    }        
                    break;
                }
                case MSG_JOIN_RESULT:
                {
                    int join_status;
                    u8 vif_idx;
                    join_status = msg_evt->MsgData1;
                    vif_idx = (u8)msg_evt->MsgData2;
                    g_switch_join_cfg_b = false;
                    LOG_PRINTF("MSG_JOIN_RESULT=%d,vif_idx=%d\r\n",join_status,vif_idx);
                    /* join success */
                    if (join_status == CONNECT)
                    {
                        netmgr_netif_link_set(true,vif_idx);

                        if (g_netmgr_config[vif_idx].s_dhcpc_enable)
                        {
                            netmgr_dhcpc_start(true,vif_idx);
                        }
                        else
                        {
                            netmgr_netif_status_set(true,vif_idx);
                        }

                        LOG_DEBUGF(LOG_L4_NETMGR, ("JOIN SUCCESS\r\n"));

                        #ifdef NET_MGR_AUTO_JOIN
                        {
                            user_ap_info *ap_info;
                            Ap_sta_status *info = NULL;

                            info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
                            if(NULL!=info)
                            {
                                if (netmgr_wifi_info_get(info,vif_idx) == 0)
                                {
                                    MEMCPY((void*)ssid_buf,(void*)info->vif_u[vif_idx].station_status.ssid.ssid,info->vif_u[vif_idx].station_status.ssid.ssid_len);
                                    LOG_DEBUGF(LOG_L4_NETMGR, ("AutoJoin: SSID[%s] connected \r\n", ssid_buf));
                                    ap_info = netmgr_apinfo_find((char *)info->u.station.ssid.ssid);
                                    if (ap_info)
                                    {
                                       netmgr_apinfo_set(ap_info, true);
                                    }
                                }

                                FREE(info);
                            }
                            else
                            {
                                LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
                            }
                        }
                        #endif

                        #ifdef NET_MGR_AUTO_RETRY
                        {
                            g_auto_retry_status = S_TRY_STOP;
                            g_auto_retry_times = 0;
                            g_auto_retry_start_time = 0;

                            LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY S_TRY_STOP\r\n"));
                        }
                        #endif
                    }
                    /* join failure */
                    else if (join_status == DISCONNECT)
                    {
                        int ret = NS_OK;
                        LOG_DEBUGF(LOG_L4_NETMGR, ("JOIN FAILED\r\n"));
                        netmgr_netif_link_set(false,vif_idx);
                        netmgr_netif_status_set(false,vif_idx);

                        if (g_netmgr_config[vif_idx].s_dhcpc_enable)
                        {
                            ret = netdev_setipv4info(g_netifdev[vif_idx].name,0,0,0);
                            dhcpc_wrapper_set(g_netifdev[vif_idx].name, false);
                            g_netmgr_config[vif_idx].s_dhcpc_status = false;
                        }
                        else
                        {
	                        netmgr_netif_status_set(false,vif_idx);
                        }

                        #ifdef NET_MGR_AUTO_JOIN
                        //netmgr_autojoin_process();
                        #endif

                        #ifdef NET_MGR_AUTO_RETRY
                        netmgr_auto_retry_update();
                        #endif
                    }
                    break;
                }
                case MSG_LEAVE_RESULT:
                {
                    int leave_reason;
                    u8 vif_idx;
                    leave_reason = msg_evt->MsgData1;
                    vif_idx = msg_evt->MsgData2;
                    LOG_DEBUGF(LOG_L4_NETMGR, ("leave result reason = %d,vif_idx=%d\r\n", leave_reason,vif_idx));

                    /* leave success */
                    if (netmgr_wifi_get_vif_mode(vif_idx)==SSV6XXX_HWM_STA)                    
                    {
                        int ret = NS_OK;

                        netmgr_netif_link_set(false,vif_idx);
                        netmgr_netif_status_set(false,vif_idx);

                        if (g_netmgr_config[vif_idx].s_dhcpc_enable)
                        {
                            ret = netdev_setipv4info(g_netifdev[vif_idx].name,0,0,0);
                            dhcpc_wrapper_set(g_netifdev[vif_idx].name, false);
                            g_netmgr_config[vif_idx].s_dhcpc_status = false;
                        }
                        else
                        {
	                        netmgr_netif_status_set(false,vif_idx);
                        }

                        #ifdef NET_MGR_AUTO_JOIN
                        if (leave_reason != 0)
                        {
                            netmgr_wifi_scan(g_sta_channel_mask, NULL, 0);
                        }
                        #endif

                        #ifdef NET_MGR_AUTO_RETRY
                        if (leave_reason != 0)
                        {
                            netmgr_auto_retry_update();
                        }
                        #endif
                    }

                    break;
                }
                case MSG_PS_WAKENED:
                {
                    LOG_PRINTF("MSG_PS_WAKENED\r\n");
                    //lastTRX_time = ssv6xxx_drv_get_TRX_time_stamp();
                    break;
                }
                case MSG_PS_SETUP_OK:
                {
                    struct cfg_ps_request wowreq;
                    LOG_PRINTF("MSG_PS_SETUP_OK\r\n");
                    wowreq.host_ps_st = HOST_PS_START;
                    ssv6xxx_wifi_pwr_saving(&wowreq,true);
                    break;
                }
                case MSG_SCONFIG_PROCESS:
                {
                    if(FALSE==netmgr_wifi_check_sconfig(NULL))
                    {
                        g_sconfig_user_mode=FALSE;
                    }
                    else
                    {
#if(ENABLE_SMART_CONFIG==1)                    
                        if((ssv6xxx_user_sconfig_op.UserSconfigSM!=NULL)&&(g_sconfig_user_mode==TRUE)){
                            if(1== ssv6xxx_user_sconfig_op.UserSconfigSM()){
                                OS_MsDelay(50);
                                //Reuse the msg_evt
                                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                                msg_evt->MsgData = MSG_SCONFIG_PROCESS;
                                msg_evt->MsgData1 = 0;
                                msg_evt->MsgData2 = 0;
                                msg_evt->MsgData3 = 0;
                                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
                                msg_evt = NULL;
                            }
                        }
#endif
                    }
                    break;
                }
            }
            if(NULL!=msg_evt)
            {
                msg_evt_free(msg_evt);
            }
        }
    }
}
#endif


s32 netmgr_show(void)
{
/* TODO(aaron): fix to show linux's ip info, temply mark it */
#if 0
    bool dhcpd_status, dhcpc_status;
    int i;
    netmgr_cfg cfg;
    netstack_ip_addr_t ip,mask,gw,dns;

    for(i=0;i<MAX_VIF_NUM;i++)
    {
        netmgr_cfg_get(&cfg,i);

        LOG_PRINTF("\r\n");

        LOG_PRINTF("NET CONFIG, VIF=%d\r\n",i);
        ip.addr = cfg.ipaddr;
        mask.addr = cfg.netmask;
        gw.addr = cfg.gw;
        dns.addr = cfg.dns;
        LOG_PRINTF("IP  : %s\r\n", netstack_inet_ntoa(ip));
        LOG_PRINTF("MASK: %s\r\n", netstack_inet_ntoa(mask));
        LOG_PRINTF("GW  : %s\r\n", netstack_inet_ntoa(gw));
        LOG_PRINTF("DNS : %s\r\n", netstack_inet_ntoa(dns));

        LOG_PRINTF("\r\n");
        LOG_PRINTF("DHCPD CONFIG\r\n");

        ip.addr = (cfg.dhcps.start_ip);
        LOG_PRINTF("start ip      : %s\r\n", netstack_inet_ntoa(ip));

        ip.addr = (cfg.dhcps.end_ip);
        LOG_PRINTF("end ip        : %s\r\n", netstack_inet_ntoa(ip));

        LOG_PRINTF("max leases    : %d\r\n", cfg.dhcps.max_leases);

        mask.addr = (cfg.dhcps.subnet);
        LOG_PRINTF("subnet        : %s\r\n", netstack_inet_ntoa(mask));

        gw.addr = (cfg.dhcps.gw);
        LOG_PRINTF("gate way      : %s\r\n", netstack_inet_ntoa(gw));

        dns.addr = (cfg.dhcps.dns);
        LOG_PRINTF("dns           : %s\r\n", netstack_inet_ntoa(dns));

        LOG_PRINTF("auto time     : %d\r\n", cfg.dhcps.auto_time);

        LOG_PRINTF("decline time  : %d\r\n", cfg.dhcps.decline_time);

        LOG_PRINTF("conflict time : %d\r\n", cfg.dhcps.conflict_time);

        LOG_PRINTF("offer time    : %d\r\n", cfg.dhcps.offer_time);

        LOG_PRINTF("max leases sec: %d\r\n", cfg.dhcps.max_lease_sec);

        LOG_PRINTF("min leases sec: %d\r\n", cfg.dhcps.min_lease_sec);

        netmgr_dhcp_status_get(&dhcpd_status, &dhcpc_status, i);
        LOG_PRINTF("Dhcpd: %s\r\n", dhcpd_status ? "on" : "off");
        LOG_PRINTF("Dhcpc: %s\r\n", dhcpc_status ? "on" : "off");
    }

    LOG_PRINTF("\r\n");

    if (dhcpd_status)
    {
        dhcpdipmac ipmac[10];
        int i, count = 10;
        int ret = 0;

        ret = netstack_dhcp_ipmac_get(ipmac, &count);

        if ((ret == 0) && (count > 0))
        {
            LOG_PRINTF("DHCPD CLIENT\r\n");
            LOG_PRINTF("-----------------------------------------\r\n");
            LOG_PRINTF("|        MAC         |          IP      |\r\n");
            LOG_PRINTF("-----------------------------------------\r\n");
            for (i = 0; i < count; i++)
            {
                LOG_PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x] --- [%d.%d.%d.%d]\r\n", ipmac[i].mac[0], ipmac[i].mac[1], ipmac[i].mac[2],
                                    ipmac[i].mac[3], ipmac[i].mac[4], ipmac[i].mac[5],
                                     netstack_ip4_addr1_16(&ipmac[i].ip),
                          netstack_ip4_addr2_16(&ipmac[i].ip),
                          netstack_ip4_addr3_16(&ipmac[i].ip),
                          netstack_ip4_addr4_16(&ipmac[i].ip));
            }
        }
    }

#ifdef  NET_MGR_AUTO_JOIN
        netmgr_apinfo_show();
#endif

#ifdef  NET_MGR_AUTO_RETRY
        netmgr_auto_retry_show();
#endif
#endif
    return 0;
}

#ifdef  NET_MGR_AUTO_JOIN

int netmgr_autojoin_process()
{
    if (g_ap_list_p)
    {
        if (!netmgr_wifi_check_sta_connected())
        {
            user_ap_info *ap_info;
            ap_info = netmgr_apinfo_find_best(g_ap_list_p, g_max_num_of_ap_list);
            if ((ap_info != NULL) && ap_info->valid)
            {
                netmgr_apinfo_autojoin(ap_info);
                return 1;
            }
       }
    }

    return 0;
}

void netmgr_apinfo_clear()
{
    int i = 0;

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        MEMSET(&g_user_ap_info[i], 0, sizeof(g_user_ap_info[i]));
    }
}

void netmgr_apinfo_remove(char *ssid)
{
    int i = 0;

    if (!ssid)
    {
        return;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        //if (STRCMP((char *)ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        if (MEMCMP((void *)ssid, (void *)g_user_ap_info[i].ssid.ssid, g_user_ap_info[i].ssid.ssid_len) == 0)
        {
            MEMSET(&g_user_ap_info[i], 0, sizeof(g_user_ap_info[i]));
            return;
        }
    }

    return;
}

user_ap_info * netmgr_apinfo_find(char *ssid)
{
    int i = 0;

    if (!ssid)
    {
        return NULL;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        //if (STRCMP((char *)ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        if (MEMCMP((void *)ssid, (void *)g_user_ap_info[i].ssid.ssid, g_user_ap_info[i].ssid.ssid_len) == 0)
        {
            return &(g_user_ap_info[i]);
        }
    }

    return NULL;
}

bool netmgr_apinfo_find_in_aplist(struct ssv6xxx_ieee80211_bss * ap_list_p, int count, char * ssid)
{
    int i = 0;
    struct ssv6xxx_ieee80211_bss *item = NULL;

    if (!ssid || !ap_list_p || !(count > 0))
    {
        return false;
    }

    for (i = 0; i < count; i++)
    {
        item = (ap_list_p + i);

        if (item && (item->channel_id != 0))
        {
            //if (STRCMP((char *)ssid, (char *)(item->ssid.ssid)) == 0)
            if (MEMCMP((void *)ssid, (void *)(item->ssid.ssid),item->ssid.ssid_len) == 0)
            {
                return true;
            }
        }

    }

    return false;
}

user_ap_info * netmgr_apinfo_find_best(struct ssv6xxx_ieee80211_bss * ap_list_p, int count)
{
    int i = 0;

    for (i = NET_MGR_USER_AP_COUNT - 1; i >= 0; i--)
    {
        if (g_user_ap_info[i].valid)
        {
            if (netmgr_apinfo_find_in_aplist(ap_list_p, count, (char *)(char *)g_user_ap_info[i].ssid.ssid))
            {
                return &g_user_ap_info[i];
            }
        }
    }

    return NULL;
}

int netmgr_apinfo_get_last()
{
    int i = 0;

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        if ((g_user_ap_info[i].valid) == 0)
        {
            break;
        }
    }

    if (i >= NET_MGR_USER_AP_COUNT)
    {
        return (NET_MGR_USER_AP_COUNT - 1);
    }

    if (i == 0)
    {
        return -1;
    }

    return (i - 1);
}

void netmgr_apinfo_save(user_ap_info *ap_info)
{
    int i = 0, j = 0;
    int last_index = 0;

    if (ap_info == NULL)
    {
        return;
    }

    last_index = netmgr_apinfo_get_last();

    if (last_index < 0)
    {
        MEMCPY((void *)&(g_user_ap_info[i]), (char *)ap_info, sizeof(user_ap_info));
    }
    else
    {
        for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
        {
            //if (STRCMP((char *)ap_info->ssid.ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
            if (MEMCMP((void *)ap_info->ssid.ssid, (void *)g_user_ap_info[i].ssid.ssid, g_user_ap_info[i].ssid.ssid_len) == 0)
            {
                break;
            }
        }

        if (i < NET_MGR_USER_AP_COUNT)
        {
            for (j = i; j < last_index; j++)
            {
                MEMCPY((void *)&(g_user_ap_info[j]), (void *)&(g_user_ap_info[j + 1]), sizeof(user_ap_info));
            }

            // replace old item, but valid/join time not change
            MEMCPY((void *)&(g_user_ap_info[last_index].ssid), (char *)&(ap_info->ssid), sizeof(struct cfg_80211_ssid));
            MEMCPY((void *)g_user_ap_info[last_index].password, (char *)ap_info->password, sizeof(char)*(MAX_PASSWD_LEN+1));
        }
        else
        {
            if (last_index == (NET_MGR_USER_AP_COUNT - 1))
            {
                for (j = 0; j < (NET_MGR_USER_AP_COUNT - 1); j++)
                {
                    MEMCPY((void *)&(g_user_ap_info[j]), (void *)&(g_user_ap_info[j + 1]), sizeof(user_ap_info));
                }

                MEMCPY((void *)&(g_user_ap_info[last_index]), (char *)ap_info, sizeof(user_ap_info));
            }
            else
            {
                MEMCPY((void *)&(g_user_ap_info[last_index + 1]), (char *)ap_info, sizeof(user_ap_info));
            }
        }
    }
}

void netmgr_apinfo_set(user_ap_info *ap_info, bool valid)
{
    if (ap_info == NULL)
    {
        return;
    }

    ap_info->valid = valid;

    if (!valid)
    {
        MEMSET(ap_info, 0, sizeof(user_ap_info));
    }
}

static int netmgr_apinfo_autojoin(user_ap_info *ap_info)
{
    int ret = 0;
    wifi_sta_join_cfg *join_cfg = NULL;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};

    if (ap_info == NULL)
    {
        return -1;
    }

    join_cfg = MALLOC(sizeof(wifi_sta_join_cfg));
    if (NULL == join_cfg)
    {
        LOG_PRINTF("%s(%d):malloc fail\r\n",__FUNCTION__,__LINE__);
        return -1;
    }
    //STRCPY((char *)join_cfg->ssid.ssid, (char *)ap_info->ssid.ssid);
    MEMCPY((void *)join_cfg->ssid, (void *)ap_info->ssid, sizeof(struct cfg_80211_ssid));
    STRCPY((char *)join_cfg->password, (char *)ap_info->password);

    ret = netmgr_wifi_join(join_cfg);
    if (ret != 0)
    {
        // do nothing
    }
    else
    {
        ap_info=netmgr_apinfo_find((char *)join_cfg->ssid.ssid);
        if(ap_info!=NULL){
            MEMCPY((void*)ssid_buf,(void*)ap_info->ssid.ssid,ap_info->ssid.ssid_len);
            LOG_PRINTF("\r\nAuto join [%s]\r\n", ssid_buf);
            ap_info->join_times++;
        }
    }

    FREE(join_cfg);

    return ret;
}

void netmgr_apinfo_show()
{
    int i = 0;
    u8 ssid_buf[MAX_SSID_LEN+1]={0};

    //LOG_PRINTF("  netmgr_apinfo_show  \r\n");
    LOG_PRINTF("\r\n");
    LOG_PRINTF("-------------------------------------------------\r\n");
    LOG_PRINTF("|%20s|    |%s|   |%8s|  V\r\n", "ssid        ",  "password", "autoJoin");
    LOG_PRINTF("-------------------------------------------------\r\n");
    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        //if(g_user_ap_info[i].valid)
        {
            MEMCPY((void*)ssid_buf,(void*)g_user_ap_info[i].ssid.ssid,g_user_ap_info[i].ssid.ssid_len);
            LOG_PRINTF("|%20.20s      %s            %3d  |  %d\r\n", ssid_buf, "****", \
            g_user_ap_info[i].join_times, g_user_ap_info[i].valid);
        }
    }

    LOG_PRINTF("-------------------------------------------------\r\n");
}

#endif

#ifdef  NET_MGR_AUTO_RETRY
void netmgr_auto_retry_update()
{
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_auto_retry_update: g_auto_retry_status=%d\r\n", g_auto_retry_status));

    if ((g_auto_retry_status == S_TRY_INIT) || (g_auto_retry_status == S_TRY_STOP))
    {
        g_auto_retry_status = S_TRY_RUN;
        g_auto_retry_times = 0;
        g_auto_retry_start_time = os_tick2ms(OS_GetSysTick()) / 1000;
        LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY S_TRY_RUN, TIMES = %u, TIME = %u\r\n", g_auto_retry_times + 1, g_auto_retry_start_time));
    }
    else if (g_auto_retry_status == S_TRY_RUN)
    {
        if ((g_auto_retry_times + 1) > g_auto_retry_times_max)
        {
            g_auto_retry_status = S_TRY_STOP;
            g_auto_retry_times  = 0;
            g_auto_retry_start_time = 0;
            LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY S_TRY_STOP\r\n"));
        }
        else
        {
            g_auto_retry_start_time = os_tick2ms(OS_GetSysTick()) / 1000;
            LOG_DEBUGF(LOG_L4_NETMGR, ("AUTO RETRY S_TRY_RUN, TIMES = %u, TIME = %u\r\n", g_auto_retry_times + 1, g_auto_retry_start_time));
        }
    }
}

void netmgr_auto_retry_show()
{
    u8 ssid_buf[MAX_SSID_LEN+1]={0};
    LOG_PRINTF("\r\nNETMGR AUTO RETRY :   \r\n");

    if (g_auto_retry_status == S_TRY_INIT)
        LOG_PRINTF("STATUS            : INIT\r\n");
    else if (g_auto_retry_status == S_TRY_RUN)
        LOG_PRINTF("STATUS            : RUN\r\n");
    else if (g_auto_retry_status == S_TRY_STOP)
        LOG_PRINTF("STATUS            : STOP\r\n");
    else
        LOG_PRINTF("STATUS            : INVALID\r\n");

    LOG_PRINTF("DELAY             : %d\r\n", g_auto_retry_times_delay);
    LOG_PRINTF("MAX TIMES         : %d\r\n", g_auto_retry_times_max);
    LOG_PRINTF("CUR TIMES         : %d\r\n", g_auto_retry_times);
    MEMCPY((void*)ssid_buf,(void*)g_auto_retry_ap.ssid.ssid,g_auto_retry_ap.ssid.ssid_len);
    LOG_PRINTF("SSID              : %s\r\n", ssid_buf);
}

#endif

static void _netmgr_wifi_recovery_cb(void)
{
    LOG_PRINTF("Clear status for recovery\r\n");
    g_wifi_is_joining_b=FALSE;
    return;
}

int netmgr_wifi_get_sconfig_result(wifi_sconfig_result *res)
{
    if(res==NULL)
    {
        return -1;
    }
    
    if(sconfig_result.valid==0)
    {
        return -1;
    }
    MEMCPY((void *)&res->ssid,(void *)(&sconfig_result.ssid),sizeof(struct cfg_80211_ssid));
    STRCPY((void *)res->password,(void *)(sconfig_result.password));
    return 0;
}
