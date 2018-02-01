/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _TXRX_HDL_TX_H
#define _TXRX_HDL_TX_H
//#include <dev.h>
//#include "host_apis.h"


#define CFG_HOST_TXREQ0(host_txreq0,len,c_type,f80211,qos,ht,use_4addr,RSVD0,padding,bc_queue,security,more_data,sub_type,extra_info) do{\
    u32 *temp;                                       \
    temp = (u32*)host_txreq0 ;                       \
    *temp = (len<<0) |                              \
            (c_type<<16)|                           \
            (f80211<<19)|                           \
            (qos<<20)|                              \
            (ht<<21)|                               \
            (use_4addr<<22)|                        \
            (RSVD0<<23)|                            \
            (padding<<24)|                          \
            (bc_queue<<26)|                         \
            (security<<27)|                         \
            (more_data<<28)|                        \
            (sub_type<<29)|                         \
            (extra_info<<31);                        \
            }while(0)

bool TxHdl_prepare_wifi_txreq(u8 vif_idx ,void *frame, u32 len, bool f80211, u32 priority, u8 tx_dscrp_flag);
s32 TxHdl_FlushFrame(void);
bool TxHdl_FrameProc(void *frame, bool apFrame, u32 priority, u32 flags, ssv_vif *vif);
s32 TxHdl_Init(void);


#endif
