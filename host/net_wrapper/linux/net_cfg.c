/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <net/cfg80211.h>
#include <net/netlink.h>
#include "dev.h"


/**
 * struct brcmf_cfg80211_vif - virtual interface specific information.
 *
 * @ifp: lower layer interface pointer
 * @wdev: wireless device.
 * @profile: profile information.
 * @sme_state: SME state using enum brcmf_vif_status bits.
 * @list: linked list.
 * @mgmt_rx_reg: registered rx mgmt frame types.
 * @mbss: Multiple BSS type, set if not first AP (not relevant for P2P).
 */
struct ssv_cfg80211_vif {
	ssv_vif *ifp;
	struct wireless_dev wdev;
	unsigned long sme_state;
};

/**
 * struct ssv_cfg80211_info - private data of cfg80211 interface
 *
 * @wiphy: wiphy object for cfg80211 interface.
 * @ops: pointer to copy of ops as registered with wiphy object.
 * @scan_request: cfg80211 scan request object.
 * @bss_list: bss_list holding scanned ap information.
 * @bss_info: bss information for cfg80211 layer.
 * @scan_status: scan activity on the dongle.
 * @channel: current channel.
 */
struct ssv_cfg80211_info {
	struct wiphy *wiphy;
	struct cfg80211_ops *ops;
	struct cfg80211_scan_request *scan_request;
	struct wl_cfg80211_bss_info *bss_info;
	unsigned long scan_status;
	u32 channel;
};


static bool check_vif_up(struct ssv_cfg80211_vif *vif)
{
	return true;
}

#define RATE_TO_BASE100KBPS(rate)   (((rate) * 10) / 2)
#define RATETAB_ENT(_rateid, _flags) \
	{                                                               \
		.bitrate        = RATE_TO_BASE100KBPS(_rateid),     \
		.hw_value       = (_rateid),                            \
		.flags          = (_flags),                             \
	}

static struct ieee80211_rate __wl_rates[] = {
	RATETAB_ENT(BRCM_RATE_1M, 0),
	RATETAB_ENT(BRCM_RATE_2M, IEEE80211_RATE_SHORT_PREAMBLE),
	RATETAB_ENT(BRCM_RATE_5M5, IEEE80211_RATE_SHORT_PREAMBLE),
	RATETAB_ENT(BRCM_RATE_11M, IEEE80211_RATE_SHORT_PREAMBLE),
	RATETAB_ENT(BRCM_RATE_6M, 0),
	RATETAB_ENT(BRCM_RATE_9M, 0),
	RATETAB_ENT(BRCM_RATE_12M, 0),
	RATETAB_ENT(BRCM_RATE_18M, 0),
	RATETAB_ENT(BRCM_RATE_24M, 0),
	RATETAB_ENT(BRCM_RATE_36M, 0),
	RATETAB_ENT(BRCM_RATE_48M, 0),
	RATETAB_ENT(BRCM_RATE_54M, 0),
};

#define wl_g_rates		(__wl_rates + 0)
#define wl_g_rates_size		ARRAY_SIZE(__wl_rates)
#define wl_a_rates		(__wl_rates + 4)
#define wl_a_rates_size		(wl_g_rates_size - 4)

#define CHAN2G(_channel, _freq) {				\
	.band			= NL80211_BAND_2GHZ,		\
	.center_freq		= (_freq),			\
	.hw_value		= (_channel),			\
	.max_antenna_gain	= 0,				\
	.max_power		= 30,				\
}

#define CHAN5G(_channel) {					\
	.band			= NL80211_BAND_5GHZ,		\
	.center_freq		= 5000 + (5 * (_channel)),	\
	.hw_value		= (_channel),			\
	.max_antenna_gain	= 0,				\
	.max_power		= 30,				\
}

