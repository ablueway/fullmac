#include "application.h"
#include "MultiMedia_Platform.h"
#include "turnkey_filesrv_task.h"
#include "turnkey_image_task.h"
#include "drv_l1_sfr.h"
#include "drv_l1_timer.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	C_IMAGE_TASK_STACK_SIZE	    1024

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT32U 	ImageTaskStack[C_IMAGE_TASK_STACK_SIZE];

void idle_task_entry(void *para)
{
	OS_CPU_SR cpu_sr;
	INT16U i;
	
	while (1) {
		OS_ENTER_CRITICAL();
		i=0x5005;
		R_SYSTEM_WAIT = i;
		i = R_CACHE_CTRL;

		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		ASM(NOP);
		OS_EXIT_CRITICAL();	    
	}
}

/**
 * @brief   platform code entrance
 * @param   free_memory[in]: free memory start address
 * @return 	none
 */
void platform_entrance(void *free_memory)
{
	INT32S ret;

	//enable uc-os timer
	timer_freq_setup(0, OS_TICKS_PER_SEC, 0, OSTimeTick);
	
	ret = filesrv_task_open();
	if(ret == STATUS_OK) {
		DBG_PRINT("FileServ Task Create[%d]\r\n", FS_PRIORITY);
	} else {
		DBG_PRINT("FileServ Task Create Fail[%d]\r\n", FS_PRIORITY);	
	}
	
	ret = OSTaskCreate(image_task_entry, (void *) 0, &ImageTaskStack[C_IMAGE_TASK_STACK_SIZE - 1], IMAGE_PRIORITY);
	if(ret == OS_NO_ERR) {
		DBG_PRINT("Image Task Create[%d]\r\n", IMAGE_PRIORITY);
	} else {
		DBG_PRINT("Image Task Create Fail[%d]\r\n", IMAGE_PRIORITY);	
	}	
}

/**
 * @brief   platform code exit
 * @param   none
 * @return 	none
 */
void platform_exit(void)
{

}