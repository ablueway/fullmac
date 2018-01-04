/************************************************************************/
/* 
	set volume
	
	zhangzha creat @2009.03.10
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if SUPPORT_VOLUME_FUNCTION == 1
INT16S set_volume(INT8U disk_id, INT8U *p_volum)
{
	INT16S ret;
	
	FS_OS_LOCK();
	ret = fs_set_volume(disk_id, p_volum);
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
