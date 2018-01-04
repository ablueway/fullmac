#include "fsystem.h"
#include "globals.h"

INT16S handle_error_code (INT16S errcode) 
{
	if (errcode >= 0)	
		return errcode;

	gFS_errno = errcode;
	return -1;		
}

INT16S open(CHAR *path,INT16S open_flag)
{
	INT16S ret;
	
	FS_OS_LOCK();

	//if file does not exist, create it. if file exist,return error
	if ((open_flag & O_EXCL) && (open_flag & O_CREAT))
	{
		open_flag = O_CREAT;
	}
	else
	{
		open_flag = open_flag | O_OPEN;
	}

	if (path == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}

	ret = fat_open(path,open_flag,D_NORMAL);

	FS_OS_UNLOCK();

	return handle_error_code(ret);
}
