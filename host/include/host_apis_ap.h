/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _HOST_APIS_H_
#define _HOST_APIS_H_
#include <porting.h>

#undef PRINTF_FORMAT

#include <host_config.h>
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif /* _MSC_VER */

#include <ssv_ex_lib.h>

#include <ssv_common.h>
//#include <ssv_ether.h>
#include "net_wrapper.h"
#include <ssv_hal.h>

#define SSV6XXX_APLIST_PAGE_COUNT  10


enum TX_DSCRP_CONF_TYPE {
	TX_DSCRP_SET_EXTRA_INFO    = BIT(0),
	TX_DSCRP_SET_BC_QUE        = BIT(1),
};
typedef enum{
  HOST_API_DEACTIVE,
  HOST_API_ACTIVE,
  HOST_API_PWR_SAVING,
}HOST_API_STATE;

typedef enum{
  HOST_PS_SETUP,
  HOST_PS_START,
  HOST_PS_WAKEUP_OK,
}HOST_PS_STATE;

enum AMPDU_OPT
{
    AMPDU_TX_OPT_ENABLE,
    AMPDU_TX_OPT_SET_LAST_TRY_BMODE,
    AMPDU_TX_OPT_SET_LAST_BMODE_RATE,
    AMPDU_TX_OPT_SET_LAST_BMODE_RETRY,
    AMPDU_TX_OPT_SET_BLOCK_NON_NMODE,
    AMPDU_TX_OPT_SET_RETRY_MAX,
    AMPDU_TX_OPT_SET_MAIN_TRY,
    AMPDU_RX_OPT_ENABLE,
    AMPDU_RX_OPT_BUF_SIZE,
    AMPDU_TX_OPT_SET_FORCE_DELAY_TIME,     
};


//#define IS_TX_DSCRP_SET_EXTRA_INFO(x)        ((x) & (1<<TX_DSCRP_SET_EXTRA_INFO) )
//#define IS_TX_DSCRP_SET_BC_QUE(x)            ((x) & (1<<TX_DSCRP_SET_BC_QUE)   )


//#define SET_TX_DSCRP_SET_EXTRA_INFO(x, _tf)     (x) = (((x) & ~(1<<TX_DSCRP_SET_EXTRA_INFO))	| ((_tf)<<TX_DSCRP_SET_EXTRA_INFO) )
//#define SET_TX_DSCRP_SET_BC_QUE(x, _tf)      	(x) = (((x) & ~(1<<TX_DSCRP_SET_BC_QUE))	| ((_tf)<<TX_DSCRP_SET_BC_QUE)   )

#if 1
//-----------------------------------------------------------
//For Host AP API
enum conn_status {
    DISCONNECT,
    AUTH,
    ASSOC,
    EAPOL,
    CONNECT,
};

struct conn_info {
    u8                  Mac[ETHER_ADDR_LEN];    //00:11:22:33:44:55
    enum conn_status    status;                 //Auth,Assoc,Eapol
};
enum channel_select{
    EN_CHANNEL_0=0,
    EN_CHANNEL_1,
    EN_CHANNEL_2,
    EN_CHANNEL_3,
    EN_CHANNEL_4,
    EN_CHANNEL_5,
    EN_CHANNEL_6,
    EN_CHANNEL_7,
    EN_CHANNEL_8,
    EN_CHANNEL_9,
    EN_CHANNEL_10,
    EN_CHANNEL_11,
    EN_CHANNEL_12,
    EN_CHANNEL_13,
    EN_CHANNEL_14,
    EN_CHANNEL_15,

    EN_CHANNEL_36=36,
    EN_CHANNEL_40=40,
    EN_CHANNEL_44=44,    
    EN_CHANNEL_48=48,
    
    EN_CHANNEL_52=52,
    EN_CHANNEL_56=56,
    EN_CHANNEL_60=60,    
    EN_CHANNEL_64=64,

