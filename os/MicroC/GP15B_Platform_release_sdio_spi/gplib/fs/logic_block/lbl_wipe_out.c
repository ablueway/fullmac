#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/*                                                              */
/* wipe out all FAT entries for create, delete, etc.            */
/*                                                              */
void wipe_out(f_node_ptr fnp)
{
  /* if already free or not valid file, just exit         */
  	if ((fnp == NULL) || checkdstart(fnp->f_dpb, &fnp->f_dir, FREE)) {	// Tristan: checkdstart belongs to file system layer, should not be called by logic layer
    	return;
    }

  	wipe_out_clusters(fnp->f_dpb, getdstart(fnp->f_dpb, &fnp->f_dir));	// Tristan: getdstart belongs to file system layer, should not be called by logic layer
}
#endif
