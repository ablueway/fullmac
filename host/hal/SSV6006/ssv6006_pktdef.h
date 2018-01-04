#ifndef _SSV6006_PKTDEF_H_
#define _SSV6006_PKTDEF_H_

#include <ssv_types.h>
//#include "ssv6006_hal.h"

typedef enum __PBuf_Type_E {
    NOTYPE_BUF  = 0,
    TX_BUF      = 1,
    RX_BUF      = 2
} PBuf_Type_E;

/**
 *
 *  Offset Table (register):
 *
 *    c_type            hdr_len
 *  ----------     ----------
 *  M0_TXREQ         8-bytes
 *  M1_TXREQ         12-bytes
 *  M2_TXREQ         sizeof(PKT_TXInfo)
 *  M0_RXEVENT
 *  M1_RXEVENT
 *  HOST_CMD
 *  HOSt_EVENT
 *
 *
 */
#define M0_TXREQ                            0
#define M1_TXREQ                            1
#define M2_TXREQ                            2
#define M0_RXEVENT                          3
#define M2_RXEVENT                          4
#define HOST_CMD                            5
#define HOST_EVENT                          6
#define RATE_RPT                            7


#define IS_RX_PKT(_p)       ((_p)->c_type==M0_RXEVENT)
#define IS_TX_PKT(_p)       (/*((_p)->c_type>=M0_TXREQ)&&*/((_p)->c_type<=M2_TXREQ))
#define IS_TX_TEST_CMD(_p)	((_p)->c_type==TEST_CMD)

/*	WMM_Specification_1-1 : Table 14  802.1D Priority to AC mappings

	UP		Access Category
	-------------------------
	1, 2	AC_BK
	0, 3	AC_BE
	4, 5	AC_VI
	6, 7	AC_VO
*/
#define	AC_BK	 0
#define	AC_BE	 1
#define	AC_VI	 2
#define	AC_VO	 3


#define	PBUF_HDR80211(p, i)				(*((u8 *)(p)+(p)->hdr_offset + (i)))

#define	GET_SC_SEQNUM(sc)				(((sc) & 0xfff0) >> 4)
#define	GET_SC_FRAGNUM(sc)				(((sc) & 0x000f)     )

#define GET_QC_TID(qc)					((qc) & 0x000f)
#define	GET_QC_UP(qc)					((qc) & 0x0007)
#define GET_QC_AC(qc) 					((GET_QC_UP(qc) == 0) ? AC_BE : \
										 (GET_QC_UP(qc) == 1) ? AC_BK : \
										 (GET_QC_UP(qc) == 2) ? AC_BK : \
										 (GET_QC_UP(qc) == 3) ? AC_BE : \
										 (GET_QC_UP(qc) == 4) ? AC_VI : \
										 (GET_QC_UP(qc) == 5) ? AC_VI : \
										 (GET_QC_UP(qc) == 6) ? AC_VO : AC_VO)


#define GET_HDR80211_FC(p)				(((p)->f80211==1) ? (((u16)PBUF_HDR80211(p, 1) << 8) | PBUF_HDR80211(p, 0)) : 0)
#define GET_HDR80211_FC_TYPE(p)			((GET_HDR80211_FC(p) & 0x0c) >> 2)
#define GET_HDR80211_FC_TYPE_STR(t)		((t == 0) ? "Mgmt" : ((t == 1) ? "Control" : ((t == 2) ? "Data" : "Reserved")))
#define	GET_HDR80211_FC_VER(p)			((GET_HDR80211_FC(p) & M_FC_VER))
#define GET_HDR80211_FC_TODS(p)			((GET_HDR80211_FC(p) & M_FC_TODS)      >>  8)
#define GET_HDR80211_FC_FROMDS(p)		((GET_HDR80211_FC(p) & M_FC_FROMDS)    >>  9)
#define GET_HDR80211_FC_MOREFRAG(p)		((GET_HDR80211_FC(p) & M_FC_MOREFRAGS) >> 10)
#define	GET_HDR80211_FC_RETRY(p)	    ((GET_HDR80211_FC(p) & M_FC_RETRY)     >> 11)
#define	GET_HDR80211_FC_PWRMGNT(p)		((GET_HDR80211_FC(p) & M_FC_PWRMGMT)   >> 12)
#define	GET_HDR80211_FC_MOREDATA(p)		((GET_HDR80211_FC(p) & M_FC_MOREDATA)  >> 13)
#define	GET_HDR80211_FC_PROTECTED(p)	((GET_HDR80211_FC(p) & M_FC_PROTECTED) >> 14)
#define	GET_HDR80211_FC_ORDER(p)		((GET_HDR80211_FC(p) & M_FC_ORDER)     >> 15)