    EN_CHANNEL_100=100,
    EN_CHANNEL_104=104,
    EN_CHANNEL_108=108,    
    EN_CHANNEL_112=112,

    EN_CHANNEL_116=116,
    EN_CHANNEL_120=120,
    EN_CHANNEL_124=124,    
    EN_CHANNEL_128=128,

    EN_CHANNEL_132=132,    
    EN_CHANNEL_136=136,    
    EN_CHANNEL_140=140,
    EN_CHANNEL_149=149,

    EN_CHANNEL_153=153,    
    EN_CHANNEL_157=157,    
    EN_CHANNEL_161=161,
    EN_CHANNEL_165=165,
    
    EN_CHANNEL_AUTO_SELECT,
};

struct ap_setting {
	u32 						proto;              //WPA_PROTO_RSN
	u32 						key_mgmt;           //WPA_KEY_MGMT_PSK
    u32 						pairwise_cipher;	//WPA_CIPHER_XXX
    u32 						group_cipher;   	//WPA_CIPHER_XXX
    bool                        status;            //on,off
    u8                          password[MAX_PASSWD_LEN+1];     //12345
    u8                          vif_idx;
    u8                          static_ip;
    enum channel_select          channel;
    ssv6xxx_sec_type            security;
    struct cfg_80211_ssid       ssid;               //i-comm
};
typedef struct ap_setting Ap_setting;
typedef struct sta_setting {
    bool            status;                         //on,off
    u8              vif_idx;
} Sta_setting;

struct ap_sta_status_ap{
    u8                          selfmac[ETHER_ADDR_LEN];       //50:11:22:33:44:55
    u8                          channel;
    struct cfg_80211_ssid       ssid;               //i-commap
    u32                         stanum;             //0,1,2
    struct conn_info            stainfo [2];
    u32                         proto;            //WPA_PROTO_RSN
    u32                         key_mgmt;         //WPA_KEY_MGMT_PSK
    u32                         pairwise_cipher;  //WPA_CIPHER_XXX
    u32                         group_cipher;     //WPA_CIPHER_XXX
    ap_state                    current_st;
};
struct ap_sta_status_station{
    u8                          selfmac[ETHER_ADDR_LEN];       //50:11:22:33:44:55
    u8                          channel;
    struct cfg_80211_ssid       ssid;               //i-commap
    struct conn_info            apinfo;
    u16                         capab_info;
    u32                         proto;              //WPA_PROTO_RSN
    u32                         key_mgmt;           //WPA_KEY_MGMT_PSK
    u32                         pairwise_cipher;    //WPA_CIPHER_XXX
    u32                         group_cipher;       //WPA_CIPHER_XXX
};

typedef struct ap_sta_status {
    bool          status;
    ssv6xxx_hw_mode vif_operate[MAX_VIF_NUM];
    union {
        struct ap_sta_status_ap ap_status;
        struct ap_sta_status_station station_status;
    }vif_u[MAX_VIF_NUM];
}Ap_sta_status ;



#endif


/**
 * Define the error numbers of wifi host api:
 *
 * @ SSV6XXX_SUCCESS:
 * @ SSV6XXX_FAILED:
 */

typedef enum{
    SSV6XXX_SUCCESS                     =     0,
    SSV6XXX_FAILED                      =    -1,
    SSV6XXX_INVA_PARAM                  =    -2,
    SSV6XXX_NO_MEM                      =    -3,
    SSV6XXX_QUEUE_FULL                  =    -4,
    SSV6XXX_WRONG_HW_MODE_CMD           =    -5,
    SSV6XXX_WRONG_CHIP_ID               =    -6,

}ssv6xxx_result;


struct stamode_setting {
  struct sta_setting *sta_cfg;
  ssv6xxx_hw_mode   mode;
};

struct apmode_setting {
  struct ap_setting *ap_cfg;
  s32   step;
};

struct apmode_sta_info {

    //Mac Adderss ETHER_ADDR
    u8 addr[ETH_ALEN];

