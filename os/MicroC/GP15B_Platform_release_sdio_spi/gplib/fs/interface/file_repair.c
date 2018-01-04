#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _DIR_UPDATA == 1
INT16S FileRepair(INT16S fd)
{
	INT16S ret;
	
	FS_OS_LOCK();	//zhangzha marked for test
	
	ret = fs_FileRepair(fd);
	
	FS_OS_UNLOCK();	//zhangzha marked for test
	
	if (ret!=0)
	{
		gFS_errno = DE_INVLDPARM; //invalid file handle		 
		return -1;
	}
	else
		return 0;     //success
}
#endif
#endif
