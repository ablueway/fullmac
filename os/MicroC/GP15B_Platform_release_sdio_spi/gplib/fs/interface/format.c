#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S _format(INT8U drv, INT8U fstype) 	 
{
	INT16S ret;
	
	if (drv >= MAX_DISK_NUM) 	 { 
		handle_error_code(DE_INVLDACC);
		return -1;
	}
	
	FS_OS_LOCK();
	ret = fat_format(drv, fstype, 32);

	FS_OS_UNLOCK();
	
	if (ret != SUCCESS) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return 0;
}
#endif
