/************************************************************************/
/* 
	splite file
	
	zhangzha creat @2006.09.22
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//把文件拆分为A,B两部分，其中A还挂在原来的文件名下，B放在一个新建的文件名下
#if _PART_FILE_OPERATE == 1
INT16S fs_SplitFile(INT16S tagfd, INT16S srcfd, INT32U splitpoint)
{
	REG f_node_ptr tagfnp, srcfnp;
	REG CLUSTER tag_split_cls, tag_cls_count;
	INT16U	cls_size;
	REG CLUSTER tag_remain_cls, tag_next_cls;
	REG CLUSTER st, next;
	
	tagfnp = xlt_fd(tagfd);

  #if WITHEXFAT == 1
	if(ISEXFAT(tagfnp->f_dpb))
	{
		return	DE_INVLDHNDL;
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
		
	//如果src文件的长度不为0，则返回失败
	if (srcfnp->f_dir.dir_size != 0)
		return DE_INVLDPARM;
	
	//cluster size
	cls_size = tagfnp->f_dpb->dpb_secsize << tagfnp->f_dpb->dpb_shftcnt;
	//算出分割点的cluster号，在splitpoint指向的位置的下一个cluster
	tag_split_cls = (splitpoint / cls_size) + 1;
	//算出总的cluster数
	tag_cls_count = (tagfnp->f_dir.dir_size + cls_size - 1) / cls_size;
	//如果分割点在最后一个cluster或者大于最后一个cluster，退出
	if (tag_split_cls >= tag_cls_count)
		return DE_INVLDPARM;
	//如果被分割的文件大小小于一个cluster，退出
	if (tagfnp->f_dir.dir_size <= cls_size)
		return DE_INVLDPARM;
	
	tag_remain_cls = tag_split_cls;
	//算出被分割位置的实际cluster号
	st = getdstart(tagfnp->f_dpb, &tagfnp->f_dir);//tagfnp->f_dir.dir_start;
	while (tag_remain_cls > 1){
		/* get the next cluster pointed to              */
		next = next_cluster(tagfnp->f_dpb, st);
		
		if ((next == 1) || (next == LONG_LAST_CLUSTER))
			 return DE_SEEK;
				
		/* and just follow the linked list              */
		st = next;
		
		tag_remain_cls--;
	}
	//另一个文件需要链到这个cluster:tag_next_cls
	tag_next_cls = next_cluster(tagfnp->f_dpb, st);	
	setdstart(srcfnp->f_dpb, &srcfnp->f_dir, tag_next_cls);
  #if WITHEXFAT == 1	
	link_fat(tagfnp->f_dpb, st, LONG_LAST_CLUSTER,tagfnp->ext_dir->SecondaryFlags);	
  #else
    link_fat(tagfnp->f_dpb, st, LONG_LAST_CLUSTER,NULL);	
  #endif  
    //seek to file start
    srcfnp->f_offset = 0L;
    srcfnp->f_cluster_offset = 0L;
    srcfnp->f_cluster = getdstart(srcfnp->f_dpb, &srcfnp->f_dir);
    srcfnp->f_dir.dir_size = tagfnp->f_dir.dir_size - tag_split_cls * cls_size;
    srcfnp->f_flags |= F_DMOD;
	
	//seek to file start
	tagfnp->f_offset = 0L;
	tagfnp->f_cluster_offset = 0L;
    tagfnp->f_cluster = getdstart(tagfnp->f_dpb, &tagfnp->f_dir);
    tagfnp->f_dir.dir_size = tag_split_cls * cls_size;
    tagfnp->f_flags |= F_DMOD;
        
    return SUCCESS;
}
#endif
#endif	//#ifndef READ_ONLY
