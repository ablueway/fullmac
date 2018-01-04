#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
//…Ë÷√dir Ù–‘
INT16S _setdirattr(CHAR *dirname, INT16U attr) 	 
{
	INT16S ret;
	
	if (!dirname) 	 
	{
		gFS_errno = DE_INVLDPARM;
		return -1; 	 // Invalid parameter
	}
	if (attr & ~(D_RDONLY | D_HIDDEN | D_SYSTEM | D_ARCHIVE | D_DIR))
	{
		gFS_errno = DE_INVLDPARM;
		return -1; 	 // Invalid attribute
	}
	
	FS_OS_LOCK();
	
	ret = fs_setdirattr((INT8S *) dirname, attr);
	
	FS_OS_UNLOCK();
	
	if (ret < 0) {
		handle_error_code(ret);
		return -1; 	
	}
	else
		return ret;
}
#endif