static struct ieee80211_channel __wl_2ghz_channels[] = {
	CHAN2G(1, 2412), CHAN2G(2, 2417), CHAN2G(3, 2422), CHAN2G(4, 2427),
	CHAN2G(5, 2432), CHAN2G(6, 2437), CHAN2G(7, 2442), CHAN2G(8, 2447),
	CHAN2G(9, 2452), CHAN2G(10, 2457), CHAN2G(11, 2462), CHAN2G(12, 2467),
	CHAN2G(13, 2472), CHAN2G(14, 2484)
};

static struct ieee80211_channel __wl_5ghz_channels[] = {
	CHAN5G(34), CHAN5G(36), CHAN5G(38), CHAN5G(40), CHAN5G(42),
	CHAN5G(44), CHAN5G(46), CHAN5G(48), CHAN5G(52), CHAN5G(56),
	CHAN5G(60), CHAN5G(64), CHAN5G(100), CHAN5G(104), CHAN5G(108),
	CHAN5G(112), CHAN5G(116), CHAN5G(120), CHAN5G(124), CHAN5G(128),
	CHAN5G(132), CHAN5G(136), CHAN5G(140), CHAN5G(144), CHAN5G(149),
	CHAN5G(153), CHAN5G(157), CHAN5G(161), CHAN5G(165)
};

/* Band templates duplicated per wiphy. The channel info
 * above is added to the band during setup.
 */
static const struct ieee80211_supported_band __wl_band_2ghz = {
	.band = NL80211_BAND_2GHZ,
	.bitrates = wl_g_rates,
	.n_bitrates = wl_g_rates_size,
};

static const struct ieee80211_supported_band __wl_band_5ghz = {
	.band = NL80211_BAND_5GHZ,
	.bitrates = wl_a_rates,
	.n_bitrates = wl_a_rates_size,
};

/* This is to override regulatory domains defined in cfg80211 module (reg.c)
 * By default world regulatory domain defined in reg.c puts the flags
 * NL80211_RRF_NO_IR for 5GHz channels (for * 36..48 and 149..165).
 * With respect to these flags, wpa_supplicant doesn't * start p2p
 * operations on 5GHz channels. All the changes in world regulatory
 * domain are to be done here.
 */
static const struct ieee80211_regdomain net_regdom = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		/* IEEE 802.11b/g, channels 1..11 */
		REG_RULE(2412-10, 2472+10, 40, 6, 20, 0),
		/* If any */
		/* IEEE 802.11 channel 14 - Only JP enables
		 * this and for 802.11b only
		 */
		REG_RULE(2484-10, 2484+10, 20, 6, 20, 0),
		/* IEEE 802.11a, channel 36..64 */
		REG_RULE(5150-10, 5350+10, 80, 6, 20, 0),
		/* IEEE 802.11a, channel 100..165 */
		REG_RULE(5470-10, 5850+10, 80, 6, 20, 0), }
};


static bool net_is_ibssmode(struct ssv_cfg80211_vif *vif)
{
	return vif->wdev.iftype == NL80211_IFTYPE_ADHOC;
}

static struct wireless_dev *net_cfg80211_add_iface(struct wiphy *wiphy,
						     const char *name,
						     unsigned char name_assign_type,
						     enum nl80211_iftype type,
						     u32 *flags,
						     struct vif_params *params)
{
	struct wireless_dev *wdev;
	int err;

	printk("enter: %s type %d\n", name, type);

	switch (type) {
	case NL80211_IFTYPE_ADHOC:
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP_VLAN:
	case NL80211_IFTYPE_WDS:
	case NL80211_IFTYPE_MONITOR:
	case NL80211_IFTYPE_MESH_POINT:
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
	case NL80211_IFTYPE_P2P_DEVICE:
	case NL80211_IFTYPE_UNSPECIFIED:
	default:
		return ERR_PTR(-EOPNOTSUPP);
	}

	return wdev;
}


