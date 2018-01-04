#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S _deleteall (CHAR *filename) 	 
{
	INT16S res;
	
	FS_OS_LOCK();	
	
	if (filename == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	res = fat_deleteall ((INT8S *)filename);
	
	FS_OS_UNLOCK();
	
	if (res < 0) 	 {
		handle_error_code (res);
		return -1;
	}
	return 0;
}
#endif
