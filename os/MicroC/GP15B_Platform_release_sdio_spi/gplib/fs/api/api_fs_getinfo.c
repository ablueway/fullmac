/************************************************************************/
/* 
	读出一个短目录项的内容  HuajunAdd
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"
INT16S FsgetFileInfo(INT16S fd, struct f_info *f_info)
{
	INT16S		ret;
	f_node_ptr	fnp;
	f_node_ptr  fnp1;
#if MALLOC_USE == 1
	tFatFind*	psFind;
#elif USE_GLOBAL_VAR == 1
	tFatFind	*psFind = &gFS_sFindData;
#else
	tFatFind	sFindData;
	tFatFind*	psFind = &sFindData;
#endif
	INT8S *		Dir = f_info->f_name;	// 直接使用文件名空间，不再需要进行空间的申请
	
	FS_OS_LOCK();
	/* Allocate an fnode if possible - error return (0) if not.     */	
	if ((fnp1 = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY; 
		
		FS_OS_UNLOCK();
		return -1;
	}
	/* Force the fnode into read-write mode                         */
	//fnp->f_mode = RDWR;
	//fnp->f_dpb = get_dpb(ppos->f_dsk);
	fnp = xlt_fd(fd);
	if (media_check(fnp->f_dpb) < 0)
	{
		release_f_node(fnp);		
		FS_OS_UNLOCK();
		
		gFS_errno = DE_INVLDDRV; 
		return -1;
	}
	
#if MALLOC_USE == 1
	psFind = (tFatFind*)FS_MEM_ALLOC(sizeof(tFatFind));
	if (NULL == psFind) 
	{
		release_f_node(fnp);
		FS_OS_UNLOCK();
		
		gFS_errno = DE_NOMEM; 
		return -1;
	}
#endif
	*fnp1=*fnp;				//复制一份fnp,防止底层操作更改fnp导致退出后不能再用
	
	fnp1->f_cluster = fnp1->f_dirstart;
	if(ISEXFAT(fnp1->f_dpb))
	{
		fnp1->f_diroff--;
	}
	ret = sfn_readlfn(fnp1, psFind);
	
	if (ret != 0)
	{
		release_f_node(fnp1);		
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)psFind);
	#endif	
		FS_OS_UNLOCK();
		
		gFS_errno = DE_MAX_FILE_NUM; 
		return ret;
	}
	memcpy((void*)&fnp1->f_dir, (void*)&psFind->SfnEntry, sizeof(tDirEntry));
	UTF8ToChart(Dir, &psFind->LongName);
	f_info->f_attrib = fnp1->f_dir.dir_attrib;
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp1->f_dpb))
	{
		f_info->f_size = fnp1->ext_dir->ValidDataLength;
		fat_getftime(fd,&f_info->f_date,&f_info->f_time);
	}
	else
  #endif
  	{
		f_info->f_time = fnp1->f_dir.dir_time;
		f_info->f_date = fnp1->f_dir.dir_date;
		f_info->f_size = fnp1->f_dir.dir_size;
		utf8_to_oemcp((UTF8*)psFind->ShortName, -1, f_info->f_short_name, 12);
  	}

	f_info->entry=fnp1->f_cluster;
    release_f_node(fnp1);
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)psFind);
#endif
	FS_OS_UNLOCK();
	return SUCCESS;
}
