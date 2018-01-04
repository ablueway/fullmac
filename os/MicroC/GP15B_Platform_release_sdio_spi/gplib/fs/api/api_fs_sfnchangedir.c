/************************************************************************/
/* 
	change dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"


#if FIND_EXT == 1
f_ppos sfn_changedir(STDiskFindControl *pstDiskFindControl, f_ppos ppos)
{
	REG f_node_ptr fnp;
	INT16S	ret;
	INT32U  cur_dir_cls;
	INT32U	next_dir_cls;
	INT32U  dir_cls_get;
	
	if (ppos->f_dsk != pstDiskFindControl->cur_pos.f_dsk)
	{
		gFS_errno = DE_DEVICE; 
		return NULL;
	}
	
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY; 
		return NULL;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode = RDWR;		//需要吗？
	
	fnp->f_dpb = get_dpb(ppos->f_dsk);
	
	if (media_check(fnp->f_dpb) < 0)
	{
		gFS_errno = DE_INVLDDRV; 
		release_f_node(fnp);
		return NULL;
	}
	
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
	ret = sfn_readdir(fnp);
	if (ret != 0)
	{
		gFS_errno = DE_MAX_FILE_NUM; 
		release_f_node(fnp);
		return NULL;
	}
	if ((fnp->f_dir.dir_attrib & D_DIR) != D_DIR)
	{
		release_f_node(fnp);
		gFS_errno = DE_INVLDCDFILE; 
		return NULL;
	}

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		next_dir_cls = fnp->ext_dir->FirstCluster;
	}
	else
#endif
	{
		next_dir_cls =  ((INT32U)fnp->f_dir.dir_start_high << 16) + fnp->f_dir.dir_start;
	}
	cur_dir_cls  =	pstDiskFindControl->cur_pos.f_entry;


#if WITHEXFAT == 1
	if(!ISEXFAT(fnp->f_dpb))
#endif
	{
		dir_init_fnode(fnp , next_dir_cls);	//读取next dir的 .和..项
		fnp->f_diroff = 0;		// .目录	
		sfn_readdir(fnp);	
		
		dir_cls_get = (((CLUSTER)fnp->f_dir.dir_start_high << 16) + fnp->f_dir.dir_start);  
		if((next_dir_cls!=dir_cls_get)||memcmp (fnp->f_dir.dir_name , ".          ",(FNAME_SIZE + FEXT_SIZE)))	// if 0,will return to root dir
		{
			release_f_node(fnp);
			gFS_errno = DE_INVLDCDFILE;
			return NULL;
		}
		
		fnp->f_diroff = 1;		// .. 目录
		sfn_readdir(fnp);	
		dir_cls_get = (((CLUSTER)fnp->f_dir.dir_start_high << 16) + fnp->f_dir.dir_start);
			
		if(((cur_dir_cls!=dir_cls_get)&&(dir_cls_get!=0))||memcmp (fnp->f_dir.dir_name , "..         ",(FNAME_SIZE + FEXT_SIZE)))	// if 0,will return to root dir
		{
			release_f_node(fnp);
			gFS_errno = DE_INVLDCDFILE;
			return NULL;
		}
	}


	pstDiskFindControl->cur_pos.f_entry = next_dir_cls;
	pstDiskFindControl->cur_pos.f_offset = 0;
	
	release_f_node(fnp);
	return(&pstDiskFindControl->cur_pos);
}

f_ppos sfn_changedisk(STDiskFindControl *pstDiskFindControl, INT16S dsk)
{
	pstDiskFindControl->cur_pos.f_dsk = dsk;
	pstDiskFindControl->cur_pos.f_entry = 0;
	pstDiskFindControl->cur_pos.f_offset = 0;
	pstDiskFindControl->cur_pos.f_is_root = 0;
	
	return(&pstDiskFindControl->cur_pos);
}

void sfn_getcurpos(STDiskFindControl *pstDiskFindControl, f_ppos ppos)
{
	*ppos = pstDiskFindControl->cur_pos;		// Tristan: is this correct?
}

void sfn_setcurpos(STDiskFindControl *pstDiskFindControl, f_ppos ppos)
{
	pstDiskFindControl->cur_pos = *ppos;		// Tristan: is this correct?
}

//获取当前文件所在目录的f_pos
void sfn_getcurdirpos(STDiskFindControl *pstDiskFindControl, f_ppos ppos)
{
	*ppos = pstDiskFindControl->cur_dir_pos;
}

//设置当前文件所在目录的f_pos
void sfn_setcurdirpos(STDiskFindControl *pstDiskFindControl, f_ppos ppos)
{
	pstDiskFindControl->cur_dir_pos = *ppos;
}

INT8S sfn_cmpdirpos(f_ppos ppos1, f_ppos ppos2)
{
	if( (ppos1->f_dsk == ppos2->f_dsk) && 
		(ppos1->f_entry == ppos2->f_entry) && 
		(ppos1->f_offset == ppos2->f_offset) )
	{
		return 0;
	}
	return 1;
}

#endif