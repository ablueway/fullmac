#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY 


#if WITHEXFAT == 1
CLUSTER ExfatMapTable( struct dpb * dpbp, CLUSTER start_cls, INT32U length)
{
	struct buffer *bp;
	INT16U idx;
	CLUSTER clussec = start_cls;
	CLUSTER max_cluster;
	INT16U	i;
	INT32U LinkCount=0;
	
	//最大簇号 = (有效簇总数 + 2 -1 )
	max_cluster = dpbp->dpb_xsize + 1;
	if (clussec <= 1 || clussec > max_cluster)
		return 1;
////////////////////////////////////////////////////////////
	clussec >>= 7;
	clussec += dpbp->dpb_fatstrt;
	if((dpbp->dpb_xflags) & EX_FAT_NO_MIRRORING)
	{
		clussec += (dpbp->dpb_xflags & 0x1) * dpbp->dpb_xfatsize;	
	}
///////////////////////////////////////////////////////////
	while(1)
	{
		/* Get the block that this cluster is in                */
		bp = getFATblock(dpbp , clussec);
		if (bp == NULL)
			return 1; 	   /* the only error code possible here */
		/* update the free space count                          */
		bp->b_flag |= BFR_DIRTY | BFR_VALID;		
				
		//在一个Secotor里最大link数量
		LinkCount=0x7f-start_cls&0x7f;
		idx = (INT16U)(start_cls & 0x7f);

		start_cls++;
		for(i = 0; i < LinkCount; i++, idx++)
		{
			putlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)], start_cls++); 
			length--;
			if(length==0)
			{
				putlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)], 0xFFFFFFFF); 
				break;
			}
		}

		if(length==0)
		{
			break;
		}
		clussec++;
	}		 

	return SUCCESS;
}




INT8U LinkMapTable(f_node_ptr fnp)
{
	CLUSTER ClusterStar;
	
	ClusterStar=getdstart(fnp->f_dpb, &fnp->f_dir);
	fnp->ext_dir->SecondaryFlags=fnp->ext_dir->SecondaryFlags&(~0x02);

	ExfatMapTable(fnp->f_dpb, ClusterStar, fnp->f_cluster-ClusterStar+1);
	return 0;
}

#endif




CLUSTER extend(f_node_ptr fnp)
{
  	CLUSTER free_fat;

  	// Get an empty cluster, so that we use it to extend the file.
  	free_fat = find_fat_free(fnp);

  	// No empty clusters, disk is FULL! Translate into a useful error message.
  	if (free_fat == LONG_LAST_CLUSTER) {
    	return free_fat;
    }

  	// Now that we've found a free FAT entry, mark it as the last entry and save.
  	if (fnp->f_cluster == FREE) {		
		setdstart(fnp->f_dpb, &fnp->f_dir, free_fat);
	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{   
			fnp->ext_dir->SecondaryFlags|=0x02;
			fnp->f_link_attribute=FLINK_ATTR_LINER;	
			fnp->f_start_cluster=free_fat; 	
			link_fat(fnp->f_dpb, free_fat, LONG_LAST_CLUSTER,fnp->ext_dir->SecondaryFlags);	  
		}	
		else
		{
			link_fat(fnp->f_dpb, free_fat, LONG_LAST_CLUSTER,NULL);
		}
	  #else 
		link_fat(fnp->f_dpb, free_fat, LONG_LAST_CLUSTER,NULL);
	  #endif		
	} else {
		if(!ISEXFAT(fnp->f_dpb))
		{
            link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,NULL);
			link_fat(fnp->f_dpb, free_fat, LONG_LAST_CLUSTER,NULL);		    
		}
		#if WITHEXFAT == 1
		else
		{		
			if(NOFatChain(fnp->ext_dir->SecondaryFlags))
			{
				fnp->f_link_attribute=FLINK_ATTR_LINER;		
				if((fnp->f_cluster+1)!=free_fat)
				{
					LinkMapTable(fnp);
					link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,fnp->ext_dir->SecondaryFlags);
					link_fat(fnp->f_dpb, free_fat, 0xFFFFFFFF,fnp->ext_dir->SecondaryFlags);					
					fnp->f_link_attribute=0;
				}
			}
			else
			{
				link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,fnp->ext_dir->SecondaryFlags);
				link_fat(fnp->f_dpb, free_fat, 0xFFFFFFFF,fnp->ext_dir->SecondaryFlags);
			}			
		}
		#endif
	}
  	//link_fat(fnp->f_dpb, free_fat, LONG_LAST_CLUSTER);

  	// Mark the directory so that the entry is updated
  	fnp->f_flags |= F_DMOD;
  	
  	return free_fat;
}

