#include "fsystem.h"
#include "globals.h"

INT16U GetSectorsPerCluster(INT16U dsk)
{	
	if ((FileSysDrv[dsk] != NULL) && (FileSysDrv[dsk]->Status != 0) )
	{
		return(CDS[dsk].cdsDpb->dpb_clsmask + 1);
	}
	return  0xffff;
}

