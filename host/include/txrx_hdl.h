/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _TXRX_HDL_H
#define _TXRX_HDL_H
#include <dev.h>
#include "host_apis.h"

struct wifi_flt
{
	u8 fc_b7b2;
	u8 b7b2mask;
	data_handler cb_fn;
};

struct eth_flt
{
	u16 ethtype;
	data_handler cb_fn;    
};

struct qos_ctrl_st {
	u16 tid:4;
	u16 bit4:1;
	u16 ack_policy:2;
	u16 rsvd:1;
	u16 bit8_15:8;
};

struct ht_ctrl_st {
	u32 ht;
};


struct a4_ctrl_st {
	ETHER_ADDR a4;
};


#define SEQ_MODULO 0x1000
#define SEQ_MASK   0xfff

void ieee80211_release_reorder_frame(struct rx_ba_session_desc *tid_agg_rx, int index);
void ieee80211_release_reorder_frames(struct rx_ba_session_desc *tid_agg_rx, u16 head_seq_num);
void ieee80211_sta_reorder_release(struct rx_ba_session_desc *tid_agg_rx);
void ieee80211_rx_reorder_ampdu(void *frame);
void ieee80211_delete_ampdu_rx(u8 wsid);
void ieee80211_delete_all_ampdu_rx(void);
void timer_sta_reorder_release(void* data1, void* data2);
void ieee80211_addba_handler(void *data);

s32 TxRxHdl_Init(void);


#endif
