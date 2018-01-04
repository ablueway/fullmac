#include "fsystem.h"
#include "globals.h"

/************************************************************************/
/*	安全退出时，进行文件句柄的检查
    关闭所有 没有注册的 处于打开状态的文件句柄

input:	void
output:	void 
            
                                                           */
/************************************************************************/
void fs_safexit(void)
{
	INT16S i;

	f_node_ptr fnp;

	FS_OS_LOCK();

	for (i=0x00; i<MAX_FILE_NUM; i++)
	{
		fnp = xlt_fd(i);

		if (!fnp->f_count) 
		{
			continue;
		}

		if (fnp->f_dir.dir_attrib & D_DIR) 
		{
			continue;
		}

		if (fnp->common_flag & F_REGISTER_OPEN) 
		{
			continue;
		}

		fs_fclose(i);
	}

	FS_OS_UNLOCK();		

	return;
}


void disk_safe_exit(INT16S dsk)
{
	INT16S i;

	f_node_ptr fnp;

	FS_OS_LOCK();

	for (i=0x00; i<MAX_FILE_NUM; i++)
	{
		fnp = xlt_fd(i);

		if (!fnp->f_count) 
		{
			continue;
		}

		if (fnp->f_dir.dir_attrib & D_DIR) 
		{
			continue;
		}

		if (fnp->common_flag & F_REGISTER_OPEN) 
		{
			continue;
		}
		
		if (fnp->f_dpb->dpb_unit != dsk)
		{
			continue;
		}

		fs_fclose(i);
	}


	FS_OS_UNLOCK();		

	return;
}




