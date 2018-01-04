#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S fstat(INT16S handle, struct stat_t *statbuf)
{
	INT16S ret = 0;
	INT16U dp,tp;
	
	dp = 0;
	tp = 0;
	statbuf->st_mode = fat_getfattr(handle); 		
	ret = statbuf->st_mode;
	statbuf->st_size = fat_getfsize(handle);
	ret = statbuf->st_size;
	fat_getftime(handle,&dp,&tp);
	statbuf->st_mtime =(( INT32U)(tp))<<16|(dp); 
	return 0;
} 
