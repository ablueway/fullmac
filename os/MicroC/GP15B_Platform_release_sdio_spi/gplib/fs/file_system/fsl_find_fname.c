#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 0
BOOLEAN find_fname(f_node_ptr fnp, INT8S *fcbname, INT16S attr)
{
  	while (dir_read(fnp) == 1) {
    	if ((fcbmatch(fnp->f_dir.dir_name, fcbname)) && 
            ((fnp->f_dir.dir_attrib & ~(D_RDONLY|D_ARCHIVE|attr))==0))
    	{
      		return TRUE;
    	}
    	fnp->f_diroff++;
  	}
  	
  	return FALSE;
}
#endif
#endif	// #ifndef READ_ONLY
