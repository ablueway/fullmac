#ifdef __linux__
#include <linux/kernel.h>
#endif

#define SSV_HAL_C

#include <config.h>
#include <log.h>
#include <host_config.h>
#include <ssv_types.h>
#include "ssv_hal_if.h"
#include <ssv_hal.h>



const chip_def_S chip_list[] = {
      {"SSV6051Q",SSV6051Q},
      {"SSV6051Z",SSV6051Z},
      {"SSV6030P",SSV6030P},
      {"SSV6052Q",SSV6052Q},
      {"SSV6006B",SSV6006B},
      {"SSV6006C",SSV6006C},
    };
/* */
chip_def_S *chip_sel;

#define MAX_SSV6XXX_HAL 1
static s16 s_hal_cnt;
static struct ssv_hal_ops *s_hal_array[MAX_SSV6XXX_HAL];
static struct ssv_hal_ops *s_hal_cur;


static bool _ssv_hal_register(const struct ssv_hal_ops *ssv_hal)
{
    u16 i;

    if (s_hal_cnt == MAX_SSV6XXX_HAL)
    {
        LOG_PRINTF("%s s_hal_cnt = MAX_SSV6XXX_DRV!\r\n",__FUNCTION__);
        return false;
    }

    //LOG_PRINTF("%s() <= : 0x%08x, %s\r\n", __FUNCTION__, ssv_hal, ssv_hal->name);

    // find empty slot in array
    for (i = 0; i < MAX_SSV6XXX_HAL; i++)
    {
        if (s_hal_array[i] == NULL)
        {
            s_hal_array[i] = (struct ssv_hal_ops *)ssv_hal;
            s_hal_cnt++;
            //LOG_PRINTF("%s() => : ok! s_hal_cnt = %d, i = %d, ssv_hal = (0x%08x, %s)\r\n", __FUNCTION__, s_hal_cnt, i, ssv_hal, ssv_hal->name);
            return TRUE;
        }
    }

    /* never reach here! */
    LOG_PRINTF("%s should never reach here!\r\n",__FUNCTION__);
    return FALSE;
}
#if 0
static bool _ssv_hal_unregister(u16 i)
{
    if (s_hal_cnt == 0)
    {
        LOG_PRINTF("%s() : s_hal_cnt = 0, return true!\r\n", __FUNCTION__);
        return TRUE;
    }
    //LOG_PRINTF("%s() <= : i = %d, 0x%08x, %s\r\n", __FUNCTION__, i, s_hal_array[i], s_hal_array[i]->name);

    // find matching slot in array
    s_hal_array[i] = 0;
    s_hal_cnt--;
    //LOG_PRINTF("%s() => : s_drv_cnt = %d\r\n", __FUNCTION__, s_hal_cnt);
    return TRUE;
}
#endif

extern LIB_APIs s32 ssv6xxx_strcmp( const char *s0, const char *s1 );

static bool _ssv_hal_select(const char name[32])
{
    u16 i;

    struct ssv_hal_ops *hal_target;


    // find the matching ssv_drv
    hal_target = 0;
    for (i = 0; i < s_hal_cnt; i++)
    {
    	/* name = "SSV6006" */
        if (STRCMP(name, s_hal_array[i]->name) == 0)
        {
            hal_target = s_hal_array[i];
            break;
        }
    }

    if (hal_target == 0)
    {
        return FALSE;
    }
    // if the target hal = current hal, just return
    if (hal_target == s_hal_cur)
    {
        return TRUE;
    }

    s_hal_cur = hal_target;

    return TRUE;
}

bool ssv_hal_init(void)
{

    s_hal_cnt = 0;
    MEMSET(s_hal_array, 0x00, MAX_SSV6XXX_HAL * sizeof(struct ssv_hal_ops *));
    s_hal_cur = 0;

/* not in */
#if(CONFIG_CHIP_ID == SSV6030P)
	_ssv_hal_register(&g_hal_ssv6030);
	_ssv_hal_select(g_hal_ssv6030.name);
#endif
/* in */
#if((CONFIG_CHIP_ID==SSV6006B)||(CONFIG_CHIP_ID==SSV6006C))
	_ssv_hal_register(&g_hal_ssv6006);
	_ssv_hal_select(g_hal_ssv6006.name);
#endif

    chip_sel = (chip_def_S *)(&chip_list[CONFIG_CHIP_ID]);

    return TRUE;
}

int ssv_hal_chip_init(void)
{
    if(NULL==s_hal_cur->chip_init)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return s_hal_cur->chip_init();
}

