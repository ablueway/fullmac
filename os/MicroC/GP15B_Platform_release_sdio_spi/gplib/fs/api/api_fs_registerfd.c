#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/************************************************************************/
/*	��ȫ�˳�ʱ�������ļ�����ļ��
    �ر����� û��ע��� ���ڴ�״̬���ļ����

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