#define SET_HDR80211_FC_MOREFRAG(p)		(PBUF_HDR80211(p, 1) |= 0x04)
#define UNSET_HDR80211_FC_MOREFRAG(p)	(PBUF_HDR80211(p, 1) &= 0xfb)

#define GET_HDR80211_SC(p)				((u16)PBUF_HDR80211(p, 23) << 8 | (PBUF_HDR80211(p, 22)))
#define GET_HDR80211_SC_SEQNUM(p)		((GET_HDR80211_SC(p) & 0xfff0) >> 4)
#define GET_HDR80211_SC_FRAGNUM(p)		((GET_HDR80211_SC(p) & 0x000f))

//
//	Function			ToDS	FromDS	Addr1	Addr2	Addr3	Addr4
//	-------------------------------------------------------------------------
//	IBSS				0		0		DA		SA		BSSID	Not_Used
//	To AP (infra.)		1		0		BSSID	SA		DA		Not_Used
//	From AP (infra.)	0		1		DA		BSSID	SA		Not_Used
//	WDS (bridge)		1		1		RA		TA		DA		SA
#define HAS_HDR80211_ADDRESS_4(p)		(GET_HDR80211_FC_TODS(p) & GET_HDR80211_FC_FROMDS(p))

// QoS Control Field
#define	GET_HDR80211_QC(p)				(((p)->qos == 1) ? (((u16)PBUF_HDR80211(p, 25 + (HAS_HDR80211_ADDRESS_4(p)*6)) << 8) | PBUF_HDR80211(p, 24 + (HAS_HDR80211_ADDRESS_4(p)*6))) : 0)
#define GET_HDR80211_ADDRESS_1(a, p)	memcpy((a), ((u8 *)(p)+(p)->hdr_offset +  4), 6)
#define GET_HDR80211_ADDRESS_2(a, p)	memcpy((a), ((u8 *)(p)+(p)->hdr_offset + 10), 6)
#define GET_HDR80211_ADDRESS_3(a, p)	memcpy((a), ((u8 *)(p)+(p)->hdr_offset + 16), 6)

/*
 * struct hci_rx_aggr_info - HCI RX Aggregation Format description
 */
struct hci_rx_aggr_info {
    u32             jmp_mpdu_len:16;
    u32             accu_rx_len:16;

    u32             RSVD0:15;
    u32             tx_page_remain:9;
    u32             tx_id_remain:8;

    u32             edca0:4;
    u32             edca1:5;
    u32             edca2:5;
    u32             edca3:5;
    u32             edca4:4;
    u32             edca5:5;
    u32             RSVD1:4;
};

/* 
 * struct hci_tx_aggr_info - HCI TX Aggregation Format description
 */
struct hci_tx_aggr_info {
    u32 jmp_len0:16;
    u32 jmp_len1:16;
    union{
        u32 extra_info;
        u32 aggr_n:8;
        u32 totol_len:16;
        u32 RSVD:8;
    }un;
};

