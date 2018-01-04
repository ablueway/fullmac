#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
//for minidvr
INT16S unlink2(CHAR *filename) 	 
{
	INT16S ret;
	
	FS_OS_LOCK();
	
	if (filename == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fat_unlink2((INT8S *) filename);
	
	FS_OS_UNLOCK();		 
	
	if (ret < 0) 	
	{
		handle_error_code(ret);
		return -1; 	
	}
	else
		return 0;
}
#endif
