/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include "os.h"
#include <net_wrapper.h>

#include <linux/if_arp.h>
#include <linux/netdevice.h>

#include <linux/ieee80211.h>
#include <linux/nl80211.h>
#include <net/cfg80211.h>

#include <linux/rtnetlink.h>
#include "dev.h"

#define NUM_NL80211_BANDS	3

struct ieee80211_bss {
	u32 device_ts_beacon, device_ts_presp;

	bool wmm_used;
	bool uapsd_supported;

#define IEEE80211_MAX_SUPP_RATES 32
	u8 supp_rates[IEEE80211_MAX_SUPP_RATES];
	size_t supp_rates_len;
	struct ieee80211_rate *beacon_rate;

	/*
	 * During association, we save an ERP value from a probe response so
	 * that we can feed ERP info to the driver when handling the
	 * association completes. these fields probably won't be up-to-date
	 * otherwise, you probably don't want to use them.
	 */
	bool has_erp_value;
	u8 erp_value;

	/* Keep track of the corruption of the last beacon/probe response. */
	u8 corrupt_data;

	/* Keep track of what bits of information we have valid info for. */
	u8 valid_data;
};

/* privid for wiphys to determine whether they belong to us or not */
const void *const net_80211_wiphy_privid = &net_80211_wiphy_privid;

int net_80211_if_add(struct wiphy *wiphy, const char *name,
		     unsigned char name_assign_type,
		     struct wireless_dev **new_wdev, enum nl80211_iftype type,
		     struct vif_params *params)
{
	printk("%s()\n",__FUNCTION__);
	return 0;
}

static struct wireless_dev *net_80211_add_iface(struct wiphy *wiphy,
						const char *name,
						enum nl80211_iftype type,
						u32 *flags,
						struct vif_params *params)
{
	return NULL;
}

static int net_80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	return 0;
}


static int net_80211_change_iface(struct wiphy *wiphy,
				  struct net_device *dev,
				  enum nl80211_iftype type, u32 *flags,
				  struct vif_params *params)
{
	return 0;
}


static int net_80211_scan(struct wiphy *wiphy,
			  struct cfg80211_scan_request *req)
{
	return 0;
}

//static void net_80211_abort_scan(struct wiphy *wiphy, struct wireless_dev *wdev)
//{
//}

static int
net_80211_sched_scan_start(struct wiphy *wiphy,
			   struct net_device *dev,
			   struct cfg80211_sched_scan_request *req)
{
	return 0;
}

static int
net_80211_sched_scan_stop(struct wiphy *wiphy, struct net_device *dev)
{
	return 0;
}

static int net_80211_auth(struct wiphy *wiphy, struct net_device *dev,
			  struct cfg80211_auth_request *req)
{
	return 0;
}

static int net_80211_assoc(struct wiphy *wiphy, struct net_device *dev,
			   struct cfg80211_assoc_request *req)
{
	return 0;
}

static int net_80211_deauth(struct wiphy *wiphy, struct net_device *dev,
			    struct cfg80211_deauth_request *req)
{
	return 0;
}

static int net_80211_disassoc(struct wiphy *wiphy, struct net_device *dev,
			      struct cfg80211_disassoc_request *req)
{
	return 0;
}

static int net_80211_join_ibss(struct wiphy *wiphy, struct net_device *dev,
			       struct cfg80211_ibss_params *params)
{
	return 0;
}

static int net_80211_leave_ibss(struct wiphy *wiphy, struct net_device *dev)
{
	return 0;
}

static int net_80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	return 0;
}


static int net_80211_cfg_get_channel(struct wiphy *wiphy,
				     struct wireless_dev *wdev,
				     struct cfg80211_chan_def *chandef)
{
	return 0;
}


int net_80211_channel_switch(struct wiphy *wiphy, struct net_device *dev,
			     struct cfg80211_csa_settings *params)
{
	return 0;
}


static struct cfg80211_ops net_80211_config_ops = {	

	.add_virtual_intf = net_80211_add_iface,
	.del_virtual_intf = net_80211_del_iface,
	.change_virtual_intf = net_80211_change_iface,
	.scan = net_80211_scan,
//	.abort_scan = net_80211_abort_scan,
	.sched_scan_start = net_80211_sched_scan_start,
	.sched_scan_stop = net_80211_sched_scan_stop,
	.auth = net_80211_auth,
	.assoc = net_80211_assoc,
	.deauth = net_80211_deauth,
	.disassoc = net_80211_disassoc,
	.join_ibss = net_80211_join_ibss,
	.leave_ibss = net_80211_leave_ibss,
	.set_wiphy_params = net_80211_set_wiphy_params,
	.get_channel = net_80211_cfg_get_channel,
	.channel_switch = net_80211_channel_switch,

};