int ssv_hal_init_mac(u8 *self_mac)
{
    if(NULL==s_hal_cur->init_mac)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return s_hal_cur->init_mac(self_mac);
}

int ssv_hal_init_sta_mac(u32 wifi_mode)
{
    if(NULL==s_hal_cur->init_sta_mac)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
    return s_hal_cur->init_sta_mac(wifi_mode);
}

int ssv_hal_init_ap_mac(u8 *bssid, u8 channel)
{
    if(NULL==s_hal_cur->init_ap_mac)
    {
        return -1;
    }

    return s_hal_cur->init_ap_mac(bssid, channel);
}

int ssv_hal_ap_wep_setting(ssv6xxx_sec_type sec_type, u8 *password, u8 vif_idx, u8* sta_mac_addr)
{
    if(NULL==s_hal_cur->ap_wep_setting)
    {
        return -1;
    }

    return s_hal_cur->ap_wep_setting(sec_type, password,vif_idx,sta_mac_addr);
}

int ssv_hal_tx_loopback_done(u8 *dat)
{
    if(NULL==s_hal_cur->tx_loopback_done)
    {
        return -1;
    }

    return s_hal_cur->tx_loopback_done(dat);
}

int ssv_hal_add_interface(u8 itf_idx,ssv6xxx_hw_mode hmode,u8 * selfmac,u8 channel)
{
    if(NULL==s_hal_cur->add_interface)
    {
        return -1;
    }

    return s_hal_cur->add_interface(itf_idx,hmode,selfmac,channel);
}

int ssv_hal_remove_interface(u8 itf_idx)
{
    if(NULL==s_hal_cur->remove_interface)
    {
        return -1;
    }

    return s_hal_cur->remove_interface(itf_idx);
}

int ssv_hal_setup_ampdu_wmm(bool IsAMPDU)
{
    if(NULL==s_hal_cur->setup_ampdu_wmm)
    {
        return -1;
    }

    return s_hal_cur->setup_ampdu_wmm(IsAMPDU);
}

int ssv_hal_pbuf_alloc(int size, int type)
{
    if(NULL==s_hal_cur->pbuf_alloc)
    {
        return -1;
    }

    return s_hal_cur->pbuf_alloc(size,type);
}

int ssv_hal_pbuf_free(int addr)
{
    if(NULL==s_hal_cur->pbuf_free)
    {
        return -1;
    }

    return s_hal_cur->pbuf_free(addr);
}

int ssv_hal_rf_enable(void)
{
    if(NULL==s_hal_cur->rf_enable)
    {
        return -1;
    }

    return s_hal_cur->rf_enable();
}

int ssv_hal_rf_disable(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->rf_disable)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->rf_disable();
}

int ssv_hal_rf_load_default_setting(void)
{
    if(NULL==s_hal_cur->rf_load_default_setting)
    {
        return -1;
    }

    return s_hal_cur->rf_load_default_setting();
}

int ssv_hal_watchdog_enable(void)
{
    if(NULL==s_hal_cur->watch_dog_enable)
    {
        return -1;
    }

    return s_hal_cur->watch_dog_enable();
}

int ssv_hal_watchdog_disable(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->watch_dog_disable)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->watch_dog_disable();
}

int ssv_hal_mcu_enable(void)
{
    if(NULL==s_hal_cur->mcu_enable)
    {
        return -1;
    }

    return s_hal_cur->mcu_enable();
}

int ssv_hal_mcu_disable(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->mcu_disable)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->mcu_disable();
}

int ssv_hal_sw_reset(u32 com)
{
    if(NULL==s_hal_cur->sw_reset)
    {
        return -1;
    }

    return s_hal_cur->sw_reset(com);
}

int ssv_hal_gen_rand(u8 *data, u32 len)
{
    if(NULL==s_hal_cur->gen_rand)
    {
        return -1;
    }

    return s_hal_cur->gen_rand(data,len);
}

int ssv_hal_promiscuous_enable(void)
{
    if(NULL==s_hal_cur->promiscuous_enable)
    {
        return -1;
    }

    return s_hal_cur->promiscuous_enable();
}

int ssv_hal_promiscuous_disable(void)
{
    if(NULL==s_hal_cur->promiscuous_disable)
    {
        return -1;
    }

    return s_hal_cur->promiscuous_disable();
}

int ssv_hal_read_chip_id(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->read_chip_id)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->read_chip_id();
}

