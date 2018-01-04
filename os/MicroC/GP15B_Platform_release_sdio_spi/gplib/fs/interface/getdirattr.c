#include	"fsystem.h"
#include	"globals.h"

//-----------------------------------------------------
INT16S _getdirattr(CHAR *dirname, INT16U *attr)
{
	INT16S ret;
	
	if (!dirname)  {
		gFS_errno = DE_INVLDPARM;
		return -1; 	 // Invalid parameter
	}
	
	FS_OS_LOCK();
	
	ret = fs_getdirattr((INT8S *) dirname, attr);
	
	FS_OS_UNLOCK();
	
	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1; 	
	}
	else
		return ret;
}