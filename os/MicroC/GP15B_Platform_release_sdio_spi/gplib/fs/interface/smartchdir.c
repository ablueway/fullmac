#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S smartchdir (INT32U path) 	 
{
	INT16S ret;

	FS_OS_LOCK();

	if (path == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fs_smartchdir (path);

	FS_OS_UNLOCK();	

	if (ret != SUCCESS) 	 
	{
		handle_error_code(ret);
		return -1;
	}
	else
		return 0;
}

#endif	//#ifndef READ_ONLY