static
int net_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	struct ssv_cfg80211_info *cfg = wiphy_priv(wiphy);
	struct net_device *ndev = wdev->netdev;
	
	switch (wdev->iftype) {
	case NL80211_IFTYPE_ADHOC:
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP_VLAN:
	case NL80211_IFTYPE_WDS:
	case NL80211_IFTYPE_MONITOR:
	case NL80211_IFTYPE_MESH_POINT:
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
	case NL80211_IFTYPE_P2P_DEVICE:
	case NL80211_IFTYPE_UNSPECIFIED:
	default:
		return -EINVAL;
	}
	return -EOPNOTSUPP;
}

static s32
net_cfg80211_change_iface(struct wiphy *wiphy, struct net_device *ndev,
			 enum nl80211_iftype type, u32 *flags,
			 struct vif_params *params)
{

	return 0;
}


static s32
net_cfg80211_scan(struct wiphy *wiphy, struct cfg80211_scan_request *request)
{
	struct ssv_cfg80211_vif *vif;
	s32 err = 0;

	return err;
}

static s32 net_set_rts(struct net_device *ndev, u32 rts_threshold)
{
	s32 err = 0;

	return err;
}

static s32 net_set_frag(struct net_device *ndev, u32 frag_threshold)
{
	s32 err = 0;

	return err;
}

static s32 net_set_retry(struct net_device *ndev, u32 retry, bool l)
{
	s32 err = 0;
	return err;
}

static s32 net_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	s32 err = 0;

	return err;
}

static s32
net_cfg80211_join_ibss(struct wiphy *wiphy, struct net_device *ndev,
		      struct cfg80211_ibss_params *params)
{
	s32 err = 0;
	return err;
}

static s32
net_cfg80211_leave_ibss(struct wiphy *wiphy, struct net_device *ndev)
{
	return 0;
}

static s32
net_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev,
		       struct cfg80211_connect_params *sme)
{
	s32 err = 0;
	return err;
}

static s32
net_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev,
		       u16 reason_code)
{
	s32 err = 0;

	return err;
}


static s32
brcmf_cfg80211_get_station_ibss(struct brcmf_if *ifp,
				struct station_info *sinfo)
{
	return 0;
}

static s32
net_cfg80211_get_station(struct wiphy *wiphy, struct net_device *ndev,
			   const u8 *mac, struct station_info *sinfo)
{
	s32 err = 0;

	return err;
}

static int
net_cfg80211_dump_station(struct wiphy *wiphy, struct net_device *ndev,
			    int idx, u8 *mac, struct station_info *sinfo)
{
	s32 err = 0;

	return err;
}


static int
net_cfg80211_del_station(struct wiphy *wiphy, struct net_device *ndev,
			   struct station_del_parameters *params)
{
	s32 err = 0;

	if (!params->mac)
		return -EFAULT;

	return err;
}

static int
net_cfg80211_change_station(struct wiphy *wiphy, struct net_device *ndev,
			      const u8 *mac, struct station_parameters *params)
{
	s32 err = 0;

	/* Ignore all 00 MAC */
	if (is_zero_ether_addr(mac))
		return 0;

	return err;
}


static struct cfg80211_ops net_cfg80211_ops = {
	.add_virtual_intf = net_cfg80211_add_iface,
	.del_virtual_intf = net_cfg80211_del_iface,
	.change_virtual_intf = net_cfg80211_change_iface,
	.scan = net_cfg80211_scan,
	.set_wiphy_params = net_cfg80211_set_wiphy_params,
	.join_ibss = net_cfg80211_join_ibss,
	.leave_ibss = net_cfg80211_leave_ibss,
	.get_station = net_cfg80211_get_station,
	.dump_station = net_cfg80211_dump_station,

	.connect = net_cfg80211_connect,
	.disconnect = net_cfg80211_disconnect,
	.del_station = net_cfg80211_del_station,
	.change_station = net_cfg80211_change_station,
};

struct ssv_cfg80211_vif *net_alloc_vif(struct ssv_cfg80211_info *cfg,
					   enum nl80211_iftype type)
{
	struct ssv_cfg80211_vif *vif_walk;
	struct ssv_cfg80211_vif *vif;
	bool mbss;

