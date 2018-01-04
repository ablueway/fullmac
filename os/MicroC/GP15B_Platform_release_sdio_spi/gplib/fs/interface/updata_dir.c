/************************************************************************/
/* 	
	zhangzha add��
	��д���ݵĹ�������ʱ��Ҫ��Ŀ¼��Ϣ���»��ļ�ϵͳ�У����糤ʱ��¼����
	�����;���磬��֮ǰ��¼�������п���ȫ����ʧ�������ô˺�����ȷ��֮ǰ��
	���ݲ��ᶪʧ��
*/
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _DIR_UPDATA == 1

INT16S fs_UpdataDir(INT16S fd);
INT16S dir_lfn_updata(f_node_ptr fnp, tDirEntry *shortdir);

INT16S UpdataDir(INT16S fd)
{
	INT16S ret;
	
	FS_OS_LOCK();	
	
	ret = fs_UpdataDir(fd);
	
	FS_OS_UNLOCK();	
	
	if (ret!=0)
	{
		gFS_errno = DE_INVLDPARM; //invalid file handle		 
		return -1;
	}
	else
		return 0;     //success
}

#endif //#endif _DIR_UPDATA
#endif //#ifndef READ_ONLY
