#ifndef _SSV_HAL_H_
#define _SSV_HAL_H_

#include <host_config.h>
#include <ssv_types.h>
#include <cmd_def.h>

#if 0 //ndef s8
typedef char             s8;
#endif

//These defines are for ssv_drv, they accesed the registers
#if((CONFIG_CHIP_ID==SSV6030P)||(CONFIG_CHIP_ID==SSV6051Z)||(CONFIG_CHIP_ID==SSV6052Q))

#ifdef _PORTING_C_

#include <hal/SSV6030/firmware/ssv6200_uart_bin.h>
#define SSV_HAL_FW_BIN ssv6200_uart_bin

#else //#ifdef _PORTING_C_

#define SSV_HAL_FW_BIN

#endif //#ifdef _PORTING_C_

//#include <hal/SSV6030/ssv6030_pktdef.h>

#elif((CONFIG_CHIP_ID==SSV6006B)||(CONFIG_CHIP_ID==SSV6006C))

#ifdef _PORTING_C_

#if 0 //(CONFIG_CHIP_ID==SSV6006B)
#include <hal/SSV6006/firmware/SSV6006B/ssv6006_uart_bin.h>
#elif (CONFIG_CHIP_ID==SSV6006C)
//#include <hal/SSV6006/firmware/SSV6006C/ssv6006_uart_bin.h>
#include "ssv6006_uart_bin.h"
#endif

#define SSV_HAL_FW_BIN ssv6006_uart_bin

#else //#ifdef _PORTING_C_

#define SSV_HAL_FW_BIN

#endif //#ifdef _PORTING_C_


//#include <hal/SSV6006/ssv6006_pktdef.h>


#else
	error!!
#endif

//#define SSV6XXX_TX_DESC                                     void
//#define CFG_HOST_TXREQ                                      void
//#define CFG_HOST_RXPKT                                      void


//typedef void CFG_HOST_RXPKT;
//typedef void CFG_HOST_TXREQ;
//typedef void SSV6XXX_TX_DESC;

#define CFG_HOST_RXPKT void
#define CFG_HOST_TXREQ void
#define SSV6XXX_TX_DESC void

enum
{
    SW_RESET_MAC,
    SW_RESET_MCU,
    SW_RESET_SDIO,
    SW_RESET_SPI_SLV,
    SW_RESET_SYS_ALL,      
};
/*
ssv_hal_init
return value:
    TRUE: Success
    FALSE Fail
*/
extern bool ssv_hal_init(void);

/*
ssv_hal_chip_init
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_chip_init(void);

/*
ssv_hal_init_mac
self_mac: input
return value:
    1: Success
    0: Fail
*/
extern int ssv_hal_init_mac(u8 *self_mac);

/*
ssv_hal_init_sta_mac
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_init_sta_mac(u32 wifi_mode);

/*
ssv_hal_init_ap_mac
bssid: input
channel: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_init_ap_mac(u8 *bssid, u8 channel);

/*
ssv_hal_init_ap_mac
sec_type: input
password: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_ap_wep_setting(ssv6xxx_sec_type sec_type, u8 *password, u8 vif_idx, u8* sta_mac_addr);

/*
ssv_hal_tx_loopback_done
dat: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_tx_loopback_done(u8 *dat);

extern int ssv_hal_add_interface(u8 itf_idx,ssv6xxx_hw_mode hmode,u8 * selfmac,u8 channel);

/*
ssv_hal_setup_ampdu_wmm
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_setup_ampdu_wmm(bool IsAMPDU);
/*
ssv_hal_pbuf_alloc
return value:
    >0: buffer address
    -1: Fail
*/
extern int ssv_hal_pbuf_alloc(int size, int type);

/*
ssv_hal_pbuf_free
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_pbuf_free(int addr);

/*
ssv_hal_rf_enable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_rf_enable(void);

/*
ssv_hal_rf_disable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_rf_disable(void);

/*
ssv_hal_rf_load_default_setting
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_rf_load_default_setting(void);

/*
ssv_hal_watchdog_enable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_watchdog_enable(void);

/*
ssv_hal_watchdog_disable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_watchdog_disable(void);


/*
ssv_hal_mcu_enable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_mcu_enable(void);

/*
ssv_hal_mcu_disable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_mcu_disable(void);

/*
ssv_hal_sw_all_reset
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_sw_reset(u32);

/*
ssv_hal_gen_rand
data: output
len: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_gen_rand(u8 *data, u32 len);

/*
ssv_hal_promiscuous_enable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_promiscuous_enable(void);

/*
ssv_hal_promiscuous_disable
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_promiscuous_disable(void);

/*
ssv_hal_read_chip_id
return value:
    xxx: chip ID
    -1: Fail
*/
extern int ssv_hal_read_chip_id(void);


