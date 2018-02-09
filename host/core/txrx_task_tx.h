/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _TX_TASK_H
#define _TX_TASK_H

#include <ssv_lib.h>

typedef struct frameList
{
    struct ssv_llist  _list;
    void *frame; //store the address of cmd/pkt
} FrmL;


#define TX_FRAME_ARRAY_SIZE 20


s32 TXTask_Init(void);
s32 TXTask_DeInit(void);
s32 TXRXTask_FrameEnqueue(void *frame, u32 priority);


#endif