int ssv_hal_read_efuse_mac(u8 *mac)
{
    if(NULL==s_hal_cur->read_efuse_mac)
    {
        return -1;
    }

    return s_hal_cur->read_efuse_mac(mac);
}

bool ssv_hal_get_diagnosis(void)
{
   if(NULL==s_hal_cur->get_diagnosis)
        return FALSE;

    return s_hal_cur->get_diagnosis();
}

int ssv_hal_is_heartbeat(void)
{
    if(NULL==s_hal_cur->is_heartbeat)
    {
        return -1;
    }

    return s_hal_cur->is_heartbeat();
}

int ssv_hal_reset_heartbeat(void)
{
    if(NULL==s_hal_cur->reset_heartbeat)
    {
        return -1;
    }

    return s_hal_cur->reset_heartbeat();
}
int ssv_hal_get_fw_status(u32 *val)
{
    if(NULL==s_hal_cur->get_fw_status)
    {
        return -1;
    }

    return s_hal_cur->get_fw_status(val);
}

int ssv_hal_set_fw_status(u32 val)
{
   if(NULL==s_hal_cur->set_fw_status)
        return -1;

    return s_hal_cur->set_fw_status(val);
}

int ssv_hal_reset_soc_irq(void)
{
    if(NULL==s_hal_cur->reset_soc_irq)
    {
        return -1;
    }

    return s_hal_cur->reset_soc_irq();
}

int ssv_hal_set_wakeup_bb_gpio(bool en)
{
    if(NULL==s_hal_cur->set_wakeup_bb_gpio)
    {
        return -1;
    }

    return s_hal_cur->set_wakeup_bb_gpio(en);
}

int ssv_hal_set_short_slot_time(bool en)
{
    if(NULL==s_hal_cur->set_short_slot_time)
    {
        return -1;
    }

    return s_hal_cur->set_short_slot_time(en);
}

int ssv_hal_beacon_set(void* beacon_skb, int dtim_offset)
{
    if(NULL==s_hal_cur->beacon_set)
    {
        return -1;
    }

    return s_hal_cur->beacon_set(beacon_skb,dtim_offset);
}


int ssv_hal_soc_set_bcn(enum ssv6xxx_tx_extra_type extra_type, void *frame, struct cfg_bcn_info *bcn_info, u8 dtim_cnt, u16 bcn_itv)
{
    if(NULL==s_hal_cur->soc_set_bcn)
    {
        return -1;
    }

    return s_hal_cur->soc_set_bcn(extra_type, frame, bcn_info, dtim_cnt, bcn_itv);
}

int ssv_hal_beacon_enable(bool bEnable)
{
    if(NULL==s_hal_cur->beacon_enable)
    {
        return -1;
    }

    return s_hal_cur->beacon_enable(bEnable);
}

bool ssv_hal_is_beacon_enable(void)
{
    if(NULL==s_hal_cur->is_beacon_enable)
    {
        return FALSE;
    }

    return s_hal_cur->is_beacon_enable();
}

int ssv_hal_get_tx_resources(u16 *pFreePages,u16 *pFreeIDs, u16 *pFreeSpaces)
{
    if(NULL==s_hal_cur->get_tx_resources)
    {
        return -1;
    }

    return s_hal_cur->get_tx_resources(pFreePages,pFreeIDs,pFreeSpaces);
}

int ssv_hal_bytes_to_pages(u32 size)
{
   if(NULL==s_hal_cur->bytes_to_pages)
        return -1;

    return s_hal_cur->bytes_to_pages(size);
}

int ssv_hal_get_rssi_from_reg(u8 vif_idx)
{
   if(NULL==s_hal_cur->get_rssi_from_reg)
        return -1;

    return s_hal_cur->get_rssi_from_reg(vif_idx);
}

int ssv_hal_get_rc_info(u16 *tx_cnt, u16 *retry_cnt, u16 *phy_rate)
{
    if(NULL==s_hal_cur->get_rc_info)
    {
        return -1;
    }

    return s_hal_cur->get_rc_info(tx_cnt,retry_cnt,phy_rate);
}

int ssv_hal_get_agc_gain(void)
{
    if(NULL==s_hal_cur->get_agc_gain)
    {
        return -1;
    }

    return s_hal_cur->get_agc_gain();
}

int ssv_hal_set_agc_gain(u32 gain)
{
    if(NULL==s_hal_cur->set_agc_gain)
    {
        return -1;
    }

    return s_hal_cur->set_agc_gain(gain);
}

