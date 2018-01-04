/************************************************************************/
/* 
	open file
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

INT16S sfn_open(f_ppos ppos)
{
	INT16S ret;
	
	FS_OS_LOCK();
	ret = fs_sfn_open(ppos, RDONLY);
	FS_OS_UNLOCK();
	return ret;
}

#endif