/*
ssv_hal_read_efuse_mac
mac: output

return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_read_efuse_mac(u8 *mac);

/*
ssv_hal_get_diagnosis
return value:
    TRUE: need to reset
    FALSE: no reset
*/
extern bool ssv_hal_get_diagnosis(void);

/*
ssv_hal_is_heartbeat
return value:
    1: heatbeat
    0: no heatbeat
    -1: Fail
*/
extern int ssv_hal_is_heartbeat(void);

/*
ssv_hal_reset_heartbeat
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_reset_heartbeat(void);

/*
ssv_hal_get_fw_status
val: output
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_get_fw_status(u32 *val);

/*
ssv_hal_set_fw_status
val: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_fw_status(u32 val);

/*
ssv_hal_reset_soc_irq
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_reset_soc_irq(void);

/*
ssv_hal_set_wakeup_bb_gpio
en: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_wakeup_bb_gpio(bool en);

/*
ssv_hal_set_short_slot_time
en: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_short_slot_time(bool en);

/*
ssv_hal_beacon_set
beacon_skb: input
dtim_offset:input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_beacon_set(void* beacon_skb, int dtim_offset);

/*
ssv_hal_soc_set_bcn
extra_type: input
frame:input
bcn_info:input
dtim_cnt:input
bcn_itv:input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_soc_set_bcn(enum ssv6xxx_tx_extra_type extra_type, void *frame, struct cfg_bcn_info *bcn_info, u8 dtim_cnt, u16 bcn_itv);

/*
ssv_hal_beacon_enable
bEnable: input

return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_beacon_enable(bool bEnable);

/*
ssv_hal_is_beacon_enable

return value:
    TRUE: beacon enable
    FALSE: beacon disable
*/
extern bool ssv_hal_is_beacon_enable(void);

/*
ssv_hal_get_tx_resources
pFreePage: output. how many pages can be stored incoming packets in wifi-chip
pFreeID: output. how many ids can be used for incoming packets in the wifi-chip
pFreeSpaces: output. how many space of HCI input queue for incoming packets

return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_get_tx_resources(u16 *pFreePage,u16 *pFreeID, u16 *pFreeSpaces);


/*
ssv_hal_bytes_to_pages

return value:
    xxx:Page numbers
    -1: Fail
*/
extern int ssv_hal_bytes_to_pages(u32 size);

/*
ssv_hal_get_rssi_from_reg

return value:
    xxx:RSSI
    -1: Fail
*/
extern int ssv_hal_get_rssi_from_reg(u8 vif_idx);

/*
ssv_hal_get_rc_info

tx_cnt: output
retry_cnt:output
phy_rate:output
return value:
    xxx:RSSI
    -1: Fail
*/
extern int ssv_hal_get_rc_info(u16 *tx_cnt, u16 *retry_cnt, u16 *phy_rate);


/*
ssv_hal_get_agc_gain

return value:
    xxx:Value
    -1: Fail
*/
extern int ssv_hal_get_agc_gain(void);

/*
ssv_hal_set_agc_gain
gain: input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_set_agc_gain(u32 gain);


/*
ssv_hal_set_acs_agc_gain
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_set_acs_agc_gain(void);
/*
ssv_hal_ap_listen_neighborhood
en: input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_ap_listen_neighborhood(bool en);


/*
ssv_hal_reduce_phy_cca_bits
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_reduce_phy_cca_bits(void);

/*
ssv_hal_recover_phy_cca_bits
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_recover_phy_cca_bits(void);

/*
ssv_hal_update_cci_setting
input_level:input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_update_cci_setting(u16 input_level);

/*
ssv_hal_set_ext_rx_int
input_level:input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_set_ext_rx_int(u32 pin);


/*
ssv_hal_set_TXQ_SRC_limit
input_level:input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_set_TXQ_SRC_limit(u32 qidx,u32 val);

extern int ssv_hal_halt_txq(u32 qidx,bool bHalt);

/*

ssv_hal_get_tempature
sar_code: output
temperature: output
return value:
    0:Success
    -1: Fail
*/

extern int ssv_hal_get_temperature(u8 *sar_code, char *temerpature);

/*

ssv_hal_set_voltage_mode
mode: input
return value:
    0:Success
    -1: Fail
*/

extern int ssv_hal_set_voltage_mode(u32 mode);


/*

ssv_hal_support_5g_band
return value:
    TRUE:Support 5G
    FALSE: Don't support 5G
*/
extern bool ssv_hal_support_5g_band(void);

extern u32 ssv_hal_get_ava_wsid(u32 init_id);

