;********************************************************************************************************
;                                               uC/OS-II
;                                         The Real-Time Kernel
;
;                               (c) Copyright 1992-2006, Micrium, Weston, FL
;                                          All Rights Reserved
;
;                                           Generic ARM Port
;
; File      : OS_CPU_A.ASM
; Version   : V1.70
; By        : Jean J. Labrosse
;
; For       : ARM7 or ARM9
; Mode      : ARM or Thumb
; Toolchain : ADS
;********************************************************************************************************
    INCLUDE arm.inc
    	
	IMPORT  OSRunning                    	; External references
	IMPORT  OSPrioCur
	IMPORT  OSPrioHighRdy
	IMPORT  OSTCBCur
	IMPORT  OSTCBHighRdy
	IMPORT  OSIntNesting
	IMPORT  OSIntExit
	
	IMPORT  OSTaskSwHook;
    IMPORT  irq_dispatcher
    IMPORT  fiq_dispatcher
	
	EXPORT  OS_CPU_SR_Save               	; Functions declared in this file
	EXPORT  OS_CPU_SR_Restore
	EXPORT  OSStartHighRdy
	EXPORT  OSCtxSw
	EXPORT  OSIntCtxSw
	EXPORT  OS_CPU_IRQ_ISR
	EXPORT  OS_CPU_FIQ_ISR


	AREA	|subr|, CODE, READONLY
	
;*********************************************************************************************************
;                                   CRITICAL SECTION METHOD 3 FUNCTIONS
;
; Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
;              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
;              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
;              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
;              into the CPU's status register.
;
; Prototypes :     OS_CPU_SR  OS_CPU_SR_Save(void);
;                  void       OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);
;
;
; Note(s)    : 1) These functions are used in general like this:
;
;                 void Task (void *p_arg)
;                 {
;                 #if OS_CRITICAL_METHOD == 3          /* Allocate storage for CPU status register */
;                     OS_CPU_SR  cpu_sr;
;                 #endif
;
;                          :
;                          :
;                     OS_ENTER_CRITICAL();             /* cpu_sr = OS_CPU_SaveSR();                */
;                          :
;                          :
;                     OS_EXIT_CRITICAL();              /* OS_CPU_RestoreSR(cpu_sr);                */
;                          :
;                          :
;                 }
;
;              2) OS_CPU_SaveSR() is implemented as recommended by Atmel's application note:
;
;                    "Disabling Interrupts at Processor Level"
;*********************************************************************************************************
OS_CPU_SR_Save
	MRS     R0, CPSR                     	; Set IRQ and FIQ bits in CPSR to disable all interrupts
	ORR     R1, R0, #INT_MASK
	MSR     CPSR_c, R1
	MRS     R1, CPSR                     	; Confirm that CPSR contains the proper interrupt disable flags
	AND     R1, R1, #INT_MASK
	CMP     R1, #INT_MASK
	BNE     OS_CPU_SR_Save              	; Not properly disabled (try again)
	BX      LR                          	; Disabled, return the original CPSR contents in R0
	
OS_CPU_SR_Restore
	MSR     CPSR_c, R0
	BX      LR


;*********************************************************************************************************
;                                          START MULTITASKING
;                                       void OSStartHighRdy(void)
;
; Note(s) : 1) OSStartHighRdy() MUST:
;              a) Call OSTaskSwHook() then,
;              b) Set OSRunning to TRUE,
;              c) Switch to the highest priority task.
;*********************************************************************************************************
OSStartHighRdy
	LDR     R0, =OSTaskSwHook
	MOV     LR, PC
	BX      R0
	
	MSR     CPSR_cxsf, #(INT_MASK | MODE_SVC)
	
	LDR     R4, =OSRunning        			; OSRunning = TRUE
	MOV     R5, #1
	STRB    R5, [R4]
	                                		; Switch to highest priority task
	LDR     R4, =OSTCBHighRdy     			; Get highest priority task TCB address
	LDR     R4, [R4]                		; Get stack pointer
	LDR     SP, [R4]                		; Switch to the new stack
	
	LDR     R4, [SP], #4           			; Pop new task's CPSR
	MSR     SPSR_cxsf, R4
	LDMFD   SP!, {R0-R12,LR,PC}^    		; Pop new task's context
	

