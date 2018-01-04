/************************************************************************/
/* 
	open file
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"


#ifndef READ_ONLY

#if FIND_EXT == 1

INT16S fs_sfn_unlink(f_ppos ppos)
{
	REG f_node_ptr fnp;
	INT16S	ret;
	INT16S	i;
	
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY; 		
		return -1;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode = RDWR;
	
	fnp->f_dpb = get_dpb(ppos->f_dsk);
	
	if (media_check(fnp->f_dpb) < 0)
	{
		gFS_errno = DE_INVLDDRV; 
		goto error;
		//release_f_node(fnp);
		//return -1;
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
		goto error;
		//release_f_node(fnp);
		//return ret;
	}
	
	if (fnp->f_dir.dir_attrib & D_DIR)
	{
		gFS_errno = DE_ISDIR; 
		goto error;
		//release_f_node(fnp);
		//return -1;
	}
	
	fnp->f_diroff--;
	
	for (i = 0; i < MAX_FILE_NUM; i ++) 	 {
		if (f_nodes_array[i].f_count != 0
			&& fnp != &(f_nodes_array[i])) 	 
		{
			if (f_nodes_array[i].f_dirstart == fnp ->f_dirstart
				&& f_nodes_array[i].f_diroff == fnp ->f_diroff) 	 
			{
				gFS_errno = DE_ACCESS; 
				goto error;
				//release_f_node(fnp);
				//return -1;
			}
		}
	}
	
	return delete_dir_entry(fnp);
	
error:
	release_f_node(fnp);
	return -1;
}
#endif
#endif //#ifndef READ_ONLY

