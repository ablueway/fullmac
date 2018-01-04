/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _COMMON_H_
#define _COMMON_H_

#include <porting.h>

typedef enum{
    SSV6XXX_HWM_STA		,
    SSV6XXX_HWM_AP		,
    SSV6XXX_HWM_IBSS	,
    SSV6XXX_HWM_WDS	    ,
    SSV6XXX_HWM_SCONFIG ,
    SSV6XXX_HWM_INVALID	
}ssv6xxx_hw_mode;

typedef enum{
	AP_STATE_INIT			,
	AP_STATE_IDLE			,	//Off
	AP_STATE_READY			,	//Init
	//AP_STATE_READY_SET_AP_MODE	,
	AP_STATE_RUNNING		,	//Running
	AP_STATE_PAUSE			
}ap_state;

enum nl80211_channel_type {
	NL80211_CHAN_NO_HT,
	NL80211_CHAN_HT20,
	NL80211_CHAN_HT40MINUS,
	NL80211_CHAN_HT40PLUS
};

//custom temperture setting
#ifndef EAPOL_ETHER_TYPPE
#define EAPOL_ETHER_TYPPE	0x888E
#endif

enum ieee80211_band {
IEEE80211_BAND_2GHZ ,
IEEE80211_BAND_5GHZ ,
/* keep last */
IEEE80211_NUM_BANDS
};

typedef enum {
	LONG_PREAMBLE = 0,
	SHORT_PREAMBLE = 1
} ePreamble;

typedef enum{
	SSV6XXX_SEC_NONE,
	SSV6XXX_SEC_WEP_40,			//5		ASCII
	SSV6XXX_SEC_WEP_104,		//13	ASCII
	SSV6XXX_SEC_WPA_PSK,		//8~63	ASCII
	SSV6XXX_SEC_WPA2_PSK,		//8~63	ASCII
	SSV6XXX_SEC_WPS,
	SSV6XXX_SEC_MAX
}ssv6xxx_sec_type;

typedef struct ssv_rf_temperature_st {
    u32 address;
    u32 high_temp;
    u32 regular_temp;
    u32 low_temp;
} ssv_rf_temperature;

#if (CONFIG_CHIP_ID==SSV6006C)
#define MAX_VIF_NUM 2
#else
#define MAX_VIF_NUM 1
#endif
//MAC: 60 11 22 33 44 55 = byte 0:1:2:3:4:5
#define MAC_MASK 0xef //For byte3

#define WSID_NOT_FOUND                0xf

#define RF_TEMPER_ARRARY_SIZE 5
#define RF_TEMPER_SETTING_SIZE RF_TEMPER_ARRARY_SIZE*sizeof(struct ssv_rf_temperature_st)

#define MAX_CCI_SENSITIVE 128
#define MIN_CCI_SENSITIVE 0

/* 30 byte 4 addr hdr, 2 byte QoS, 2304 byte MSDU, 12 byte crypt, 4 byte FCS, 80 byte rx_desc */
#define MAX_FRAME_SIZE                      2432

//HCI RX AGG
#define MAX_RX_PKT_RSVD                     512
#define HCI_AGGR_SIZE                   0x1b00
#define MAX_HCI_AGGR_SIZE                (HCI_AGGR_SIZE+MAX_FRAME_SIZE)  //AGGR_SIZE+MPDU

#define WPA_CIPHER_NONE BIT(0)
#define WPA_CIPHER_WEP40 BIT(1)
#define WPA_CIPHER_WEP104 BIT(2)
#define WPA_CIPHER_TKIP BIT(3)
#define WPA_CIPHER_CCMP BIT(4)
#ifdef CONFIG_IEEE80211W
#define WPA_CIPHER_AES_128_CMAC BIT(5)
#endif /* CONFIG_IEEE80211W */

#define WPA_KEY_MGMT_IEEE8021X BIT(0)
#define WPA_KEY_MGMT_PSK BIT(1)
#define WPA_KEY_MGMT_NONE BIT(2)
#define WPA_KEY_MGMT_IEEE8021X_NO_WPA BIT(3)
#define WPA_KEY_MGMT_WPA_NONE BIT(4)
#define WPA_KEY_MGMT_FT_IEEE8021X BIT(5)
#define WPA_KEY_MGMT_FT_PSK BIT(6)
#define WPA_KEY_MGMT_IEEE8021X_SHA256 BIT(7)
#define WPA_KEY_MGMT_PSK_SHA256 BIT(8)
#define WPA_KEY_MGMT_WPS BIT(9)

#define WPA_PROTO_WPA BIT(0)
#define WPA_PROTO_RSN BIT(1)

#define WPA_AUTH_ALG_OPEN BIT(0)
#define WPA_AUTH_ALG_SHARED BIT(1)
//#define WPA_AUTH_ALG_LEAP BIT(2)
//#define WPA_AUTH_ALG_FT BIT(3)


#define MAX_SSID_LEN 32
#define MAX_PASSWD_LEN 63
#define MAX_2G_CHANNEL_NUM 16
#define MAX_5G_CHANNEL_NUM 25
#define MAX_CHANNEL_NUM (MAX_2G_CHANNEL_NUM+MAX_5G_CHANNEL_NUM)
#define IS_5G_BAND(CH) ((CH)>MAX_2G_CHANNEL_NUM)

