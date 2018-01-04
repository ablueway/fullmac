#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/************************************************************************/
/*	安全退出时，进行文件句柄的检查
    关闭所有 没有注册的 处于打开状态的文件句柄

input:	void
output:	void                                                            */
/************************************************************************/
void fs_registerfd(INT16S fd)
{
	f_node_ptr fnp;

	FS_OS_LOCK();

	fnp = xlt_fd(fd);

	if (fnp != NULL)
	{
		if (fnp->f_count && !(fnp->f_dir.dir_attrib&D_DIR)) 
		{
			fnp->common_flag |= F_REGISTER_OPEN;
		}
	}

	FS_OS_UNLOCK();

	return;
}
#endif






