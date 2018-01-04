#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
INT16S _rename(CHAR *oldname, CHAR *newname) 	 
{
	INT16S ret;	

	FS_OS_LOCK();

	if (oldname == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	if (newname == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fat_rename ((INT8S *)oldname, (INT8S *)newname);
	
	FS_OS_UNLOCK();
	 
	if (ret != 0) 	 {
		handle_error_code(ret);
		return -1; 	
	}
	else
		return 0;
}
#endif