int ssv_hal_set_acs_agc_gain()
{
    if(NULL==s_hal_cur->set_acs_agc_gain)
    {
        return -1;
    }

    return s_hal_cur->set_acs_agc_gain();
}

int ssv_hal_ap_listen_neighborhood(bool en)
{
    if(NULL==s_hal_cur->ap_listen_neighborhood)
    {
        return -1;
    }

    return s_hal_cur->ap_listen_neighborhood(en);
}

int ssv_hal_reduce_phy_cca_bits(void)
{
    if(NULL==s_hal_cur->reduce_phy_cca_bits)
    {
        return -1;
    }

    return s_hal_cur->reduce_phy_cca_bits();
}

int ssv_hal_recover_phy_cca_bits(void)
{
    if(NULL==s_hal_cur->recover_phy_cca_bits)
    {
        return -1;
    }

    return s_hal_cur->recover_phy_cca_bits();
}

int ssv_hal_update_cci_setting(u16 input_level)
{
    if(NULL==s_hal_cur->update_cci_setting)
    {
        return -1;
    }

    return s_hal_cur->update_cci_setting(input_level);
}

int ssv_hal_set_ext_rx_int(u32 pin)
{
    if(NULL==s_hal_cur->set_ext_rx_int)
    {
        return -1;
    }

    return s_hal_cur->set_ext_rx_int(pin);
}

int ssv_hal_pause_resuem_recovery_int(bool resume)
{
    if(NULL==s_hal_cur->pause_resuem_recovery_int)
    {
        return -1;
    }

    return s_hal_cur->pause_resuem_recovery_int(resume);
}

int ssv_hal_set_TXQ_SRC_limit(u32 qidx,u32 val)
{
    if(NULL==s_hal_cur->set_TXQ_SRC_limit)
    {
        return -1;
    }

    return s_hal_cur->set_TXQ_SRC_limit(qidx,val);
}

int ssv_hal_display_hw_queue_status(void)
{
    if(NULL==s_hal_cur->display_hw_queue_status)
    {
        return -1;
    }

    return s_hal_cur->display_hw_queue_status();
}

bool ssv_hal_support_5g_band(void)
{
    if(NULL==s_hal_cur->support_5g_band)
    {
        return FALSE;
    }

    return s_hal_cur->support_5g_band();
}

u32 ssv_hal_get_ava_wsid(u32 init_id)
{
    if(NULL==s_hal_cur->get_ava_wsid)
    {
        return FALSE;
    }

    return s_hal_cur->get_ava_wsid(init_id);
}

int ssv_hal_read_hw_queue(void)
{
    if(NULL==s_hal_cur->read_hw_queue)
    {
        return -1;
    }

    return s_hal_cur->read_hw_queue();
}

int ssv_hal_l2_off(u8 vif_idx)
{
    if(NULL==s_hal_cur->l2_off)
    {
        return -1;
    }

    return s_hal_cur->l2_off(vif_idx);
}

int ssv_hal_l2_on(u8 vif_idx)
{
    if(NULL==s_hal_cur->l2_on)
    {
        return -1;
    }

    return s_hal_cur->l2_on(vif_idx);
    }


int ssv_hal_halt_txq(u32 qidx,bool bHalt)
{
    if(NULL==s_hal_cur->halt_txq)
    {
        return -1;
    }

    return s_hal_cur->halt_txq(qidx,bHalt);
}

int ssv_hal_get_temperature(u8 *sar_code, char *temperature)
{
    if(NULL==s_hal_cur->get_temperature)
    {
        return -1;
    }

    return s_hal_cur->get_temperature(sar_code,temperature);
}

int ssv_hal_set_voltage_mode(u32 mode)
{
    if(NULL==s_hal_cur->set_voltage_mode)
    {
        return -1;
    }

    return s_hal_cur->set_voltage_mode(mode);
}

int ssv_hal_dump_txinfo(void *p)
{
    if(NULL==s_hal_cur->dump_txinfo)
    {
        return -1;
    }

    return s_hal_cur->dump_txinfo(p);
}

int ssv_hal_get_valid_txinfo_size(void)
{
    if(NULL==s_hal_cur->get_valid_txinfo_size)
    {
        return -1;
    }

    return s_hal_cur->get_valid_txinfo_size();
}

int ssv_hal_get_txreq0_size(void)
{
    if (NULL == s_hal_cur->get_txreq0_size)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_size();
}

int ssv_hal_get_txreq0_ctype(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_ctype)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_ctype(p);
}

