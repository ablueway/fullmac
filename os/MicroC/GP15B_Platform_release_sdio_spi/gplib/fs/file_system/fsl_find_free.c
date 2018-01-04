#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 0
BOOLEAN find_free(f_node_ptr fnp)
{
  	INT16S rc;

  	while ((rc=dir_read(fnp)) == 1) {
  	 	dbg_info1("find free-> rc = %d\n", rc);
    	if (fnp->f_dir.dir_name[0] == DELETED) {
      		return TRUE;
      	}
    	fnp->f_diroff++;
  	}
  	
  	return rc >= 0;
}
#endif
#endif
