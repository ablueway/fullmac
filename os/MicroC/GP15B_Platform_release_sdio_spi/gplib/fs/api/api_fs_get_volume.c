/************************************************************************/
/* 
	get volume
	
	zhangzha creat @2009.03.10
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if SUPPORT_VOLUME_FUNCTION == 1
INT16S fs_get_volume(INT8U disk_id, STVolume *pstVolume)
{
	INT32S i;
	f_node_ptr fnp;
	INT8U  path[8] = "A:\\";
	tFatFind	*psFind = &gFS_sFindData;
	
	if (disk_id >= MAX_DISK_NUM) 
	{
		return DE_INVLDPARM;
	}
	path[0] = 'A' + disk_id;
	
	if((fnp = descend_path((CHAR *)path, psFind)) == NULL)
    {
        return gFS_errno;
    }
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		fnp->f_cluster = fnp->f_dirstart;
		fnp->f_diroff = fnp->f_cluster_offset = 0;
		while (dir_lfnread(fnp, &psFind->SfnEntry)== DIRENT_SIZE)
		{
			
			if (psFind->SfnEntry.Name[0] == ENDOFDIR)
			{
				dir_close(fnp);
				return FD32_ENMFILE;
			}		
			//volume
			if((INT8U)(psFind->SfnEntry.Name[0]) == 0x83)
			{
				memset((void *)pstVolume->name,0,12);
				fd32_utf16to8_limit((UTF16 *)&psFind->SfnEntry.Name[2], (UTF8 *) pstVolume->name,(psFind->SfnEntry.Name[1])*2);
				//ExFat 没有该信息
				pstVolume->f_time = 0;
				pstVolume->f_date = 0;			
				
				dir_close(fnp);
				return SUCCESS;
			}	
			
			fnp->f_diroff += 1;			
		}
		
	}
	else	
  #endif
	{
	    /* Loop through the directory                                   */
		while (lfn_readdir(fnp, psFind) == 0)
		{
			memcpy((void*)&fnp->f_dir, (void*)&psFind->SfnEntry, sizeof(tDirEntry));
			if ( (fnp ->f_dir.dir_attrib & D_VOLID) )
		    {
				gp_memcpy((INT8S*)pstVolume->name, (INT8S*)fnp->f_dir.dir_name, 11);
				for(i = 0; i < 11; i++)
				{
					if(pstVolume->name[i] == 0x20)
					{
						pstVolume->name[i] = 0;
						break;
					}
				}
				if(i == 11)
				{
					pstVolume->name[11] = 0;
				}
				pstVolume->f_time = fnp->f_dir.dir_time;
				pstVolume->f_date = fnp->f_dir.dir_date;
				dir_close(fnp);
				return SUCCESS;
			}
		}
	}

	
	dir_close(fnp);
	return DE_FILENOTFND;
}
#endif
