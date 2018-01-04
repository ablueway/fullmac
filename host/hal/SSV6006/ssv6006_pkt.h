#ifndef _SSV6006_PKT_H_
#define _SSV6006_PKT_H_

//tx
int ssv6006_hal_dump_txinfo(void *_p);
int ssv6006_hal_get_valid_txinfo_size(void);
int ssv6006_hal_get_txreq0_size(void);
int ssv6006_hal_get_txreq0_ctype(void *p);
int ssv6006_hal_set_txreq0_ctype(void *p,u8 c_type);
int ssv6006_hal_get_txreq0_len(void *p);
int ssv6006_hal_set_txreq0_len(void *p,u32 len);
int ssv6006_hal_get_txreq0_rsvd0(void *p);
int ssv6006_hal_set_txreq0_rsvd0(void *p,u32 val);
int ssv6006_hal_get_txreq0_padding(void *p);
int ssv6006_hal_set_txreq0_padding(void *p, u32 val);
int ssv6006_hal_get_txreq0_qos(void *p);
int ssv6006_hal_get_txreq0_ht(void *p);
int ssv6006_hal_get_txreq0_4addr(void *p);
int ssv6006_hal_get_txreq0_f80211(void *p);
int ssv6006_hal_set_txreq0_f80211(void *p,u8 f80211);
int ssv6006_hal_get_txreq0_more_data(void *p);
int ssv6006_hal_set_txreq0_more_data(void *p,u8 more_data);
u8 *ssv6006_hal_get_txreq0_qos_ptr(void *_req0);
u8 *ssv6006_hal_get_txreq0_data_ptr(void *_req0);

int ssv6006_hal_tx_8023to80211(void *frame, u32 len, u32 priority,
                                       u16 *qos, u32 *ht, u8 *addr4,
                                       bool f80211, u8 security);

void * ssv6006_hal_fill_txreq0(void *frame, u32 len, u32 priority,
                                            u16 *qos, u32 *ht, u8 *addr4,
                                            bool f80211, u8 security, u8 tx_dscrp_flag, u8 vif_idx);


// rx
int ssv6006_hal_rx_80211to8023(void *_PktRxInfo);
int ssv6006_hal_dump_rxinfo(void *_p);
int ssv6006_hal_get_rxpkt_size(void);
int ssv6006_hal_get_rxpkt_ctype(void *p);
int ssv6006_hal_get_rxpkt_len(void *p);
int ssv6006_hal_get_rxpkt_channel(void *p);
int ssv6006_hal_get_rxpkt_rcpi(void *p);
int ssv6006_hal_set_rxpkt_rcpi(void *p, u32 RCPI);
int ssv6006_hal_get_rxpkt_qos(void *p);
int ssv6006_hal_get_rxpkt_f80211(void *p);
int ssv6006_hal_get_rxpkt_wsid(void *p);
int ssv6006_hal_get_rxpkt_tid(void *p);
int ssv6006_hal_get_rxpkt_seqnum(void *p);
int ssv6006_hal_get_rxpkt_psm(void *p);
u8 *ssv6006_hal_get_rxpkt_qos_ptr(void *_rxpkt);
u8 *ssv6006_hal_get_rxpkt_data_ptr(void *_rxpkt);
int ssv6006_hal_get_rxpkt_data_len(void *rxpkt);
u8  ssv6006_hal_get_rxpkt_bssid_idx(void *rxpkt);
u32 ssv6006_hal_process_hci_rx_aggr(void* pdata, u32 data_length, RxPktHdr cbk_fh);
u32 ssv6006_hal_process_hci_aggr_tx(void* tFrame, void* aggr_buf, u32* aggr_len);
int ssv6006_hal_hci_aggr_en(HCI_AGGR_HW trx, bool en);

#endif //#ifndef _SSV6006_PKT_H_

