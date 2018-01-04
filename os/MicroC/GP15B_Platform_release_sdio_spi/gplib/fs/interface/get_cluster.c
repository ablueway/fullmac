#include "fsystem.h"
#include "globals.h"

INT32S _GetCluster(INT16S fd)
{
	INT32S ret;

	FS_OS_LOCK();		

	ret = fs_GetCluster(fd);

	FS_OS_UNLOCK();

	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return ret;
}
