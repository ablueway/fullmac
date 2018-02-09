/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _HOST_CMD_ENGINE_PRIV_H_
#define _HOST_CMD_ENGINE_PRIV_H_

#include "os_wrapper.h"
#include <ssv_lib.h>


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



