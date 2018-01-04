#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S _setfattr (CHAR *filename,  INT16U attr) 	 
{
	INT16S fd, res;
	
	if (!filename) 	 
	{
		gFS_errno = DE_INVLDPARM;
		return -1; 	 // Invalid parameter
	}
	if (attr & ~(D_RDONLY | D_HIDDEN | D_SYSTEM | D_ARCHIVE)) 	 {
		gFS_errno = DE_INVLDPARM;
		return -1; 	 // Invalid attribute
	}
	
	FS_OS_LOCK();
	
	if (filename == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	res = fat_open ((CHAR *)filename, O_RDONLY | O_OPEN, 0x80); // 0x80 = D_OPEN_DIR_FOR_STAT;
	
	fd = handle_error_code (res);

	if (fd == -1) 	 
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	res = fs_setfattr (fd, attr);
	
	FS_OS_UNLOCK();
	
	close (fd);
	if (res != SUCCESS) 	 
	{
		gFS_errno = DE_INVLDPARM;		
		return -1;
	}
	return 0;
}
#endif
