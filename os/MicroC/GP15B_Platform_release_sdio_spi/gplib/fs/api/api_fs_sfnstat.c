/************************************************************************/
/* 
	get file stat
	
	zhangzha creat @2006.11.28
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

INT16S fs_sfn_stat(INT16S fd, struct sfn_info *sfn_info)
{
	REG f_node_ptr fnp;
	INT16S	i;
	
	fnp = xlt_fd(fd);
	if ( (fnp == (f_node_ptr)0))
	{
		return 	 DE_INVLDHNDL;
	}
	
	if (!fnp ->f_dpb ->dpb_mounted || !fnp->f_count){						
		return DE_INVLDHNDL;		
	}

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		sfn_info->f_time = (INT16U)fnp->extmain_dir.CreateTimestamp;
		sfn_info->f_date = (INT16U)(fnp->extmain_dir.CreateTimestamp>>16);
		sfn_info->f_size = fnp->ext_dir->ValidDataLength;
		sfn_info->f_attrib = fnp->f_dir.dir_attrib;		
	}
	else
#endif
	{
		sfn_info->f_time = fnp->f_dir.dir_time;
		sfn_info->f_date = fnp->f_dir.dir_date;
		sfn_info->f_size = fnp->f_dir.dir_size;
		sfn_info->f_attrib = fnp->f_dir.dir_attrib;
	}	


#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		memcpy(sfn_info->f_name,fnp->ShortName,8);
		memcpy(sfn_info->f_extname,(void *)&fnp->ShortName[8],3);
	}
	else
#endif
	{
		for (i = 0; i < 8; i++)
		{
			if ( (fnp->f_dir.dir_name[i] == 0) || (fnp->f_dir.dir_name[i] == ' ') )
				break;

			sfn_info->f_name[i] = fnp->f_dir.dir_name[i];

		}
		sfn_info->f_name[i] = 0;
		
		for (i = 0; i < 3; i++)
		{
			if ( (fnp->f_dir.dir_name[8+i] == 0) || (fnp->f_dir.dir_name[8+i] == ' ') )
				break;

			sfn_info->f_extname[i] = fnp->f_dir.dir_name[8+i];

		}
		sfn_info->f_extname[i] = 0;
	}	
	sfn_info->f_pos.f_dsk=fnp->f_dpb->dpb_unit;
	sfn_info->f_pos.f_entry=fnp->f_dirstart;
	sfn_info->f_pos.f_offset=fnp->f_diroff;

	return SUCCESS;
}

#endif
