#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S mkdir (CHAR *filename) 	 
{
	INT16S ret;

	FS_OS_LOCK();

	if (filename == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fat_mkdir((CHAR *) filename);

	FS_OS_UNLOCK();

	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1; 	
	}
	else
		return ret;
}
#endif



