#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/*                                                              */
/* dos_setfsize for the file time                               */
/*                                                              */
BOOLEAN fs_setfsize(INT16S fd, INT32S size)
{
	f_node_ptr fnp;
	
	/* Translate the fd into an fnode pointer, since all internal   */
	/* operations are achieved through fnodes.                      */
	fnp = xlt_fd(fd);
	
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return FALSE;
	
	/* Change the file size                                         */
	fnp->f_dir.dir_size = size;
	
	merge_file_changes(fnp, FALSE);       /* /// Added - Ron Cemer */
	save_far_f_node(fnp);
	
	return TRUE;
}
#endif
