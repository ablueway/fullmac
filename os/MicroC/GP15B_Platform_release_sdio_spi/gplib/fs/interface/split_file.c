/************************************************************************/
/* 
	splite file
	
	zhangzha creat @2006.09.22
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//把文件拆分为A,B两部分，其中A还挂在原来的文件名下，B放在一个新建的文件名下
#if _PART_FILE_OPERATE == 1

INT16S SplitFile(INT16S tagfd, INT16S srcfd, INT32U splitpoint)
{
	INT16S ret;

	FS_OS_LOCK();
	
	ret = fs_SplitFile(tagfd, srcfd, splitpoint);
    
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
