/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "dev.h"


/*  Global Variables Declaration: */
//ETHER_ADDR WILDCARD_ADDR = { {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} };

u32 g_free_msgevt_cnt;


/* Task Count to indicate if all tasks are running. */
u32 g_RunTaskCount;
//u32 g_MaxTaskCount;

//u8 g_sta_mac[6];


//#include <sim_regs.h>
//#include <security/drv_security.h>


//extern SRAM_KEY			*pGSRAM_KEY;

/* For cli_cmd_soc commands: */
//s32 g_soc_cmd_rx_ready = 0;
//char g_soc_cmd_rx_buffer[1024];


//u32 g_sim_net_up = 0;
//u32 g_sim_link_up = 0;

DeviceInfo_st *gDeviceInfo=NULL;
struct rcs_info tx_rcs;
OsMutex txsrcMutex;

u32 gMaxAID;
u32 gMaxTimLen;
//debug
/* in */
#if CONFIG_STATUS_CHECK
u32 g_l2_tx_packets;
u32 g_l2_tx_copy;
u32 g_l2_tx_late;
u32 g_notpool;
u32 g_heap_used;
u32 g_heap_max;
u32 g_dump_tx;
u32 g_tx_allocate_fail;
#endif


void host_global_init(void)
{
/* not in */
#if CONFIG_STATUS_CHECK    
    g_l2_tx_packets = 0;
    g_l2_tx_copy=0;
    g_l2_tx_late=0;
	g_notpool=0;
    g_heap_used=0;
    g_heap_max=0;
    g_dump_tx = 0;
    g_tx_allocate_fail = 0;
#endif 

    g_RunTaskCount = 0;
}



