/********************************************************************************************************
*                                               uC/OS-II
*                                         The Real-Time Kernel
*
*
*                             (c) Copyright 1992-2006, Micrium, Weston, FL
*                                          All Rights Reserved
*
*                                           Generic ARM Port
*
* File      : OS_CPU_C.C
* Version   : V1.70
* By        : Jean J. Labrosse
*
* For       : ARM7 or ARM9
* Mode      : ARM or Thumb
* Toolchain : ADS
********************************************************************************************************/

#include "ucos_ii.h"

/********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*                                            (BEGINNING)
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts should be disabled during this call.
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION > 203
void  OSInitHookBegin (void)
{
}
#endif

/********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*                                               (END)
*
* Description: This function is called by OSInit() at the end of OSInit().
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts should be disabled during this call.
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION > 203
void  OSInitHookEnd (void)
{
}
#endif

/********************************************************************************************************
*                                          TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : ptcb   is a pointer to the task control block of the task being created.
*
* Note(s)    : 1) Interrupts are disabled during this call.
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0
void  OSTaskCreateHook (OS_TCB *ptcb)
{
    (void) ptcb;                        // Prevent compiler warning
}
#endif

/********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)    : 1) Interrupts are disabled during this call.
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0
void  OSTaskDelHook (OS_TCB *ptcb)
{
    (void) ptcb;                        // Prevent compiler warning
}
#endif

/********************************************************************************************************
*                                             IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are enabled during this call.
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION >= 251
void  OSTaskIdleHook (void)
{
}
#endif

/********************************************************************************************************
*                                           STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-II's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : none
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0
extern void TaskStatHook(void);
void  OSTaskStatHook (void)
{
    TaskStatHook();
}
#endif

/********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* Arguments  : task          is a pointer to the task code
*
*              p_arg         is a pointer to a user supplied data area that will be passed to the task
*                            when the task first executes.
*
*              ptos          is a pointer to the top of stack.  It is assumed that 'ptos' points to
*                            a 'free' entry on the task stack.  If OS_STK_GROWTH is set to 1 then
*                            'ptos' will contain the HIGHEST valid address of the stack.  Similarly, if
*                            OS_STK_GROWTH is set to 0, the 'ptos' will contains the LOWEST valid address
*                            of the stack.
*
*              opt           specifies options that can be used to alter the behavior of OSTaskStkInit().
*                            (see uCOS_II.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when your task starts executing.
*              2) All tasks run in SVC mode.
********************************************************************************************************/
OS_STK * OSTaskStkInit(void (*task)(void *p_arg), void *p_arg, OS_STK *ptos, INT16U opt)
{
    OS_STK	*stk;

    opt = opt;                        		// 'opt' is not used, prevent warning

    stk = ptos;                       		// Load stack pointer
    *(stk) = (INT32U) task;         		// PC: Entry point
    *(--stk) = (INT32U) 0xEEEEEEEEL;       	// R14 (LR)
    *(--stk) = (INT32U) 0xCCCCCCCCL;       	// R12
    *(--stk) = (INT32U) 0xBBBBBBBBL;       	// R11
    *(--stk) = (INT32U) 0xAAAAAAAAL;       	// R10
    *(--stk) = (INT32U) 0x99999999L;       	// R9
    *(--stk) = (INT32U) 0x88888888L;       	// R8
    *(--stk) = (INT32U) 0x77777777L;       	// R7
    *(--stk) = (INT32U) 0x66666666L;       	// R6
    *(--stk) = (INT32U) 0x55555555L;       	// R5
    *(--stk) = (INT32U) 0x44444444L;       	// R4
    *(--stk) = (INT32U) 0x33333333L;       	// R3
    *(--stk) = (INT32U) 0x22222222L;       	// R2
    *(--stk) = (INT32U) 0x11111111L;       	// R1
    *(--stk) = (INT32U) p_arg;             	// R0: Argument
    *(--stk) = (INT32U) 0x13;   			// CPSR: Enable IRQ and FIQ interrupts, ARM instruction set, SVC mode

    return (stk);
}

/********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                 will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                 task being switched out (i.e. the preempted task).
********************************************************************************************************/
#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0)
extern void TaskSwHook(void);
void  OSTaskSwHook (void)
{
    TaskSwHook();
}
#endif

/********************************************************************************************************
*                                           OS_TCBInit() HOOK
*
* Description: This function is called by OS_TCBInit() after setting up most of the TCB.
*
* Arguments  : ptcb    is a pointer to the TCB of the task being created.
*
* Note(s)    : 1) Interrupts may or may not be ENABLED during this call.
********************************************************************************************************/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION > 203
void  OSTCBInitHook (OS_TCB *ptcb)
{
    (void) ptcb;                            // Prevent Compiler warning
}
#endif


/********************************************************************************************************
*                                               TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts may or may not be ENABLED during this call.
********************************************************************************************************/
#if (OS_CPU_HOOKS_EN > 0) && (OS_TIME_TICK_HOOK_EN > 0)
void  OSTimeTickHook (void)
{

#if 1
	// Check patch here
	INT32U patch_id;

	patch_id = *((volatile INT32U *) 0xC015000C);
	if ((patch_id & 0x00020000) != 0x00020000) {
		if (((*((volatile INT32U *) 0xD0500380)) & 0x0FFF)==0x015A) {
			
//			patch_id = *((volatile INT32U *) 0xD0500380);
//			patch_id &= 0x00FF;
			(*((volatile INT32U *) 0xD05001FC)) = ((*((volatile INT32U *) 0xD05001FC)) & ~0x70000) | 0x20000;
			(*((volatile INT32U *) 0xD050036C)) = 0x00640064;//patch_id<<16 | patch_id;
			if(((*((volatile INT32U *) 0xD0900140)) & 0x1000) || ((*((volatile INT32U *) 0xC00A0018)) & 0x0800) || ((*((volatile INT32U *) 0xC00F0018)) & 0x0800))
			{
				(*((volatile INT32U *) 0xD0900140)) &= ~0x107F;
			}
			
		} else if (((*((volatile INT32U *) 0xD0500380)) & 0x0FFF)==0x0A5A){
			(*((volatile INT32U *) 0xD0000028)) |= 0x8007; 
		}
	}
#endif	
}
#endif