#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT32S write (INT16S fd, INT32U buf,  INT32U size) 	 
{
	INT32S ret;

	FS_OS_LOCK();
	ret = fat_write (fd, buf, size);	
	FS_OS_UNLOCK();
	if (ret < 0) 	
	{
		handle_error_code(ret);
		return -1;
	}
	else
	{
		if (ret != size)
		{
			handle_error_code (gFS_errno);
		}	
		return ret;
	}
}

#ifdef unSP
INT16S writeB (INT16S fd, INT32U buf,  INT16U size) 	 
{
	INT16S ret;
	
	FS_OS_LOCK();
	ret = fs_fwriteB (fd, buf, size);
	FS_OS_UNLOCK();

	if (ret < 0) 	
	{
		handle_error_code(ret);
		return -1; 	
	}
	else
		return ret;
}
#endif
#endif	//#ifndef READ_ONLY
