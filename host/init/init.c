/* 
 *  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
 *
 *  All Rights Reserved
 */
#ifdef __linux__
#include <linux/kernel.h>
#endif

#include "init.h"


/*******************************************************************************
*							Global	Variables								   *
*******************************************************************************/
#if SSV_LOG_DEBUG
u32 g_log_module;
u32 g_log_min_level;
#endif
// ----------------------------------------------------
// Configurations
// ----------------------------------------------------
// Mac address
u8 config_mac[] = { 0x60, 0x11, 0x22, 0x33, 0x44, 0x55 };

// Max number of AP list
u8 g_max_num_of_ap_list = NUM_AP_INFO;

// Auto channel selection in AP mode
u16 g_acs_channel_mask = ACS_CHANNEL_MASK;
u32 g_acs_5g_channel_mask = ACS_5G_CHANNEL_MASK;
u16 g_acs_channel_scanning_interval = ACS_SCANNING_INTERVAL;
u8 g_acs_channel_scanning_loop = ACS_SCANNING_LOOP;

// Default channel mask in sta and smart config mode
u16 g_sta_channel_mask = DEFAULT_STA_CHANNEL_MASK;
u32 g_sta_5g_channel_mask = DEFAULT_STA_5G_CHANNEL_MASK;

// ------------------------- rate control ---------------------------
struct Host_cfg g_host_cfg;

// ------------- User mode SmartConfig
// ...............................................
#if(ENABLE_SMART_CONFIG==1)
SSV6XXX_USER_SCONFIG_OP ssv6xxx_user_sconfig_op;
u32 g_sconfig_solution = SMART_CONFIG_SOLUTION;
u8 g_sconfig_auto_join = SMART_CONFIG_AUTO_JOIN;
#endif
// ----------------------------------------------------
u8 g_lwip_tcp_ignore_cwnd = LWIP_TCP_IGNORE_CWND;

/* in */
#ifdef NET_MGR_AUTO_RETRY
int g_auto_retry_times_delay = NET_MGR_AUTO_RETRY_DELAY;        // 5
int g_auto_retry_times_max = NET_MGR_AUTO_RETRY_TIMES;  // 0xFFFF
#endif


// #define SSV_IGMP_DEMO // multicast udp demo

#ifdef SSV_IGMP_DEMO

struct udp_pcb *g_upcb = NULL;
struct ip_addr ipMultiCast;

#define LISTEN_PORT       52000
#define LISTEN_ADDR       0xef0101ff    // 239.1.1.255
#define DST_PORT          52001

void ssv_test_print(const char *title, const u8 * buf, size_t len)
{
    size_t i;

    LOG_PRINTF("%s - hexdump(len=%d):\r\n    ", title, len);
    if (buf == NULL)
    {
        LOG_PRINTF(" [NULL]");
    }
    else
    {
        for (i = 0; i < 16; i++)
            LOG_PRINTF("%02X ", i);

        LOG_PRINTF("\r\n---\r\n00|");
        for (i = 0; i < len; i++)
        {
            LOG_PRINTF(" %02x", buf[i]);
            if ((i + 1) % 16 == 0)
                LOG_PRINTF("\r\n%02x|", (i + 1));
        }
    }
    LOG_PRINTF("\r\n-----------------------------\r\n");
}

int ssv_test_udp_tx(u8_t * data, u16_t len, u16_t port)
{
    int ret = -2;

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);

    memcpy(p->payload, data, len);

    ret = udp_sendto(g_upcb, p, (struct ip_addr *)(&ipMultiCast), port);

    pbuf_free(p);

    return ret;
}

void ssv_test_udp_rx(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                     struct ip_addr *addr, u16_t port)
{
    unsigned char g_tData[256];
    int ret = 0;

    // LOG_PRINTF("addr:0x%x port:%d len:%d\r\n", addr->addr, port,
    // p->tot_len);

    // ssv_test_print("ssv_test_udp_rx", OS_FRAME_GET_DATA(p),
    // OS_FRAME_GET_DATA_LEN(p));

    ret = udp_connect(upcb, addr, port);        /* connect to the remote host */
    if (ret != 0)
    {
        LOG_PRINTF("udp_connect: ret = %d\r\n", ret);
    }

    if (p->len >= 256)
        p->len = 128;

    memcpy(g_tData, p->payload, p->len);
    g_tData[p->len] = 0;
    LOG_PRINTF("rxdata: %s\r\n", g_tData);

    STRCPY((char *)g_tData, "recv it, ok!");
    ssv_test_udp_tx(g_tData, STRLEN((char *)g_tData), port);

    pbuf_free(p);               /* don't leak the pbuf! */
}


