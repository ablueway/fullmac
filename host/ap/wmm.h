/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef WME_H
#define WME_H


#include <ssv_types.h>
#include "common/ieee802_11_defs.h"
#include "ap_config.h"
#include "os_wrapper.h"



struct ApInfo;
struct ieee80211_mgmt;
struct wmm_tspec_element;

u8 * hostapd_eid_wmm(struct ApInfo *pApInfo, u8 *eid);
int hostapd_eid_wmm_valid(struct ApInfo *pApInfo, const u8 *eid, size_t len);
void hostapd_wmm_action(struct ApInfo *pApInfo,
			const struct ieee80211_mgmt *mgmt, size_t len);
int wmm_process_tspec(struct wmm_tspec_element *tspec);

#endif /* WME_H */
