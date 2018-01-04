#include "fsystem.h"
#include "globals.h"

INT16S _devicemount(INT16S disked)
{
	INT16S ret;
	
	FS_OS_LOCK();
	
	ret = fs_mount(disked);
	
	FS_OS_UNLOCK();
	
	if (ret != SUCCESS) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return 0;
}
