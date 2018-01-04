#include "fsystem.h"
#include "globals.h"
INT64S lseek64(INT16S handle, INT64S offset0,INT16S fromwhere) 	 
{
	INT32S ret;

	FS_OS_LOCK();

	ret = fat_seek64 (handle, offset0, fromwhere);

	FS_OS_UNLOCK();
	 

	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return ret;
}

//-----------------------------------------------------
INT32S lseek(INT16S handle, INT32S offset0,INT16S fromwhere)	 
{
	INT32S ret;

	FS_OS_LOCK();

	ret = fat_seek (handle, offset0, fromwhere);

	FS_OS_UNLOCK();
	 

	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return ret;
}