;*********************************************************************************************************
;                         PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Note(s) : 1) OSCtxSw() is called in SYS mode with BOTH FIQ and IRQ interrupts DISABLED
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) Save the current task's context onto the current task's stack
;              b) OSTCBCur->OSTCBStkPtr = SP;
;              c) OSTaskSwHook();
;              d) OSPrioCur             = OSPrioHighRdy;
;              e) OSTCBCur              = OSTCBHighRdy;
;              f) SP                    = OSTCBHighRdy->OSTCBStkPtr;
;              g) Restore the new task's context from the new task's stack
;              h) Return to new task's code
;
;           3) Upon entry:
;              OSTCBCur      points to the OS_TCB of the task to suspend
;              OSTCBHighRdy  points to the OS_TCB of the task to resume
;*********************************************************************************************************
OSCtxSw
	                                		; Save current task's context
	STMFD   SP!, {LR}               		; Push return address
	STMFD   SP!, {LR}
	STMFD   SP!, {R0-R12}           		; Push registers
	MRS     R4, CPSR               			; Push current CPSR
	STMFD   SP!, {R4}
	
	LDR     R4, =OSTCBCur         			; OSTCBCur->OSTCBStkPtr = SP;
	LDR     R5, [R4]
	STR     SP, [R5]
	
	LDR     R0, =OSTaskSwHook
	MOV     LR, PC
	BX      R0
	
	LDR     R4, =OSPrioCur        			; OSPrioCur = OSPrioHighRdy
	LDR     R5, =OSPrioHighRdy
	LDRB    R6, [R5]
	STRB    R6, [R4]
	
	LDR     R4, =OSTCBCur         			; OSTCBCur = OSTCBHighRdy;
	LDR     R6, =OSTCBHighRdy
	LDR     R6, [R6]
	STR     R6, [R4]
	
	LDR     SP, [R6]                		; SP = OSTCBHighRdy->OSTCBStkPtr;
	
	                                		; Restore new task's context
	LDMFD   SP!, {R4}               		; Pop new task's CPSR
	MSR     SPSR_cxsf, R4

	LDMFD   SP!, {R0-R12,LR,PC}^    		; Pop new task's context


;*********************************************************************************************************
;                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Note(s) : 1) OSIntCtxSw() is called in SYS mode with BOTH FIQ and IRQ interrupts DISABLED
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) OSTaskSwHook();
;              b) OSPrioCur             = OSPrioHighRdy;
;              c) OSTCBCur              = OSTCBHighRdy;
;              d) SP                    = OSTCBHighRdy->OSTCBStkPtr;
;              e) Restore the new task's context from the new task's stack
;              f) Return to new task's code
;
;           3) Upon entry:
;              OSTCBCur      points to the OS_TCB of the task to suspend
;              OSTCBHighRdy  points to the OS_TCB of the task to resume
;*********************************************************************************************************
OSIntCtxSw
	LDR     R0, =OSTaskSwHook
	MOV     LR, PC
	BX      R0
	
	LDR     R4, =OSPrioCur         			; OSPrioCur = OSPrioHighRdy
	LDR     R5, =OSPrioHighRdy
	LDRB    R6, [R5]
	STRB    R6, [R4]
	
	LDR     R4, =OSTCBCur          			; OSTCBCur = OSTCBHighRdy;
	LDR     R6, =OSTCBHighRdy
	LDR     R6, [R6]
	STR     R6, [R4]
	
	LDR     SP, [R6]                 		; SP = OSTCBHighRdy->OSTCBStkPtr;

	                                		; Restore new task's context
	LDMFD   SP!, {R4}               		; Pop new task's CPSR
	MSR     SPSR_cxsf, R4
	
	LDMFD   SP!, {R0-R12, LR, PC}^    		; Pop new task's context

	
;*********************************************************************************************************
;                                      IRQ Interrupt Service Routine
;*********************************************************************************************************
OS_CPU_IRQ_ISR
	                                       	; Disable FIQ for a moment 
	MSR     CPSR_c, #(INT_MASK | MODE_IRQ) 	; Change to IRQ mode (to use the IRQ stack to handle interrupt)
	STMFD   SP!, {R1-R3}                   	; Push working registers onto IRQ stack
	MOV     R1, SP                         	; Save IRQ stack pointer
	ADD     SP, SP, #12                     ; Adjust IRQ stack pointer
	SUB     R2, LR, #4                      ; Adjust PC for return address to task
	MRS     R3, SPSR                       	; Copy SPSR (i.e. interrupted task's CPSR) to R3
	MSR     CPSR_c, #(INT_MASK | MODE_SVC) 	; Change to SVC mode
	
	                                       	; Save task's context onto task's stack
	STMFD   SP!, {R2}                      	; Push task's Return PC
	STMFD   SP!, {LR}                      	; Push task's LR
	STMFD   SP!, {R4-R12}                  	; Push task's R12-R4
	
	LDMFD   R1!, {R4-R6}                   	; Move task's R1-R3 from IRQ stack to SVC stack
	STMFD   SP!, {R4-R6}
	STMFD   SP!, {R0}                      	; Push task's R0    onto task's stack
	STMFD   SP!, {R3}                      	; Push task's CPSR (i.e. IRQ's SPSR)
	
	                                       	; Handle nesting counter
	LDR     R0, =OSIntNesting            	; OSIntNesting++;
	LDRB    R1, [R0]
	ADD     R1, R1, #1
	STRB    R1, [R0]
	
	CMP     R1, #1                         	; if (OSIntNesting == 1) {
	BNE     OS_CPU_IRQ_ISR_1
	
	LDR     R4, =OSTCBCur                	;     OSTCBCur->OSTCBStkPtr = SP
	LDR     R5, [R4]
	STR     SP, [R5]                       	; }