int ssv_hal_set_txreq0_ctype(CFG_HOST_TXREQ *p, u8 ctype)
{
    if(NULL==s_hal_cur->set_txreq0_ctype)
    {
        return -1;
    }

    return s_hal_cur->set_txreq0_ctype(p, ctype);
}

int ssv_hal_get_txreq0_len(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_len)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_len(p);
}

int ssv_hal_set_txreq0_len(CFG_HOST_TXREQ *p,u32 len)
{
    if(NULL==s_hal_cur->set_txreq0_len)
    {
        return -1;
    }

    return s_hal_cur->set_txreq0_len(p,len);
}

int ssv_hal_get_txreq0_rsvd0(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_rsvd0)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_rsvd0(p);
}

int ssv_hal_set_txreq0_rsvd0(CFG_HOST_TXREQ *p,u32 val)
{
    if(NULL==s_hal_cur->set_txreq0_rsvd0)
    {
        return -1;
    }

    return s_hal_cur->set_txreq0_rsvd0(p,val);
}

int ssv_hal_get_txreq0_padding(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_padding)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_padding(p);
}

int ssv_hal_set_txreq0_padding(CFG_HOST_TXREQ *p, u32 val)
{
    if(NULL==s_hal_cur->set_txreq0_padding)
    {
        return -1;
    }

    return s_hal_cur->set_txreq0_padding(p,val);
}

int ssv_hal_get_txreq0_qos(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_qos)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_qos(p);
}

int ssv_hal_get_txreq0_ht(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_ht)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_ht(p);
}

int ssv_hal_get_txreq0_4addr(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_4addr)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_4addr(p);
}

int ssv_hal_set_txreq0_f80211(CFG_HOST_TXREQ *p, u8 f80211)
{
    if(NULL==s_hal_cur->set_txreq0_f80211)
    {
        return -1;
    }

    return s_hal_cur->set_txreq0_f80211(p, f80211);
}

int ssv_hal_get_txreq0_f80211(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_f80211)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_f80211(p);
}

int ssv_hal_get_txreq0_more_data(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_more_data)
    {
        return -1;
    }

    return s_hal_cur->get_txreq0_more_data(p);
}

int ssv_hal_set_txreq0_more_data(CFG_HOST_TXREQ *p, u8 more_data)
{
    if(NULL==s_hal_cur->set_txreq0_more_data)
    {
        return -1;
    }

    return s_hal_cur->set_txreq0_more_data(p,more_data);
}

u8 * ssv_hal_get_txreq0_qos_ptr(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_qos_ptr)
    {
        return NULL;
    }

    return s_hal_cur->get_txreq0_qos_ptr(p);
}

u8 * ssv_hal_get_txreq0_data_ptr(CFG_HOST_TXREQ *p)
{
    if(NULL==s_hal_cur->get_txreq0_data_ptr)
    {
        return NULL;
    }

    return s_hal_cur->get_txreq0_data_ptr(p);
}

int ssv_hal_tx_8023to80211(void *frame, u32 len, u32 priority,
                                    u16 *qos, u32 *ht, u8 *addr4,
                                    bool f80211,u8 security)
{
    if(NULL==s_hal_cur->tx_3to11)
    {
        return -1;
    }

    return s_hal_cur->tx_3to11(frame, len, priority,
                                qos, ht, addr4,
                                f80211, security);

}
void * ssv_hal_fill_txreq0(void *frame, u32 len, u32 priority,
                                    u16 *qos, u32 *ht, u8 *addr4,
                                    bool f80211,u8 security, u8 tx_dscrp_flag,u8 vif_idx)
{
    if(NULL==s_hal_cur->fill_txreq0)
    {
        return NULL;
    }

    return s_hal_cur->fill_txreq0(frame, len, priority,
                                qos, ht, addr4,
                                f80211, security, tx_dscrp_flag,vif_idx);
}


int ssv_hal_rx_80211to8023(void *p)
{
    if(NULL==s_hal_cur->rx_11to3)
    {
        return -1;
    }

    return s_hal_cur->rx_11to3(p);
}

int ssv_hal_dump_rxinfo(void *p)
{
    if(NULL==s_hal_cur->dump_rxinfo)
    {
        return -1;
    }

    return s_hal_cur->dump_rxinfo(p);
}

int ssv_hal_get_rxpkt_size(void)
{
    if(NULL==s_hal_cur->get_rxpkt_size)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_size();
}

int ssv_hal_get_rxpkt_ctype(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_ctype)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_ctype(p);
}

