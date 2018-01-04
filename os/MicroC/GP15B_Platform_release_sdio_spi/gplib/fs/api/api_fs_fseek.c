#include "fsystem.h"
#include "globals.h"

/* Position the file pointer to the desired offset                      */
/* Returns a long current offset or a negative error code               */
INT64S fs_fseek64(INT16S fd, INT64S foffset, INT16S origin)
{

	REG f_node_ptr fnp;

	#if WITHEXFAT == 1
	INT64U filesize;
	#else
	INT32U filesize;
	#endif

	/* Translate the fd into a useful pointer                       	*/

	fnp = xlt_fd(fd);

	/* If the fd was invalid because it was out of range or the     	*/
	/* requested file was not open, tell the caller and exit            */
	/* note: an invalid fd is indicated by a 0 return               	*/

	if (fnp == (f_node_ptr) 0)
		return (INT32S) DE_INVLDHNDL;
	
	if (!fnp ->f_dpb ->dpb_mounted)
		return (INT32S) DE_INVLDHNDL;

	#if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			filesize=fnp->ext_dir->DataLength;
		}
		else
	#endif
		filesize=fnp->f_dir.dir_size;

	






	/* now do the actual lseek adjustment to the file poitner       	*/
	switch (origin)
	{
		/* offset from beginning of file                                */
		case 0:			
			if(foffset<0)					//Huajun
				fnp->f_offset = 0;
			else if(filesize<foffset)
				fnp->f_offset=filesize;
			else
				fnp->f_offset = foffset;
			break;
		/* offset from current location                                 */
		case 1:
			if(foffset>=0)					//Huajun
			{
				if((filesize-fnp->f_offset)<foffset)
					fnp->f_offset=filesize;
				else
					fnp->f_offset += foffset;
			}
			else
			{
				foffset*=-1;
				if(fnp->f_offset<foffset)
					fnp->f_offset=0;
				else
					fnp->f_offset -= foffset;
			}
			break;
		/* offset from eof                                              */
		case 2:
			if(foffset>0)					//Huajun
				fnp->f_offset = filesize;
			else 
			{
				foffset*=-1;
				if(foffset>filesize) 
					fnp->f_offset=0;
				else
					fnp->f_offset =filesize-foffset;
			}
			break;
		
		/* default to an invalid function                               */
		default:
			release_near_f_node(fnp);
			return (INT32S) DE_INVLDACC;
	}
	save_far_f_node(fnp);
	return fnp->f_offset;
}
/* Position the file pointer to the desired offset                      */
/* Returns a long current offset or a negative error code               */
INT32S fs_fseek(INT16S fd, INT32S foffset, INT16S origin)
{

	REG f_node_ptr fnp;

	#if WITHEXFAT == 1
	INT64U filesize;
	#else
	INT32U filesize;
	#endif

	/* Translate the fd into a useful pointer                       	*/

	fnp = xlt_fd(fd);

	/* If the fd was invalid because it was out of range or the     	*/
	/* requested file was not open, tell the caller and exit            */
	/* note: an invalid fd is indicated by a 0 return               	*/

	if (fnp == (f_node_ptr) 0)
		return (INT32S) DE_INVLDHNDL;
	
	if (!fnp ->f_dpb ->dpb_mounted)
		return (INT32S) DE_INVLDHNDL;

	#if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			filesize=fnp->ext_dir->DataLength;
		}
		else
	#endif
		filesize=fnp->f_dir.dir_size;

	/* now do the actual lseek adjustment to the file poitner       	*/
	switch (origin)
	{
		/* offset from beginning of file                                */
		case 0:			
			if(foffset<0)					//Huajun
				fnp->f_offset = 0;
			else if(filesize<foffset)
				fnp->f_offset=filesize;
			else
				fnp->f_offset = foffset;
			break;
		/* offset from current location                                 */
		case 1:
			if(foffset>=0)					//Huajun
			{
				if((filesize-fnp->f_offset)<foffset)
					fnp->f_offset=filesize;
				else
					fnp->f_offset += foffset;
			}
			else
			{
				foffset*=-1;
				if(fnp->f_offset<foffset)
					fnp->f_offset=0;
				else
					fnp->f_offset -= foffset;
			}
			break;
		/* offset from eof                                              */
		case 2:
			if(foffset>0)					//Huajun
				fnp->f_offset = filesize;
			else 
			{
				foffset*=-1;
				if(foffset>filesize) 
					fnp->f_offset=0;
				else
					fnp->f_offset =filesize-foffset;
			}
			break;
		
		/* default to an invalid function                               */
		default:
			release_near_f_node(fnp);
			return (INT32S) DE_INVLDACC;
	}
	save_far_f_node(fnp);
	return fnp->f_offset;
}

