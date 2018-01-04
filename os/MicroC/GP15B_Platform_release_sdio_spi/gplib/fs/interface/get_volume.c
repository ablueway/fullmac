/************************************************************************/
/* 
	get volume
	
	zhangzha creat @2009.03.10
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if SUPPORT_VOLUME_FUNCTION == 1
INT16S get_volume(INT8U disk_id, STVolume *pstVolume)
{
	INT16S ret;
	
	FS_OS_LOCK();
	ret = fs_get_volume(disk_id, pstVolume);
	FS_OS_UNLOCK();
	if (ret!=0)
    {
		handle_error_code(ret);
		return -1;
	}
	else
	{
		return 0;     //success
	}
}
#endif