/**
 * struct cfg_host_txreq0 - Host frame transmission Request  Header
 *
 * TX-REQ0 uses 4-byte header to carry host message to wifi-controller.
 * The first two-byte is the length indicating the whole message length (
 * including 2-byte header length).
 */
 struct cfg_host_txreq0 {
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             rvdtx_0:1;//used for rate control report event.
    u32             padding:2;
    u32             bc_que:1;
    u32             security:1;
    u32             more_data:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;   /* 0: don't trap to cpu after parsing, 1: trap to cpu after parsing */

    /* The definition of WORD_2: */
    u32             fCmd;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             no_pkt_buf_reduction:1;
    u32             tx_burst_obsolete:1;     /* 0: normal, 1: burst tx */
    u32             ack_policy_obsolete:2;   /* See Table 8-6, IEEE 802.11 Spec. 2012 */
    u32             aggr:2;
    u32             rsvdtx_1:1;              // for sw retry ampdu
    u32             is_rate_stat_sample_pkt:1;
                                             // new , v2.13
    u32             bssidx:2;                // new , v2.13, change location
    u32             reason:6;
 }STRUCT_PACKED;

struct cfg_host_rxpkt {
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             rsvdrx0_1:1;    // v2.14
    u32             running_no:4;   // v2.14
    u32             psm:1;
    u32             stype_b5b4:2;
    u32             rsvdrx0_2:1;

    /* The definition of WORD_2: */
    union{
        u32             fCmd;
        u32             edca0_used:4;
        u32             edca1_used:5;
        u32             edca2_used:5;
        u32             edca3_used:5;
        u32             mng_used:4;
        u32             tx_page_used:9;
    }w2;
    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             RxResult:8;
    u32             bssid:2;        // new; for v2.13
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             channel:8;      // v2.14
    u32             rx_pkt_run_no:8;      // new; for v2.14; from tx_pkt_run_no at BA
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             rsvd_rx_3b:9;

	/* The definition of WORD_5: */
    /*
        Redfine this WORD.
        Fill the seqnum and tid in fw for ampdu tx
     */
    #if 0
	u32 			phy_packet_length:16;
	u32				phy_rate:8;
    u32             phy_smoothing:1;
    u32             phy_no_sounding:1;
    u32             phy_aggregation:1;
    u32             phy_stbc:2;
    u32             phy_fec:1;
    u32             phy_n_ess:2;
    #else
	u32 			seq_num:16;
	u32				tid:16;
    #endif

	/* The definition of WORD_6: */
	u32				phy_l_length:12;
	u32				phy_l_rate:3;
	u32				phy_mrx_seqn:1;
	u32				phy_rssi:8;
	u32				phy_snr:8;

	/* The definition of WORD_7: */
	u32				phy_rx_freq_offset:16;
	u32				phy_service:6;

};

typedef struct PKT_Info_st
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             rsvd_0:6;
    u32             stype_b5b4:2;
    u32             extra_info:1;   /* 0: don't trap to cpu after parsing, 1: trap to cpu after parsing */

    /* The definition of WORD_2: */
    u32             fCmd;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             rsvd_1:10;
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             rsvd_2:16;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             rsvd_3:9;
} PKT_Info, *PPKT_Info;


