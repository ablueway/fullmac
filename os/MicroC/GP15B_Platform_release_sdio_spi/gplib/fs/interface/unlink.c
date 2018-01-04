#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S unlink(CHAR *filename) 	 
{
	INT16S ret;
	
	FS_OS_LOCK();
	
	if (filename == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fat_unlink((INT8S *) filename);
	
	FS_OS_UNLOCK();
	
	if (ret < 0) 	
	{
		handle_error_code(ret);
		return -1; 	
	}
	else
		return 0;
}

void unlink_step_start(void) 	 
{
	FS_OS_LOCK();
    //fat_l1_hit_cnt_disable();    
    //fat_l1_hit_cnt_suspend();
    fs_lfndel_step_start();
	FS_OS_UNLOCK();
}

INT32S unlink_step_work(void) 	 
{
	INT32S ret;
	
	FS_OS_LOCK();
    //fat_l1_hit_cnt_suspend();
    ret = fs_lfndel_step();
    //fat_l1_hit_cnt_resume();
	FS_OS_UNLOCK();

    return ret;
}

void unlink_step_flush(void) 	 
{
	FS_OS_LOCK();
    fs_lfndel_step_end();
    //fat_l1_hit_cnt_resume();
    FS_OS_UNLOCK();
}


#endif
