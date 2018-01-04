#include "fsystem.h"
#include "globals.h"

/* get current directory structure for drive
   return NULL if the CDS is not valid or the
   drive is not within range */
struct cds * get_cds(INT16U drive)
{
	struct cds *cdsp;
	INT16U flags;

	if (drive >= MAX_DISK_NUM) {
		return NULL;
	}

	if ((FileSysDrv[drive]==NULL) || (FileSysDrv[drive]->Status==0)) {
		return NULL;
	}

	cdsp = &CDS[drive];
	flags = cdsp->cdsFlags;
	
	// Entry is disabled or JOINed drives are accessable by the path only
	if (!(flags & CDSVALID)) {
		return NULL;
	}
	
	if (cdsp->cdsDpb == NULL) {
		return NULL;
	}
	
	return cdsp;
}
