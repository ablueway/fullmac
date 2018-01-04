#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S chdir(CHAR *path) 	 
{
	INT16S ret;

	FS_OS_LOCK();

	if (path == NULL)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fat_chdir(path);
	
	FS_OS_UNLOCK();	

	if (ret != SUCCESS) 	 
	{
		handle_error_code(ret);
		return -1;
	}
	else
		return 0;
}
