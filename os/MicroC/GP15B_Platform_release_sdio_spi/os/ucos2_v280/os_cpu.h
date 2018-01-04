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
* File      : OS_CPU.H
* Version   : V1.70
* By        : Jean J. Labrosse
*
* For       : ARM7 or ARM9
* Mode      : ARM or Thumb
* Toolchain : ADS
********************************************************************************************************/
#ifndef  OS_CPU_H
#define  OS_CPU_H


/********************************************************************************************************
*                                              DATA TYPES
*                                         (Compiler Specific)
********************************************************************************************************/
typedef unsigned int   OS_CPU_SR;           // Define size of CPU status register (PSR = 32 bits)

/********************************************************************************************************
*                                                ARM
*
* Method #1:  Disable/Enable interrupts using simple instructions.  After critical section, interrupts
*             will be enabled even if they were disabled before entering the critical section.
*             NOT IMPLEMENTED
*
* Method #2:  Disable/Enable interrupts by preserving the state of interrupts.  In other words, if
*             interrupts were disabled before entering the critical section, they will be disabled when
*             leaving the critical section.
*             NOT IMPLEMENTED
*
* Method #3:  Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
*             would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
*             disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
*             disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
*             into the CPU's status register.
********************************************************************************************************/
#define  OS_CRITICAL_METHOD    3

#define  OS_ENTER_CRITICAL()  {cpu_sr = OS_CPU_SR_Save();}
#define  OS_EXIT_CRITICAL()   {OS_CPU_SR_Restore(cpu_sr);}

/********************************************************************************************************
*                                         ARM Miscellaneous
********************************************************************************************************/
#define  OS_STK_GROWTH        1             // Stack grows from HIGH to LOW memory on ARM
#define  OS_TASK_SW()         OSCtxSw()

/********************************************************************************************************
*                                              PROTOTYPES
********************************************************************************************************/
extern OS_CPU_SR  OS_CPU_SR_Save(void);
extern void       OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);

extern void       OS_CPU_IRQ_ISR(void);
extern void       OS_CPU_FIQ_ISR(void);

extern void       OSCtxSw(void);
extern void       OSIntCtxSw(void);
extern void       OSStartHighRdy(void);


#endif