    /* STA's unique AID (1 .. 2007) or 0 if not yet assigned */
    u16 aid; 

    //ARP retry count
    u8 arp_retry_count;

    //record RCPI value
    u8 rcpi;
    u32 prev_rcpi;
};



//=========================================
//                Public Command Data
//=========================================
/**
 *  struct cfg_sonfig_request  - smart config request description
 *
 *  @ dwell_time: Time to wait for receiving beacon or probe response in each channel. (in 10ms).
 *  @ channel_mask: channel bit mask indicates which channels to scan
 */
struct cfg_sconfig_request {
    u32         channel_5g_mask; /* channel 0 ~ channel 15 */
    u16         channel_mask; /* channel 0 ~ channel 15 */
    u16         dwell_time;    
    bool        user_mode;  // TRUE: user mode (do smart configuration on host). FALSE:non-user mode  (do smart configuration on fw)
} ;//__attribute__ ((packed));

/**
 *  struct cfg_ps_wow_request  - PWR saving request description
 *
 *  @ ipv4addr.
 *  @ ethtyp: ether type to filter
 */
struct cfg_ps_request {
    HOST_PS_STATE   host_ps_st;
    u32             ipv4addr; /*ipaddr*/
    u32             ethtyp[MAX_ETHTYPE_TRAP]; /* 8 ethtype filter*/
    u32             dtim_multiple; /*listen dtim interval to multiple dtim period*/
} ;//__attribute__ ((packed));

/**
 *  struct cfg_scan_request  - scan request description
 *
 *  @ is_active: do active scan or passive scan
 *  @ n_ssids: the number of ssid to scan
 *  @ dwell_time: Time to wait for receiving beacon or probe response in each channel. (in 10ms).
 *  @ ssids: array of ssids to scan
 *  @ channel_mask: channel bit mask indicates which channels to scan
 *  @ ie_len: length of ie in octects
 *  @ ie_offset: optional information element(s) to add into probe request
 *                    frame.
 */
struct cfg_scan_request {
    u8      RSVD[1];
    u8      vif_idx;
    bool    is_active;
    u8      n_ssids;
    u8      dwell_time;
    u16     channel_mask; /* channel 0 ~ channel 15 */
    u32     channel_5g_mask;     
    //bool    ht_supported; /*whether STA could support ht */
    u8      ht_cap_ie[sizeof(struct ssv_ht_cap)]; /*STA's ht capability ie*/

    /* keep last */
    struct cfg_80211_ssid ssids[0];
} ;//__attribute__ ((packed));

//-------------------------------------------------------------------------------------------------------------------------------------------------
//HOST_CMD_JOIN

/**
 *  struct cfg_join_request - join request description
 */
struct cfg_join_request {
    ssv6xxx_sec_type    sec_type;
    int                 wep_keyidx;
    u8                  password[MAX_PASSWD_LEN+1];
    u8                  auth_alg;
    u8                  vif_idx;
#ifdef _INTERNAL_MCU_SUPPLICANT_SUPPORT_
    u8                  request_supplicant_bypass;
#endif
    u8                  no_bcn_timeout;
    struct ssv6xxx_ieee80211_bss bss;
    bool                ht_supported;
    u8                  ht_cap_ie[sizeof(struct ssv_ht_cap)]; /*STA's ht capability ie*/
    u32                 assoc_ie_len;
    u8                  assoc_ie[0];
} ;//__attribute__ ((packed));

//-------------------------------------------------------------------------------------------------------------------------------------------------
//HOST_CMD_LEAVE

/**
 *  struct cfg_leave_request - leave request description
 */
struct cfg_leave_request {
    //ETHER_ADDR      bssid;
    u16             info_len;
    u16             reason;
    u16             vif_idx;
    u16             rsvd;
} ;//__attribute__ ((packed));

/*
  *struct cfg_addba_resp-DEL_BA cmd parameter
  */
