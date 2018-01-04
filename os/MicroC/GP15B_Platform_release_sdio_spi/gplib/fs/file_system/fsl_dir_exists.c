#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 0
/* checks whether directory part of path exists */
BOOLEAN dir_exists(INT8S * path)
{
	REG f_node_ptr fnp;
	INT8S fcbname[FNAME_SIZE + FEXT_SIZE];
	
	if ((fnp = split_path(path, fcbname)) == NULL) {
		return FALSE;
	}
	
	dir_close(fnp);
	return TRUE;
}

#endif
#endif	// #ifndef READ_ONLY