int ssv_hal_get_rxpkt_len(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_len)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_len(p);
}

int ssv_hal_get_rxpkt_channel_info(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_chl_info)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_chl_info(p);
}

int ssv_hal_get_rxpkt_rcpi(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_rcpi)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_rcpi(p);
}

int ssv_hal_set_rxpkt_rcpi(CFG_HOST_RXPKT *p, u32 rcpi)
{
    if(NULL==s_hal_cur->get_rxpkt_rcpi)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_rcpi(p);
}

int ssv_hal_get_rxpkt_qos(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_qos)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_qos(p);
}

int ssv_hal_get_rxpkt_f80211(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_f80211)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_f80211(p);
}

int ssv_hal_get_rxpkt_wsid(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_wsid)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_wsid(p);
}

int ssv_hal_get_rxpkt_tid(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_tid)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_tid(p);
}

int ssv_hal_get_rxpkt_seqnum(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_seqnum)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_seqnum(p);
}

int ssv_hal_get_rxpkt_psm(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_psm)
    {
        return -1;
    }

    return s_hal_cur->get_rxpkt_psm(p);
}

u8 * ssv_hal_get_rxpkt_qos_ptr(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_qos_ptr)
    {
        return NULL;
    }

    return s_hal_cur->get_rxpkt_qos_ptr(p);
}

u8 * ssv_hal_get_rxpkt_data_ptr(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_data_ptr)
    {
        return NULL;
    }

    return s_hal_cur->get_rxpkt_data_ptr(p);
}

int ssv_hal_get_rxpkt_data_len(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_data_len)
    {
        return 0;
    }

    return s_hal_cur->get_rxpkt_data_len(p);
}

u8 ssv_hal_get_rxpkt_bssid_idx(CFG_HOST_RXPKT *p)
{
    if(NULL==s_hal_cur->get_rxpkt_bssid_idx)
    {
        return 0;
    }

    return s_hal_cur->get_rxpkt_bssid_idx(p);
}
u32 ssv_hal_process_hci_rx_aggr(void* pdata, u32 len, RxPktHdr cbk_fh)
{
    if(NULL==s_hal_cur->process_hci_rx_aggr)
    {
        return -1;
    }

    return s_hal_cur->process_hci_rx_aggr(pdata,len,cbk_fh);
}

u32 ssv_hal_process_hci_aggr_tx(void* tFrame, void* aggr_buf, u32* aggr_len)
{
    if(NULL==s_hal_cur->process_hci_aggr_tx)
    {
        return -1;
    }

    return s_hal_cur->process_hci_aggr_tx(tFrame, aggr_buf, aggr_len);
}

int ssv_hal_hci_aggr_en(HCI_AGGR_HW trx, bool en)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->hci_aggr_en)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->hci_aggr_en(trx, en);
}
int ssv_hal_download_fw(u8 *fw_bin, u32 len)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->download_fw)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->download_fw(fw_bin,len);
}

int ssv_hal_accept_none_wsid_frame(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->accept_none_wsid)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->accept_none_wsid();
}

int ssv_hal_drop_none_wsid_frame(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
	if(NULL==s_hal_cur->drop_none_wsid)
	{		
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
		return -1;
	}
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->drop_none_wsid();
}

int ssv_hal_drop_probe_request(bool IsDrop)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->drop_porbe_request)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->drop_porbe_request(IsDrop);
}

int ssv_hal_sta_rcv_all_bcn(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->sta_rcv_all_bcn)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->sta_rcv_all_bcn();
}

int ssv_hal_sta_rcv_specific_bcn(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->sta_rcv_specific_bcn)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->sta_rcv_specific_bcn();
}

int ssv_hal_sta_reject_bcn(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->sta_reject_bcn)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->sta_reject_bcn();
}

int ssv_hal_acs_rx_mgmt_flow(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->acs_rx_mgmt_flow)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->acs_rx_mgmt_flow();
}

int ssv_hal_ap_rx_mgmt_flow(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->ap_rx_mgmt_flow)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->ap_rx_mgmt_flow();
}

int ssv_hal_sconfig_rx_data_flow(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->sconfig_rx_data_flow)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->sconfig_rx_data_flow();
}

int ssv_hal_sta_rx_data_flow(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    if(NULL==s_hal_cur->sta_rx_data_flow)
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
        return -1;
    }
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return s_hal_cur->sta_rx_data_flow();
}

const char *ssv_hal_get_chip_name(void)
{
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return chip_sel->chip_str;
}