	printk("allocating virtual interface (size=%zu)\n",
		  sizeof(*vif));
	vif = kzalloc(sizeof(*vif), GFP_KERNEL);
	if (!vif)
		return ERR_PTR(-ENOMEM);

	vif->wdev.wiphy = cfg->wiphy;
	vif->wdev.iftype = type;

	if (type == NL80211_IFTYPE_AP) {
		mbss = false;
		list_for_each_entry(vif_walk, &cfg->vif_list, list) {
			if (vif_walk->wdev.iftype == NL80211_IFTYPE_AP) {
				mbss = true;
				break;
			}
		}
		vif->mbss = mbss;
	}

	return vif;
}


static s32 wl_init_priv(struct ssv_cfg80211_info *cfg)
{
	s32 err = 0;

	cfg->scan_request = NULL;
	return err;
}

static void wl_deinit_priv(struct ssv_cfg80211_info *cfg)
{
}



static void net_update_bw40_channel_flag(struct ieee80211_channel *channel,
					   struct brcmu_chan *ch)
{
	u32 ht40_flag;

	ht40_flag = channel->flags & IEEE80211_CHAN_NO_HT40;
	if (ch->sb == BRCMU_CHAN_SB_U) {
		if (ht40_flag == IEEE80211_CHAN_NO_HT40)
			channel->flags &= ~IEEE80211_CHAN_NO_HT40;
		channel->flags |= IEEE80211_CHAN_NO_HT40PLUS;
	} else {
		/* It should be one of
		 * IEEE80211_CHAN_NO_HT40 or
		 * IEEE80211_CHAN_NO_HT40PLUS
		 */
		channel->flags &= ~IEEE80211_CHAN_NO_HT40;
		if (ht40_flag == IEEE80211_CHAN_NO_HT40)
			channel->flags |= IEEE80211_CHAN_NO_HT40MINUS;
	}
}

