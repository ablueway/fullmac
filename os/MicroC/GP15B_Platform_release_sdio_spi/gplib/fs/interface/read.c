#include "fsystem.h"
#include "globals.h"

INT32S read(INT16S fd, INT32U buf, INT32U size) 	 
{
	INT32S ret;

	FS_OS_LOCK();

	ret = fat_read(fd, buf, size);

	FS_OS_UNLOCK();

	if (ret < 0) {
		handle_error_code(ret);
		return -1;
	} else {
		if (ret != size) {
			handle_error_code(gFS_errno);
		}		
		return ret;
	}
}

#ifdef unSP
INT16S readB(INT16S fd, INT32U buf, INT16U size) 	
{
	INT16S ret;

	FS_OS_LOCK();

	ret = fs_freadB(fd, buf, size);

	FS_OS_UNLOCK();

	if (ret < 0) {
		handle_error_code(ret);
		return -1; 
	} else {
		return ret;
	}
}
#endif