struct cfg_delba {
    u16 tid;
    u16 initiator;
    u16 reason_code;
} ;//__attribute__ ((packed));

/*
  *struct cfg_addba_resp-ADDBA_RESP cmd parameter
  */
struct cfg_addba_resp {
    u8  dialog_token;
    u16 policy;
    u16 tid;
    u16 buf_size;
    u16 timeout;
    u16 status;
    u16 start_seq_num;
} ;//__attribute__ ((packed));


/************************************************************************************************/
/*                                 SSV  WIFI Command function							 		*/
/************************************************************************************************/


H_APIs ssv6xxx_result ssv6xxx_wifi_init(void);
H_APIs ssv6xxx_result ssv6xxx_wifi_deinit(void);
H_APIs ssv6xxx_result ssv6xxx_wifi_pause(bool pause);

H_APIs s32 ssv6xxx_wifi_scan(struct cfg_scan_request *csreq);
H_APIs s32 ssv6xxx_wifi_join(struct cfg_join_request *cjreq);
H_APIs s32 ssv6xxx_wifi_leave(struct cfg_leave_request *clreq);
H_APIs s32 ssv6xxx_wifi_sconfig(struct cfg_sconfig_request *csreq);
H_APIs void ssv6xxx_wifi_set_channel(u8 num, ssv6xxx_hw_mode h_mode);
H_APIs void ssv6xxx_wifi_set_channel_ex(u8 num, u8 channel_offset);
//----------------------------I/O Ctrl-----------------------------

//id = ssv6xxx_host_cmd_id
H_APIs s32 ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len);


//H_APIs s32 ssv6xxx_wifi_ioctl(struct cfg_ioctl_request *cireq);


/************************************************************************************************/
/*                                  Data related function																				*/
/************************************************************************************************/

enum ssv6xxx_data_priority {
	ssv6xxx_data_priority_0,
	ssv6xxx_data_priority_1,
	ssv6xxx_data_priority_2,
	ssv6xxx_data_priority_3,
	ssv6xxx_data_priority_4,
	ssv6xxx_data_priority_5,
	ssv6xxx_data_priority_6,
	ssv6xxx_data_priority_7,
};

//------------------------------------Send-------------------------------------


H_APIs s32 ssv6xxx_wifi_send_ethernet(u8 vif_idx, void *frame, s32 len, enum ssv6xxx_data_priority priority);
//H_APIs s32 ssv6xxx_wifi_send_80211(void *frame, s32 len);



//H_APIs s32 ssv_wifi_send(void *data, s32 len, struct cfg_host_txreq *txreq);
//time to remove
//H_APIs s32 ssv_wifi_send(void *data, s32 len, struct cfg_host_txreq *txreq);


//------------------------------------Receive-------------------------------------

// struct cfg_host_rxpkt {
//
// 	 /* The definition of WORD_1: */
// 	u32             len:16;
// 	u32             c_type:3;
//     u32             f80211:1;
// 	u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
//     u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
//     u32             use_4addr:1;
// 	u32             l3cs_err:1;
//     u32             l4cs_err:1;
//     u32             align2:1;
//     u32             RSVD_0:2;
// 	u32             psm:1;
//     u32             stype_b5b4:2;
//     u32             extra_info:1;
//
//     /* The definition of WORD_2: */
//     u32             fCmd;
//
//     /* The definition of WORD_3: */
//     u32             hdr_offset:8;
//     u32             frag:1;
//     u32             unicast:1;
//     u32             hdr_len:6;
//     u32             RxResult:8;
//     u32             wildcard_bssid:1;
//     u32             RSVD_1:1;
//     u32             reason:6;
//
//     /* The definition of WORD_4: */
//     u32             payload_offset:8;
//     u32             next_frag_pid:7;
//     u32             RSVD_2:1;
//     u32             fCmdIdx:3;
//     u32             wsid:4;
// 	u32				RSVD_3:3;
// 	u32				drate_idx:6;
//
// 	/* The definition of WORD_5: */
// 	u32 			seq_num:16;
// 	u32				tid:16;
//
// 	/* The definition of WORD_6: */
// 	u32				pkt_type:8;
// 	u32				RCPI:8;
// 	u32				SNR:8;
// 	u32				RSVD:8;
//
//
//
// };
//