static const struct ieee80211_txrx_stypes net_mgmt_stypes[NUM_NL80211_IFTYPES] = {

	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
};

static const struct ieee80211_ht_cap net_80211_ht_capa_mod_mask = {
	.ampdu_params_info = IEEE80211_HT_AMPDU_PARM_FACTOR |
			     IEEE80211_HT_AMPDU_PARM_DENSITY,

	.cap_info = cpu_to_le16(IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				IEEE80211_HT_CAP_MAX_AMSDU |
				IEEE80211_HT_CAP_SGI_20 |
				IEEE80211_HT_CAP_SGI_40 |
				IEEE80211_HT_CAP_LDPC_CODING |
				IEEE80211_HT_CAP_40MHZ_INTOLERANT),
	.mcs = {
		.rx_mask = { 0xff, 0xff, 0xff, 0xff, 0xff,
			     0xff, 0xff, 0xff, 0xff, 0xff, },
	},
};

static const struct ieee80211_vht_cap net_80211_vht_capa_mod_mask = {
	.vht_cap_info =
		cpu_to_le32(IEEE80211_VHT_CAP_RXLDPC |
			    IEEE80211_VHT_CAP_SHORT_GI_80 |
			    IEEE80211_VHT_CAP_SHORT_GI_160 |
			    IEEE80211_VHT_CAP_RXSTBC_1 |
			    IEEE80211_VHT_CAP_RXSTBC_2 |
			    IEEE80211_VHT_CAP_RXSTBC_3 |
			    IEEE80211_VHT_CAP_RXSTBC_4 |
			    IEEE80211_VHT_CAP_TXSTBC |
			    IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE |
			    IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE |
			    IEEE80211_VHT_CAP_TX_ANTENNA_PATTERN |
			    IEEE80211_VHT_CAP_RX_ANTENNA_PATTERN |
			    IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK),
	.supp_mcs = {
		.rx_mcs_map = cpu_to_le16(~0),
		.tx_mcs_map = cpu_to_le16(~0),
	},
};

struct wiphy *net_cfg80211_init(size_t priv_data_len)
{
	int priv_size;
	struct wiphy *wiphy;
	struct dev_info_wrapper *p_dev_info_wrap;

	/* Ensure 32-byte alignment of our private data and hw private data.
	 * We use the wiphy priv data for both our ieee80211_local and for
	 * the driver's private data
	 *
	 * In memory it'll be like this:
	 *
	 * +----------------------------+
	 * | struct wiphy	    	 	|
	 * +----------------------------+
	 * | struct dev_info_wrapper 	|
	 * +----------------------------+
	 *
	 */
	priv_size = ALIGN(priv_data_len, NETDEV_ALIGN);
	wiphy = wiphy_new(&net_80211_config_ops, priv_size);
	if (!wiphy)
		return NULL;

	wiphy->mgmt_stypes = net_mgmt_stypes;

	wiphy->privid = net_80211_wiphy_privid;

	wiphy->flags |= WIPHY_FLAG_NETNS_OK 		|
					WIPHY_FLAG_4ADDR_AP 		|
					WIPHY_FLAG_4ADDR_STATION 	|
					WIPHY_FLAG_REPORTS_OBSS 	|
					WIPHY_FLAG_OFFCHAN_TX;

	wiphy->features |= 	NL80211_FEATURE_SK_TX_STATUS 			|
						NL80211_FEATURE_SAE 					|
						NL80211_FEATURE_HT_IBSS 				|
						NL80211_FEATURE_VIF_TXPOWER 			|
						NL80211_FEATURE_USERSPACE_MPM 			|
						NL80211_FEATURE_FULL_AP_CLIENT_STATE 	|
						NL80211_FEATURE_LOW_PRIORITY_SCAN 		|
						NL80211_FEATURE_AP_SCAN;
		
	wiphy->bss_priv_size = sizeof(struct ieee80211_bss);

	/* TODO(aaron): need to store cfg_priv data, we need record wiphy object */
	p_dev_info_wrap = (struct dev_info_wrapper *)wiphy_priv(wiphy);
	p_dev_info_wrap->g_dev_info = gDeviceInfo;

	//cfg_priv.wiphy = wiphy;

	wiphy->ht_capa_mod_mask = &net_80211_ht_capa_mod_mask;
	wiphy->vht_capa_mod_mask = &net_80211_vht_capa_mod_mask;

	/*	TODO(aaron): should we need fill this ? */
	wiphy->extended_capabilities = NULL;
	wiphy->extended_capabilities_mask = NULL;
	wiphy->extended_capabilities_len = 0;

