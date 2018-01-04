#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
INT16S delete_dir_entry(f_node_ptr fnp)
{
	INT16S rc;
	CLUSTER d_start;
	struct dpb *dpbp;
	INT16S dsk;
	
	// Ok, so we can delete. Start out by clobbering all FAT entries for this file (or, in English, truncate the FAT).
	if ((rc=remove_lfn_entries(fnp)) < 0) {
		return rc;
	}

	if(!ISEXFAT(fnp->f_dpb))
	{
		fnp->f_dir.dir_name[0] = DELETED;
		fnp->f_flags |= F_DMOD;					// Set the bit before closing it, allowing it to be updated
	}
	dpbp = fnp->f_dpb;
	dsk = fnp->f_dpb->dpb_unit;
	d_start = getdstart(dpbp, &fnp->f_dir);

    if(unlink_step_status_get()==UNLINK_STEP_DISABLE) {    
		if (d_start != FREE) {
			DBG_PRINT ("Normal Delete\r\n");
			
			#if WITHEXFAT == 1
			  if(ISEXFAT(dpbp))
				  exfat_wipe_out_clusters(fnp);
			  else	  
			#endif		  
				  wipe_out_clusters(dpbp, d_start);
		}
		flush_buffers(dsk);
    } else {
        if (d_start != FREE) {
            DBG_PRINT ("Step Delete\r\n");
            unlink_step_init(fnp, d_start);  // Initial Step work only
    	}
    }

	dir_close(fnp);

	return SUCCESS;
}
#endif
