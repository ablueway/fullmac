/************************************************************************/
/* 
	读出一个短目录项的内容
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
INT16S GetFileInfo(f_ppos ppos, struct f_info *f_info)
{
	INT16S		ret;
	f_node_ptr	fnp;
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
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY; 
		
		FS_OS_UNLOCK();
		return -1;
	}

	/* Force the fnode into read-write mode                         */
	//fnp->f_mode = RDWR;
	
	fnp->f_dpb = get_dpb(ppos->f_dsk);
	
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

	dir_init_fnode(fnp, ppos->f_entry);

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		fnp->f_diroff = ppos->f_offset-1;		
	}
	else
#endif
	{
		fnp->f_diroff = ppos->f_offset;	
	}
	ret = sfn_readlfn(fnp, psFind);
	if (ret != 0)
	{
		release_f_node(fnp);		
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)psFind);
	#endif	
		FS_OS_UNLOCK();
		
		gFS_errno = DE_MAX_FILE_NUM; 
		return ret;
	}
	memcpy((void*)&fnp->f_dir, (void*)&psFind->SfnEntry, sizeof(tDirEntry));
	UTF8ToChart(Dir, &psFind->LongName);

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		f_info->f_time = (INT16U)fnp->extmain_dir.CreateTimestamp;
		f_info->f_date = (INT16U)(fnp->extmain_dir.CreateTimestamp>>16);
		f_info->f_size = fnp->ext_dir->ValidDataLength;
		f_info->f_attrib = fnp->f_dir.dir_attrib;
	}
	else
#endif
	{
		f_info->f_time = fnp->f_dir.dir_time;
		f_info->f_date = fnp->f_dir.dir_date;
		f_info->f_size = fnp->f_dir.dir_size;
		f_info->f_attrib = fnp->f_dir.dir_attrib;
	}	
	utf8_to_oemcp((UTF8*)psFind->ShortName, -1, f_info->f_short_name, 12);
  
	release_f_node(fnp);
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)psFind);
#endif
	FS_OS_UNLOCK();
	return SUCCESS;
}

INT16S GetFolderInfo(f_ppos ppos, struct f_info *f_info)
{
	if (ppos->f_is_root)
	{
		f_info->f_time = 0;
		f_info->f_date = 0;
		f_info->f_size = 0;
		f_info->f_attrib = D_DIR;
		
		strcpy((CHAR *) f_info->f_name, "RootDir");
		strcpy((CHAR *) f_info->f_short_name, "RootDir");
		return SUCCESS;
	} else {
		return GetFileInfo(ppos, f_info);
	}
}

#endif
