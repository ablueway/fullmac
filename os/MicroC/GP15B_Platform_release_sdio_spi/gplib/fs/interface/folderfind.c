#ifndef __FS_AVOID__ 

/************************************************************************/
/* 
	find file from disk ignore dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"


#if FIND_EXT == 1
f_ppos get_first_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir, INT32S (*filter)(INT8S *str))
{
	f_ppos ppos;
	
	FS_OS_LOCK();
	ppos = fs_get_first_file_in_folder(pstDiskFindControl, path, extname, f_info, attr, find_child_dir, filter);
	FS_OS_UNLOCK();
	return ppos;
}

f_ppos get_next_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir, INT32S (*filter)(INT8S *str))
{
	f_ppos ppos;
	
	FS_OS_LOCK();
	ppos = fs_get_next_file_in_folder(pstDiskFindControl, path, extname, f_info, attr, find_child_dir, filter);
	FS_OS_UNLOCK();
	return ppos;
}

#endif
#endif //__FS_AVOID__
