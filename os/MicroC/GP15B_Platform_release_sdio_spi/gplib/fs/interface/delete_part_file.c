#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _PART_FILE_OPERATE == 1
INT16S DeletePartFile(INT16S fd, INT32U offset, INT32U length)
{
	INT16S ret;
	
	FS_OS_LOCK();	
	
	ret = fs_DeletePartFile(fd, offset, length);
    
	FS_OS_UNLOCK();
	
	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return ret;
}
#endif
#endif