typedef struct PKT_TxInfo_st
{
    /* The definition of WORD_1: */
     u32             len:16;
     u32             c_type:3;
     u32             f80211:1;
     u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
     u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
     u32             use_4addr:1;
     u32             rvdtx_0:1;//used for rate control report event.
     u32             padding:2;
     u32             bc_que:1;
     u32             security:1;
     u32             more_data:1;
     u32             stype_b5b4:2;
     u32             extra_info:1;   /* 0: don't trap to cpu after parsing, 1: trap to cpu after parsing */

     /* The definition of WORD_2: */
     u32             fCmd;

     /* The definition of WORD_3: */
     u32             hdr_offset:8;
     u32             frag:1;
     u32             unicast:1;
     u32             hdr_len:6;
     u32             no_pkt_buf_reduction:1;
     u32             tx_burst_obsolete:1;     /* 0: normal, 1: burst tx */
     u32             ack_policy_obsolete:2; //ack_policy_obsolete:2;   /* See Table 8-6, IEEE 802.11 Spec. 2012 */
     u32             aggr:2;
     u32             rsvdtx_1:1;              // for sw retry ampdu
     u32             is_rate_stat_sample_pkt:1;
                                              // new , v2.13
     u32             bssidx:2;                // new , v2.13, change location
     u32             reason:6;

     /* The definition of WORD_4: */
     u32             payload_offset_obsolete:8;
     u32             tx_pkt_run_no:8;         // new, v2.13
     u32             fCmdIdx:3;
     u32             wsid:4;
     u32             txq_idx:3;
     u32             TxF_ID:6;

     /* The definition of WORD_5: */
     u32             rateidx1_data_duration:16;
     u32             rateidx2_data_duration:16;

     /* The definition of WORD_6: */
     u32             rateidx3_data_duration:16;
     u32             rsvd_tx05 :2;
     u32             rate_rpt_mode:2;        //new
     u32             ampdu_tx_ssn:12;        //new for ampdu 1.3 not used in turismo
     /* The definition of WORD_7 */
     u32             drate_idx0:8;           //new, normal rate setting for series 0
     u32             crate_idx0:8;           //new, control rate setting(cts/rts) for series 0
     u32             rts_cts_nav0:16;        //new, rts_cts_nav for series 0
     /* The definition of WORD_8 */
     u32             dl_length0:12;          //new, dl_length for series 0
     u32             try_cnt0:4;             //new, retry count for series 0
     u32             ack_policy0:2;          //new, ack policy for series 0
     u32             do_rts_cts0:2;          //new, do rts_cts for series 0
     u32             is_last_rate0:1;        //new, is v2.13 last rate for rate control series
     u32             rsvdtx_07b:1;
     u32             rpt_result0:2;          /*new, for v2.13;
                                             //     0: typical maxtry fail ,     1: typical success
                                             //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/

     u32             rpt_trycnt0:4;          //new, report field for the number of tries excuted by MAC Tx
     u32             no_rsp_rxbusyfall0:1;
     u32             no_rsp_rxreqcutin0:1;
     u32             no_rsp_timeout0:1;
     u32             no_rsp0:1;
     /* The definition of WORD_9 */
     u32             drate_idx1:8;           //new, normal rate setting for series 1
     u32             crate_idx1:8;           //new, control rate setting(cts/rts) for series 1
     u32             rts_cts_nav1:16;        //new, rts_cts_nav for series 1
     /* The definition of WORD_10 */
     u32             dl_length1:12;          //new, dl_length for series 1
     u32             try_cnt1:4;             //new, retry count for series 1
     u32             ack_policy1:2;          //new, ack policy for series 1
     u32             do_rts_cts1:2;          //new, do rts_cts for series 1
     u32             is_last_rate1:1;        //new, is v2.13 last rate for rate control series
     u32             rsvdtx_09b:1;
     u32             rpt_result1:2;          /*new, for v2.13;
                                             //     0: typical maxtry fail ,     1: typical success
                                             //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/
     u32             rpt_trycnt1:4;          //new, report field for the number of tries excuted by MAC Tx
     u32             no_rsp_rxbusyfall1:1;
     u32             no_rsp_rxreqcutin1:1;
     u32             no_rsp_timeout1:1;
     u32             no_rsp1:1;
      /* The definition of WORD_11 */
     u32             drate_idx2:8;           //new, normal rate setting for series 2
     u32             crate_idx2:8;           //new, control rate setting(cts/rts) for series 2
     u32             rts_cts_nav2:16;        //new, rts_cts_nav for series 2
     /* The definition of WORD_12 */
     u32             dl_length2:12;          //new, dl_length for series 2
     u32             try_cnt2:4;             //new, retry count for series 2
     u32             ack_policy2:2;          //new, ack policy for series 2
     u32             do_rts_cts2:2;          //new, do rts_cts for series 2
     u32             is_last_rate2:1;        //new, is v2.13 last rate for rate control series
     u32             rsvdtx_11b:1;
     u32             rpt_result2:2;          /*new, for v2.13;
                                             //     0: typical maxtry fail ,     1: typical success
                                             //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/
     u32             rpt_trycnt2:4;          //new, report field for the number of tries excuted by MAC Tx
     u32             no_rsp_rxbusyfall2:1;
     u32             no_rsp_rxreqcutin2:1;
     u32             no_rsp_timeout2:1;
     u32             no_rsp2:1;
     /* The definition of WORD_13 */
     u32             drate_idx3:8;           //new, normal rate setting for series 3
     u32             crate_idx3:8;           //new, control rate setting(cts/rts) for series 3
     u32             rts_cts_nav3:16;        //new, rts_cts_nav for series 3
     /* The definition of WORD_14 */
     u32             dl_length3:12;          //new, dl_length for series 3
     u32             try_cnt3:4;             //new, retry count for series 3
     u32             ack_policy3:2;          //new, ack policy for series 3
     u32             do_rts_cts3:2;          //new, do rts_cts for series 3
     u32             is_last_rate3:1;        //new, is v2.13 last rate for rate control series
     u32             rsvdtx_13b:1;
     u32             rpt_result3:2;          /*new, for v2.13;
                                             //     0: typical maxtry fail ,     1: typical success
                                             //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/
     u32             rpt_trycnt3:4;          //new, report field for the number of tries excuted by MAC Tx
     u32             no_rsp_rxbusyfall3:1;
     u32             no_rsp_rxreqcutin3:1;
     u32             no_rsp_timeout3:1;
     u32             no_rsp3:1;
     /* The definition of WORD_15 */
     u32             ampdu_whole_length:16;  //new, for ampdu tx 1.3 whole ampdu length
     u32             ampdu_next_pkt:8;       //new, for ampdu tx 1.3 pointer for next pkt id
     u32             ampdu_last_pkt:1;       //new, for ampdu tx 1.3 last pkt indicator
     u32             rsvdtx_14a:3;                //rsvdtx_14a:3;
     u32             ampdu_dmydelimiter_num:4;   // new,v2.13 for ampdu tx 1.3

     /* The definition of WORD_16 */
     u32             ampdu_tx_bitmap_lw;     //new, for ampdu tx 1.3, bitmap low word for ampdu packet
     /* The definition of WORD_17 */
     u32             ampdu_tx_bitmap_hw;     //new, for ampdu tx 1.3, bitmap high word for ampdu packet
     /* The definition of WORD_18~20: */
     u32             dummy0;
     u32             dummy1;
     u32             dummy2;
} PKT_TxInfo, *PPKT_TxInfo;


