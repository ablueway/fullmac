#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//����ʱ���lengthΪ0�Ͳ��������ļ�
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
	
	//���offset���ڱ�������ļ��Ĵ�С������ļ�β
	if (tagoff >= tagfnp->f_dir.dir_size)
		tagoff = tagfnp->f_dir.dir_size - 1;
	
	//���length���ڲ�����ļ����ȣ�����lengthΪ0������������ļ�
	if ((srclen > srcfnp->f_dir.dir_size) || (srclen == 0))
		srclen = srcfnp->f_dir.dir_size;
		
	//�������ļ���ƫ����(��tag_insert_cls֮��ʼ����)
	tag_insert_cls = (CLUSTER)((tagoff / tagfnp->f_dpb->dpb_secsize) >>
                         tagfnp->f_dpb->dpb_shftcnt);

	//Ҫ�����ļ���cluster��(0~src_insert_cls)
	src_insert_cls = (CLUSTER)(((srclen - 1) / srcfnp->f_dpb->dpb_secsize) >>
                         srcfnp->f_dpb->dpb_shftcnt);
    src_cls_count = src_insert_cls + 1;
	
	//���������λ�õ�ʵ��cluster��
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
	//��һ���ļ���Ҫ�������cluster:tag_next_cls
	tag_next_cls = next_cluster(tagfnp->f_dpb, st);
	
	//�����ļ�
	next = getdstart(srcfnp->f_dpb, &srcfnp->f_dir);
  #if WITHEXFAT == 1	
    link_fat(tagfnp->f_dpb, st, next,tagfnp->ext_dir->SecondaryFlags);
  #else
    link_fat(tagfnp->f_dpb, st, next,NULL);
  #endif  
	//��������ļ��Ľ���cluster��
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
	//�����ļ��Ľ���cluster�ŵ���һ��cluster��֮���������Ҫ����
	next = next_cluster(srcfnp->f_dpb, st);
	
	//���ӻ�ȥ
  #if WITHEXFAT == 1	
    link_fat(srcfnp->f_dpb, st, tag_next_cls,srcfnp->ext_dir->SecondaryFlags);
  #else
    link_fat(srcfnp->f_dpb, st, tag_next_cls,NULL);
  #endif  
    //��ʣ���cluster���
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
    
    //ɾ��������ļ�
    //delete_dir_entry(srcfnp);
    
    return SUCCESS;
}
#endif
#endif	//#ifndef READ_ONLY
