/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifndef _OS_DEF_H_
#define _OS_DEF_H_

#include <porting.h>

#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>



#define OS_EVENT_TBL_SIZE (8)   	/* Size of event table */
#define OS_RDY_TBL_SIZE   (8)   	/* Size of ready table */
#define OS_EVENT_NAME_SIZE (32)    	/* Determine the size of the name of a Sem, Mutex, Mbox or Q    */

/*============OS parameter setting===================*/

typedef int (*OsTask)(void *data);
typedef void *OsTaskHandle;
typedef void *OsTimer;
typedef void (*OsTimerHandler)(OsTimer xTimer);

typedef struct mutex *OsMutex;
typedef struct semaphore *OsSemaphore;
typedef struct kfifo *OsMsgQ;

#define OS_TASK_ENTER_CRITICAL()        
#define OS_TASK_EXIT_CRITICAL()         

#define TICK_RATE_MS 			(1)
#define TASK_IDLE_STK_SIZE 		(100)

extern void TaskSwHook(void);
typedef struct {
    u32 TaskCtr;
    u32 TaskTotExecTimeFromTick;
    u32 TaskTotExecTimeFromTiny;
    char TaskName[30];
    u8  valid;

} TASK_USER_DATA;


#endif