typedef enum{
	SSV6XXX_DATA_ACPT		= 0,	//Accept
	SSV6XXX_DATA_CONT		= 1,	//Pass data
	SSV6XXX_DATA_QUEUED		= 2,	//Data Queued
}ssv6xxx_data_result;




typedef ssv6xxx_data_result (*data_handler)(void *data, u32 len, u8 vif_idx);
typedef void (*evt_handler)(u32 nEvtId, void *data, s32 len);
typedef void (*promiscuous_data_handler)(u8 channel, u8 *rx_buf, u32 len);
typedef void (*recovery_handler)(void);

//Register handler to get RX data
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_rx_cb(data_handler handler);
//Register handler to get event
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_evt_cb(evt_handler evtcb);

//Register handler to get promiscuous data
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_promiscuous_rx_cb(promiscuous_data_handler promiscuous_cb);

//UnRegister Rx data handler
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_rx_cb(data_handler handler);

//UnRegister Rx event handler
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_evt_cb(evt_handler evtcb);

//UnRegister promiscuous call back function
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_promiscuous_rx_cb(promiscuous_data_handler promiscuous_cb);

//Register the recovery handler of up layer
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_recovery_cb(recovery_handler recovery_cb);
//Unregister the recovery handler of up layer
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_recovery_cb(recovery_handler recovery_cb);

H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_pwr_mode(u32 pwr_mode);

H_APIs HOST_API_STATE ssv6xxx_wifi_host_api_state(void);

H_APIs void ssv6xxx_wifi_clear_security (void);
H_APIs void ssv6xxx_wifi_apply_security (void);

H_APIs s32 ssv6xxx_wifi_send_addba_resp(struct cfg_addba_resp *addba_resp);
H_APIs s32 ssv6xxx_wifi_send_delba(struct cfg_delba *delba);


#if 0
//Data to soc
#if 1
H_APIs void* ssv_wifi_allocate(u32 size);
#else
H_APIs void* ssv_wifi_allocate(enum ssv_ts_type type,u32 size);
H_APIs struct cfg_host_txreq0* ssv_wifi_get_tx_req(void *data);
#endif
//H_APIs s32 ssv_wifi_free(void *data);
#endif

#if (CONFIG_HOST_PLATFORM == 1 )
H_APIs void ssv6xxx_wifi_apply_security_SIM (u8 bValue);
#endif


H_APIs s32 ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len);
H_APIs s32 ssv6xxx_wifi_ioctl_Ext(u32 cmd_id, void *data, u32 len,bool blocking);
H_APIs ssv6xxx_result ssv6xxx_wifi_station(u8 hw_mode,struct sta_setting *sta_station);
H_APIs ssv6xxx_result ssv6xxx_wifi_status(struct ap_sta_status *status_info);
H_APIs ssv6xxx_result ssv6xxx_wifi_ap(Ap_setting *ap_setting,s32 step); //step 0: hw register and global variable; step 1: auto-channel-selection; step 2: AP start
//H_APIs ssv6xxx_result ssv6xxx_wifi_station(u8 hw_mode,Sta_setting *sta_station);
//H_APIs ssv6xxx_result ssv6xxx_wifi_status(Ap_sta_status *status_info);
H_APIs s32 ssv6xxx_get_rssi_by_mac(u8 *macaddr,u8 vif_idx);
H_APIs u32 ssv6xxx_get_aplist_info(void **ap_list);
H_APIs ssv6xxx_result ssv6xxx_get_sta_info_by_aid(struct apmode_sta_info *sta_info,u8 aid_num);
H_APIs bool ssv6xxx_wifi_wakeup(void);
H_APIs s32 ssv6xxx_wifi_pwr_saving(struct cfg_ps_request* wowreq, bool on);

