#include "fsystem.h"
#include "globals.h"

/*                                                              */
/* dos_getftime for the file time                               */
/*                                                              */
INT16S fs_getftime(INT16S fd, dosdate *dp, dostime *tp)
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

	#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		*tp = (INT16U)fnp->extmain_dir.CreateTimestamp;
		*dp = (INT16U)(fnp->extmain_dir.CreateTimestamp>>16);
	}
	else
	#endif
	{
	/* Get the date and time from the fnode and return              */
		*dp = fnp->f_dir.dir_date;
		*tp = fnp->f_dir.dir_time;
	}
	return SUCCESS;
}

