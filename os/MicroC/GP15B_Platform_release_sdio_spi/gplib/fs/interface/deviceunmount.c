#include "fsystem.h"
#include "globals.h"

INT16S _deviceunmount (INT16S diskID)
{
	INT16S ret;
	
	FS_OS_LOCK();
	
	ret = fs_unmount(diskID);
	
	FS_OS_UNLOCK();
	
	if (ret != SUCCESS) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return 0;
}