int ssv_test_udp_init(void)
{
    int ret = 1;

#if LWIP_IGMP
    ret = netmgr_igmp_enable(true);
    if (ret != 0)
    {
        LOG_PRINTF("netmgr_igmp_enable: ret = %d\r\n", ret);
        return ret;
    }
#endif /* LWIP_IGMP */

    g_upcb = udp_new();
    if (g_upcb == NULL)
    {
        LOG_PRINTF("udp_new fail\r\n");
        return -1;
    }

    ipMultiCast.addr = lwip_htonl(LISTEN_ADDR); // 239.1.1.255

#if LWIP_IGMP
    ret = igmp_joingroup(IP_ADDR_ANY, (struct ip_addr *)(&ipMultiCast));
    if (ret != 0)
    {
        LOG_PRINTF("igmp_joingroup: ret = %d\r\n", ret);
        return ret;
    }
#endif

    ret = udp_bind(g_upcb, IP_ADDR_ANY, LISTEN_PORT);
    if (ret != 0)
    {
        LOG_PRINTF("udp_bind: ret = %d\r\n", ret);
        return ret;
    }

    udp_recv(g_upcb, ssv_test_udp_rx, (void *)0);

    LOG_PRINTF("ssv_test_udp_init ok\r\n");

    return ret;
}
#endif


void ssv6xxx_init_task_para(void)
{
    g_txrx_task_info[0].prio = OS_TX_TASK_PRIO;
    g_txrx_task_info[1].prio = OS_RX_TASK_PRIO;
    g_host_task_info[0].prio = OS_CMD_ENG_PRIO;
    g_timer_task_info[0].prio = OS_TMR_TASK_PRIO;
    /* in */
    st_netmgr_task[0].prio = OS_NETMGR_TASK_PRIO;

    /* in */
#if DHCPD_SUPPORT
    st_dhcpd_task[0].prio = OS_DHCPD_TASK_PRIO;
#endif
    /* not in */
#if (MLME_TASK == 1)
    g_mlme_task_info[0].prio = OS_MLME_TASK_PRIO;
#endif
}


int ssv6xxx_start(ssv_vif *vif)
{
    // u32 wifi_mode;
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    ssv6xxx_drv_start();

    /* Reset MAC & Re-Init */

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    /* Initialize ssv6200 mac */
    if (-1 == ssv6xxx_init_mac(vif))
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return SSV6XXX_FAILED;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    // Set ap or station register
    if ((SSV6XXX_HWM_STA == vif->hw_mode)
        || (SSV6XXX_HWM_SCONFIG == vif->hw_mode))
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        if (-1 == ssv6xxx_init_sta_mac(vif->hw_mode))
        {
			LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
			return SSV6XXX_FAILED;
        }
    }
    else
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);    
        if (-1 == ssv6xxx_init_ap_mac(vif))
        {
			LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
			return SSV6XXX_FAILED;
        }
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);	
    customer_extra_setting();
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return SSV6XXX_SUCCESS;
}

#if (ENABLE_SMART_CONFIG == 1)
void init_smart_config(void)
{
    /* not in */
#if(SMART_CONFIG_SOLUTION == CUSTOMER_SOLUTION)
    // Register customer operation function
    // ssv6xxx_user_sconfig_op.UserSconfigInit= xxx ;
    // ssv6xxx_user_sconfig_op.UserSconfigPaserData= xxx ;
    // ssv6xxx_user_sconfig_op.UserSconfigSM= xxx;
    // ssv6xxx_user_sconfig_op.UserSconfigConnect= xxx;
    // ssv6xxx_user_sconfig_op.UserSconfigDeinit= xxx;
    /* in */
#else
    ssv6xxx_user_sconfig_op.UserSconfigInit = SmartConfigInit;
    ssv6xxx_user_sconfig_op.UserSconfigInit = SmartConfigInit;
    ssv6xxx_user_sconfig_op.UserSconfigPaserData = SmartConfigPaserData;
    ssv6xxx_user_sconfig_op.UserSconfigSM = SmartConfigSM;
    ssv6xxx_user_sconfig_op.UserSconfigConnect = SmartConfigConnect;
    ssv6xxx_user_sconfig_op.UserSconfigDeinit = SmartConfigDeinit;
#endif
}
#endif


