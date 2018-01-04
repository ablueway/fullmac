#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
// for minidvr
INT16S delete_dir_entry2(f_node_ptr fnp)
{
	INT16S rc;
	CLUSTER d_start;
	struct dpb *dpbp;
	INT16S dsk;
	
	// Ok, so we can delete. Start out by clobbering all FAT entries for this file (or, in English, truncate the FAT).
	if ((rc=remove_lfn_entries(fnp)) < 0) {
		return rc;
	}
	
	fnp->f_dir.dir_name[0] = DELETED;
	fnp->f_flags |= F_DMOD;					// Set the bit before closing it, allowing it to be updated
	
	dpbp = fnp->f_dpb;
	dsk = fnp->f_dpb->dpb_unit;
	d_start = getdstart(dpbp, &fnp->f_dir);

	dir_close(fnp);
	
	flush_buffers(dsk);

	return SUCCESS;
}
#endif
