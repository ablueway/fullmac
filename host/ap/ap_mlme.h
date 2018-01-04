/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _AP_MLME_H_
#define _AP_MLME_H_

#if (AP_MODE_ENABLE == 1)


#include <pbuf.h>

/**
 *  Define MLME error numbers:
 */
#define AP_MLME_OK                                  0
#define AP_MLME_FAILED                             -1
#define AP_MLME_TIMEOUT                            -2

#define IS_AP_IN_5G_BAND() IS_5G_BAND(gDeviceInfo->APInfo->nCurrentChannel)
#define IS_40MHZ_LEGAL_IN_THIS_COUNTRY() (TRUE==ssv6xxx_wifi_is_40MHZ_legal(gDeviceInfo->APInfo->nCurrentChannel))
#define IS_AP_HT20_40() (g_host_cfg.ap_ht20_only==FALSE)
#define IS_40MHZ_AVAILABLE() (IS_AP_IN_5G_BAND()&&IS_40MHZ_LEGAL_IN_THIS_COUNTRY()&&IS_AP_HT20_40())

struct ApInfo;

typedef s32 (*AP_MGMT80211_RxHandler)(struct ApInfo*, struct ieee80211_mgmt *mgmt, u16 len, u8 bssid_idx);

extern AP_MGMT80211_RxHandler AP_RxHandler[];




int i802_sta_deauth(const u8 *own_addr, const u8 *addr, int reason);
int i802_sta_disassoc(const u8 *own_addr, const u8 *addr, int reason);
int nl80211_poll_client(const u8 *own_addr, const u8 *addr, int qos);
s32 send_deauth_and_remove_sta(u8 *hwaddr);


#endif
#endif /* _AP_MLME_H_ */

