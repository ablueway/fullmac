#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//插入时如果length为0就插入整个文件
#if _PART_FILE_OPERATE == 1
INT16S fs_InserPartFile(INT16S tagfd, INT16S srcfd, INT32U tagoff, INT32U srclen)
{
	REG f_node_ptr tagfnp, srcfnp;
	REG CLUSTER src_insert_cls, src_cls_count;
	REG CLUSTER tag_insert_cls, tag_next_cls;
	REG CLUSTER st, next;
	
	tagfnp = xlt_fd(tagfd);
	
#if WITHEXFAT == 1
	if(ISEXFAT(tagfnp->f_dpb))
	{
	  return  DE_INVLDHNDL;
	}
#endif
	
	if ((tagfnp == (f_node_ptr)0))
	{
		return	DE_INVLDHNDL;
	}
	
	srcfnp = xlt_fd(srcfd);
	if ((srcfnp == (f_node_ptr)0))
	{
		return	DE_INVLDHNDL;
	}
		
	if (!srcfnp->f_dpb->dpb_mounted || !srcfnp->f_count || !tagfnp->f_dpb->dpb_mounted || !tagfnp->f_count)
	{						
		return DE_INVLDHNDL;
	}
	
	//如果offset大于被插入的文件的大小则插入文件尾
	if (tagoff >= tagfnp->f_dir.dir_size)
		tagoff = tagfnp->f_dir.dir_size - 1;
	
	//如果length大于插入的文件长度，或者length为0，则插入整个文件
	if ((srclen > srcfnp->f_dir.dir_size) || (srclen == 0))
		srclen = srcfnp->f_dir.dir_size;
		
	//被插入文件的偏移量(从tag_insert_cls之后开始插入)
	tag_insert_cls = (CLUSTER)((tagoff / tagfnp->f_dpb->dpb_secsize) >>
                         tagfnp->f_dpb->dpb_shftcnt);

	//要插入文件的cluster号(0~src_insert_cls)
	src_insert_cls = (CLUSTER)(((srclen - 1) / srcfnp->f_dpb->dpb_secsize) >>
                         srcfnp->f_dpb->dpb_shftcnt);
    src_cls_count = src_insert_cls + 1;
	
	//算出被插入位置的实际cluster号
	st = getdstart(tagfnp->f_dpb, &tagfnp->f_dir);//tagfnp->f_dir.dir_start;
	while (tag_insert_cls){
		/* get the next cluster pointed to              */
		next = next_cluster(tagfnp->f_dpb, st);
		
		if ((next == 1) || (next == LONG_LAST_CLUSTER))
			 return DE_SEEK;
				
		/* and just follow the linked list              */
		st = next;
		
		tag_insert_cls--;
	}
	//另一个文件需要链回这个cluster:tag_next_cls
	tag_next_cls = next_cluster(tagfnp->f_dpb, st);
	
	//插入文件
	next = getdstart(srcfnp->f_dpb, &srcfnp->f_dir);
  #if WITHEXFAT == 1	
    link_fat(tagfnp->f_dpb, st, next,tagfnp->ext_dir->SecondaryFlags);
  #else
    link_fat(tagfnp->f_dpb, st, next,NULL);
  #endif  
	//算出插入文件的结束cluster号
	st = getdstart (srcfnp->f_dpb, &srcfnp->f_dir);//srcfnp->f_dir.dir_start;
	while (src_insert_cls){
		/* get the next cluster pointed to              */
		next = next_cluster(srcfnp->f_dpb, st);
		
		if ((next == 1) || (next == LONG_LAST_CLUSTER))
			 return DE_SEEK;
				
		/* and just follow the linked list              */
		st = next;
		
		src_insert_cls--;
	}
	//插入文件的结束cluster号的下一个cluster，之后的内容需要清零
	next = next_cluster(srcfnp->f_dpb, st);
	
	//链接回去
  #if WITHEXFAT == 1	
    link_fat(srcfnp->f_dpb, st, tag_next_cls,srcfnp->ext_dir->SecondaryFlags);
  #else
    link_fat(srcfnp->f_dpb, st, tag_next_cls,NULL);
  #endif  
    //把剩余的cluster清空
    st = next;  
    while (st != LONG_LAST_CLUSTER){		
		/* get the next cluster pointed to              */
		next = next_cluster(srcfnp->f_dpb, st);
		
		if (next == 1)
			 return DE_SEEK;
		
		/* free cur cluster	*/
	  #if WITHEXFAT == 1								
		link_fat(srcfnp->f_dpb, st, FREE,srcfnp->ext_dir->SecondaryFlags);		
	  #else
	    link_fat(srcfnp->f_dpb, st, FREE,NULL);		
	  #endif					
		/* and just follow the linked list              */
		st = next;		
	}
	
    tagfnp->f_dir.dir_size += src_cls_count * (tagfnp->f_dpb->dpb_secsize << tagfnp->f_dpb->dpb_shftcnt);
    tagfnp->f_flags |= F_DMOD;
    
    srcfnp->f_dir.dir_size = 0;
    //srcfnp->f_dir.dir_start = 0;
    setdstart(srcfnp->f_dpb, &srcfnp->f_dir, FREE);
    srcfnp->f_flags |= F_DMOD;
    
    //删除插入的文件
    //delete_dir_entry(srcfnp);
    
    return SUCCESS;
}
#endif
#endif	//#ifndef READ_ONLY