static int net_construct_chaninfo(struct brcmf_cfg80211_info *cfg,
				    u32 bw_cap[])
{
	struct brcmf_if *ifp = netdev_priv(cfg_to_ndev(cfg));
	struct ieee80211_supported_band *band;
	struct ieee80211_channel *channel;
	struct wiphy *wiphy;
	struct brcmf_chanspec_list *list;
	struct brcmu_chan ch;
	int err;
	u8 *pbuf;
	u32 i, j;
	u32 total;
	u32 chaninfo;

	pbuf = kzalloc(BRCMF_DCMD_MEDLEN, GFP_KERNEL);

	if (pbuf == NULL)
		return -ENOMEM;

	list = (struct brcmf_chanspec_list *)pbuf;

	err = brcmf_fil_iovar_data_get(ifp, "chanspecs", pbuf,
				       BRCMF_DCMD_MEDLEN);
	if (err) {
		brcmf_err("get chanspecs error (%d)\n", err);
		goto fail_pbuf;
	}

	wiphy = cfg_to_wiphy(cfg);
	band = wiphy->bands[NL80211_BAND_2GHZ];
	if (band)
		for (i = 0; i < band->n_channels; i++)
			band->channels[i].flags = IEEE80211_CHAN_DISABLED;
	band = wiphy->bands[NL80211_BAND_5GHZ];
	if (band)
		for (i = 0; i < band->n_channels; i++)
			band->channels[i].flags = IEEE80211_CHAN_DISABLED;

	total = le32_to_cpu(list->count);
	for (i = 0; i < total; i++) {
		ch.chspec = (u16)le32_to_cpu(list->element[i]);
		cfg->d11inf.decchspec(&ch);

		if (ch.band == BRCMU_CHAN_BAND_2G) {
			band = wiphy->bands[NL80211_BAND_2GHZ];
		} else if (ch.band == BRCMU_CHAN_BAND_5G) {
			band = wiphy->bands[NL80211_BAND_5GHZ];
		} else {
			brcmf_err("Invalid channel Spec. 0x%x.\n", ch.chspec);
			continue;
		}
		if (!band)
			continue;
		if (!(bw_cap[band->band] & WLC_BW_40MHZ_BIT) &&
		    ch.bw == BRCMU_CHAN_BW_40)
			continue;
		if (!(bw_cap[band->band] & WLC_BW_80MHZ_BIT) &&
		    ch.bw == BRCMU_CHAN_BW_80)
			continue;

		channel = NULL;
		for (j = 0; j < band->n_channels; j++) {
			if (band->channels[j].hw_value == ch.control_ch_num) {
				channel = &band->channels[j];
				break;
			}
		}
		if (!channel) {
			/* It seems firmware supports some channel we never
			 * considered. Something new in IEEE standard?
			 */
			brcmf_err("Ignoring unexpected firmware channel %d\n",
				  ch.control_ch_num);
			continue;
		}

		/* assuming the chanspecs order is HT20,
		 * HT40 upper, HT40 lower, and VHT80.
		 */
		if (ch.bw == BRCMU_CHAN_BW_80) {
			channel->flags &= ~IEEE80211_CHAN_NO_80MHZ;
		} else if (ch.bw == BRCMU_CHAN_BW_40) {
			brcmf_update_bw40_channel_flag(channel, &ch);
		} else {
			/* enable the channel and disable other bandwidths
			 * for now as mentioned order assure they are enabled
			 * for subsequent chanspecs.
			 */
			channel->flags = IEEE80211_CHAN_NO_HT40 |
					 IEEE80211_CHAN_NO_80MHZ;
			ch.bw = BRCMU_CHAN_BW_20;
			cfg->d11inf.encchspec(&ch);
			chaninfo = ch.chspec;
			err = brcmf_fil_bsscfg_int_get(ifp, "per_chan_info",
						       &chaninfo);
			if (!err) {
				if (chaninfo & WL_CHAN_RADAR)
					channel->flags |=
						(IEEE80211_CHAN_RADAR |
						 IEEE80211_CHAN_NO_IR);
				if (chaninfo & WL_CHAN_PASSIVE)
					channel->flags |=
						IEEE80211_CHAN_NO_IR;
			}
		}
	}

fail_pbuf:
	kfree(pbuf);
	return err;
}

static int net_enable_bw40_2g(struct brcmf_cfg80211_info *cfg)
{
	struct brcmf_if *ifp = netdev_priv(cfg_to_ndev(cfg));
	struct ieee80211_supported_band *band;
	struct brcmf_fil_bwcap_le band_bwcap;
	struct brcmf_chanspec_list *list;
	u8 *pbuf;
	u32 val;
	int err;
	struct brcmu_chan ch;
	u32 num_chan;
	int i, j;

	/* verify support for bw_cap command */
	val = WLC_BAND_5G;
	err = brcmf_fil_iovar_int_get(ifp, "bw_cap", &val);

	if (!err) {
		/* only set 2G bandwidth using bw_cap command */
		band_bwcap.band = cpu_to_le32(WLC_BAND_2G);
		band_bwcap.bw_cap = cpu_to_le32(WLC_BW_CAP_40MHZ);
		err = brcmf_fil_iovar_data_set(ifp, "bw_cap", &band_bwcap,
					       sizeof(band_bwcap));
	} else {
		brcmf_dbg(INFO, "fallback to mimo_bw_cap\n");
		val = WLC_N_BW_40ALL;
		err = brcmf_fil_iovar_int_set(ifp, "mimo_bw_cap", val);
	}

	if (!err) {
		/* update channel info in 2G band */
		pbuf = kzalloc(BRCMF_DCMD_MEDLEN, GFP_KERNEL);

		if (pbuf == NULL)
			return -ENOMEM;

		ch.band = BRCMU_CHAN_BAND_2G;
		ch.bw = BRCMU_CHAN_BW_40;
		ch.sb = BRCMU_CHAN_SB_NONE;
		ch.chnum = 0;
		cfg->d11inf.encchspec(&ch);

		/* pass encoded chanspec in query */
		*(__le16 *)pbuf = cpu_to_le16(ch.chspec);

		err = brcmf_fil_iovar_data_get(ifp, "chanspecs", pbuf,
					       BRCMF_DCMD_MEDLEN);
		if (err) {
			brcmf_err("get chanspecs error (%d)\n", err);
			kfree(pbuf);
			return err;
		}

		band = cfg_to_wiphy(cfg)->bands[NL80211_BAND_2GHZ];
		list = (struct brcmf_chanspec_list *)pbuf;
		num_chan = le32_to_cpu(list->count);
		for (i = 0; i < num_chan; i++) {
			ch.chspec = (u16)le32_to_cpu(list->element[i]);
			cfg->d11inf.decchspec(&ch);
			if (WARN_ON(ch.band != BRCMU_CHAN_BAND_2G))
				continue;
			if (WARN_ON(ch.bw != BRCMU_CHAN_BW_40))
				continue;
			for (j = 0; j < band->n_channels; j++) {
				if (band->channels[j].hw_value == ch.control_ch_num)
					break;
			}
			if (WARN_ON(j == band->n_channels))
				continue;

			brcmf_update_bw40_channel_flag(&band->channels[j], &ch);
		}
		kfree(pbuf);
	}
	return err;
}