//申请新的cluster，输入需要申请的cluster数量，返回起始cluster id和实际申请到的cluster数量
//如果申请不到了，说明磁盘满了，返回LONG_LAST_CLUSTER，实际的cluster数量为0
CLUSTER fast_extend(f_node_ptr fnp, INT32U InLength, INT16U* OutLength)
{
	CLUSTER free_fat;
	INT32U  sect1,sect2;	// add by zurong 2011.3.22
	
//	CLUSTER	next_cls;
//	INT16U	i;
	
	/* get an empty cluster, so that we use it to extend the file.  */
	//从fat表中申请一段连续的cluster， InLength是要申请的cluster数量，OutLength是实际申请到的数量
	//如果申请不到了，说明磁盘满了，返回LONG_LAST_CLUSTER，实际的cluster数量为0
	free_fat = fast_find_fat_free(fnp, InLength, OutLength);
	
	/* No empty clusters, disk is FULL! Translate into a useful     */
	/* error message.                                               */
	if (free_fat == LONG_LAST_CLUSTER)
		return free_fat;

	/* Now that we've found a free FAT entry, mark it as the last   */
	/* entry and save.                                              */
	if (fnp->f_cluster == FREE)
	{
		setdstart(fnp->f_dpb, &fnp->f_dir, free_fat);
	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb)&&(LinkFlag[fnp->f_dpb->dpb_unit]==0))
		{
			fnp->ext_dir->SecondaryFlags|=0x02;
			fnp->f_link_attribute=FLINK_ATTR_LINER; 
			fnp->f_start_cluster=free_fat;		
		}	
	  #endif		
	}
	else
	{
	  #if WITHEXFAT == 1		
		if(NOFatChain(fnp->ext_dir->SecondaryFlags)&&(ISEXFAT(fnp->f_dpb)))
		{
			fnp->f_link_attribute=FLINK_ATTR_LINER; 
			if((fnp->f_cluster+1)!=free_fat)
			{
				LinkMapTable(fnp);
				link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,fnp->ext_dir->SecondaryFlags);		
				fnp->f_link_attribute=0;
			}
		}
		else
	  #endif	
	  #if WITHEXFAT == 1
	  	if(ISEXFAT(fnp->f_dpb))
		{
			link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,fnp->ext_dir->SecondaryFlags);
	  	}
	  	else
	  	{
	  		link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,NULL);
	  	}
	  #else
	    link_fat(fnp->f_dpb, fnp->f_cluster, free_fat,NULL);
	  #endif  
	}
  #if WITHEXFAT == 1	
  	if(ISEXFAT(fnp->f_dpb))
  	{
		fast_link_fat(fnp->f_dpb, free_fat, *OutLength,fnp->ext_dir->SecondaryFlags);	
  	}
  	else
  	{
  		fast_link_fat(fnp->f_dpb, free_fat, *OutLength,NULL);	
  	}
  #else
    fast_link_fat(fnp->f_dpb, free_fat, *OutLength,NULL);	
  #endif	
//	 
//	next_cls = free_fat;
//	for(i = 1; i < *OutLength; i++)
//	{
//		link_fat(fnp->f_dpb, next_cls, next_cls + 1);
//		next_cls += 1;
//	}
//	
//	link_fat(fnp->f_dpb, next_cls, LONG_LAST_CLUSTER);

#if 1//zurong add 2011.3.23;当结束fat chain结束换到其他sector时及时回写到disk，以免掉电后，导致fat chain无结束cluster 0xffff
		if (ISFAT16(fnp->f_dpb))
		{			
			sect1 = (free_fat-1)>>8;
			sect2 = (free_fat-1 + *OutLength)>>8;
		}
	#if WITHFAT32 == 1|| WITHEXFAT == 1	
		else if (ISFAT32(fnp->f_dpb)||ISEXFAT(fnp->f_dpb))
		{
			sect1 = (free_fat-1)>>7;
			sect2 = (free_fat-1 + *OutLength)>>7;
		}
	#endif
	#if WITHFAT12 == 1
		else if (ISFAT12(fnp->f_dpb)) 
		{
			sect1 =  (free_fat-1) * 3;
			sect1 = (sect1 >> 10);
			
			sect2 =  (free_fat-1 + *OutLength) * 3;
			sect2 = (sect2 >> 10);			
		}
	#endif
		else
		{
			sect1 = sect2;
		}
	
	if(sect1 != sect2)
	{
		flush_fat_buffers(fnp->f_dpb->dpb_unit);
	}

#endif
	
	/* Mark the directory so that the entry is updated              */
	fnp->f_flags |= F_DMOD;
	return free_fat;
}

#endif //#ifndef READ_ONLY
