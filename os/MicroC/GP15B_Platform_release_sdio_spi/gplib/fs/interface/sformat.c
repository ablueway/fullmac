#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _SMALL_DISK_FORMAT == 1
//-----------------------------------------------------
INT16S sformat(INT8U drv, INT32U totalsectors, INT32U realsectors) 
{
	INT16S ret;
	
	if (drv >= MAX_DISK_NUM) 	 {
		handle_error_code (DE_INVLDACC);
		return -1;
	}
	
	FS_OS_LOCK();
	 
	ret = fat_sformat (drv, totalsectors, realsectors);

	FS_OS_UNLOCK();
	 

	if (ret != SUCCESS) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return 0;
}
#endif
#endif	//READ_ONLY