static void net_get_bwcap(struct brcmf_if *ifp, u32 bw_cap[])
{
	u32 band, mimo_bwcap;
	int err;

	band = WLC_BAND_2G;
	err = brcmf_fil_iovar_int_get(ifp, "bw_cap", &band);
	if (!err) {
		bw_cap[NL80211_BAND_2GHZ] = band;
		band = WLC_BAND_5G;
		err = brcmf_fil_iovar_int_get(ifp, "bw_cap", &band);
		if (!err) {
			bw_cap[NL80211_BAND_5GHZ] = band;
			return;
		}
		WARN_ON(1);
		return;
	}
	brcmf_dbg(INFO, "fallback to mimo_bw_cap info\n");
	mimo_bwcap = 0;
	err = brcmf_fil_iovar_int_get(ifp, "mimo_bw_cap", &mimo_bwcap);
	if (err)
		/* assume 20MHz if firmware does not give a clue */
		mimo_bwcap = WLC_N_BW_20ALL;

	switch (mimo_bwcap) {
	case WLC_N_BW_40ALL:
		bw_cap[NL80211_BAND_2GHZ] |= WLC_BW_40MHZ_BIT;
		/* fall-thru */
	case WLC_N_BW_20IN2G_40IN5G:
		bw_cap[NL80211_BAND_5GHZ] |= WLC_BW_40MHZ_BIT;
		/* fall-thru */
	case WLC_N_BW_20ALL:
		bw_cap[NL80211_BAND_2GHZ] |= WLC_BW_20MHZ_BIT;
		bw_cap[NL80211_BAND_5GHZ] |= WLC_BW_20MHZ_BIT;
		break;
	default:
		brcmf_err("invalid mimo_bw_cap value\n");
	}
}

static void net_update_ht_cap(struct ieee80211_supported_band *band,
				u32 bw_cap[2], u32 nchain)
{
	band->ht_cap.ht_supported = true;
	if (bw_cap[band->band] & WLC_BW_40MHZ_BIT) {
		band->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
		band->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
	}
	band->ht_cap.cap |= IEEE80211_HT_CAP_SGI_20;
	band->ht_cap.cap |= IEEE80211_HT_CAP_DSSSCCK40;
	band->ht_cap.ampdu_factor = IEEE80211_HT_MAX_AMPDU_64K;
	band->ht_cap.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16;
	memset(band->ht_cap.mcs.rx_mask, 0xff, nchain);
	band->ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;
}

