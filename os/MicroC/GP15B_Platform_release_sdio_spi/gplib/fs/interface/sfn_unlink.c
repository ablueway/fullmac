/************************************************************************/
/* 
	open file
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if FIND_EXT == 1

INT16S sfn_unlink(f_ppos ppos)
{
	INT16S	ret;
	
	FS_OS_LOCK();
	ret = fs_sfn_unlink(ppos);
	FS_OS_UNLOCK();
	return ret;
}
#endif
#endif

