/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _RTOS_DEF_H_
#define _RTOS_DEF_H_

#include <porting.h>

/*============OS parameter setting===================*/
typedef struct OsMsgQ_st
{
	T_hQueue   m_hQueue;// msg que handle
	T_pVOID    m_pQueueAddr;// msg que address
    T_U32      m_queNum;// msg que len
    T_U32      m_msgSize;// msg que atomic size
} OsMessgQ;

typedef void (*OsTask)(void *);

typedef struct tagCSubThread
{
    T_hTask	     m_hTask;
    T_pVOID	     m_pStackAddr;
    T_U32        m_priority;       //Point to the user data which will be used as a param by m_fnEntry.
    T_VOID      *m_pUserData;       //Point to the user data which will be used as a param by m_fnEntry.
    OsTask       m_pTaskFunc; 
}OsThread;

typedef void *              OsTaskHandle;
typedef void *              OsTimer;
typedef void (*OsTimerHandler)( OsTimer xTimer );

typedef T_hSemaphore           OsMutex;
typedef T_hSemaphore        OsSemaphore;

typedef OsMessgQ*           OsMsgQ;


#define OS_TASK_ENTER_CRITICAL()   OS_EnterCritical()     
#define OS_TASK_EXIT_CRITICAL()    OS_ExitCritical(0)

//#define OS_TASK_ENTER_CRITICAL()   u32 task_pri = AK_Change_Priority(AK_GetCurrent_Task(), 1);
//#define OS_TASK_EXIT_CRITICAL()    AK_Change_Priority(AK_GetCurrent_Task(), task_pri);

#define TICK_RATE_MS 5 //1
//#define TASK_IDLE_STK_SIZE 1024


#endif
