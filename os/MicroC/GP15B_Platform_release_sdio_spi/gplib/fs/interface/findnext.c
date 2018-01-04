#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S _findnext (struct f_info *f_info) 	 
{
	INT16S ret;
	
	FS_OS_LOCK();
	
	ret = fat_findnext(f_info);
	
	FS_OS_UNLOCK();
	
	if (ret < 0) 	 
	{     //if error
		handle_error_code(ret);
		return -1; 	
	}
	else
		return 0;
}

INT16S get_fnode_pos(f_pos *fpos)
{
	INT16S ret;
	
	FS_OS_LOCK();
	ret = fs_get_fnode_pos(fpos);
	FS_OS_UNLOCK();
	return ret;
}