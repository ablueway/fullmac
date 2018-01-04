/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "rtos.h"
#include "akos_api.h"
#include "hal_timer.h"
//#include "fwl_oscom.h"

#include <log.h>

volatile u8 gOsFromISR;

static T_hSemaphore akcritSec = AK_INVALID_SEMAPHORE;


typedef struct TC_PROTECT_STRUCT
{
    void             *tc_tcb_pointer;        /* Owner of the protection */
    unsigned long            tc_thread_waiting;     /* Waiting thread flag     */
} TC_PROTECT;

TC_PROTECT rtos_tc_protect;
extern void  TCT_Protect(TC_PROTECT *protect);
extern void  TCT_Unprotect_Specific(TC_PROTECT *protect);

OS_APIs s32  OS_Init( void )
{
    gOsFromISR = 0;
    akcritSec = AK_Create_Semaphore(1, AK_PRIORITY);
    return OS_SUCCESS;
}

OS_APIs unsigned long OS_Random(void)
{
	return OS_GetSysTick()%65536+54*18;
}

OS_APIs void OS_Terminate( void )
{

}

OS_APIs u32 OS_EnterCritical(void)
{
    s32 ret = 0;
    #if 1
    if (akcritSec != AK_INVALID_SEMAPHORE)
    {
        ret = AK_Obtain_Semaphore(akcritSec, AK_SUSPEND);
        if (ret != AK_SUCCESS)
        {
            LOG_PRINTF("OS_EnterCritical failed. %d\r\n", ret);
        }
    }
    #else
    OS_MemSET(&rtos_tc_protect, 0, sizeof(rtos_tc_protect));
   TCT_Protect(&rtos_tc_protect);
    //return AK_Change_Priority(AK_GetCurrent_Task(), 1);

    #endif

    return 0;
}

OS_APIs void OS_ExitCritical(u32 val)
{
    s32 ret = 0;
    #if 1
    if (akcritSec != AK_INVALID_SEMAPHORE)
    {
        ret = AK_Release_Semaphore(akcritSec);
        if (ret != AK_SUCCESS)
        {
            LOG_PRINTF("OS_ExitCritical failed. %d\r\n", ret);
        }
    }
    #else

    //AK_Change_Priority(AK_GetCurrent_Task(), val);
    TCT_Unprotect_Specific(&rtos_tc_protect);
    #endif
}

static void dummy_task(u32 argc, void *argv)
{
    OsThread *pThread = (OsThread *)argv;

    if (pThread)
    {
        if (pThread->m_pTaskFunc)
        {
            pThread->m_pTaskFunc(pThread->m_pUserData);
        }
    }
    else
    {
        LOG_PRINTF("dummy_task: pThread Is NULL\r\n");
    }
}

/* Task: */
OS_APIs s32 OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{
    OsThread *pThread = AK_NULL;
         pThread = OS_MemAlloc(sizeof(OsThread));
    if (AK_NULL == pThread)
    {
        LOG_PRINTF("pThread Is NULL\r\n");
        return AK_NULL;
    }

        pThread->m_pStackAddr = OS_MemAlloc(stackSize);
    if (AK_NULL == pThread->m_pStackAddr)
    {
        LOG_PRINTF("m_pStackAddr Malloc Error\r\n");
        goto ErrorQuit;
    }

	OS_MemSET(pThread->m_pStackAddr, 0, stackSize);

    pThread->m_priority = pri;
    pThread->m_pUserData = param;
    pThread->m_pTaskFunc = task;

	pThread->m_hTask  = AK_Create_Task((T_VOID*)dummy_task, name,
                                   1 ,pThread, pThread->m_pStackAddr,
                                   stackSize,
                                   pThread->m_priority,
                                   2,
                                   AK_PREEMPT, AK_START);

    if (AK_IS_INVALIDHANDLE(pThread->m_hTask))
    {
		goto ErrorQuit;
    }

    if (taskHandle)
    {
        *taskHandle = pThread;
    }

    LOG_PRINTF("[%s] => %x pri[%d] stack[%p] size[%d]\r\n", name, pThread->m_hTask, pThread->m_priority, pThread->m_pStackAddr, stackSize);

    return OS_SUCCESS;

ErrorQuit:
    OS_TaskDelete(pThread);
    return OS_FAILED;
}

OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{
    OsThread *pThread = (OsThread *)taskHandle;

    if (pThread == NULL)
    {
        return;
    }

    if (AK_IS_VALIDHANDLE(pThread->m_hTask))
    {
        AK_Terminate_Task(pThread->m_hTask);
        AK_Delete_Task(pThread->m_hTask);
        pThread->m_hTask = AK_INVALID_TASK;
    }

    if (AK_NULL != pThread->m_pStackAddr)
    {
        OS_MemFree(pThread->m_pStackAddr);
    }

    OS_MemFree(pThread);

    return;
}



OS_APIs void OS_StartScheduler( void )
{

}

OS_APIs u32 OS_GetSysTick(void)
{
    u32 tick = get_tick_count();
    return (tick / TICK_RATE_MS) ? (tick / TICK_RATE_MS) : 1;
}


#define OS_MUTEX_INIT(mutex)        ((mutex) = AK_Create_Semaphore(1, AK_PRIORITY))
#define OS_MUTEX_IS_ERR(mutex)      (((mutex) <= 0) && ((mutex) > -100))
#define OS_MUTEX_IS_LOCKED(mutex)   (0 == AK_Get_SemVal(mutex))
#undef OS_MUTEX_LOCK
#undef OS_MUTEX_UNLOCK
#define OS_MUTEX_LOCK(mutex)        AK_Obtain_Semaphore((mutex), AK_SUSPEND)
#define OS_MUTEX_TRY_LOCK(mutex)    (0 < (AK_Try_Obtain_Semaphore((mutex), AK_SUSPEND)))
#define OS_MUTEX_UNLOCK(mutex)      AK_Release_Semaphore(mutex)
#define OS_MUTEX_DEINIT(mutex)      {\
	if (!(OS_MUTEX_IS_ERR(mutex)))\
	{\
		AK_Delete_Semaphore(mutex);\
		(mutex) = 0;\
	}\
}

/* Mutex APIs: */
OS_APIs s32 OS_MutexInit( OsMutex *mutex )
{
    T_hSemaphore sem_ak;

    OS_MUTEX_INIT(sem_ak);
    if (OS_MUTEX_IS_ERR(sem_ak))
    {
        *mutex = NULL;
		LOG_PRINTF("OS_MutexInit err=%d\r\n", OS_MUTEX_IS_ERR(sem_ak));
        return OS_FAILED;
    }
    else
    {
        *mutex = sem_ak;
        return OS_SUCCESS;
    }
}

OS_APIs void OS_MutexLock( OsMutex mutex )
{
    s32 err=0;
    if (OS_MUTEX_IS_ERR(mutex))
    {
        LOG_PRINTF("OS_MutexLock err=%d,mutex=%d\r\n", err,mutex);
        return;
    }

    err = OS_MUTEX_LOCK(mutex);
    if (err != AK_SUCCESS)
    {
        LOG_PRINTF("OS_SemWait err=%d,mutex=%d\r\n", err,mutex);
        return;
    }
    else
    {
        return;
    }
}

OS_APIs void OS_MutexUnLock( OsMutex mutex )
{
    s32 err;

    if (OS_MUTEX_IS_ERR(mutex))
    {
        LOG_PRINTF("OS_MutexUnLock err=%d\r\n", err);
        return;
    }

    err = OS_MUTEX_UNLOCK(mutex);
    if(err != AK_SUCCESS)
    {
        LOG_PRINTF("OS_SemSignal err=%d\r\n", err);
        return;
    }

    return;
}

OS_APIs void OS_MutexDelete( OsMutex mutex )
{
    OS_MUTEX_DEINIT(mutex);
}

OS_APIs void OS_MsDelay(u32 ms)
{
    u32 ticks = 0;
    ticks = ms/TICK_RATE_MS;
    if (ticks == 0)
    {
        ticks = 1;
    }
    AK_Sleep(ticks);
}

OS_APIs void OS_TickDelay(u32 ticks)
{
    AK_Sleep (ticks);
}