static __le16 net_get_mcs_map(u32 nchain, enum ieee80211_vht_mcs_support supp)
{
	u16 mcs_map;
	int i;

	for (i = 0, mcs_map = 0xFFFF; i < nchain; i++)
		mcs_map = (mcs_map << 2) | supp;

	return cpu_to_le16(mcs_map);
}

static void net_update_vht_cap(struct ieee80211_supported_band *band,
				 u32 bw_cap[2], u32 nchain, u32 txstreams,
				 u32 txbf_bfe_cap, u32 txbf_bfr_cap)
{

}

static int net_setup_wiphybands(struct wiphy *wiphy)
{

	return 0;
}

static const struct ieee80211_txrx_stypes
net_txrx_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
};

/**
 * brcmf_setup_ifmodes() - determine interface modes and combinations.
 *
 * @wiphy: wiphy object.
 * @ifp: interface object needed for feat module api.
 *
 * The interface modes and combinations are determined dynamically here
 * based on firmware functionality.
 *
 * no p2p and no mbss:
 *
 *	#STA <= 1, #AP <= 1, channels = 1, 2 total
 *
 * no p2p and mbss:
 *
 *	#STA <= 1, #AP <= 1, channels = 1, 2 total
 *	#AP <= 4, matching BI, channels = 1, 4 total
 *
 * p2p, no mchan, and mbss:
 *
 *	#STA <= 1, #P2P-DEV <= 1, #{P2P-CL, P2P-GO} <= 1, channels = 1, 3 total
 *	#STA <= 1, #P2P-DEV <= 1, #AP <= 1, #P2P-CL <= 1, channels = 1, 4 total
 *	#AP <= 4, matching BI, channels = 1, 4 total
 *
 * p2p, mchan, and mbss:
 *
 *	#STA <= 1, #P2P-DEV <= 1, #{P2P-CL, P2P-GO} <= 1, channels = 2, 3 total
 *	#STA <= 1, #P2P-DEV <= 1, #AP <= 1, #P2P-CL <= 1, channels = 1, 4 total
 *	#AP <= 4, matching BI, channels = 1, 4 total
 */
static int net_setup_ifmodes(struct wiphy *wiphy, struct brcmf_if *ifp)
{
	return 0;
}


static int net_setup_wiphy(struct wiphy *wiphy, struct brcmf_if *ifp)
{

	return 0;
}


static void net_cfg80211_reg_notifier(struct wiphy *wiphy,
					struct regulatory_request *req)
{
	struct ssv_cfg80211_info *cfg = wiphy_priv(wiphy);
	int i;

	/* ignore non-ISO3166 country codes */
	for (i = 0; i < sizeof(req->alpha2); i++)
		if (req->alpha2[i] < 'A' || req->alpha2[i] > 'Z') {
			printk("not a ISO3166 code (0x%02x 0x%02x)\n",
				  req->alpha2[0], req->alpha2[1]);
			return;
		}

	printk("Enter: initiator=%d, alpha=%c%c\n", 
		req->initiator, req->alpha2[0], req->alpha2[1]);


	net_setup_wiphybands(wiphy);
}

static void net_free_wiphy(struct wiphy *wiphy)
{
	int i;

	if (!wiphy)
		return;

	if (wiphy->iface_combinations) {
		for (i = 0; i < wiphy->n_iface_combinations; i++)
			kfree(wiphy->iface_combinations[i].limits);
	}
	kfree(wiphy->iface_combinations);
	if (wiphy->bands[NL80211_BAND_2GHZ]) {
		kfree(wiphy->bands[NL80211_BAND_2GHZ]->channels);
		kfree(wiphy->bands[NL80211_BAND_2GHZ]);
	}
	if (wiphy->bands[NL80211_BAND_5GHZ]) {
		kfree(wiphy->bands[NL80211_BAND_5GHZ]->channels);
		kfree(wiphy->bands[NL80211_BAND_5GHZ]);
	}
	wiphy_free(wiphy);
}

