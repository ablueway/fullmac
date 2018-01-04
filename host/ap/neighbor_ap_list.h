/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifndef AP_LIST_H
#define AP_LIST_H

#include <ssv_types.h>
#include <os_wrapper.h>
#include "ap_config.h"
#include "ap_def.h"
struct neighbor_ap_info {
	
	u8 addr[6];
	u16 beacon_int;
	u16 capability;
	u8 supported_rates[AP_SUPP_RATES_MAX];
	u8 ssid[AP_MAX_SSID_LEN];
	size_t ssid_len;
	int wpa;
	int erp; /* ERP Info or -1 if ERP info element not present */

	int channel;
	//int datarate; /* in 100 kbps */
	//int ssi_signal;

	int ht_support;

	u32 num_beacons; /* number of beacon frames received */
	os_time_t last_beacon;

	//int already_seen; /* whether API call AP-NEW has already fetched
	//		   * information about this AP */
};

struct ieee802_11_elems;
//struct hostapd_frame_info;
struct ApInfo;
struct ieee80211_mgmt;


//struct neighbor_ap_info * ap_get_ap(ApInfo_st *pApInfo, const u8 *sta);


void neighbor_ap_list_process_beacon(struct ApInfo *pApInfo,
			    const struct ieee80211_mgmt *mgmt,
			    struct ieee802_11_elems *elems);

int neighbor_ap_list_init(struct ApInfo *pApInfo);
void neighbor_ap_list_deinit(struct ApInfo *pApInfo);






#endif /* AP_LIST_H */
