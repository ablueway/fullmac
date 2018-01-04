#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 0
/*                                                                      */
/* split a path into it's component directory and file name             */
/*                                                                      */
f_node_ptr split_path(INT8S *path, INT8S *fcbname)
{
  	REG f_node_ptr fnp;
  	INT8S tmp
  	INT16S dirlength;

	// Start off by parsing out the components.
	dirlength = ParseDosName(path, fcbname, FALSE);
	if (dirlength < SUCCESS) {
    	return NULL;
    }

  	// Translate the path into a useful pointer
  	tmp = path[dirlength];
    path[dirlength] = '\0';
    fnp = dir_open(path);
    path[dirlength] = tmp;

  	// If the fd was invalid because it was out of range or the requested file was not open, tell the caller and exit.
  	// note: an invalid fd is indicated by a 0 return
  	if (fnp==(f_node_ptr) 0 || fnp->f_count<=0) {
    	dir_close(fnp);
     	return NULL;
  	}

  	return fnp;
}
#endif
#endif	//#ifndef READ_ONLY