//------------------------------------Other Setting-------------------------------------
/* The mask of rate is depends on the sorted rates.
 * The order of sorted rates are [1M, 2M, 5.5M, 11M, 6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M].
 * So the bit 0 of mask means 1M, if bit 0 = 1, 1M rate is available.
 * Example: the mask of all rates would be 0x0FFF, the mask with b mode rates only would be 0x000F.
 */
H_APIs s32 ssv6xxx_set_rate_mask(u16 mask);

//Customized
H_APIs void ssv6xxx_get_tx_info(u16 *tx_cnt, u16 *retry_cnt, u16 *phy_rate);
H_APIs s32 ssv6xxx_set_ampdu_param(u8 mode, u8 vif_idx, u8 value);

H_APIs s32 ssv6xxx_wifi_get_mac(u8 *mac,u8 vif_idx);

//txTime Max: 200, Min: 70      qkTime Max: 50, Min: 10 ms
H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_duty(u32 qidx, u32 txTime, u32 qkTime);
H_APIs void ssv6xxx_set_TXQ_SRC_limit(u32 qidx,u32 val);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_trx_res_page(u8 tx_page, u8 rx_page);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_task_sleep(bool on);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_task_sleep_tick(u8  tick);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_ap_erp(bool on);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_ap_short_preamble(bool on);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_ap_rx_support_rate(u16 bas_msk,u16 mcs_msk);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_tx_task_retry_cnt(u8 cnt);
H_APIs int ssv6xxx_sw_reset(u32 com);
H_APIs int ssv6xxx_halt_tx_queue(u32 qidx, bool bHalt);
H_APIs ssv6xxx_result ssv6xxx_set_voltage_mode(u32 mode);
H_APIs bool ssv6xxx_wifi_support_5g_band(void);
H_APIs bool ssv6xxx_wifi_enable_5g_band(bool en);
H_APIs int ssv6xxx_wifi_ch_to_bitmask(u8 ch);
H_APIs int ssv6xxx_wifi_ch_bitmask_to_num(bool b5GBand, u8 idx);
H_APIs int ssv6xxx_wifi_show_reg_list(void);
H_APIs int ssv6xxx_wifi_get_reg(char *reg, u8 len);
H_APIs struct ieee80211_regdomain *ssv6xxx_wifi_get_current_regdomain(void);
H_APIs int ssv6xxx_wifi_set_reg(char *reg);
H_APIs int ssv6xxx_wifi_update_available_channel(void);
H_APIs int ssv6xxx_wifi_show_available_channel(void);
H_APIs bool ssv6xxx_wifi_is_available_channel(ssv6xxx_hw_mode mode, u8 ch);
H_APIs int ssv6xxx_wifi_align_available_channel_mask(ssv6xxx_hw_mode mode,u16 *channel_2g_mask, u32 *channel_5g_mask);
H_APIs int ssv6xxx_wifi_set_ap_no_dfs(bool no_dfs);
H_APIs bool ssv6xxx_wifi_is_40MHZ_legal(u8 ch);
H_APIs int ssv6xxx_wifi_set_ap_ht20_only(bool en);
H_APIs ssv6xxx_result ssv6xxx_wifi_set_sta_no_bcn_timeout(u8 value);
H_APIs int ssv6xxx_wifi_set_hci_aggr(HCI_AGGR_HW trx, bool en);
H_APIs HOST_API_STATE ssv6xxx_wifi_get_host_api_state(void);
H_APIs bool ssv6xxx_wifi_set_eco_mode(void);

//For instance ,hwaddr:a[0]~a[5]=60:11:22:33:44:55
H_APIs s32 ssv6xxx_wifi_ap_del_sta(u8 *hwaddr);

#ifdef _MSC_VER
#pragma pack(pop)
#endif /* _MSC_VER */

#endif /* _HOST_APIS_H_ */