#define OS_SEM_INIT(sem,cnt)       ((sem) = AK_Create_Semaphore((cnt), AK_PRIORITY))
#define OS_SEM_IS_ERR(sem)         (((sem) <= 0) && ((sem) > -100))
#define OS_SEM_IS_EMPTY(sem)     (0 == AK_Get_SemVal(sem))
#define OS_SEM_WAIT(sem)            AK_Obtain_Semaphore((sem), AK_SUSPEND)
#define OS_SEM_WAIT_T(sem, time)    AK_Obtain_Semaphore((sem), time)
#define OS_SEM_TRY_WAIT(sem)        (0 < (AK_Try_Obtain_Semaphore((sem), AK_SUSPEND)))
#define OS_SEM_POST(sem)            AK_Release_Semaphore((sem))
#define OS_SEM_DEINIT(sem)      {\
	if (!(OS_SEM_IS_ERR(sem)))\
	{\
		AK_Delete_Semaphore(sem);\
		(sem) = 0;\
	}\
}

OS_APIs s32 OS_SemInit( OsSemaphore* Sem, u16 maxcnt, u16 cnt)
{
    T_hSemaphore sem_ak;

    OS_SEM_INIT(sem_ak, cnt);
    if (OS_SEM_IS_ERR(sem_ak))
    {
        *Sem = NULL;
        LOG_PRINTF("sem init error\r\n");
        return OS_FAILED;
    }
    else
    {
        *Sem = sem_ak;
        return OS_SUCCESS;
    }
}

OS_APIs bool OS_SemWait( OsSemaphore Sem , u16 timeout)
{
    s32 err;
    u32 time;

    if (OS_SEM_IS_ERR(Sem))
    {
        LOG_PRINTF("sem error\r\n");
        return OS_FAILED;
    }

    if (timeout == 0)
    {
        time = AK_SUSPEND;
    }
    else
    {
        time = timeout;
    }

    err = OS_SEM_WAIT_T((Sem), time);
    if (err != AK_SUCCESS)
    {
        //LOG_PRINTF("OS_SemWait Sem=%x err=%d, timeout=%x\r\n", Sem, err, timeout);
        return OS_FAILED;
    }
    else
    {
        return OS_SUCCESS;
    }
}

OS_APIs u8 OS_SemSignal( OsSemaphore Sem)
{
    s32 err;
    if (OS_SEM_IS_ERR(Sem))
    {
        LOG_PRINTF("sem error\r\n");
        return OS_FAILED;
    }

    err = OS_SEM_POST(Sem);
    if(err != AK_SUCCESS)
    {
        LOG_PRINTF("OS_SemSignal err=%d\r\n", err);
        return OS_FAILED;
    }

	return OS_SUCCESS;
}

OS_APIs u32 OS_SemCntQuery( OsSemaphore Sem)
{
    return 0;
}

OS_APIs u8 OS_SemSignal_FromISR( OsSemaphore Sem)
{
	return OS_SemSignal(Sem);
}

OS_APIs void OS_SemDelete(OsSemaphore Sem)
{
    OS_SEM_DEINIT(Sem);
}


/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen )
{
    OsMessgQ *pMsgQue = AK_NULL;

	pMsgQue = OS_MemAlloc(sizeof(OsMessgQ));
    if (AK_NULL == pMsgQue)
	{
		LOG_PRINTF("MsgQue Is NULL\r\n");
		return AK_NULL;
	}

	pMsgQue->m_msgSize = sizeof( OsMsgQEntry );
	pMsgQue->m_queNum = QLen * pMsgQue->m_msgSize;

    // get message queen size(byte)
	pMsgQue->m_pQueueAddr = OS_MemAlloc(pMsgQue->m_queNum);
	if (AK_NULL == pMsgQue->m_pQueueAddr)
	{
		LOG_PRINTF("m_pQueueAddr Malloc Error\r\n");
		goto ErrorQuit;
	}

	pMsgQue->m_hQueue = AK_Create_Queue(pMsgQue->m_pQueueAddr, pMsgQue->m_queNum, AK_FIXED_SIZE, \
                                           pMsgQue->m_msgSize, AK_PRIORITY);

	if (AK_IS_INVALIDHANDLE(pMsgQue->m_hQueue))
	{
        LOG_PRINTF("pMsgQue->m_hQueue = %d QLen = %d\r\n", pMsgQue->m_hQueue, QLen);
		goto ErrorQuit;
	}

    *MsgQ = pMsgQue;

    return OS_SUCCESS;

ErrorQuit:
	OS_MsgQDelete(pMsgQue);
    return OS_FAILED;
}