#define MAX_WEP_PASSWD_LEN (13+1)

#define MAX_ETHTYPE_TRAP 8

/*station timeout ARP request count*/

#define STA_TIMEOUT_RETRY_COUNT (5)
#define STA_TIMEOUT_RETRY_TIMER (3 * 1000) //msec


#define PHY_INFO_TBL1_SIZE          39
#define PHY_INFO_TBL2_SIZE          39
#define PHY_INFO_TBL3_SIZE          8



//------------------------------------------------

/**
 *  struct cfg_sta_info - STA structure description
 *
*/
#if 0
struct cfg_sta_info {
    ETHER_ADDR      addr;
#if 1
		u32 			bit_rates; /* The first eight rates are the basic rate set */

		u8				listen_interval;

		u8				key_id;
		u8				key[16];
		u8				mic_key[8];

		/* TKIP IV */
		u16 			iv16;
		u32 			iv32;
#endif

} ;//__attribute__ ((packed));

#endif



/**
 *  struct cfg_bss_info - BSS/IBSS structure description
 *
 */
#if 0
struct cfg_bss_info {
    ETHER_ADDR          bssid;

};// __attribute__ ((packed));

#endif
//PACK(struct ETHER_ADDR_st
//{
//    u8      addr[ETHER_ADDR_LEN];
//})
struct ETHER_ADDR_st
{
    u8      addr[ETHER_ADDR_LEN];
};
typedef struct ETHER_ADDR_st        ETHER_ADDR;

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))

typedef enum
{
    MT_STOP,
    MT_RUNNING,
    MT_EXIT
}  ModeType; //CmdEngine/TXRX_Task mode type

/**
 *  struct cfg_80211_ssid - SSID description, the max of the length of 32-byte.
 *
 *  @ ssid: the SSID
 *  @ ssid_len: length of the SSID
 */
//PACK( struct cfg_80211_ssid {
//    u8              ssid[MAX_SSID_LEN];//ssid[] didnot include '\0' at end of matrix
//    u8              ssid_len;
//})
struct cfg_80211_ssid {
    u8              ssid[MAX_SSID_LEN];//ssid[] didnot include '\0' at end of matrix
    u8              ssid_len;
};
//=========================================
//                Public Command Data
//=========================================

//-------------------------------------------------------------------------------------------------------------------------------------------------
//HOST_CMD_SCAN

#define IEEE80211_MAX_SSID_LEN		32
#define IEEE80211_MAX_SUPP_RATES    32
#define IEEE80211_HT_MCS_MASK_LEN   10

/**
 * struct ssv_mcs_info - MCS information
 * @rx_mask: RX mask
 * @rx_highest: highest supported RX rate. If set represents
 *	the highest supported RX data rate in units of 1 Mbps.
 *	If this field is 0 this value should not be used to
 *	consider the highest RX data rate supported.
 * @tx_params: TX parameters
 */
//PACK( 
struct ssv_mcs_info {
	u8 rx_mask[IEEE80211_HT_MCS_MASK_LEN];
	u16 rx_highest;
	u8 tx_params;
	u8 reserved[3];
};
//)

/**
 * struct ssv_ht_cap - HT capabilities IE
 *
 */
//PACK( 
struct ssv_ht_cap {
    u16 cap_info;
    u8 ampdu_params_info;

    /* 16 bytes MCS information */
    struct ssv_mcs_info mcs;

    u16 extended_ht_cap_info;
    u32 tx_BF_cap_info;
    u8 antenna_selection_info;
};
//)

/**
 * struct ssv_ht_info - HT operation IE
 *
 */
//PACK( 
struct ssv_ht_info {
    u8 control_chan;
    u8 ht_param;
    u16 operation_mode;
    u16 stbc_param;
    u8 basic_set[16];
};
//)


//PACK( 
struct ssv6xxx_rxphy_info_padding {
    /* WORD 1: for B, G, N mode */
    u32             rpci:8;     /* RSSI */
    u32             snr:8;
    u32             RSVD:16;
};
//)

//PACK( 
struct ssv6xxx_ieee80211_bss {
    struct cfg_80211_ssid ssid;
    u16 capab_info;
    u16 parameter_set_count;

    ETHER_ADDR	bssid;

    u8 dtim_period;
    u8 wmm_used;
    u8 uapsd_supported;
    u8 channel_id;

    //RSN
    u16 proto;
    u32	group_cipher;
    u32	pairwise_cipher[2];

    u32	last_probe_resp;

    u8 supp_rates[IEEE80211_MAX_SUPP_RATES];
    u8 supp_rates_len;

    /*
     * During association, we save an ERP value from a probe response so
     * that we can feed ERP info to the driver when handling the
     * association completes. these fields probably won't be up-to-date
     * otherwise, you probably don't want to use them.
     */
    u8 has_erp_value;
    u8 erp_value;

    u8 ht_used;
    struct ssv_ht_cap ht_cap;
    struct ssv6xxx_rxphy_info_padding rxphypad;
    u32 prev_rcpi;
};
//)

#endif /* _COMMON_H_ */

