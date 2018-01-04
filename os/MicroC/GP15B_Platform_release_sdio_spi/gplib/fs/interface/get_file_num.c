/************************************************************************/
/* 	
	zhangzha add��
	ͳ��ĳ�����͵��ļ���������
*/
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

INT16S StatFileNumByExtName(INT16S dsk, CHAR *extname, INT32U *filenum)
{
	int ret;
	
	FS_OS_LOCK();
	
	if (extname == 0)
	{
		FS_OS_UNLOCK();	
		return -1;
	}
	
	ret = fs_StatFileNumByExtName(dsk, (INT8S *)extname, filenum);
	
	FS_OS_UNLOCK();
	
	if (ret!=0) 	 
	{ 
		handle_error_code(ret);
		return -1; 	
	}
	else
		return 0;
}



