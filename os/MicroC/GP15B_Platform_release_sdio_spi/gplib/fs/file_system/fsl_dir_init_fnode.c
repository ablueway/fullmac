#include "fsystem.h"
#include "globals.h"

/* Description.
 *  Initialize a fnode so that it will point to the directory with 
 *  dirstart starting cluster; in case of passing dirstart == 0
 *  fnode will point to the start of a root directory */
void dir_init_fnode(f_node_ptr fnp, CLUSTER dirstart)
{
	// reset the directory flags
	fnp->f_flags = F_DDIR;
	fnp->f_diroff = 0;
	fnp->f_offset = 0;
	fnp->f_cluster_offset = 0;
	
	// root directory
  #if WITHFAT32 == 1 || WITHEXFAT == 1 
	if (dirstart == 0) {
		if (ISFAT32(fnp->f_dpb)||(ISEXFAT(fnp->f_dpb))) {
			dirstart = fnp->f_dpb->dpb_xrootclst;
		}
	}
  #endif
	fnp->f_cluster = fnp->f_dirstart = dirstart;
}
