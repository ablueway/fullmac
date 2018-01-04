/************************************************************************/
/* 	
	zhangzha add。
	在写数据的过程中有时需要把目录信息更新回文件系统中，比如长时间录音，
	如果中途掉电，则之前的录音数据有可能全部丢失，而调用此函数可确保之前的
	数据不会丢失。
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