	return wiphy;
}
//EXPORT_SYMBOL(ieee80211_alloc_hw_nm);

int net_cfg80211_register(struct wiphy *wiphy)
{
	int result, i;
	enum nl80211_band band;
	int channels, max_bitrates;
	bool supp_ht, supp_vht;
	netdev_features_t feature_whitelist;
	struct cfg80211_chan_def dflt_chandef = {};


	/* DFS is not supported with multi-channel combinations yet */
	for (i = 0; i < wiphy->n_iface_combinations; i++) {
		const struct ieee80211_iface_combination *comb;

		comb = &wiphy->iface_combinations[i];

		if (comb->radar_detect_widths &&
		    comb->num_different_channels > 1)
			return -EINVAL;
	}

	/* Only HW csum features are currently compatible with mac80211 */
	feature_whitelist = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
			    NETIF_F_HW_CSUM | NETIF_F_SG | NETIF_F_HIGHDMA |
			    NETIF_F_GSO_SOFTWARE | NETIF_F_RXCSUM;
	/*
	 * generic code guarantees at least one band,
	 * set this very early because much code assumes
	 * that hw.conf.channel is assigned
	 */
	channels = 0;
	max_bitrates = 0;
	supp_ht = false;
	supp_vht = false;
	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		struct ieee80211_supported_band *sband;

		sband = wiphy->bands[band];
		if (!sband)
			continue;

		if (!dflt_chandef.chan) {
			cfg80211_chandef_create(&dflt_chandef, 
				&sband->channels[0], NL80211_CHAN_NO_HT);
		}

		channels += sband->n_channels;

		if (max_bitrates < sband->n_bitrates)
			max_bitrates = sband->n_bitrates;
		supp_ht = supp_ht || sband->ht_cap.ht_supported;
		supp_vht = supp_vht || sband->vht_cap.vht_supported;

		if (!sband->ht_cap.ht_supported)
			continue;

		/* no need to mask, SM_PS_DISABLED has all bits set */
		sband->ht_cap.cap |= WLAN_HT_CAP_SM_PS_DISABLED <<
			             IEEE80211_HT_CAP_SM_PS_SHIFT;
	}

	/* if low-level driver supports AP, we also support VLAN */
	if (wiphy->interface_modes & BIT(NL80211_IFTYPE_AP)) {
		wiphy->interface_modes |= BIT(NL80211_IFTYPE_AP_VLAN);
		wiphy->software_iftypes |= BIT(NL80211_IFTYPE_AP_VLAN);
	}

	/* mac80211 always supports monitor */
	wiphy->interface_modes |= BIT(NL80211_IFTYPE_MONITOR);
	wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);

	/* mac80211 doesn't support more than one IBSS interface right now */
	for (i = 0; i < wiphy->n_iface_combinations; i++) {
		const struct ieee80211_iface_combination *c;
		int j;

		c = &wiphy->iface_combinations[i];

		for (j = 0; j < c->n_limits; j++)
			if ((c->limits[j].types & BIT(NL80211_IFTYPE_ADHOC)) &&
			    c->limits[j].max > 1)
				return -EINVAL;
	}

	/* not support mesh */
	wiphy->interface_modes &= ~BIT(NL80211_IFTYPE_MESH_POINT);

	/* mac80211 supports control port protocol changing */
	wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;

	/* For hw_scan, driver needs to set these up. */
	wiphy->max_scan_ssids = 4;
	wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;

	/* TODO(aaron): need to know the len of scan_ies_len in mac80211, temply set 0 */
	if (wiphy->max_scan_ie_len)
		wiphy->max_scan_ie_len -= 0;

	/* TODO(aaron): should we need cipher ? temp mark it */
#if 0
	result = ieee80211_init_cipher_suites(local);
	if (result < 0)
		goto fail_wiphy_register;
#endif

	wiphy->max_remain_on_channel_duration = 5000;

	/* mac80211 based drivers don't support internal TDLS setup */
	if (wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS)
		wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;


	result = wiphy_register(wiphy);
	if (result < 0)
		return result;

	rtnl_lock();
	/* add one default STA interface if supported */
	if (wiphy->interface_modes & BIT(NL80211_IFTYPE_STATION)) {

//		result = ieee80211_if_add(local, "wlan%d", NET_NAME_ENUM, NULL,
//					  NL80211_IFTYPE_STATION, NULL);
		if (result)
			wiphy_warn(wiphy, "Failed to add default virtual iface\n");
	}
	rtnl_unlock();

	return 0;
}

void net_cfg80211_unregister(struct wiphy *wiphy)
{
	wiphy_free(wiphy);
}
