#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S _setftime (const INT8S * filename, const struct timesbuf * times) 	 
{
	INT16U acdate, moddate, modtime;
	INT32S fd;
	
	if (!filename) 	 
	{
		gFS_errno = DE_INVLDPARM;		 
		return -1; // Invalid parameter
	}
	if (!times) 	 { 	 // Get system time as file's last access time and last modify time
		dosdate_t cu_date;
		dostime_t cu_time;
		FS_OS_GetTime (&cu_time);
		FS_OS_GetDate (&cu_date);
		acdate = ((cu_date.year - 1980) & 127) << 9; 	 // Convert into DOS format time
		acdate += cu_date.month << 5;
		acdate += cu_date.monthday;
		moddate = acdate;
		
		modtime = cu_time.hour << 11;
		modtime += cu_time.minute << 5;
		modtime += cu_time.second >> 1;
	}
	else 	 {
		acdate = times ->accdate;
		moddate = times ->moddate;
		modtime = times ->modtime;
	}
	fd = fat_open ((CHAR *) filename, O_RDONLY | O_OPEN, 0x80); // 0x80 = D_OPEN_DIR_FOR_STAT;
	fd = handle_error_code (fd);
	if (fd == -1) 	 {
		return -1;
	}
	fs_setftime (fd, acdate, moddate, modtime);
	close (fd);
	return 0;
}
#endif
