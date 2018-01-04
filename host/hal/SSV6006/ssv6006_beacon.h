#ifndef _SSV6006_BEACON_H_
#define _SSV6006_BEACON_H_

struct ssv6xxx_beacon_info {
	u32 pubf_addr;
	u16 len;
	u8 tim_offset;
	u8 tim_cnt;
};

int ssv6006_hal_soc_set_bcn(enum ssv6xxx_tx_extra_type extra_type, void *frame, struct cfg_bcn_info *bcn_info, u8 dtim_cnt, u16 bcn_itv);
int ssv6006_hal_beacon_enable(bool bEnable);
bool ssv6006_hal_is_beacon_enable(void);
int ssv6006_hal_beacon_get_len(void *frame);
int ssv6006_hal_beacon_set(void* beacon_skb, int dtim_offset);

#endif /* _SSV6006_BEACON_H_ */