/**
* struct ssv6006_rx_desc - ssv6006 rx frame descriptor.
* This descriptor is shared with ssv6006 hardware and driver.
*/
typedef struct PKT_RxInfo_st
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             rsvdrx0_1:1;    // v2.14
    u32             running_no:4;   // v2.14
    u32             psm:1;
    u32             stype_b5b4:2;
    u32             rsvdrx0_2:1;

    /* The definition of WORD_2: */
    union{
        u32             fCmd;
        u32             edca0_used:4;
        u32             edca1_used:5;
        u32             edca2_used:5;
        u32             edca3_used:5;
        u32             mng_used:4;
        u32             tx_page_used:9;
    }w2;
    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             RxResult:8;
    u32             bssid:2;        // new; for v2.13
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             channel:8;      // v2.14
    u32             rx_pkt_run_no:8;      // new; for v2.14; from tx_pkt_run_no at BA
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             rsvd_rx_3b:9;

	/* The definition of WORD_5: */
	u32 			phy_packet_length:16;
	u32				phy_rate:8;
    u32             phy_smoothing:1;
    u32             phy_no_sounding:1;
    u32             phy_aggregation:1;
    u32             phy_stbc:2;
    u32             phy_fec:1;
    u32             phy_n_ess:2;

	/* The definition of WORD_6: */
	u32				phy_l_length:12;
	u32				phy_l_rate:3;
	u32				phy_mrx_seqn:1;
	u32				phy_rssi:8;
	u32				phy_snr:8;

	/* The definition of WORD_7: */
	u32				phy_rx_freq_offset:16;
	u32				phy_service:6;

} PKT_RxInfo, *PPKT_RxInfo;

