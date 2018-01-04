#include "fsystem.h"
#include "globals.h"

/*                                                              */
/* dos_getfsize for the file time                               */
/*                                                              */
#if WITHEXFAT == 1
INT64S fs_getfsize(INT16S fd)
#else
INT32S fs_getfsize(INT16S fd)
#endif
{
	f_node_ptr fnp;

	/* Translate the fd into an fnode pointer, since all internal   */
	/* operations are achieved through fnodes.                      */
	fnp = xlt_fd(fd);
	
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return (INT32S)DE_INVLDHNDL; 	

	/* Return the file size                                         */
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		return (INT64S)(fnp->ext_dir->ValidDataLength);
	}
	else
  #endif
		return (INT32S)(fnp->f_dir.dir_size);
}

/************************************************************************/
/*  fs_tell
input:	int file handle
output: long int  file pointer 
                                                                    */
/************************************************************************/
INT32S fs_tell(INT16S fd)
{
	f_node_ptr fnp;

	/* Translate the fd into an fnode pointer, since all internal   */
	/* operations are achieved through fnodes.                      */
	fnp = xlt_fd(fd);
	
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return (INT32S)DE_INVLDHNDL; 			 //modify by zhangyan 	 05/03/25

	/* Return the file size                                         */
	//	release_near_f_node(fnp);		//mark by zhangyan 05/04/28
	return (INT32S)(fnp->f_offset);
}





