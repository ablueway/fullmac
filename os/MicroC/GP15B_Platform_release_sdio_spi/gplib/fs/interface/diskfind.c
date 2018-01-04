/************************************************************************/
/* 
	find file from disk ignore dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

f_ppos getfirstfile(INT16S dsk, CHAR *extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S* str))
{
	f_ppos ppos;
	
	FS_OS_LOCK();		
	ppos = fs_getfirstfile(&gstDiskFindControl, dsk, (INT8S*)extname, f_info, attr, 0,filter);
	FS_OS_UNLOCK();
	return ppos;
}

f_ppos getnextfile(INT16S dsk, CHAR * extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S* str))
{
	f_ppos ppos;
	
	FS_OS_LOCK();
	ppos = fs_getnextfile(&gstDiskFindControl, dsk, (INT8S*)extname, f_info, attr, 0,filter);
	FS_OS_UNLOCK();
	return ppos;
}

#endif
