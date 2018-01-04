#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT32U get_freeclu_misc(INT8U disk) 	 
{
	INT32U ret;

	FS_OS_LOCK();

	ret = fs_get_freeclu_misc(disk);
	
	FS_OS_UNLOCK();	

	return ret;
}


INT32U get_freearea_misc(INT8U disk) 	 
{
	INT32U ret;

	FS_OS_LOCK();

	ret = fs_get_freearea_misc(disk);
	
	FS_OS_UNLOCK();	

	return ret;
}


INT32U get_fatsize_misc(INT8U disk) 	 
{
	INT32U ret;

	FS_OS_LOCK();

	ret = fs_get_fatsize(disk);
	
	FS_OS_UNLOCK();	

	return ret;
}

INT32U read_fat_frist_misc(INT32U disk, INT32U buf, INT32U size) 	 
{
	INT32U ret;

	FS_OS_LOCK();

	ret = fs_read_fat_frist(disk, buf, size);
	
	FS_OS_UNLOCK();	

	return ret;
}

INT32U read_fat_next_misc(INT32U disk, INT32U buf, INT32U size) 	 
{
	INT32U ret;

	FS_OS_LOCK();

	ret = fs_read_fat_next(disk, buf, size);
	
	FS_OS_UNLOCK();	

	return ret;
}