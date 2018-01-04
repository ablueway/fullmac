#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S close (INT16S filedes) 	 
{
	INT16S ret;

	FS_OS_LOCK();

	ret = fat_close (filedes);

	FS_OS_UNLOCK();

	if (ret!=0)
    {
		handle_error_code(ret);
		return -1;
	}
	 else
		return 0;     //success
}