/**
* struct ssv6006_tx_desc - ssv6006 tx frame descriptor.
* This descriptor is shared with ssv6006 hardware and driver.
*/

#define SSV62XX_TX_MAX_RATES    3

STRUCT_PACKED struct fw_rc_retry_params {
    u32 count:4;
    u32 drate:6;
    u32 crate:6;
    u32 rts_cts_nav:16;
    u32 frame_consume_time:10;
    u32 dl_length:12;
    u32 RSVD:10;
};

struct ssv6006_tx_desc
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             rvdtx_0:3;//used for rate control report event.
    u32             bc_que:1;
    u32             security:1;
    u32             more_data:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;   /* 0: don't trap to cpu after parsing, 1: trap to cpu after parsing */

    /* The definition of WORD_2: */
    u32             fCmd;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             no_pkt_buf_reduction:1;
    u32             tx_burst_obsolete:1;     /* 0: normal, 1: burst tx */
    u32             ack_policy_obsolete:2;   /* See Table 8-6, IEEE 802.11 Spec. 2012 */
    u32             aggr:2;
    u32             rsvdtx_1:1;              // for sw retry ampdu
    u32             is_rate_stat_sample_pkt:1;
                                             // new , v2.13
    u32             bssidx:2;                // new , v2.13, change location
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             payload_offset_obsolete:8;
    u32             tx_pkt_run_no:8;         // new, v2.13
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             txq_idx:3;
    u32             TxF_ID:6;

    /* The definition of WORD_5: */
    u32             rateidx1_data_duration:16;
    u32             rateidx2_data_duration:16;

    /* The definition of WORD_6: */
    u32             rateidx3_data_duration:16;
    u32             rsvd_tx05 :2;
    u32             rate_rpt_mode:2;        //new
    u32             ampdu_tx_ssn:12;        //new for ampdu 1.3 not used in turismo
    /* The definition of WORD_7 */
    u32             drate_idx0:8;           //new, normal rate setting for series 0
    u32             crate_idx0:8;           //new, control rate setting(cts/rts) for series 0
    u32             rts_cts_nav0:16;        //new, rts_cts_nav for series 0
    /* The definition of WORD_8 */
    u32             dl_length0:12;          //new, dl_length for series 0
    u32             try_cnt0:4;             //new, retry count for series 0
    u32             ack_policy0:2;          //new, ack policy for series 0
    u32             do_rts_cts0:2;          //new, do rts_cts for series 0
    u32             is_last_rate0:1;        //new, is v2.13 last rate for rate control series
    u32             rsvdtx_07b:1;
    u32             rpt_result0:2;          /*new, for v2.13;
                                            //     0: typical maxtry fail ,     1: typical success
                                            //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/

    u32             rpt_trycnt0:4;          //new, report field for the number of tries excuted by MAC Tx
    u32             rpt_noctstrycnt0:4;     //new, report field for the number of tries excuted by MAC Tx failing to receive CTS.
    /* The definition of WORD_9 */
    u32             drate_idx1:8;           //new, normal rate setting for series 1
    u32             crate_idx1:8;           //new, control rate setting(cts/rts) for series 1
    u32             rts_cts_nav1:16;        //new, rts_cts_nav for series 1
    /* The definition of WORD_10 */
    u32             dl_length1:12;          //new, dl_length for series 1
    u32             try_cnt1:4;             //new, retry count for series 1
    u32             ack_policy1:2;          //new, ack policy for series 1
    u32             do_rts_cts1:2;          //new, do rts_cts for series 1
    u32             is_last_rate1:1;        //new, is v2.13 last rate for rate control series
    u32             rsvdtx_09b:1;
    u32             rpt_result1:2;          /*new, for v2.13;
                                            //     0: typical maxtry fail ,     1: typical success
                                            //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/
    u32             rpt_trycnt1:4;          //new, report field for the number of tries excuted by MAC Tx
    u32             rpt_noctstrycnt1:4;     //new, report field for the number of tries excuted by MAC Tx failing to receive CTS.
     /* The definition of WORD_11 */
    u32             drate_idx2:8;           //new, normal rate setting for series 2
    u32             crate_idx2:8;           //new, control rate setting(cts/rts) for series 2
    u32             rts_cts_nav2:16;        //new, rts_cts_nav for series 2
    /* The definition of WORD_12 */
    u32             dl_length2:12;          //new, dl_length for series 2
    u32             try_cnt2:4;             //new, retry count for series 2
    u32             ack_policy2:2;          //new, ack policy for series 2
    u32             do_rts_cts2:2;          //new, do rts_cts for series 2
    u32             is_last_rate2:1;        //new, is v2.13 last rate for rate control series
    u32             rsvdtx_11b:1;
    u32             rpt_result2:2;          /*new, for v2.13;
                                            //     0: typical maxtry fail ,     1: typical success
                                            //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/
    u32             rpt_trycnt2:4;          //new, report field for the number of tries excuted by MAC Tx
    u32             rpt_noctstrycnt2:4;     //new, report field for the number of tries excuted by MAC Tx failing to receive CTS.
    /* The definition of WORD_13 */
    u32             drate_idx3:8;           //new, normal rate setting for series 3
    u32             crate_idx3:8;           //new, control rate setting(cts/rts) for series 3
    u32             rts_cts_nav3:16;        //new, rts_cts_nav for series 3
    /* The definition of WORD_14 */
    u32             dl_length3:12;          //new, dl_length for series 3
    u32             try_cnt3:4;             //new, retry count for series 3
    u32             ack_policy3:2;          //new, ack policy for series 3
    u32             do_rts_cts3:2;          //new, do rts_cts for series 3
    u32             is_last_rate3:1;        //new, is v2.13 last rate for rate control series
    u32             rsvdtx_13b:1;
    u32             rpt_result3:2;          /*new, for v2.13;
                                            //     0: typical maxtry fail ,     1: typical success
                                            //     2: by-packet rts maxtry fail 3: peer-ps reject tx*/
    u32             rpt_trycnt3:4;          //new, report field for the number of tries excuted by MAC Tx
    u32             rpt_noctstrycnt3:4;     //new, report field for the number of tries excuted by MAC Tx failing to receive CTS.
    /* The definition of WORD_15 */
    u32             ampdu_whole_length:16;  //new, for ampdu tx 1.3 whole ampdu length
    u32             ampdu_next_pkt:8;       //new, for ampdu tx 1.3 pointer for next pkt id
    u32             ampdu_last_pkt:1;       //new, for ampdu tx 1.3 last pkt indicator
    u32             rsvdtx_14a:3;
    u32             ampdu_dmydelimiter_num:4;   // new,v2.13 for ampdu tx 1.3

    /* The definition of WORD_16 */
    u32             ampdu_tx_bitmap_lw;     //new, for ampdu tx 1.3, bitmap low word for ampdu packet
    /* The definition of WORD_17 */
    u32             ampdu_tx_bitmap_hw;     //new, for ampdu tx 1.3, bitmap high word for ampdu packet
    /* The definition of WORD_18~20: */
    u32             dummy0;
    u32             dummy1;
    u32             dummy2;
};

