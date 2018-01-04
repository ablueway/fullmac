#include "fsystem.h"
#include "globals.h"

INT16S fs_unmount(INT8U dsk)
{
	INT16S i;
	INT16S ret;
	
	if (dsk >= MAX_DISK_NUM)
		return DE_INVLDACC;
	
	if ((FileSysDrv[dsk] == NULL) || (FileSysDrv[dsk]->Status == 0) )
		return DE_INVLDACC;
	 
	for (i = 0; i < MAX_FILE_NUM; i++)
	{
		if (f_nodes_array[i].f_count != 0)
		{
			if (f_nodes_array[i].f_dpb == get_dpb(dsk))
			{
				return -1;
			}
		}
	}
	
	clear_buffers(dsk);//flush_buffers(dsk);
	
	DPB[dsk].dpb_mounted = 0;
	
	ret = FileSysDrv[dsk]->Drv_Uninitial();
	
	return ret;
}
