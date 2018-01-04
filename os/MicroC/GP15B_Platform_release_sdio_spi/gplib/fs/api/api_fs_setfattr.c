#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY

INT16S fs_setfattr(INT16S fd, INT16U attrp)
{
	f_node_ptr fnp;
	
	/* JPP-If user tries to set VOLID or DIR bits, return error */
	if ((attrp & (D_VOLID | D_DIR | 0xC0)) != 0)
		return DE_ACCESS;
	
	fnp = xlt_fd(fd);

	#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		fnp->extmain_dir.FileAttributes &= (D_VOLID | D_DIR);
		fnp->extmain_dir.FileAttributes |= attrp;
		fnp->f_flags |= F_DMOD | F_DDATE|F_ExMDir;
	}
	else
	#endif
	{
		/* Set the attribute from the fnode and return          */
		/* clear all attributes but DIR and VOLID */
		fnp->f_dir.dir_attrib &= (D_VOLID | D_DIR);   /* JPP */
		
		/* set attributes that user requested */
		fnp->f_dir.dir_attrib |= attrp;       /* JPP */
		fnp->f_flags |= F_DMOD | F_DDATE;
	}	
	return SUCCESS;
}
#endif
