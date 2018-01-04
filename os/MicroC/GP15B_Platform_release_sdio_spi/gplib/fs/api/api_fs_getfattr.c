#include "fsystem.h"
#include "globals.h"

INT16S fs_getfattr(INT16S fd)
{
	f_node_ptr fnp = xlt_fd(fd);
	
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return DE_TOOMANY;

	return fnp->f_dir.dir_attrib;
}
