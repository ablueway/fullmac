#include "fsystem.h"
#include "globals.h"

INT16S fs_fclose(INT16S fd)
{
	f_node_ptr fnp;
	
	/* Translate the fd into a useful pointer                       */
	fnp = xlt_fd(fd);
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return DE_INVLDHNDL;

	if (!fnp->f_count) 
	{
		return DE_INVLDHNDL;			
	}

	if (fnp->common_flag & F_REGISTER_OPEN)
	{
		fnp->common_flag &= ~F_REGISTER_OPEN;
	}	

	if (fnp->f_flags & F_DMOD)
	{
	#ifndef READ_ONLY
		if (!(fnp->f_flags & F_DDATE))
		{
			if(!ISEXFAT(fnp->f_dpb))
			{
				fnp->f_dir.dir_time = dos_gettime();
				fnp->f_dir.dir_date = dos_getdate();
			}
		  #if WITHEXFAT == 1	
			else
			{
				fnp->extmain_dir.LastAccessedTimestamp=dostimeget_ExFat();
				fnp->extmain_dir.LastAccmsIncrement=dosms_Exfat();
				fnp->f_flags|=F_ExMDir;
			}
		  #endif	
		}
		
		merge_file_changes(fnp, FALSE); 
	#else
		return DE_ACCESS;		
	#endif	
	}
	
	fnp->f_flags |= F_DDIR;
	
	dir_close(fnp);
	
	return SUCCESS;
}
