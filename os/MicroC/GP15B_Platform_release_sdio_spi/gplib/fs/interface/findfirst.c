#include "fsystem.h"
#include "globals.h"

/******************** the extra interface not POSIX compatible ****************************/
INT16S _findfirst(CHAR *name, struct f_info *f_info,  INT16U attr) 	
{
	INT16S ret;
	
	FS_OS_LOCK();
	
	if (name == 0) {
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fat_findfirst((INT8S *) name, f_info, attr);
	
	FS_OS_UNLOCK();
	
	if (ret!=0) { 
		//if error
		handle_error_code(ret);
		return -1; 	
	} else {
		return 0;
	}
}