OS_APIs s32 OS_MsgQDelete( OsMsgQ MsgQ)
{
	if (AK_NULL != MsgQ)
	{
		if (AK_IS_VALIDHANDLE(MsgQ->m_hQueue))
		{
			AK_Delete_Queue(MsgQ->m_hQueue);
			MsgQ->m_hQueue = AK_INVALID_QUEUE;
		}

		if (AK_NULL != MsgQ->m_pQueueAddr)
		{
			OS_MemFree(MsgQ->m_pQueueAddr);
			MsgQ->m_pQueueAddr = AK_NULL;
		}

        OS_MemFree(MsgQ);

        return OS_SUCCESS;
	}

    return OS_FAILED;
}

OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
	s32 err = -1;

	if ((AK_NULL != MsgQ) && (AK_IS_VALIDHANDLE(MsgQ->m_hQueue)))
	{
		err = AK_Send_To_Queue(MsgQ->m_hQueue,
								(T_VOID *)MsgItem, MsgQ->m_msgSize,
								AK_SUSPEND);

		if (AK_SUCCESS != err)
		{
            //if (err != AK_QUEUE_FULL)
			    LOG_PRINTF("OS_MsgQEnqueue ret = %d, task[%x]\r\n", err, AK_GetCurrent_Task());//AK_Send_To_Queue
			return OS_FAILED;
		}
	}

	return (AK_SUCCESS == err) ? OS_SUCCESS : OS_FAILED;
}


OS_APIs s32 OS_MsgQEnqueueTry( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    s32 err = -1;

    if ((AK_NULL != MsgQ) && (AK_IS_VALIDHANDLE(MsgQ->m_hQueue)))
    {
        err = AK_Send_To_Queue(MsgQ->m_hQueue,
                                (T_VOID *)MsgItem, MsgQ->m_msgSize,
                                AK_NO_SUSPEND);

        if (AK_SUCCESS != err)
        {
            if (err != AK_QUEUE_FULL)
                LOG_PRINTF("OS_MsgQEnqueue ret = %d\r\n", err);//AK_Send_To_Queue
            return OS_FAILED;
        }
    }

    return (AK_SUCCESS == err) ? OS_SUCCESS : OS_FAILED;
}


OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, u32 timeOut, bool fromISR )
{
	s32 err = -1;
	u32 actual_size = 0;
    u32 suspend = AK_SUSPEND;

    if (timeOut == 0)
    {
        suspend = AK_SUSPEND;
    }
    else
    {
        suspend = timeOut;
    }

	if ((AK_NULL != MsgQ) && (AK_IS_VALIDHANDLE(MsgQ->m_hQueue)))
	{
		err = AK_Receive_From_Queue(MsgQ->m_hQueue,
								     MsgItem, MsgQ->m_msgSize,
								     &actual_size, suspend);

		if (AK_SUCCESS != err)
		{
			//LOG_PRINTF("RecMsg ret = %d\n",err);
            return OS_FAILED;
		}
	}

    return (AK_SUCCESS == err) ? OS_SUCCESS : OS_FAILED;
}

OS_APIs s32 OS_MsgQDequeueTry( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
	s32 err = -1;
	u32 actual_size = 0;
    u32 suspend = 0;

	if ((AK_NULL != MsgQ) && (AK_IS_VALIDHANDLE(MsgQ->m_hQueue)))
	{
		err = AK_Receive_From_Queue(MsgQ->m_hQueue,
								     MsgItem, MsgQ->m_msgSize,
								     &actual_size, suspend);

		if (AK_SUCCESS != err)
		{
			//LOG_PRINTF("RecMsg ret = %d\n",err);
            return OS_FAILED;
		}
	}

    return (AK_SUCCESS == err) ? OS_SUCCESS : OS_FAILED;
}

OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ )
{
    return 0;
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

