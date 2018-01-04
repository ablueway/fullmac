#include "fsystem.h"
#include "globals.h"

#ifndef __CS_COMPILER__
const INT8U GPLIB_FS[] = "GLB_GP_0610L_FS-GPLIB-ADS_0.0.2";
#else
const INT8U GPLIB_FS[] = "GLB_GP_0610L_FS-GPLIB-CS_0.0.2";
#endif

#ifndef READ_ONLY

#if LFN_FLAG == 0
/* alloc_find_free: resets the directory by a close followed by   */
/* an open. Then finds a spare directory entry and if not         */
/* available, tries to extend the directory.                      */
INT16S alloc_find_free(f_node_ptr fnp, INT8S *path, INT8S *fcbname)
{
  	fnp->f_flags &= ~F_DMOD;
  	dir_close(fnp);
  	fnp = split_path(path, fcbname);
  	dbg_info1("alloc_find_free(), fnp = %p\n", fnp);  

  	// Get a free f_node pointer so that we can use it in building the new file.
  	// Note that if we're in the root and we don't find an empty slot, we need to abort.
  	if (find_free(fnp) == 0) {
    	if (fnp->f_dirstart == 0) {
      		fnp->f_flags &= ~F_DMOD;
      		dir_close(fnp);
	  		dbg_info("alloc_find_free error: TOOMANY\n");
      		return DE_TOOMANY;
   	 	} else {
      		// Otherwise just expand the directory
      		INT16S ret;

      		if ((ret=extend_dir(fnp)) != SUCCESS) {
				dbg_info1("extend_dir->ret = %d\n", ret);
	  	 		// fnp already closed in extend_dir
       			return ret;
	  		}
    	}
  	}
  
  	return SUCCESS;
}
#endif
#endif	//#ifndef READ_ONLY
