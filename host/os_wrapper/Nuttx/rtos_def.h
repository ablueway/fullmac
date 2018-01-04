#ifndef _RTOS_DEF_H_
#define _RTOS_DEF_H_
#include <pthread.h>
#include <mqueue.h>

/*============OS parameter setting===================*/
typedef void (*OsTask)(void *);

typedef struct OsMsgQ_st
{
    mqd_t	m_hQueue;// msg que handle
	size_t  m_msgSize;// msg que atomic size
}OsMessgQ;

typedef struct tagCSubThread
{
    int	         m_hTask;
    void*		 m_pUserData;       //Point to the user data which will be used as a param by m_fnEntry.
    OsTask       m_pTaskFunc; 
}OsThread;

typedef void*		    OsTaskHandle;
typedef pthread_mutex_t*	OsMutex;
typedef sem_t*			OsSemaphore;
typedef OsMessgQ*				OsMsgQ;
typedef int                OsTimer;
typedef int           OsTimerHandler;



#define OS_TASK_ENTER_CRITICAL()        irqstate_t   nuttx_flags; nuttx_flags = OS_EnterCritical()
                                        

#define OS_TASK_EXIT_CRITICAL()         OS_ExitCritical(nuttx_flags)


#endif
