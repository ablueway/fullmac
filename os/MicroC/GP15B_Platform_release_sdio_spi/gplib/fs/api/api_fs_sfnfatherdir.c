/************************************************************************/
/* 
	change to father dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
f_ppos sfn_fatherdir(STDiskFindControl *pstDiskFindControl, f_ppos ppos)
{
	REG f_node_ptr fnp;
	INT16S	ret;



	
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
	fnp->f_mode = RDWR;
	
	fnp->f_dpb = get_dpb(ppos->f_dsk);
	
	if (media_check(fnp->f_dpb) < 0)
	{
		gFS_errno = DE_INVLDDRV; 
		release_f_node(fnp);
		return NULL;
	}
#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		pstDiskFindControl->cur_pos.f_entry = pstDiskFindControl->FatherClu[pstDiskFindControl->level-1];
	}
	else	
#endif
	{	
		dir_init_fnode(fnp, ppos->f_entry);
		fnp->f_diroff = 1;		//read file ".." to get the father dir info
		ret = sfn_readdir(fnp);
		if (ret != 0)
		{
			gFS_errno = DE_MAX_FILE_NUM; 
			release_f_node(fnp);
			return NULL;
		}
		if (memcmp(fnp->f_dir.dir_name,  "..         ", 11) )	//如果不是".."目录，则表示当前在根目录
		{
			gFS_errno = DE_INVLDCDFILE; 
			release_f_node(fnp);
			return NULL;
		}
		pstDiskFindControl->cur_pos.f_entry = ((INT32U) fnp->f_dir.dir_start_high << 16) + fnp->f_dir.dir_start;//fnp->f_dir.dir_start;

	}
	
	pstDiskFindControl->cur_pos.f_offset = 0;
	
	release_f_node(fnp);
	return(&pstDiskFindControl->cur_pos);
}
#endif
