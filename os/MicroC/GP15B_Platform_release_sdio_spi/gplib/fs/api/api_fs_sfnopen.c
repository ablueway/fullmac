/************************************************************************/
/* 
	open file
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

INT16S fs_sfn_open(f_ppos ppos, INT16S open_flag)
{
	REG f_node_ptr fnp;
	INT16S	ret;
	
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY;
		return -1;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode = open_flag;	//fnp->f_mode = RDWR;
	
	fnp->f_dpb = get_dpb(ppos->f_dsk);
	
	if (media_check(fnp->f_dpb) < 0)
	{
		gFS_errno = DE_INVLDDRV; 
		release_f_node(fnp);
		return -1;
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
	fnp->f_diroff --;
	if (ret != 0)
	{
		gFS_errno = DE_MAX_FILE_NUM; 
		release_f_node(fnp);
		return ret;
	}
	
	fnp->f_offset = 0;
	fnp->f_flags = 0;
	merge_file_changes(fnp, status == S_OPENED); 

	fnp->f_cluster = getdstart(fnp->f_dpb, &fnp->f_dir);
	fnp->f_cluster_offset = 0;
	save_far_f_node(fnp);
	
#if _SEEK_SPEED_UP == 1
	// Ö»¶Á´ò¿ª
	if (O_RDONLY == (open_flag & O_ACCMODE) && !(ISFAT12(fnp->f_dpb)))
	{
		check_flink_sequence(fnp);	// just for file O_RDONLY
	}
#endif
	
	// reset file common_flag
	fnp->common_flag = 0x00;	
	
	return xlt_fnp(fnp);
}
#endif