/* Host config default value */
void init_host_default_config(bool support_ht)
{
    OS_MemSET((void *)&g_host_cfg, 0, sizeof(g_host_cfg));
    g_host_cfg.rate_mask = RC_DEFAULT_RATE_MSK;
    g_host_cfg.resent_fail_report = RC_DEFAULT_RESENT_REPORT;
    g_host_cfg.upgrade_per = RC_DEFAULT_UP_PF;
    g_host_cfg.downgrade_per = RC_DEFAULT_DOWN_PF;
    g_host_cfg.pre_alloc_prb_frm = RC_DEFAULT_PREPRBFRM;
    g_host_cfg.upper_fastestb = RC_DEFAULT_PER_FASTESTB;
    g_host_cfg.direct_rc_down = RC_DIRECT_DOWN;
    g_host_cfg.rc_drate_endian = RC_DEFAULT_DRATE_ENDIAN;
    g_host_cfg.tx_power_mode = CONFIG_TX_PWR_MODE;
    g_host_cfg.pool_size = POOL_SIZE;
    g_host_cfg.pool_sec_size = POOL_SEC_SIZE;
    g_host_cfg.recv_buf_size = RECV_BUF_SIZE;
    g_host_cfg.bcn_interval = AP_BEACON_INT;
    g_host_cfg.trx_hdr_len = TRX_HDR_LEN;
    g_host_cfg.erp = AP_ERP;
    g_host_cfg.b_short_preamble = AP_B_SHORT_PREAMBLE;
    g_host_cfg.tx_sleep = TX_TASK_SLEEP;
    g_host_cfg.tx_sleep_tick = TX_TASK_SLEEP_TICK;
    g_host_cfg.tx_retry_cnt = TX_TASK_RETRY_CNT;
    g_host_cfg.tx_res_page = TX_RESOURCE_PAGE;
    g_host_cfg.rx_res_page = RX_RESOURCE_PAGE;
    g_host_cfg.ap_rx_short_GI = AP_RX_SHORT_GI;
    g_host_cfg.ap_rx_support_legacy_rate_msk = AP_RX_SUPPORT_BASIC_RATE;
    g_host_cfg.ap_rx_support_mcs_rate_msk = AP_RX_SUPPORT_MCS_RATE;
    g_host_cfg.txduty.mode = TXDUTY_MODE;
    g_host_cfg.volt_mode = SSV_VOLT_REGULATOR;
    g_host_cfg.ampdu_rx_buf_size = AMPDU_RX_BUF_SIZE;

    if (support_ht == true)
    {
        g_host_cfg.support_ht = PHY_SUPPORT_HT;
        g_host_cfg.ampdu_rx_enable = AMPDU_RX_ENABLE;   // AMPDU_RX_ENABLE(0)
        g_host_cfg.ampdu_tx_enable = AMPDU_TX_ENABLE;   // AMPDU_TX_ENABLE(0)
    }
    else
    {
        g_host_cfg.support_ht = PHY_SUPPORT_HT;
        g_host_cfg.ampdu_rx_enable = 0;
        g_host_cfg.ampdu_tx_enable = 0;
    }

    g_host_cfg.support_rf_band = RF_BAND;
    g_host_cfg.support_tx_SGI = RC_DEFAULT_TX_SGI;
    g_host_cfg.ap_no_dfs = AP_NO_DFS_FUN;
    g_host_cfg.ap_ht20_only = AP_HT20_ONLY;
    g_host_cfg.sta_no_bcn_timeout = STA_NO_BCN_TIMEOUT;
    g_host_cfg.hci_rx_aggr = HCI_RX_AGGR;
    g_host_cfg.hci_aggr_tx = HCI_AGGR_TX;
    g_host_cfg.rxIntGPIO = RXINTGPIO;
    g_host_cfg.usePA = USE_EXT_PA;
    g_host_cfg.AP_TimAllOne = AP_MODE_BEACON_VIRT_BMAP_0XFF;
    g_host_cfg.ApStaInactiveTime = AP_SAT_INACTIVE;

    g_host_cfg.extRxInt = 0;

    /* TODO:aaron */
#if 1
    if (STRCMP(INTERFACE, "spi") == 0)
    {
        g_host_cfg.extRxInt = 1;
    }
    else
    {
        /* not in */
#ifdef EXT_RX_INT
        g_host_cfg.extRxInt = 1;
#else
        g_host_cfg.extRxInt = 0;
#endif
    }
#endif
}