/*
ssv_hal_read_hw_queue
return value:
    0:success
    -1: fail
*/
extern int ssv_hal_read_hw_queue(void);

/*

ssv_hal_l2_off
vif_idx: intervace index
return value:
    -1:fail
     0:ok
*/
extern int ssv_hal_l2_off(u8 vif_idx);

/*

ssv_hal_l2_on
vif_idx: intervace index
return value:
    -1:fail
     0:ok
*/
extern int ssv_hal_l2_on(u8 vif_idx);

/*

ssv_hal_dump_txinfo
p: input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_dump_txinfo(void *p);

/*
ssv_hal_get_valid_txinfo_size
return value:
    xxx: size(unit is bytes)
    -1: Fail
*/

extern int ssv_hal_get_valid_txinfo_size(void);
/*
ssv_hal_get_txreq0_size
return value:
    xxx: size(unit is bytes)
    -1: Fail
*/
extern int ssv_hal_get_txreq0_size(void);

/*
ssv_hal_get_txreq0_ctype
p: input
return value:
    xxx: type
    -1: Fail
*/
extern int ssv_hal_get_txreq0_ctype(CFG_HOST_TXREQ *p);

/*
ssv_hal_set_txreq0_ctype
p: input
ctype: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_txreq0_ctype(CFG_HOST_TXREQ *p,u8 ctype);

/*
ssv_hal_get_txreq0_len
p: input
return value:
    xxx: size
    -1: Fail
*/
extern int ssv_hal_get_txreq0_len(CFG_HOST_TXREQ *p);

/*
ssv_hal_set_txreq0_len
p: input
len: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_txreq0_len(CFG_HOST_TXREQ *p,u32 len);

/*
ssv_hal_get_txreq0_rsvd0
p: input
return value:
    xxx: Value
    -1: Fail
*/
extern int ssv_hal_get_txreq0_rsvd0(CFG_HOST_TXREQ *p);

/*
ssv_hal_set_txreq0_rsvd0
p: input
val: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_txreq0_rsvd0(CFG_HOST_TXREQ *p,u32 val);

/*
ssv_hal_get_txreq0_padding
p: input
return value:
    xxx: value
    -1: Fail
*/
extern int ssv_hal_get_txreq0_padding(CFG_HOST_TXREQ *p);

/*
ssv_hal_set_txreq0_padding
p: input
val: input
return value:
    xxx: value
    -1: Fail
*/
extern int ssv_hal_set_txreq0_padding(CFG_HOST_TXREQ *p, u32 val);
/*
ssv_hal_get_txreq0_qos
p: input
return value:
    1: qos packet
    0: not qos packet
    -1: Fail
*/
extern int ssv_hal_get_txreq0_qos(CFG_HOST_TXREQ *p);

/*
ssv_hal_get_txreq0_ht
p: input
return value:
    1: ht packet
    0: non-ht packet
    -1: Fail
*/
extern int ssv_hal_get_txreq0_ht(CFG_HOST_TXREQ *p);

/*
ssv_hal_get_txreq0_4addr
p: input
return value:
    1: use 4addr
    0: no 4addr
    -1: Fail
*/
extern int ssv_hal_get_txreq0_4addr(CFG_HOST_TXREQ *p);

/*
ssv_hal_set_txreq0_f80211
p: input
f80211: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_txreq0_f80211(CFG_HOST_TXREQ *p, u8 f80211);

/*
ssv_hal_get_txreq0_f80211
p: input
return value:
    1: f80211 frame
    0: non-f80211 frame
    -1: Fail
*/
extern int ssv_hal_get_txreq0_f80211(CFG_HOST_TXREQ *p);

/*
ssv_hal_get_txreq0_more_data
p: input
return value:
    xxx: value
    -1: Fail
*/
extern int ssv_hal_get_txreq0_more_data(CFG_HOST_TXREQ *p);

/*
ssv_hal_set_txreq0_more_data
p: input
more_data:input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_txreq0_more_data(CFG_HOST_TXREQ *p, u8 more_data);

/*
ssv_hal_get_txreq0_qos_ptr
p: input
return value:
    xxx: qos ptr
    Null: Fail
*/
extern u8 * ssv_hal_get_txreq0_qos_ptr(CFG_HOST_TXREQ *p);

/*
ssv_hal_get_txreq0_data_ptr
p: input
return value:
    xxx: data ptr
    Null: Fail
*/
extern u8 * ssv_hal_get_txreq0_data_ptr(CFG_HOST_TXREQ *p);

