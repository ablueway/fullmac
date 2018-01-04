#include "fsystem.h"
#include "globals.h"

INT32U GetBytesPerCluster (INT16S dsk) 	
{
	INT32U byte_per_clusters;
	
	if ((FileSysDrv[dsk] != NULL) && (FileSysDrv[dsk]->Status != 0) )
	{
		byte_per_clusters = (CDS[dsk].cdsDpb->dpb_clsmask+1) * CDS[dsk].cdsDpb->dpb_secsize;
		return (byte_per_clusters);
	}
	return  0xffffffff;
}