OS_CPU_IRQ_ISR_1
	MSR     CPSR_c, #(INT_IRQ | MODE_IRQ) 	; Re-enable FIQ, Change to IRQ mode (to use the IRQ stack to handle interrupt)
	
	LDR     R0, =irq_dispatcher
	MOV     LR, PC
	BX      R0
	
	MSR     CPSR_c, #(INT_MASK | MODE_SVC) 	; Change to SVC mode
	
	LDR     R0, =OSIntExit               	; OSIntExit();
	MOV     LR, PC
	BX      R0
	
	                                       	; Restore new task's context
	LDMFD   SP!, {R4}                      	; Pop new task's CPSR
	MSR     SPSR_cxsf, R4
	
	LDMFD   SP!, {R0-R12, LR, PC}^          ; Pop new task's context


;*********************************************************************************************************
;                                      FIQ Interrupt Service Routine
;*********************************************************************************************************
OS_CPU_FIQ_ISR
	STMFD   SP!, {R1-R4}                   	; Push working registers onto FIQ stack
	MOV     R1, SP                         	; Save FIQ stack pointer
	SUB     R2, LR, #4                      ; Adjust PC for return address to task
	MRS     R3, SPSR                       	; Copy SPSR (i.e. interrupted task's CPSR) to R3 and R4
	MOV     R4, R3
	
	AND     R4, R4, #0x1F                  	; Isolate Mode bits
	CMP     R4, #MODE_IRQ                	; See if we interrupted an IRQ
	BEQ     OS_CPU_FIQ_ISR_2               	; Branch if yes.

	                                       	; =============== FIQ interrupted Task ===============
	MSR     CPSR_c, #(INT_MASK | MODE_SVC) 	; Change to SVC mode
	                                       	; Save task's context onto task's stack
	STMFD   SP!, {R2}                      	; Push task's Return PC
	STMFD   SP!, {LR}                      	; Push task's LR
	STMFD   SP!, {R5-R12}                  	; Push task's R12-R5
	
	LDMFD   R1!, {R5-R8}                   	; Move task's R1-R4 from FIQ stack to SVC stack
	STMFD   SP!, {R5-R8}
	STMFD   SP!, {R0}                      	; Push task's R0 onto task's stack
	STMFD   SP!, {R3}                      	; Push task's CPSR (i.e. FIQ's SPSR)
	                                       	; Handle nesting counter
	LDR     R0, =OSIntNesting            	; OSIntNesting++;
	LDRB    R1, [R0]
	ADD     R1, R1, #1
	STRB    R1, [R0]
	
	CMP     R1, #1                         	; if (OSIntNesting == 1) {
	BNE     OS_CPU_FIQ_ISR_1
	LDR     R4, =OSTCBCur                	;     OSTCBCur->OSTCBStkPtr = SP
	LDR     R5, [R4]
	STR     SP, [R5]                       	; }

OS_CPU_FIQ_ISR_1
	MSR     CPSR_c, #(INT_MASK | MODE_FIQ) 	; Change to FIQ mode (to use the FIQ stack to handle interrupt)
	ADD     SP, SP, #16                    	; Adjust FIQ stack pointer (Working register were saved)
	LDR     R0, =fiq_dispatcher
	MOV     LR, PC
	BX      R0
	
	MSR     CPSR_c, #(INT_MASK | MODE_SVC) 	; Change to SVC mode
	LDR     R0, =OSIntExit               	; OSIntExit();
	MOV     LR, PC
	BX      R0

	                                       	; Restore new task's context
	LDMFD   SP!, {R4}                      	; Pop new task's CPSR
	MSR     SPSR_cxsf, R4

	LDMFD   SP!, {R0-R12,LR,PC}^           	; Pop new task's context 

OS_CPU_FIQ_ISR_2							; =============== FIQ interrupted IRQ ===============
	                                       	; Save IRQ's context onto FIQ's stack
	STMFD   SP!, {R0-R7,LR}                	; Push shared registers
	                                       	; Handle nesting counter
	LDR     R0, =OSIntNesting            	; OSIntNesting++;    (Notify uC/OS-II)
	LDRB    R1, [R0]
	ADD     R1, R1, #1
	STRB    R1, [R0]
	
	
	LDR     R0, =fiq_dispatcher
	MOV     LR, PC
	BX      R0
	                                       	; Handle nesting counter
	LDR     R0, =OSIntNesting            	; OSIntNesting--;
	LDRB    R1, [R0]                       	; NO need to call OSIntExit() we are returning to IRQ handler
	SUB     R1, R1, #1
	STRB    R1, [R0]
	
	LDMFD   SP!, {R0-R7,LR}                	; Pop IRQ's context
	LDMFD   SP!, {R1-R4}                   	; Pull working registers onto FIQ stack
	SUBS    PC, LR, #4                     	; Return to IRQ


	END