#include "config.h"
#include "rtos.h"
#include <log.h>
#include <fcntl.h>
#include <errno.h>


volatile u8 gOsFromISR;
#define MSG_QUEUE_PRI  MQ_PRIO_MAX


OS_APIs s32  OS_Init( void )
{
    gOsFromISR = 0;

    return OS_SUCCESS;
}

OS_APIs unsigned long OS_Random(void)
{
	return clock_systimer()%65536+54*18;
}


OS_APIs void OS_Terminate( void )
{
}

OS_APIs u32 OS_EnterCritical(void)
{
    irqstate_t   flags;
	flags = irqsave(); /* No interrupts */
	
    sched_lock();      /* No context switches */
    	
    return flags;
}

OS_APIs void OS_ExitCritical(u32 val)
{
	sched_unlock();
    irqrestore(val);
}

static int dummy_task(u32 argc, char *argv[])
{
    char *str = argv[1];
    OsThread *pThread = (OsThread *)ssv6xxx_atoi_base(str, 16);

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

#if 1

/* Task: */
OS_APIs s32 OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{	
    
    const char *argv[2];
    OsThread *pThread = NULL;
    char str[16];
    pThread = OS_MemAlloc(sizeof(OsThread));
    if (NULL == pThread)
    {
        LOG_PRINTF("pThread Is NULL\r\n");
        return OS_FAILED;
    }
    
    pThread->m_pUserData = param;
    pThread->m_pTaskFunc = task;
    sprintf(str,"%x",(unsigned int)pThread);
    argv[0] = str;
    argv[1] = NULL;
	pThread->m_hTask = task_create(name, pri, stackSize, (main_t)dummy_task, (char *)argv);

	if(pThread->m_hTask > 0)
	{
        if (taskHandle)
        {
            *taskHandle = pThread;
        }
        
        LOG_PRINTF("task[%s] => pid (%d)\r\n", name, pThread->m_hTask);
        
        return OS_SUCCESS;
	}
        
ErrorQuit:
    OS_TaskDelete(pThread);
    return OS_FAILED;

}

#else

/* Task: */
OS_APIs s32 OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{	
	
	pthread_attr_t attr;
	pthread_t pthread;

	struct sched_param sparam;

	int status;

	status = pthread_attr_init(&attr);
	 if (status != 0)
    {
      printf("11111111111%d\n", status);
    }
	
	sparam.sched_priority = pri;
	
  	status = pthread_attr_setschedparam(&attr,&sparam);
	 if (status != 0)
    {
      printf("22222222222222%d\n", status);
    }

	status = pthread_attr_setstacksize(&attr, 2048);
	 if (status != 0)
    {
      printf("333333333333%d\n", status);
    }

	status = pthread_create(&pthread, &attr, task, param);
	 if (status != 0)
    {
      printf("44444444444444444=%d\n", status);
    }

	 
      //(void)pthread_detach(pthread);

      /* Name the thread */

      pthread_setname_np(pthread, name);

	if(status == 0)
	{    
		//if(taskHandle)
		//*taskHandle = pthread;
        LOG_PRINTF("task[%s] => pid (%d)\r\n", name, *taskHandle);
        
        return OS_SUCCESS;
	}
        
ErrorQuit:
    
    return OS_FAILED;

}

#endif

OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{
	OsThread *pThread = (OsThread *)taskHandle;
    
    if (pThread == NULL)
    {
        return;
    }
	task_delete(pThread->m_hTask);

	OS_MemFree(pThread);

}

OS_APIs void OS_StartScheduler( void )
{
	
}

OS_APIs u32 OS_GetSysTick(void)
{  
	return clock_systimer();
}


/* Mutex APIs: */
OS_APIs s32 OS_MutexInit( OsMutex *mutex )
{
	s32 err;
	pthread_mutex_t *p_mutex = NULL;
#if 1	
	p_mutex = OS_MemAlloc(sizeof(pthread_mutex_t));
    if (NULL == p_mutex)
	{
		LOG_PRINTF("OsMutex Is NULL\r\n");
		return OS_FAILED;
	}
#endif	
	//printf(">>>>>>%x\r\n",p_mutex);
	
	//err = sem_init(*mutex,0,1);
	err = pthread_mutex_init(p_mutex,NULL);
	
	if ( -1 == err ){
		LOG_PRINTF("mutex fail!!!!\r\n");
        return OS_FAILED;
    }
    else{
		*mutex = p_mutex;
        return OS_SUCCESS;
    }
}


OS_APIs void OS_MutexDelete( OsMutex mutex )
{
	//sem_destroy(mutex);
	pthread_mutex_destroy(mutex);
    OS_MemFree(mutex);
}


OS_APIs void OS_MutexLock( OsMutex mutex )
{
	//sem_wait(mutex);
	pthread_mutex_lock(mutex);
}

OS_APIs void OS_MutexUnLock( OsMutex mutex )
{
	//sem_post(mutex);
	pthread_mutex_unlock(mutex);
}


OS_APIs void OS_MsDelay(u32 ms)
{
	struct timespec requested, remaining;

	if(ms >= 1000)
	{
		requested.tv_sec  = ms / 1000;
		requested.tv_nsec = (ms - 1000 * requested.tv_sec) * 1000 *1000L;
	}
	else
	{
	    requested.tv_sec  = 0;
	    requested.tv_nsec = ms * 1000 * 1000L;
	}

    while (nanosleep(&requested, &remaining) == -1)
        if (errno == EINTR)
            requested = remaining;
        else {
           
            break;
        }
}

OS_APIs void OS_TickDelay(u32 ticks)
{
    OS_MsDelay(TICK_RATE_MS*ticks);
}

/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen )
{
	struct mq_attr attr;
	static int num = 0;
	char msg_str[16];
	OsMessgQ *pmsg = NULL;

	sprintf(msg_str,"OsMsgQ_%d",num);

	attr.mq_maxmsg = QLen;
	attr.mq_msgsize = sizeof( OsMsgQEntry );
	attr.mq_flags = 0;
	attr.mq_curmsgs = 0;

	pmsg = OS_MemAlloc(sizeof(OsMessgQ));
    if (NULL == pmsg)
	{
		LOG_ERROR("MsgQue Is NULL\r\n");
		return OS_FAILED;
	}

	pmsg->m_msgSize = sizeof( OsMsgQEntry );
	
	pmsg->m_hQueue = mq_open(msg_str, O_RDWR|O_CREAT,0666,&attr);
	
	//LOG_PRINTF("queue[%s] => qid (%x) pid (%d)\r\n", msg_str, (*MsgQ)->m_hQueue, getpid());

	if(NULL == pmsg->m_hQueue){        
        LOG_PRINTF("mq_open: failed\n");
		OS_MemFree(pmsg);
		return OS_FAILED;
	}
	else{
		num++;
		*MsgQ = pmsg;
		return OS_SUCCESS;
	}
}


OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
	s32 err;
	
    err = mq_send(MsgQ->m_hQueue,MsgItem,MsgQ->m_msgSize,MSG_QUEUE_PRI);
    if (err != 0)
    {
        LOG_PRINTF("OS_MsgQEnqueue: err=%d\n", err);
    }
    
	return ( 0!=err )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueueTry( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
	s32 err;

	struct timespec ts;
	int status = clock_gettime(CLOCK_REALTIME, &ts);
	if (status != 0)
	{
		LOG_PRINTF("OS_MsgQEnqueueTry: ERROR clock_gettime failed\n");
	}
	ts.tv_nsec += TICK_RATE_MS * 1000 *1000;
    if ((ts.tv_nsec >= 1000000000))
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    
    err = mq_timedsend(MsgQ->m_hQueue,MsgItem,MsgQ->m_msgSize,MSG_QUEUE_PRI,&ts);
    if (err != 0)
    {
        if (get_errno() != ETIMEDOUT)
        {
            LOG_PRINTF("OS_MsgQEnqueueTry: err=%d errno=%d\n", err, get_errno());
        }
    }
    
	return ( 0!=err )? OS_FAILED: OS_SUCCESS;
}


OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, u32 timeout,bool fromISR )
{
	s32 err;
    int pri;

	//LOG_PRINTF("pid (%x)\r\n", MsgQ->m_hQueue);
    
	if(timeout == 0)
	{
		err = mq_receive(MsgQ->m_hQueue,MsgItem,MsgQ->m_msgSize, &pri);
	}
	else
	{
		struct timespec ts;
		int status = clock_gettime(CLOCK_REALTIME, &ts);
		if (status != 0)
		{
			LOG_PRINTF("OS_MsgQDequeue: ERROR clock_gettime failed\n");
		}
        
        if (timeout * TICK_RATE_MS >= 1000)
        {
            ts.tv_sec += (timeout * TICK_RATE_MS) / 1000;
            timeout -= (timeout * TICK_RATE_MS) / 1000 * 1000 / TICK_RATE_MS;
        }
		ts.tv_nsec += (timeout * TICK_RATE_MS * 1000 * 1000) ;
        if ((ts.tv_nsec >= 1000000000))
        {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
		
		err = mq_timedreceive(MsgQ->m_hQueue,MsgItem,MsgQ->m_msgSize,&pri,&ts);
	}
    if (err < 0)
    {
        if (get_errno() != ETIMEDOUT)
        {
            LOG_PRINTF("OS_MsgQDequeue: err=%d errno=%d\n", err, get_errno());
        }
    }
	return (err < 0)? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDelete( OsMsgQ MsgQ)
{
	s32 err;

	//LOG_PRINTF("dequeue[] => qid (%x) current pid (%d)  delete pid (%d)\r\n", MsgQ->m_hQueue, getpid(),MsgQ->pid);
	
	err = mq_close(MsgQ->m_hQueue);
	OS_MemFree(MsgQ);
    if ( 0!=err )
    {
        LOG_PRINTF("OS_MsgQDelete: err=%d\n", err);
    }

	return ( 0!=err )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_SemInit( OsSemaphore* Sem , u16 maxcnt , u16 cnt)
{
	s32 err;
	sem_t *psem = NULL;
#if 1
	psem = OS_MemAlloc(sizeof(sem_t));
    if (NULL == psem)
	{
		LOG_ERROR("OsSemaphore Is NULL\r\n");
		return OS_FAILED;
	}
#endif	
	err = sem_init(psem,0,cnt);
	
	if ( 0!=err ){
		LOG_ERROR("sem fail!!!!\r\n");
        return OS_FAILED;
    }
    else{
		*Sem = psem;
        return OS_SUCCESS;
    }
}

OS_APIs bool OS_SemWait( OsSemaphore Sem , u16 timeout)
{
	struct timespec ts;
	s32 err;

    if(timeout)
    {
        int status;
    	status = clock_gettime(CLOCK_REALTIME, &ts);
    	if (status != 0)
    	{
    		LOG_PRINTF("OS_SemWait: ERROR clock_gettime failed\n");
    	}
        if (timeout * TICK_RATE_MS >= 1000)
        {
            ts.tv_sec += (timeout * TICK_RATE_MS) / 1000;
            timeout -= (timeout * TICK_RATE_MS) / 1000 * 1000 / TICK_RATE_MS;
        }
		ts.tv_nsec += (timeout * TICK_RATE_MS * 1000 * 1000) ;
        if ((ts.tv_nsec >= 1000000000))
        {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
        
        err = sem_timedwait(Sem, &ts);
    }
    else
    {
        err = sem_wait(Sem);
    }
	
	return ( 0!=err )? OS_FAILED: OS_SUCCESS;
}

OS_APIs u8 OS_SemSignal( OsSemaphore Sem)
{
	return ((sem_post(Sem) == OK) ? OS_SUCCESS:OS_FAILED);
}

OS_APIs void OS_SemDelete(OsSemaphore Sem)
{
	sem_destroy(Sem);
	OS_MemFree(Sem);
}

OS_APIs u32 OS_SemCntQuery( OsSemaphore Sem)
{
    return 0;
}

OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ )
{
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
    return OS_SUCCESS;
}

OS_APIs u8 OS_SemSignal_FromISR( OsSemaphore Sem)
{
	return OS_SemSignal(Sem);
}

OS_APIs s32 OS_SysProfiling(void *pTextBuf)
{
    return OS_SUCCESS;
}


