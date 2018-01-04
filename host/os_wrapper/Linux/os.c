/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "os.h"
/*TOSO: aaron */
//#include "os_cfg.h"
#include <log.h>


volatile u8 gOsFromISR;
//static u8 os_init_flag = 0;

#define STATIC_STK
#ifdef STATIC_STK
//(TOTAL_STACK_SIZE<<4) --> transfer unit to bytes
//((TOTAL_STACK_SIZE<<4)>>2) --> transfer unit to word
u32 task_stack[((TOTAL_STACK_SIZE<<4)>>2)];
u32 stk_used=0;
OsMutex StkMutex;
#endif

u32 task_stack_size = 0;

OS_APIs s32 OS_Init( void )
{
    return OS_SUCCESS;
}

OS_APIs unsigned long OS_Random(void)
{
    return OS_SUCCESS;
}

OS_APIs void OS_Terminate( void )
{
 
}

OS_APIs u32 OS_EnterCritical(void)
{
    return OS_SUCCESS;
}

OS_APIs void OS_ExitCritical(u32 val)
{

}

/* Task: */

OS_APIs s32 OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{
    return OS_SUCCESS;
}


OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{

}



OS_APIs void OS_StartScheduler( void )
{

}

OS_APIs u32 OS_GetSysTick(void)
{
    return OS_SUCCESS;
}


/* Mutex APIs: */
OS_APIs s32 OS_MutexInit( OsMutex *mutex )
{
    return OS_SUCCESS;
}

OS_APIs void OS_MutexLock( OsMutex mutex )
{

}

OS_APIs void OS_TickDelay(u32 ticks)
{

}


OS_APIs void OS_MutexUnLock( OsMutex mutex )
{

}

OS_APIs void OS_MutexDelete( OsMutex mutex )
{

}

OS_APIs void OS_MsDelay(u32 ms)
{

}

#define SEMA_LOCK()		OSSchedLock()
#define SEMA_UNLOCK()		OSSchedUnlock()

OS_APIs s32 OS_SemInit( OsSemaphore* Sem, u16 maxcnt, u16 cnt)
{
    return OS_SUCCESS;
}

OS_APIs bool OS_SemWait( OsSemaphore Sem , u16 timeout)
{
    return OS_SUCCESS;
}

OS_APIs u8 OS_SemSignal( OsSemaphore Sem)
{
    return OS_SUCCESS;
}

OS_APIs u32 OS_SemCntQuery( OsSemaphore Sem)
{
    return OS_SUCCESS;
}

OS_APIs u8 OS_SemSignal_FromISR( OsSemaphore Sem)
{
    return OS_SUCCESS;
}

OS_APIs void OS_SemDelete(OsSemaphore Sem)
{

}

#define MSGQ_LOCK()		OSSchedLock()
#define MSGQ_UNLOCK()		OSSchedUnlock()

/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen )
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDelete( OsMsgQ MsgQ)
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueueTry( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, u32 timeOut, bool fromISR )

{
    return OS_SUCCESS;
}


OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ )
{
    return OS_SUCCESS;
}



/* Timer: */
OS_APIs s32 OS_TimerCreate( OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler )
{    
	return OS_SUCCESS;
}

OS_APIs s32 OS_TimerSet( OsTimer timer, u32 ms, u8 autoReload, void *args )
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_TimerStart( OsTimer timer )
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_TimerStop( OsTimer timer )
{
    return OS_SUCCESS;
}

OS_APIs void *OS_TimerGetData( OsTimer timer )
{
   return NULL;
}

#if 0
/*==================OS Profiling=========================*/
#if ((OS_CPU_HOOKS_EN>0)&&(OS_TASK_STAT_EN==1))

extern u8 cmd_top_enable;
u32 lastTinyCount=0;
u32 currentTinyCount=0;
u32 lastTickCount=0;
u32 currentTickCount=0;

u32 lastTinyCountStatTask=0;
u32 currentTinyCountStatTask=0;
u32 lastTickCountStatTask=0;
u32 currentTickCountStatTask=0;

u32 acc_cpu_usage=0;
u32 acc_counts=0;

u32 OSCtxSwCtrPerSec;
#define TINY_COUNT(x) ((x*96)/144) // tiny_counter_get is the reference
#define TINY_COUNT_TO_US(x) (((x*266)/100) )
#endif

#if OS_TASK_CREATE_EXT_EN > 0
TASK_USER_DATA taskUserData[64];
#endif

#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0) && (OS_TASK_CREATE_EXT_EN>0)
void output_message(char *s)
{
    char *p=s;
    while(*p!='\0'){
        hal_putchar(*p);
        p++;
    }

}
void TaskSwHook(void)
{

}
#endif

#if ((OS_CPU_HOOKS_EN>0)&&(OS_TASK_STAT_EN==1))
void TaskStatHook(void)
{

}

void DispTaskStatus(void)
{

}

void ClearTaskStatus(void)
{
  
}
#else
void TaskStatHook(void)
{
}
#endif

#endif