ssv6xxx_result STAmode_default(bool bJoin)
{
    int ret = 0;
    wifi_mode mode = SSV6XXX_HWM_STA;
    u32 start_tick = 0;
    bool timeout = false;

    if (bJoin)
    {
    	wifi_sta_join_cfg join_cfg;
	    MEMSET((void *)&join_cfg, 0, sizeof(join_cfg));
        MEMCPY((void *)join_cfg.ssid.ssid, JOIN_DEFAULT_SSID,
               STRLEN(JOIN_DEFAULT_SSID));
        join_cfg.ssid.ssid_len = STRLEN(JOIN_DEFAULT_SSID);
        STRCPY((void *)join_cfg.password, JOIN_DEFAULT_PSK);
        join_cfg.vif_idx = 0;
        /* TODO: aaron */
        ret = netmgr_wifi_switch_async(mode, NULL, &join_cfg);
    }
    else
    {
        Sta_setting sta;

        MEMSET(&sta, 0, sizeof(Sta_setting));

        sta.status = TRUE;
        sta.vif_idx = 0;

        ret = netmgr_wifi_control_async(mode, NULL, &sta);
        printk("STAmode_default ret(%d)\r\n", ret);
    }
	
    if (ret != 0)
    {
        LOG_PRINTF("STA mode error (%d)\r\n", bJoin);
        return SSV6XXX_FAILED;
    }

    start_tick = OS_GetSysTick();
    while (!netmgr_wifi_check_sta(NULL))
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
        LOG_PRINTF("STA mode timeout\r\n");
		printk("%s(%d)\n",__FUNCTION__,__LINE__);		
        return SSV6XXX_FAILED;
    }

    LOG_PRINTF("STA mode success\r\n");
	printk("%s(%d)\n",__FUNCTION__,__LINE__);
    return SSV6XXX_SUCCESS;
}

ssv6xxx_result wifi_sta_start(bool static_ip)
{
    /* in */
#if USE_ICOMM_LWIP
    /* static_ip = false */
    if (static_ip)
    {
        netmgr_dhcpc_set(false, 0);
        {
            ipinfo info;

            info.ipv4 = ipaddr_addr("192.168.100.100");
            info.netmask = ipaddr_addr("255.255.255.0");
            info.gateway = ipaddr_addr("192.168.100.1");
            info.dns = ipaddr_addr("192.168.100.1");

            netmgr_ipinfo_set(0, &info, false);
        }
    }
    else
    {
        netmgr_dhcpc_set(true, 0);
    }
#endif
    return STAmode_default(false);      // false: don't auto join
                                        // AP_DEFAULT_SSDI
}

ssv6xxx_result wifi_start(wifi_mode mode, bool static_ip, bool dhcp_server)
{
    LOG_PRINTF("wifi start \r\n");
    /* station mode */
    if (mode == SSV6XXX_HWM_STA)
    {
        return wifi_sta_start(static_ip);
    }
    /* in */
#if (AP_MODE_ENABLE == 1)
    /* ap mode */
    else if (mode == SSV6XXX_HWM_AP)
    {
        return wifi_ap_start(static_ip, dhcp_server);
    }
#endif
    /* in */
#if (ENABLE_SMART_CONFIG == 1)
    else if (mode == SSV6XXX_HWM_SCONFIG)
    {
        return wifi_smart_config_start();
    }
#endif
    return SSV6XXX_FAILED;
}


