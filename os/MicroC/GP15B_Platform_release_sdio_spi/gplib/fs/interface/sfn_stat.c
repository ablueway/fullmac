/************************************************************************/
/* 
	get file stat
	
	zhangzha creat @2006.11.28
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

INT16S sfn_stat(INT16S fd, struct sfn_info *sfn_info)
{
	INT16S	ret;
	
	FS_OS_LOCK();
	ret = fs_sfn_stat(fd, sfn_info);
	FS_OS_UNLOCK();
	return ret;
}

#endif
