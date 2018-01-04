#include "fsystem.h"
#include "globals.h"

INT16S fs_changedisk( INT8U disk)
{
	if (disk > MAX_DISK_NUM-1) return DE_INVLDACC;
	
	if (DPB[disk].dpb_mounted == 0)
	{
		return DE_INVLDFUNC;
	}
	
	default_drive = disk;
	
	return 0;
}
