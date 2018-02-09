/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifndef __INIT_H__
#define __INIT_H__


/*******************************************************************************
*								Include										   *
*******************************************************************************/
#include <os_wrapper.h>
#include <log.h>
#include <host_apis.h>
#include <cli_cmd_wifi.h>
#if HTTPD_SUPPORT
#include <httpd.h>
#endif
#if(ENABLE_SMART_CONFIG==1)
#include <SmartConfig.h>
#endif
#include <net_mgr.h>
//#include <netstack.h>
#include <net_wrapper.h>
#include <dev.h>
#include "hif_wrapper.h"

/*******************************************************************************
*								Define										   *
*******************************************************************************/

/*******************************************************************************
*								Declare										   *
*******************************************************************************/

void customer_extra_setting(void);
/*******************************************************************************
*								Macro										   *
*******************************************************************************/
#define JOIN_DEFAULT_SSID    "asus-Ian"//"Default_ap" //"china"
#define JOIN_DEFAULT_PSK     "123456789" //"12345678"

#define IS_HT_SUPPORT (1)
#define MAC_EVENT_COUNT (g_host_cfg.pool_size+g_host_cfg.pool_sec_size+SSV_TMR_MAX)	
/*******************************************************************************
*								Extern										   *
*******************************************************************************/
/* in */
#if DHCPD_SUPPORT
extern int udhcpd_init(void);
#endif

extern void host_global_init(void);
extern bool ssv6xxx_drv_module_init(void);
extern bool ssv6xxx_drv_select(char name[SSV_HIF_DRV_NAME_MAX_LEN], union ssv_drv_info drv_info);
extern void os_timer_init(void);
extern int ssv6xxx_wifi_init_regdom(void);
extern int netmgr_wifi_switch_async(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_join_cfg *join_cfg);

#ifdef SSV_START_DEMO
extern ssv6xxx_result wifi_start(wifi_mode mode, bool static_ip, bool dhcp_server);
#endif
#if (CLI_ENABLE == 1)
extern void Cli_Init(s32 argc, char *argv[]);
#endif

//=====================Task parameter setting========================
extern struct task_info_st g_tx_task_info[];
extern struct task_info_st g_rx_task_info[];
extern struct task_info_st g_host_task_info[];
extern struct task_info_st g_timer_task_info[];
extern struct task_info_st st_netmgr_task[];
/* in */
#if DHCPD_SUPPORT
extern struct task_info_st st_dhcpd_task[];
#endif
/* not in */
#if (MLME_TASK==1)
extern struct task_info_st g_mlme_task_info[];
#endif

//=============================================================
extern bool ssv6xxx_drv_start (void);
extern int ssv6xxx_init_ap_mac(void *vif);

#if (AP_MODE_ENABLE == 1)
extern s32 AP_Init(u32 max_sta_num);
#endif
extern void ssv_netmgr_init_netdev(bool default_cfg);

#endif /* __INIT_H__ */
