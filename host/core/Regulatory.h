/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _REGULATORY_H_
#define _REGULATORY_H_
#include <ssv_types.h>

#define BW 20

extern int freq_reg_info_regd(u32 center_freq,u32 desired_bw_khz,const struct ieee80211_regdomain *regd);
extern bool freq_need_dfs(u32 center_freq,u32 desired_bw_khz,const struct ieee80211_regdomain *regd);
extern bool freq_40MHZ_Available(u32 center_freq,const struct ieee80211_regdomain *regd);

#endif //_REGULATORY_H_
