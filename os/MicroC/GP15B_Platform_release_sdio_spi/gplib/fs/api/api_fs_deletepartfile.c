#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _PART_FILE_OPERATE == 1
INT16S fs_DeletePartFile(INT16S fd, INT32U offset, INT32U length)
{
	int i;
	REG f_node_ptr fnp;
	REG CLUSTER start_cls, end_cls;
	REG CLUSTER relstart_cls, relend_cls;
	REG CLUSTER st, next;
	
	fnp = xlt_fd(fd);

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
	  return  DE_INVLDHNDL;
	}
#endif	
	if ( (fnp == (f_node_ptr)0))
	{
		return 	 DE_INVLDHNDL;
	}
	
	if (!fnp ->f_dpb ->dpb_mounted || !fnp->f_count)
	{						
		return DE_INVLDHNDL;
	}
	
	if (length > fnp->f_dir.dir_size)
		length = fnp->f_dir.dir_size;
	
	//the delete part is (start_cls + 1) to (end_cls - 1)
	start_cls = (CLUSTER)((offset / fnp->f_dpb->dpb_secsize) >>
                         fnp->f_dpb->dpb_shftcnt);
	
	end_cls = (CLUSTER)(((offset + length - 1) / fnp->f_dpb->dpb_secsize) >>
                         fnp->f_dpb->dpb_shftcnt);
	
	if ((start_cls + 1) >= end_cls)
	{
		return DE_INVLDPARM;
	}
	
	//find start delete cluster
	i = start_cls;
	st = getdstart(fnp->f_dpb, &fnp->f_dir);//fnp->f_dir.dir_start;
	while (i){
		/* get the next cluster pointed to              */
		next = next_cluster(fnp->f_dpb, st);
		
		if ((next == 1) || (next == LONG_LAST_CLUSTER))
			 return DE_SEEK;
		
		/* and just follow the linked list              */
		st = next;
		
		i--;
	}
	relstart_cls = st;
	
	i = end_cls - start_cls;
	for (i = 0; i < end_cls - start_cls; i++)
	{
		/* get the next cluster pointed to              */
		next = next_cluster(fnp->f_dpb, st);
		
		if ((next == 1) || (next == LONG_LAST_CLUSTER))
			 return DE_SEEK;
		
		if (i != 0)
			link_fat(fnp->f_dpb, st, FREE,NULL);
				
		/* and just follow the linked list              */
		st = next;
	}
	relend_cls = st;
   
	/* zap the FAT pointed to                       */
  #if WITHEXFAT == 1
  	if(ISEXFAT(fnp->f_dpb))
  	{
    	link_fat(fnp->f_dpb, relstart_cls, relend_cls,fnp->ext_dir->SecondaryFlags);
  	}
  	else
  	{
  		link_fat(fnp->f_dpb, relstart_cls, relend_cls,NULL);
  	}
  #else
    link_fat(fnp->f_dpb, relstart_cls, relend_cls,NULL);
  #endif  
    fnp->f_dir.dir_size -= (end_cls - start_cls - 1) * (fnp->f_dpb->dpb_secsize << fnp->f_dpb->dpb_shftcnt);
    fnp->f_flags |= F_DMOD;
    
    return SUCCESS;
}

#endif
#endif	//#ifndef READ_ONLY