/*
ssv_hal_fill_txreq0
frame: input
len:input
f80211:input
priority:input
tx_dscrp_flag:input
return value:
    xxx: frame address
    Null: Fail
*/
extern void * ssv_hal_fill_txreq0(void *frame, u32 len, u32 priority,
                                                u16 *qos, u32 *ht, u8 *addr4,
                                                bool f80211, u8 security, u8 tx_dscrp_flag,u8 vif_idx);


/*
ssv_hal_dump_rxinfo
p: input
return value:
    0:Success
    -1: Fail
*/
extern int ssv_hal_dump_rxinfo(void *p);

/*
ssv_hal_get_rxpkt_size

return value:
    xxx: size(unit is bytes)
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_size(void);

/*
ssv_hal_get_rxpkt_ctype
p: input
return value:
    xxx: type
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_ctype(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_len
p: input
return value:
    xxx: size
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_len(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_channel_info
p: input
return value:
    xxx: value
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_channel_info(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_rcpi
p: input
return value:
    xxx: value
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_rcpi(CFG_HOST_RXPKT *p);

/*
ssv_hal_set_rxpkt_rcpi
p: input
rcpi: input
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_set_rxpkt_rcpi(CFG_HOST_RXPKT *p, u32 rcpi);

/*
ssv_hal_get_rxpkt_qos
p: input
return value:
    1: qos packet
    0: not qos packet
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_qos(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_f80211
p: input
return value:
    1: f80211 frame
    0: non-f80211 frame
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_f80211(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_psm
p: input
return value:
    1: psm
    0: not psm
    -1: Fail
*/
extern int ssv_hal_get_rxpkt_psm(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_qos_ptr
p: input
return value:
    xxx: qos ptr
    Null: Fail
*/
extern u8 * ssv_hal_get_rxpkt_qos_ptr(CFG_HOST_RXPKT *p);
/*
ssv_hal_get_rxpkt_data_ptr
p: input
return value:
    xxx: data ptr
    Null: Fail
*/
extern u8 * ssv_hal_get_rxpkt_data_ptr(CFG_HOST_RXPKT *p);

/*
ssv_hal_get_rxpkt_data_len
p: input
return value:
*/
extern int ssv_hal_get_rxpkt_data_len(CFG_HOST_RXPKT *p);

extern u8 ssv_hal_get_rxpkt_bssid_idx(CFG_HOST_RXPKT *p);

/*
ssv_hal_download_fw
fw_bin: input
len: input
return value:
    0: Success
    -1l: Fail
*/
extern int ssv_hal_download_fw(u8 *fw_bin, u32 len);

/*
ssv_hal_process_hci_rx_aggr

pdata: input, agggr data start address
len: input, aggr data len
cbk_fh: input, rx mpdu handler
return value: next aggr data len
*/
extern u32 ssv_hal_process_hci_rx_aggr(void* pdata, u32 len, RxPktHdr cbk_fh);

/*
ssv_hal_process_hci_aggr_tx

tFrame: input, tx frame to be aggregated
aggr_buf: input, target buffer for aggregated data
aggr_len: output, Total aggregated data length
return value: aggregated pkt number
*/
extern u32 ssv_hal_process_hci_aggr_tx(void* tFrame, void* aggr_buf, u32* aggr_len);

/*
ssv_hal_hci_aggr_en

trx: input, tx frame to be aggregated
en: input, target buffer for aggregated data
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_hci_aggr_en(HCI_AGGR_HW trx, bool en);

/*
ssv_hal_accept_none_wsid_frame
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_accept_none_wsid_frame(void);

/*
ssv_hal_drop_none_wsid_frame
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_drop_none_wsid_frame(void);

/*
ssv_hal_drop_probe_request
return value:
    0: Success
    -1: Fail
*/

extern int ssv_hal_drop_probe_request(bool IsDrop);
/*
ssv_hal_sta_rcv_all_bcn
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_sta_rcv_all_bcn(void);

/*
ssv_hal_sta_rcv_specific_bcn
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_sta_rcv_specific_bcn(void);

/*
ssv_hal_sta_reject_bcn
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_sta_reject_bcn(void);

/*
ssv_hal_acs_rx_mgmt_flow
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_acs_rx_mgmt_flow(void);

/*
ssv_hal_ap_rx_mgmt_flow
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_ap_rx_mgmt_flow(void);

/*
ssv_hal_sconfig_rx_data_flow
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_sconfig_rx_data_flow(void);

/*
ssv_hal_sta_rx_data_flow
return value:
    0: Success
    -1: Fail
*/
extern int ssv_hal_sta_rx_data_flow(void);

/*
ssv_hal_get_chip_name
return value:
    */
extern const char *ssv_hal_get_chip_name(void);
#endif /* _SSV_HAL_H_ */

