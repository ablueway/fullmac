/*                                                              */
/* dos_setftime for the file time                               */
/*                                                              */
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
// 2004-11-23 Yongliang add this function change access time together
INT16S fs_setftime(INT16S fd, INT16U acdate, INT16U moddate, INT16U modtime) 	 
{
	 f_node_ptr fnp;

	 /* Translate the fd into an fnode pointer, since all internal   */
	 /* operations are achieved through fnodes.                      */
	 fnp = xlt_fd(fd);
	 /* If the fd was invalid because it was out of range or the     */
	 /* requested file was not open, tell the caller and exit        */
	 /* note: an invalid fd is indicated by a 0 return               */
	 if (fnp == (f_node_ptr) 0)
		 return DE_INVLDHNDL;
	 /* Set the date and time from the fnode and return              */
	 fnp ->f_dir.dir_date = moddate;
	 fnp ->f_dir.dir_time = modtime;
	 fnp ->f_dir.dir_accdate = acdate;

	 /* mark file as modified and set this date upon closing */
	 fnp ->f_flags |= F_DMOD | F_DDATE; 	
	 save_far_f_node (fnp);
	 return SUCCESS;
}
#endif
