/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_DEF_H_
#define _OS_DEF_H_

//#include <linux/kernel.h>
//#include <linux/random.h>
//#include <linux/spinlock.h>
#include <porting.h>

#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
//#include <linux/kthread.h>
//#include <linux/timer.h>



#define OS_EVENT_TBL_SIZE (8)   	/* Size of event table */
#define OS_RDY_TBL_SIZE   (8)   	/* Size of ready table */
#define OS_EVENT_NAME_SIZE (32)    	/* Determine the size of the name of a Sem, Mutex, Mbox or Q    */

typedef struct os_event {
    u8    OSEventType;                    	/* Type of event control block (see OS_EVENT_TYPE_xxxx)    */
    void  *OSEventPtr;                  	/* Pointer to message or queue structure                   */
    u16   OSEventCnt;                     	/* Semaphore Count (not used if other EVENT type)          */
    u16   OSEventGrp;                     	/* Group corresponding to tasks waiting for event to occur */
    u16   OSEventTbl[OS_EVENT_TBL_SIZE];  	/* List of tasks waiting for event to occur                */

#if OS_EVENT_NAME_SIZE > 1
    u8    OSEventName[OS_EVENT_NAME_SIZE];
#endif
} OS_EVENT;

/*============OS parameter setting===================*/
typedef struct OsMsgQ_st
{
    void *qpool;
    OS_EVENT *msssageQ;
} OsMessgQ;

//typedef void (*OsTask)(void *);
typedef int (*OsTask)(void *data);

typedef void *OsTaskHandle;
typedef void *OsTimer;
typedef void (*OsTimerHandler)(OsTimer xTimer);

//typedef OS_EVENT *OsMutex;
typedef struct mutex *OsMutex;

typedef struct {
    OS_EVENT *Sema;
    u16 MaxCnt;
} OsSemaphore_impl;

//typedef OsSemaphore_impl *OsSemaphore;

typedef struct semaphore *OsSemaphore;

typedef OsMessgQ *OsMsgQ;

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
