/*
 * Copyright (c) 2014 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Semiconductor Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SSV6200_H_
#define _SSV6200_H_

#include <linux/device.h>
#include <linux/interrupt.h>
#include <net/mac80211.h>

// Include defines from config.mak to feed eclipse defines from ccflags-y
#if 0
#include <ssv_mod_conf.h>
#endif // ECLIPSE

#if 0 //ndef SSV_SUPPORT_HAL
#include <ssv6200_reg.h>
#include <ssv6200_aux.h>
#endif

#include <hwif/hwif.h>
//#include <hci/ssv_hci.h>

#include "ssv6200_common.h"

//Reserve 5 pages for KEY & PIH info & rate control
/* tx page size could not more than 255
  * tx id could not more than 63
  *  TX_ID_ALL_INFO (TX_PAGE_USE_7_0 only 8bits and TX_ID_USE_5_0 only 6bits)
  */
 
#define SSV6200_TOTAL_PAGE                  (256)
#define SSV6200_TOTAL_ID                    (128)
//Security use 8 ids. 
//#define SSV6200_ID_TX_THRESHOLD 			60//64
//Fix WSD-144 issue
#define SSV6200_ID_TX_THRESHOLD 			19//64            
#define SSV6200_ID_RX_THRESHOLD 			60//64 

//Reserve 4 for KEY & PIH info and 7*3 for security
//WSD-124 issue reserve 1
#define SSV6200_RESERVED_PAGE				(26)
#define SSV6200_PAGE_TX_THRESHOLD          115	//128
#define SSV6200_PAGE_RX_THRESHOLD          (SSV6200_TOTAL_PAGE - SSV6200_PAGE_TX_THRESHOLD - SSV6200_RESERVED_PAGE)	//128

#define SSV6200_AMPDU_DIVIDER                   (2)
#define SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER    (SSV6200_PAGE_TX_THRESHOLD - (SSV6200_PAGE_TX_THRESHOLD/SSV6200_AMPDU_DIVIDER))
#define SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER      2
 
#define SSV6200_ID_NUMBER                   (SSV6200_TOTAL_ID)

#define PACKET_ADDR_2_ID(addr)              ((addr >> 16) & 0x7F)

#define SSV6200_ID_AC_RESERVED              1


#define SSV6200_ID_AC_BK_OUT_QUEUE          8
#define SSV6200_ID_AC_BE_OUT_QUEUE          15
#define SSV6200_ID_AC_VI_OUT_QUEUE          16
#define SSV6200_ID_AC_VO_OUT_QUEUE          16
#define SSV6200_ID_MANAGER_QUEUE            8

#define	HW_MMU_PAGE_SHIFT			0x8 //256 bytes
#define	HW_MMU_PAGE_MASK			0xff

#define SSV6200_BT_PRI_SMP_TIME     0
#define SSV6200_BT_STA_SMP_TIME     (SSV6200_BT_PRI_SMP_TIME+0)
#define SSV6200_WLAN_REMAIN_TIME    0
#define BT_2WIRE_EN_MSK                        0x00000400

struct txResourceControl {
    u32 txUsePage:8;
    u32 txUseID:6;
    u32 edca0:4;
    u32 edca1:4;
    u32 edca2:5;
    u32 edca3:5;
};


// unit : Bytes
#define SSV_SKB_info_size (sizeof(struct SKB_info_st))

//#include <ssv_cfg.h>


#endif /* _SSV6200_H_ */
