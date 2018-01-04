#include "fsystem.h"
#include "globals.h"

CHAR * fs_getcwd(CHAR *buf, INT16S buflen)
{
#if WITH_CDS_PATHSTR == 1
	INT16S i;
	
	i = strlen((CHAR *) CDS[default_drive].cdsCurrentPath)+1;
	if ((i>buflen) || i>FD32_LFNPMAX)
	{
		return NULL;					
	}

	buf[0] = '\0';
	strcpy(buf, (CHAR *) CDS[default_drive].cdsCurrentPath);
#else
	f_node_ptr fnp;
	
	if (buf == NULL) 
		return NULL;
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
    	return NULL;
	}
	fnp->f_mode = RDWR;
	fnp->f_dpb = get_dpb(default_drive);
	dir_init_fnode(fnp, CDS[default_drive].cdsStrtClst);
#endif	
	return buf;	
}

