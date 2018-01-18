/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
//#include <semaphore.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/timer.h>

#include <linux/kthread.h>
#include <linux/timer.h>

#include "os.h"
/*TOSO: aaron */
//#include "os_cfg.h"
//#include <log.h>



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

spinlock_t g_os_cs_lock;
unsigned long g_os_cpu_flags;

wait_queue_head_t g_os_task_wait_q;

OS_APIs s32 OS_Init( void )
{
	spin_lock_init(&g_os_cs_lock);
	init_waitqueue_head(&g_os_task_wait_q);
    return OS_SUCCESS;
}

OS_APIs unsigned long OS_Random(void)
{
	unsigned long randNum = 0;
	get_random_bytes(&randNum, sizeof(unsigned long));
    return randNum;
}

OS_APIs void OS_Terminate(void)
{

}

OS_APIs u32 OS_EnterCritical(void)
{
	spin_lock_irqsave(&g_os_cs_lock, g_os_cpu_flags);
    return OS_SUCCESS;
}

OS_APIs void OS_ExitCritical(u32 val)
{
	spin_unlock_irqrestore(&g_os_cs_lock, g_os_cpu_flags);
}

/* Task: */
OS_APIs s32 OS_TaskCreate(OsTask task, const char *name, u32 stackSize, 
					void *param, u32 pri, OsTaskHandle *taskHandle)
{
	/* TODO: aaron 
	 * For now, task create the sys will hit two kind of kernel panic :
	 * 1. the null pointer of task function (ex: tx/rx)
     * 	  the task function need to review and re-program. temply mark it.
     * 2. for workaround step 1, the task will block too long to make kernel 
     *    report this as issue. temply mark the api content.				
	 */
#if 0
	struct task_struct *new_task = kthread_create(task, param, name);
	if (IS_ERR(new_task))
	{
		return OS_FAILED;
	}
	wake_up_process(new_task);
#endif
	return OS_SUCCESS;
}


OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{
	kthread_stop((struct task_struct *)(taskHandle));
}


OS_APIs void OS_StartScheduler(void)
{

}

OS_APIs u32 OS_GetSysTick(void)
{
    return OS_SUCCESS;
}


/* Mutex APIs: */
OS_APIs s32 OS_MutexInit(OsMutex *mutex)
{
//	struct mutex *lock = (struct mutex *)mutex;
	*mutex = kzalloc(sizeof(struct mutex), GFP_KERNEL);
	mutex_init(*mutex);
    return OS_SUCCESS;
}

OS_APIs void OS_MutexLock(OsMutex *mutex)
{
	struct mutex *lock = (struct mutex *)mutex;
	mutex_lock(lock);
}

OS_APIs void OS_MutexUnLock(OsMutex *mutex)
{
	struct mutex *lock = (struct mutex *)mutex;
	mutex_unlock(lock);
}

OS_APIs void OS_TickDelay(u32 ticks)
{

}


OS_APIs void OS_MutexDelete(OsMutex *mutex)
{

}


OS_APIs void OS_MsDelay(u32 ms)
{
	msleep(ms);
}

#define SEMA_LOCK()		OSSchedLock()
#define SEMA_UNLOCK()		OSSchedUnlock()

OS_APIs s32 OS_SemInit(OsSemaphore *Sem, u16 maxcnt, u16 cnt)
{
//	struct semaphore *sem = (struct semaphore *)Sem;
	*Sem = kzalloc(sizeof (struct semaphore), GFP_KERNEL);
	sema_init(*Sem, maxcnt);
	return OS_SUCCESS;
}

OS_APIs bool OS_SemWait(OsSemaphore Sem , u16 timeout)
{
//	struct semaphore *sem = (struct semaphore *)Sem;
//	down(sem);
	return down_timeout(Sem, timeout);
//    return OS_SUCCESS;
//    return OS_SUCCESS;
}

OS_APIs u8 OS_SemSignal(OsSemaphore Sem)
{
	//struct semaphore *sem = (struct semaphore *)Sem;

	up(Sem);

    return OS_SUCCESS;
}

OS_APIs u32 OS_SemCntQuery(OsSemaphore Sem)
{
    return OS_SUCCESS;
}

OS_APIs u8 OS_SemSignal_FromISR(OsSemaphore Sem)
{
    return OS_SUCCESS;
}

OS_APIs void OS_SemDelete(OsSemaphore *Sem)
{
//	struct semaphore *sem = (struct semaphore *)Sem;
//	sem_destroy(sem);
	kfree(Sem);
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
OS_APIs s32 OS_TimerCreate(OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler)
{    
	return OS_SUCCESS;
}

OS_APIs s32 OS_TimerSet(OsTimer timer, u32 ms, u8 autoReload, void *args)
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_TimerStart(OsTimer timer)
{
    return OS_SUCCESS;
}

OS_APIs s32 OS_TimerStop(OsTimer timer)
{
    return OS_SUCCESS;
}

OS_APIs void *OS_TimerGetData(OsTimer timer)
{
   return NULL;
}

