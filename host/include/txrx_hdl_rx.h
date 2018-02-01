/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _TXRX_HDL_RX_H
#define _TXRX_HDL_RX_H
//#include <dev.h>
//#include "host_apis.h"




u16 RxHdl_GetRawRxDataOffset(CFG_HOST_RXPKT *p);
s32 RxHdl_FrameProc(void *frame);
s32 RxHdl_SetWifiRxFlt(struct wifi_flt *flt, ssv6xxx_cb_action act);
s32 RxHdl_SetEthRxFlt(struct eth_flt *flt, ssv6xxx_cb_action act);
s32 RxHdl_Init(void);



#endif
