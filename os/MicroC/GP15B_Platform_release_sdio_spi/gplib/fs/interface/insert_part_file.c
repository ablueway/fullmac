#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//����ʱ���lengthΪ0�Ͳ��������ļ�
#if _PART_FILE_OPERATE == 1
INT16S InserPartFile(INT16S tagfd, INT16S srcfd, INT32U tagoff, INT32U srclen)
{
	INT16S ret;

	FS_OS_LOCK();
	
	ret = fs_InserPartFile(tagfd, srcfd, tagoff, srclen);
    
	FS_OS_UNLOCK();
	
	if (ret < 0) 	 {
		handle_error_code(ret);
		return -1;
	}
	else
		return ret;
}
#endif
#endif
