/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _HOST_CMD_ENGINE_PRIV_H_
#define _HOST_CMD_ENGINE_PRIV_H_

#include "os_wrapper.h"
#include <ssv_lib.h>

#if 0
#include "host_apis.h"
#include "host_cmd_engine.h"
#include "os_wrapper.h"
#include <ssv_lib.h>
#include <pbuf.h>


//#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
//#endif




typedef u32 tx_result;
#define TX_CONTINUE	((tx_result) 0u)
#define TX_DROP		((tx_result) 1u)
#define TX_QUEUED	((tx_result) 2u)


//Off->Init->Running->Pause->Off


enum TXHDR_ENCAP {
	TXREQ_ENCAP_ADDR4               =0,
	TXREQ_ENCAP_QOS                 ,
	TXREQ_ENCAP_HT                  ,
};


#define IS_TXREQ_WITH_ADDR4(x)          ((x)->txhdr_mask & (1<<TXREQ_ENCAP_ADDR4) )
#define IS_TXREQ_WITH_QOS(x)            ((x)->txhdr_mask & (1<<TXREQ_ENCAP_QOS)   )
#define IS_TXREQ_WITH_HT(x)             ((x)->txhdr_mask & (1<<TXREQ_ENCAP_HT)    )

#define SET_TXREQ_WITH_ADDR4(x, _tf)    (x)->txhdr_mask = (((x)->txhdr_mask & ~(1<<TXREQ_ENCAP_ADDR4))	| ((_tf)<<TXREQ_ENCAP_ADDR4) )
#define SET_TXREQ_WITH_QOS(x, _tf)      (x)->txhdr_mask = (((x)->txhdr_mask & ~(1<<TXREQ_ENCAP_QOS))	| ((_tf)<<TXREQ_ENCAP_QOS)   )
#define SET_TXREQ_WITH_HT(x, _tf)       (x)->txhdr_mask = (((x)->txhdr_mask & ~(1<<TXREQ_ENCAP_HT))		| ((_tf)<<TXREQ_ENCAP_HT)    )



typedef enum txreq_type_en {
	TX_TYPE_M0 = 0,
	TX_TYPE_M1,
	TX_TYPE_M2,   
} txreq_type;




typedef enum{
	SSV6XXX_HWM_STA		,	
	SSV6XXX_HWM_AP		,	
	SSV6XXX_HWM_IBSS	,	
	SSV6XXX_HWM_WDS	    ,	
	SSV6XXX_HWM_INVALID	,		
}ssv6xxx_hw_mode;




#define HOST_DATA_CB_NUM  5
#define HOST_EVT_CB_NUM  5

#endif

typedef struct frameQ
{
    struct ssv_list_q  _list;
    void *frame; //store the address of cmd
} FrmQ;

typedef struct HostCmdEngInfo{
    OsMutex CmdEng_mtx;
    ModeType  curr_mode;    
    struct ssv_list_q  pending_cmds;
    struct ssv_list_q  free_FrmQ;
    u32 pending_cmd_seqno;
    bool blockcmd_in_q;

}HostCmdEngInfo_st;


extern HostCmdEngInfo_st *gHCmdEngInfo;








#endif//_HOST_CMD_ENGINE_PRIV_H_



