/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_H_
#define _OS_H_

#include <os_def.h>
#include <porting.h>
#include "host_config.h"

#undef MEMCPY

#define OS_MemFree(ptr) if((ptr)==NULL){ \
        LOG_PRINTF("Error %s @line %d free null pointer\r\n",__func__,__LINE__); \
    }else{ \
        __OS_MemFree((ptr)); \
    }
#define PRINTF LOG_PRINTF
#define OS_MUTEX_LOCK(x) { \
    OS_MutexLock(x); \
}

#define OS_MUTEX_UNLOCK(x) { \
    OS_MutexUnLock(x); \
}

#ifdef __MTRACE_CHECK__
#define OS_MemAlloc malloc
#define MALLOC OS_MemAlloc
#undef OS_MemFree
#define OS_MemFree free
#define FREE   OS_MemFree
#else//#ifdef __MTRACE_CHECK__
#define MALLOC OS_MemAlloc
#define FREE   OS_MemFree
#endif//#ifdef __MTRACE_CHECK__


#define STRRPOS    		ssv6xxx_strrpos
#define ISALPHA    		ssv6xxx_isalpha
#define STR_TOUPPER    	ssv6xxx_str_toupper
#define STR_TOLOWER    	ssv6xxx_str_tolower
#define ATOI_BASE    	ssv6xxx_atoi_base
#define ATOI    		ssv6xxx_atoi

#define TOUPPER 		ssv6xxx_toupper
#define TOLOWER 		ssv6xxx_tolower
#define ISUPPER 		ssv6xxx_isupper
#define STRCMP  		ssv6xxx_strcmp
#define STRNCMP 		ssv6xxx_strncmp
#define STRCAT  		ssv6xxx_strcat
#define STRNCAT 		ssv6xxx_strncat
#define STRCPY 			ssv6xxx_strcpy
#define STRNCPY 		ssv6xxx_strncpy
#define STRLEN  		ssv6xxx_strlen
#define STRCHR  		ssv6xxx_strchr
#define MEMSET 			OS_MemSET //ssv6xxx_memset
#define MEMCPY 			OS_MemCPY //ssv6xxx_memcpy
#define MEMCMP 			ssv6xxx_memcmp

#ifdef __linux__
#define SPRINTF 		sprintf
#endif


/* Define Task Priority: 0 is the lowest priority */
#define OS_CMD_ENG_PRIO         CMD_ENG_PRIORITY	//20
#define OS_RX_TASK_PRIO         WIFI_RX_PRIORITY	//21
#define OS_TX_TASK_PRIO         WIFI_TX_PRIORITY	//22
#define OS_TCP_TASK_PRIO        TCPIP_PRIORITY		//23
#define OS_NETAPP_TASK0_PRIO    NETAPP_PRIORITY		//25
#define OS_NETAPP_TASK1_PRIO    NETAPP_PRIORITY_1	//26
#define OS_NETAPP_TASK2_PRIO    NETAPP_PRIORITY_2	//27
#define OS_NETAPP_TASK3_PRIO    NETAPP_PRIORITY_3	//28
#define OS_NETAPP_TASK4_PRIO    NETAPP_PRIORITY_4	//29
#define OS_DHCPD_TASK_PRIO      DHCPD_PRIORITY		//24
#define OS_NETMGR_TASK_PRIO     NETMGR_PRIORITY		//30
#define OS_TMR_TASK_PRIO        TMR_TASK_PRIORITY	//18
#define OS_MLME_TASK_PRIO       MLME_TASK_PRIORITY  //19
#define OS_RX_ISR_PRIO          RX_ISR_PRIORITY		// 0

//#define OS_NETAPP_TASK3_PRIO    NETAPP_PRIORITY+3

//#define OS_TASK_END_PRIO        OS_NETAPP_TASK2_PRIO

#define OS_CLI_PRIO             TASK_END_PRIO		//33

#ifdef FGTEST_STACK_CHECK_VALUE
#define RTOS_STACK_CHECK_VALUE FGTEST_STACK_CHECK_VALUE
#else
#define RTOS_STACK_CHECK_VALUE 0x0
#endif

/* Define OS error values */
#define OS_SUCCESS                  0
#define OS_FAILED                   1

#define OS_MS2TICK(_ms)             ( (_ms)/TICK_RATE_MS )


/* Message Commands: */
#define OS_MSG_FRAME_TRAPPED        1

//#define OS_TICKTYPE portTickType //freertos
#define OS_TICKTYPE unsigned long
//#define OS_TICK_RATE_MS (1000/OS_TICKS_PER_SEC) //freertos:portTICK_RATE_MS
#define OS_TICK_RATE_MS TICK_RATE_MS
#define OS_TICK_HZ     (1000/OS_TICK_RATE_MS) 
#define OS_MIN_STK_SIZE  TASK_IDLE_STK_SIZE //freerots:configMINIMAL_STACK_SIZE