struct ssv_cfg80211_info *net_cfg80211_attach(NET_DEV *p_ndev)
{
	struct ssv_cfg80211_info *cfg;
	struct wiphy *wiphy;
	struct cfg80211_ops *ops;
	
	ssv_vif *curr_if; = &((struct DeviceInfo *)(netdev_priv(p_ndev))->vif[0]);
	s32 err = 0;
	u16 *cap = NULL;

	if (!p_ndev) {
		printk("ndev is invalid\n");
		return NULL;
	}

	ops = kmemdup(&net_cfg80211_ops, sizeof(*ops), GFP_KERNEL);
	if (!ops)
		return NULL;

	curr_if = &((struct DeviceInfo *)(netdev_priv(p_ndev))->vif[0]);

	wiphy = wiphy_new(ops, sizeof(struct ssv_cfg80211_info));
	if (!wiphy) {
		printk("Could not allocate wiphy device\n");
		goto ops_out;
	}

	OS_MemCPY(wiphy->perm_addr, curr_if->self_mac, ETH_ALEN);

	set_wiphy_dev(wiphy, busdev);

	cfg = wiphy_priv(wiphy);
	cfg->wiphy = wiphy;
	cfg->ops = ops;	

net_creat_if
	vif = net_alloc_vif(cfg, NL80211_IFTYPE_STATION);
	if (IS_ERR(vif))
		goto wiphy_out;

	vif->ifp = ifp;
	vif->wdev.netdev = p_ndev;
	p_ndev->ieee80211_ptr = &vif->wdev;
	SET_NETDEV_DEV(p_ndev, wiphy_dev(cfg->wiphy));

	err = wl_init_priv(cfg);
	if (err) {
		printk("Failed to init iwm_priv (%d)\n", err);
		brcmf_free_vif(vif);
		goto wiphy_out;
	}
	ifp->vif = vif;

	/* determine d11 io type before wiphy setup */
	err = brcmf_fil_cmd_int_get(ifp, BRCMF_C_GET_VERSION, &io_type);
	if (err) {
		printk("Failed to get D11 version (%d)\n", err);
		goto priv_out;
	}
	cfg->d11inf.io_type = (u8)io_type;

	err = brcmf_setup_wiphy(wiphy, ifp);
	if (err < 0)
		goto priv_out;

	printk("Registering custom regulatory\n");
	wiphy->reg_notifier = brcmf_cfg80211_reg_notifier;
	wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
	wiphy_apply_custom_regulatory(wiphy, &brcmf_regdom);

	/* firmware defaults to 40MHz disabled in 2G band. We signal
	 * cfg80211 here that we do and have it decide we can enable
	 * it. But first check if device does support 2G operation.
	 */
	if (wiphy->bands[NL80211_BAND_2GHZ]) {
		cap = &wiphy->bands[NL80211_BAND_2GHZ]->ht_cap.cap;
		*cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
	}
	err = wiphy_register(wiphy);
	if (err < 0) {
		printk("Could not register wiphy device (%d)\n", err);
		goto priv_out;
	}

	err = brcmf_setup_wiphybands(wiphy);
	if (err) {
		printk("Setting wiphy bands failed (%d)\n", err);
		goto wiphy_unreg_out;
	}


	return cfg;

wiphy_unreg_out:
	wiphy_unregister(cfg->wiphy);
priv_out:
	wl_deinit_priv(cfg);
	net_free_vif(vif);
	ifp->vif = NULL;
wiphy_out:
	net_free_wiphy(wiphy);
ops_out:
	kfree(ops);
	return NULL;
}

void net_cfg80211_detach(struct ssv_cfg80211_info *cfg)
{
	if (!cfg)
		return;

	wiphy_unregister(cfg->wiphy);
	kfree(cfg->ops);
	wl_deinit_priv(cfg);
	net_free_wiphy(cfg->wiphy);
}