ssv6xxx_result wifi_start_by_host_mode(ssv6xxx_hw_mode hmode)
{
    ssv6xxx_result res = SSV6XXX_SUCCESS;

    /* default hmode = SSV6XXX_HWM_STA */
	printk("%s(%d),hmode(%d)\n",__FUNCTION__,__LINE__,hmode);
    switch (hmode)
    {
    case SSV6XXX_HWM_STA:
        res = wifi_start(SSV6XXX_HWM_STA, false, false);
        break;
    case SSV6XXX_HWM_AP:
#if (AP_MODE_ENABLE == 1)
        res = wifi_start(SSV6XXX_HWM_AP, false, true);
#else
        res = SSV6XXX_FAILED;
#endif
        break;
    case SSV6XXX_HWM_SCONFIG:
#if (ENABLE_SMART_CONFIG==1)
        res = wifi_start(SSV6XXX_HWM_SCONFIG, false, true);
#else
        res = SSV6XXX_FAILED;
#endif
        break;
    default:
        res = wifi_start(SSV6XXX_HWM_STA, false, false);
    }
	printk("%s(%d),res(%d)\n",__FUNCTION__,__LINE__,res);
	return res;
}

/**
 *  Entry point of the firmware code. After system booting, this is the
 *  first function to be called from boot code. This function need to
 *  initialize the chip register, software protoctol, RTOS and create
 *  tasks. Note that, all memory resources needed for each software
 *  modulle shall be pre-allocated in initialization time.
 */
int ssv6xxx_dev_init(ssv6xxx_hw_mode hmode)
{
	union ssv_drv_info drv_info;
    ssv6xxx_result res = SSV6XXX_SUCCESS;

	platform_dev_init();

#if (SSV_LOG_DEBUG == 1)
    g_log_module = CONFIG_LOG_MODULE;
    g_log_min_level = CONFIG_LOG_LEVEL;
#endif
#if (ENABLE_SMART_CONFIG == 1)
    init_smart_config();
#endif

    init_host_default_config(IS_HT_SUPPORT);

    OS_Init();

    /* TODO:aaron */
    host_global_init();

	ssv6xxx_init_task_para();


    /* Total = 112 g_host_cfg.pool_size = POOL_SIZE(72),
       g_host_cfg.pool_sec_size = POOL_SEC_SIZE(16) SSV_TMR_MAX(24) */
	ASSERT(msg_evt_init(MAC_EVENT_COUNT) == OS_SUCCESS);

#if (CONFIG_USE_LWIP_PBUF == 0)    
	ASSERT(PBUF_Init(POOL_SIZE) == OS_SUCCESS);
#endif


	net_init(NULL);
	//netstack_init(NULL);

	/**
	 * Initialize Host simulation platform. The Host initialization sequence
	 * shall be the same as the sequence on the real host platform.
	 * @ Initialize host device drivers (SDIO/SIM/UART/SPI ...)
	 */
    ASSERT(ssv6xxx_drv_module_init() == true);

    LOG_PRINTF("Try to connecting CABRIO via %s...\n\r", INTERFACE);

	/* #define INTERFACE "usb" */
	drv_info.fields.os_type = DRV_INFO_FLAG_OS_TYPE_LINUX;
	drv_info.fields.register_type = DRV_INFO_FLAG_REGISTER_TYPE_PASSIVE;
	drv_info.fields.hw_type = DRV_INFO_FLAG_HW_TYPE_USB;	

    if (ssv6xxx_drv_select(INTERFACE, drv_info) == false)
    {
        LOG_PRINTF("==============================\n\r");
        LOG_PRINTF("Please Insert %s wifi device\n", INTERFACE);
        LOG_PRINTF("==============================\n\r");
        return -1;
    }


    ASSERT(ssv_hal_init() == true);

    if (ssv6xxx_wifi_init() != SSV6XXX_SUCCESS)
        return -1;

    os_timer_init();

    ssv6xxx_wifi_init_regdom();
    ssv6xxx_wifi_set_reg(DEFAULT_COUNTRY_CODE);

#if (AP_MODE_ENABLE == 1)
    AP_Init(WLAN_MAX_STA);
#endif

	/* TODO(aaron): linux need this ? */
    /* netmgr int */
    ssv_netmgr_init(true);

    /* we can set default ip address and default dhcpd server config */
    ssv_netmgr_init_netdev(true);

    wifi_start_by_host_mode(hmode);

#if (CLI_ENABLE==1)
    Cli_Init(0, NULL);

    ssv6xxx_wifi_cfg();
#endif
	//OS_StartScheduler();
	
    LOG_PRINTF("%s at(%d)\n", __FUNCTION__, __LINE__);
    return res;
}
