/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _RX_TASK_H
#define _RX_TASK_H


#define RXSEMA_MAX_NUM 1 //pbuf maximun


void _dq_status_handler (void *data);
s32 TXRXTask_RxFrameProc(void* frame);
s32 RXTask_Init(void);
s32 RXTask_DeInit(void);

#endif
