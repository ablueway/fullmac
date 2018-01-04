#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S stat(CHAR *filename, struct stat_t * buf)
{
	INT16S fd;
	INT16U dp,tp;    //date and time;
	INT16S ret = 0;

	dp = 0;
	tp = 0;

	if (!filename || !buf) 	 
	{
		gFS_errno = DE_INVLDPARM;		 		 
		return -1; // Invalid parameter
	}
	
	FS_OS_LOCK();

	if (filename == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}

	fd = fat_open ((CHAR *) filename, O_RDONLY | O_OPEN, 0x80); // 0x80 = D_OPEN_DIR_FOR_STAT;	 

	fd = handle_error_code (fd);
	if (fd == -1) 	 
	{
		FS_OS_UNLOCK();
		return -1;
	}

	buf->st_mode = fat_getfattr(fd); 		
	ret = buf->st_mode;
	buf->st_size = fat_getfsize(fd);
	ret = buf->st_size;
	fat_getftime(fd,&dp,&tp);
	 
	buf->st_mtime =(( INT32U)(tp))<<16|(dp); 	 

	FS_OS_UNLOCK();

	close (fd);
	
	return 0;
}
