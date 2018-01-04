/************************************************************************/
/* 
	find file from disk ignore dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

f_ppos getpaperfirstfile(CHAR *path, CHAR *extname, struct sfn_info* f_info, INT16S attr, INT32S (*filter)(INT8S *str))
{
	f_ppos ppos;
	
	FS_OS_LOCK();		
	ppos = fs_getpaperfirstfile(&gstDiskFindControl, path, (INT8S*)extname, f_info, attr, 0, filter);
	FS_OS_UNLOCK();
	return ppos;
}

f_ppos getpapernextfile(CHAR *extname,struct sfn_info* f_info, INT16S attr, INT32S (*filter)(INT8S *str))
{
	f_ppos ppos;
	
	FS_OS_LOCK();
	ppos = fs_getpapernextfile(&gstDiskFindControl ,(INT8S *)extname ,f_info ,attr ,0, filter);
	FS_OS_UNLOCK();
	return ppos;
}

#endif