typedef struct PKT_TxPhyInfo_st
{
	/* The definition of WORD_1: */
	u32 Llength:8;

	u32 Mlength:8;

	u32 RESV1:8;

	u32 RESV2:8;

	/* The definition of WORD_2: */
	u32 mode:3;
	u32 ch_bw:3;
	u32 preamble_option:1;
	u32 HTshortGI:1;

	u32 rate:7;
	u32 RESV3:1;

	u32 smoothing:1;
	u32 no_sounding:1;
	u32 aggregation:1;
	u32 stbc:2;
	u32 fec:1;
	u32 n_ess:2;

	u32 txpwrlvl:8;

	/* The definition of WORD_3: */
	u32 Ll_length:8;

	u32 Ml_length:4;
	u32 l_rate:3;
	u32 RESV4:1;

	u32 RESV5:16;

	/* The definition of WORD_4: */
	u32 RESV6:32;

	/* The definition of WORD_5: */
	u32 RESV7:16;

	u32 Lservice:8;

	u32 Mservice:8;

	/* The definition of WORD_6: */
	u32 RESV8:32;

	/* The definition of WORD_7: */
	u32 RESV9:32;
}PKT_TxPhyInfo, *PPKT_TxPhyInfo;

typedef struct PKT_RxPhyInfo_st
{
	/* The definition of WORD_1: */
	u32 Llength:8;

	u32 Mlength:8;

	u32 RESV1:8;

	u32 RESV2:8;

	/* The definition of WORD_2: */
	u32 mode:3;
	u32 ch_bw:3;
	u32 preamble_option:1;
	u32 HTshortGI:1;

	u32 rate:7;
	u32 RESV3:1;

	u32 smoothing:1;
	u32 no_sounding:1;
	u32 aggregation:1;
	u32 stbc:2;
	u32 fec:1;
	u32 n_ess:2;

	u32 RESV4:8;

	/* The definition of WORD_3: */
	u32 Ll_length:8;

	u32 Ml_length:4;
	u32 l_rate:3;
	u32 RESV5:1;

	u32 RESV6:16;

	/* The definition of WORD_4: */
	u32 RESV7:32;

	/* The definition of WORD_5: */
	u32 RESV8:16;

	u32 Lservice:8;

	u32 Mservice:8;

}PKT_RxPhyInfo, *PPKT_RxPhyInfo;
typedef struct PHY_Info_st
{
    u32             WORD1;
    u32             WORD2;
    u32             WORD3;
    u32             WORD4;
    u32             WORD5;
    u32             WORD6;
    u32             WORD7;

} PHY_Info, *PPHY_Info;

