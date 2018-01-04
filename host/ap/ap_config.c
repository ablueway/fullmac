/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#define __SFILE__ "ap_config.c"

#include <os_wrapper.h>
#include <log.h>
#include <dev.h>
#include "ap_config.h"

extern struct Host_cfg g_host_cfg;

#if (AP_MODE_ENABLE == 1)

//11a		6¡B9¡B12¡B18¡B24¡B36¡B48 ,54 						Basic 6,12,24
//11b only	1, 2, 5.5, 11										Basic 1 (default), 2
//11bg		1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54			Basic 1, 2 (default), 5.5, 11

struct ieee80211_rate hw_rates[] = {	
	{	IEEE80211_RATE_NUM_1M,
	IEEE80211_RATE_FLAGS_MANDATORY_B|IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
//#ifdef __SSV_GN__	
	{	IEEE80211_RATE_NUM_2M,
	IEEE80211_RATE_FLAGS_MANDATORY_B|IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	
	{	IEEE80211_RATE_NUM_5_5M,
	IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},	
	{	IEEE80211_RATE_NUM_11M,
	IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},	
	{	IEEE80211_RATE_NUM_6M,
	IEEE80211_RATE_FLAGS_MANDATORY_A|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},	
	{	IEEE80211_RATE_NUM_9M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_12M,
	IEEE80211_RATE_FLAGS_MANDATORY_A|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_18M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_24M,
	IEEE80211_RATE_FLAGS_MANDATORY_A|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_36M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_48M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_54M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
//#endif	
};

void ssv6xxx_config_init_rates(struct ssv6xxx_host_ap_config *pConfig)
{
    u32 i, num=0,count=0;

	/* not in, pConfig->preamble = LONG_PREAMBLE */
    if(pConfig->preamble == SHORT_PREAMBLE)
    {
        hw_rates[1].flags |= IEEE80211_RATE_FLAGS_SHORT_PREAMBLE; //2M
        hw_rates[2].flags |= IEEE80211_RATE_FLAGS_SHORT_PREAMBLE; //5.5M
        hw_rates[3].flags |= IEEE80211_RATE_FLAGS_SHORT_PREAMBLE; //11M
    }

    for (i = 0; i<(sizeof(hw_rates)/sizeof(hw_rates[0])); i++)
    {
        if ((hw_rates[i].flags & ( 1 << (pConfig->eApMode+AP_MODE_TO_HW_RATE_FLAGS_SUPPORT_SHIFT) )) != 0)
        {
            num++;
        }
    }

    pConfig->nNumrates = (u8)num;
    if(NULL==pConfig->pRates)
    {
        pConfig->pRates = (struct ieee80211_rate **)MALLOC(sizeof(u32)*pConfig->nNumrates);
        ASSERT(pConfig->pRates != NULL);
    }
    OS_MemSET((void *)pConfig->pRates,0,(sizeof(u32)*pConfig->nNumrates));
    pConfig->nNumrates=0;
    LOG_PRINTF("RX support rate:");
    for (i=0;i<(sizeof(hw_rates)/sizeof(hw_rates[0]));i++)
    {
        if(g_host_cfg.ap_rx_support_legacy_rate_msk& (1<<i))
        {            
            if ((hw_rates[i].flags & ( 1 << (pConfig->eApMode+AP_MODE_TO_HW_RATE_FLAGS_SUPPORT_SHIFT) )) !=0  )
            {
                LOG_PRINTF("%d ",hw_rates[i].bitrate);
                pConfig->pRates[count++] = &hw_rates[i];
                pConfig->nNumrates++;
            }
        }
    }
    LOG_PRINTF("\r\n");
}

extern struct Host_cfg g_host_cfg;
void ssv6xxx_config_init(struct ssv6xxx_host_ap_config *pConfig)
{
	/* ssid_len = 3, DEFAULT_SSID = ssv */
    pConfig->ssid_len = strlen(DEFAULT_SSID);
    MEMCPY(pConfig->ssid, DEFAULT_SSID, pConfig->ssid_len);

	/* not in, g_host_cfg.b_short_preamble = AP_B_SHORT_PREAMBLE(0) */
    if (g_host_cfg.b_short_preamble == 1)
        pConfig->preamble = SHORT_PREAMBLE;			// SHORT_PREAMBLE(1)
    else
        pConfig->preamble = LONG_PREAMBLE;       	// LONG_PREAMBLE(0) 

    pConfig->eApMode = AP_DEFAULT_HW_MODE;			// AP_MODE_IEEE80211G(1)

    pConfig->nChannel = AP_DEFAULT_CHANNEL; 		// AP_DEFAULT_CHANNEL(6)

    /* in, g_host_cfg.support_ht = PHY_SUPPORT_HT(1) */
    if (g_host_cfg.support_ht)
    {
        pConfig->b80211n = AP_DEFAULT_80211N;
        pConfig->bRequire_ht = AP_DEFAULT_REQUIRE_HT;


        pConfig->ht_capab =(LDPC_Coding_Capability<<0)
				            |(Supported_Channel_Width_Set<<1)
				            |(SM_Power_Save<<2)
				            |(HT_Greenfield<<4)
				            |(g_host_cfg.ap_rx_short_GI<<5)
				            |(Short_GI_for_40_MHz<<6)
				            |(Tx_STBC<<7)
				            |(Rx_STBC<<8)
				            |(HT_Delayed_Block_Ack<<10)
				            |(Maximum_AMSDU_Length<<11)
				            |(DSSS_CCK_Mode_in_40_MHz<<12)
				            |(Forty_MHz_Intolerant<<14)
				            |(L_SIG_TXOP_Protection_Support<<15);

		pConfig->mcs_set[0] = g_host_cfg.ap_rx_support_mcs_rate_msk;//0xff;

		pConfig->a_mpdu_params = (Maximum_AMPDU_Length_Exponent<<0) 
								|(Minimum_MPDU_Start_Spacing<<2);
    }
    else
    {
        pConfig->b80211n = FALSE;
    }

    ssv6xxx_config_init_rates(pConfig);
}




struct ap_wmm_ac_params {
	u32 aifsn:4;
	u32 acm:1;
	u32 aci:2;	
	u32 reserved:1;

	u32 ecwin:4;
	u32 ecwax:4;

	u32 txop_limit:16;
};

#if AP_DEFAULT_HW_MODE > AP_MODE_IEEE80211B

//g/n/a
const struct ap_wmm_ac_params ac_params[4]=
{
	//# Normal priority / AC_BE = best effort	
	{3, 0, 0, 0, 4, 10, 0},
	//# Low priority / AC_BK = background
	{7, 0, 1, 0, 4, 10, 0},
	//# High priority / AC_VI = video	
	{2, 0, 2, 0, 3, 4, 94},	
	//# Highest priority / AC_VO = voice	
	{2, 0, 3, 0, 2, 3, 47},
};

#else

//b
const struct ap_wmm_ac_params ac_params[4]=
{	
	//# Normal priority / AC_BE = best effort
	{3, 0, 0, 0, 5, 10, 0},	
	//# Low priority / AC_BK = background
	{7, 0, 1, 0, 5, 10, 0},
	//# High priority / AC_VI = video	
	{2, 0, 2, 0, 4, 5, 188},
	//# Highest priority / AC_VO = voice	
	{2, 0, 3, 0, 3, 4, 102},
};

#endif
#endif