typedef struct OsMsgQEntry_st
{
	u32         MsgCmd;
    void        *MsgData;
    
} OsMsgQEntry, *POsMsgQEntry;


typedef struct task_info_st 
{
    const char   *task_name;
    OsMsgQ      qevt;
    u8          qlength;
    u8          prio;
    u8          stack_size;  /* unit: 16 */
    void        *args;
    TASK_FUNC   task_func; 
    
} task_info;


#if 1// (CONFIG_HOST_PLATFORM == 1)

#define OS_INTR_DISABLE()
#define OS_INTR_ENABLE()
#define OS_INTR_MAY_DISABLE()
#define OS_INTR_MAY_ENABLE()

#else

#define OS_INTR_DISABLE()         \
    asm volatile (                \
    "STMDB  SP!, {R0}       \n\t" \
    "MRS    R0, CPSR        \n\t" \
    "ORR    R0, R0, #0xC0   \n\t" \
    "MSR    CPSR, R0        \n\t" \
    "LDMIA  SP!, {R0}           " \
    )

#define OS_INTR_ENABLE()          \
    asm volatile (                \
    "STMDB  SP!, {R0}       \n\t" \
    "MRS    R0, CPSR        \n\t" \
    "BIC    R0, R0, #0xC0   \n\t" \
    "MSR    CPSR, R0        \n\t" \
    "LDMIA  SP!, {R0}           " \
    )

#define OS_INTR_MAY_DISABLE()     \
    if (gOsFromISR == false)      \
        OS_INTR_DISABLE()

#define OS_INTR_MAY_ENABLE()      \
    if (gOsFromISR == false)      \
        OS_INTR_ENABLE()          \

#endif


/**
 *  Flag to indicate whether ISR handler is running or not.
 */
extern volatile u8 gOsFromISR;




OS_APIs s32 OS_Init( void );
OS_APIs unsigned long OS_Random(void);
OS_APIs u32 OS_GetSysTick(void);

OS_APIs u32 OS_EnterCritical(void);
OS_APIs void OS_ExitCritical(u32 val);


/* Task: */
OS_APIs s32  OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle );
OS_APIs void OS_TaskDelete( OsTaskHandle taskHandle );
OS_APIs void OS_StartScheduler( void );
OS_APIs void OS_Terminate( void );



/* Mutex: */
OS_APIs s32  OS_MutexInit( OsMutex *mutex );
OS_APIs void OS_MutexLock( OsMutex mutex );
OS_APIs void OS_MutexUnLock( OsMutex mutex );
OS_APIs void OS_MutexDelete( OsMutex mutex );

/*semaphore*/
OS_APIs s32 OS_SemInit( OsSemaphore* Sem , u16 maxcnt , u16 cnt);
OS_APIs bool OS_SemWait( OsSemaphore Sem , u16 timeout);
OS_APIs u8 OS_SemSignal( OsSemaphore Sem);
OS_APIs u8 OS_SemSignal_FromISR( OsSemaphore Sem);
OS_APIs void OS_SemDelete(OsSemaphore Sem);
OS_APIs u32 OS_SemCntQuery( OsSemaphore Sem);

/* Delay: */
OS_APIs void OS_MsDelay(u32 ms);
OS_APIs void OS_TickDelay(u32 ticks);


/* Timer: */
OS_APIs s32 OS_TimerCreate( OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler );
OS_APIs s32 OS_TimerSet( OsTimer timer, u32 ms, u8 autoReload, void *args );
OS_APIs s32 OS_TimerStart( OsTimer timer );
OS_APIs s32 OS_TimerStop( OsTimer timer );
OS_APIs void *OS_TimerGetData( OsTimer timer );

//OS_APIs void OS_TimerGetSetting( OsTimer timer, u8 *autoReload, void **args );
//OS_APIs bool OS_TimerIsRunning( OsTimer timer );



/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen );
OS_APIs s32 OS_MsgQDelete( OsMsgQ MsgQ);
OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR );
OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, u32 timeOut, bool fromISR );
OS_APIs s32 OS_MsgQEnqueueTry( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR );
OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ );
#if 0
OS_APIs void *OS_MsgAlloc( void );
OS_APIs void OS_MsgFree( void *Msg );
#endif

/* Profiling: */
//#ifdef __SSV_UNIX_SIM__
#if 0 //(__SSV_UNIX_SIM__ == 1)
OS_APIs s32 OS_SysProfiling( void *ptr);
#endif
#endif /* _RTOS_H_ */