/* not in */
#if(SW_8023TO80211==1)
#define SSV6006_HOST_TXREQ0         PKT_TxInfo
/* in */
#else
#define SSV6006_HOST_TXREQ0         struct cfg_host_txreq0
#endif
#define SSV6006_HOST_RXPKT          struct cfg_host_rxpkt
#define PKT_INFO_DATA_TYPE          PKT_Info
#define PKT_TXINFO_DATA_TYPE        PKT_TxInfo
#define PKT_RXINFO_DATA_TYPE        PKT_RxInfo

/**
 * Define constants for do_rts_cts field of PKT_TxInfo structure
 *
 * @ TX_NO_RTS_CTS
 * @ TX_RTS_CTS
 * @ TX_CTS
 */
#define TX_NO_RTS_CTS                   0
#define TX_RTS_CTS                      1
#define TX_CTS                          2

#ifdef THROUGHPUT_TEST

typedef struct SDIO_THROUGHPUT_COMMAND_st
{
	u32		transferCount;
	u16		transferLength;
	u16		resverd;
} SDIO_THROUGHPUT_COMMAND;


typedef struct THROUGHPUT_COMMAND_st
{
    u8		mode;
	u8		rateIndex;//0-38
	u8      noack;
	u16		transferLength;
	u16      qos;
	u32		transferCount;
} THROUGHPUT_COMMAND;

#endif

enum fcmd_seek_type {
    FCMD_SEEK_PREV  = 0,
    FCMD_SEEK_CUR,
    FCMD_SEEK_NEXT,
};

#define M0_HDR_LEN							4
#define M1_HDR_LEN							8
#define M2_HDR_LEN							16

#define RX_M0_HDR_LEN						SSV6006_RXINFO_SIZE

#endif	/* _SSV6006_PKTDEF_H_ */

