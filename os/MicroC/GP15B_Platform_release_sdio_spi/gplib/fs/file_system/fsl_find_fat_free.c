#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/*                                                              */
/* Find free cluster in disk FAT table                          */
/*                                                              */
CLUSTER find_fat_free(f_node_ptr fnp)
{
	REG CLUSTER idx, size, idxsec = 0;
	struct dpb *dpbp = fnp->f_dpb;
	INT8S searchall;  
	struct buffer* bp;
	INT16U *p16;
  #if WITHFAT32 == 1 || WITHEXFAT == 1
	INT32U *p32;
  #endif
	INT16U off = 0;

	// Start from optimized lookup point for start of FAT
	idx = 2;
	searchall = 0;
	
  #if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)||ISEXFAT(dpbp)) 
	{
		if (dpbp->dpb_xnfreeclst == 0) {
			return LONG_LAST_CLUSTER;
		}
		if (dpbp->dpb_xcluster != UNKNCLUSTER) {
			idx = dpbp->dpb_xcluster;
			searchall = 1;
		}
		size = dpbp->dpb_xsize + 2;
	}
	else
  #endif
	{  	
		if (dpbp->dpb_nfreeclst == 0) {
			return LONG_LAST_CLUSTER;
		}
		if (dpbp->dpb_cluster != UNKNCLUSTER) {
			idx = dpbp->dpb_cluster;
			searchall = 1;
		}
		size = dpbp->dpb_size + 2;
	}

	 // Search the FAT table looking for the first free entry.
	while (1) {
		if (ISFAT16(dpbp)) {
			idxsec = (idx >> 8) + dpbp->dpb_fatstrt;
			off = (INT16U) idx & 0xff;
	  #if WITHFAT32 == 1
		} else if (ISFAT32(dpbp)) {
			idxsec = (idx >> 7) + dpbp->dpb_fatstrt;
			off = (INT16U) idx & 0x7f;
			if (dpbp->dpb_xflags & FAT_NO_MIRRORING) {
				idxsec += (dpbp->dpb_xflags & 0xf) * dpbp->dpb_xfatsize;   // for link active fat
			}
	  #endif
	  #if WITHFAT12 == 1
	  	} else if (ISFAT12(dpbp)) {
			idxsec =  (INT16U)idx * 3;
			off = (INT16U)(idxsec & 0x3ff);
			idxsec = (idxsec >> 10) + dpbp->dpb_fatstrt;
	  #endif
	  #if WITHEXFAT == 1
	  	} else if(ISEXFAT(dpbp)){
			idxsec=((idx-2) >> 12) + dpbp->dpb_exmtable;			// 2^12=4096
			off = (INT16U) ((idx-2) & 0xFFF)>>5;
			if (dpbp->dpb_xflags & EX_FAT_NO_MIRRORING) {
				idxsec += (dpbp->dpb_xflags & 0x1) * dpbp->dpb_exmlength;   // for link active fat
			}			
	  #endif	  	
		}

		while (idx < size) {	
		  #if WITHEXFAT == 1
		    if(ISEXFAT(dpbp))
		    {	
			    bp = getMapblock(dpbp, idxsec++);
  		    }
  		    else
  		  #endif
  		        bp = getFATblock(dpbp, idxsec++);
  		        	
			if (bp == NULL) {
				return LONG_LAST_CLUSTER; 	// no other errno used
			}

			if (ISFAT16(dpbp)) {
				p16 = (INT16U *) bp->b_buffer;
				for (; off<256; off++, idx++) {
					if (p16[off] == 0) {
						break;
					}
				}
				if (off < 256) {
					break;
				}
				off = 0;
		  #if WITHFAT32 == 1
			} else if (ISFAT32(dpbp)) {
				p32 = (INT32U *) bp->b_buffer;
				for (; off<128; off++, idx++) {
					if (p32[off] == 0) {
						break;
					}
				}
				if (off < 128) {
					break;
				}
				off = 0;
		  #endif
		  #if WITHEXFAT == 1
		  }else if (ISEXFAT(dpbp)) {
		  		INT8U i,exitfag=0;
				INT8U  BitPos=(idx-2)&0x1F;
				INT32U k=1;
				k=k<<BitPos;
				
				p32 = (INT32U *) bp->b_buffer;				
				for (; off<128; off++) {
					for(i=0;i<32;i++,idx++)
					{
						if ((p32[off]&k) == 0) {
							exitfag=1;
							break;
						}						
						k=k<<1;
					}	
					k=1;
					if (exitfag) {
						break;
					}						
				}				
				if (off < 128) {
					break;
				}
				off = 0;
		  #endif
		  #if WITHFAT12 == 1
		  	} else if (ISFAT12(dpbp)) {
		  		REG INT8U *fbp0, *fbp1;
				struct buffer *bp1;
				INT16U cluster;

				for(; off < (INT16U)dpbp->dpb_secsize * 2; off += 3, idx++)
				{
					fbp0 = &bp->b_buffer[off >> 1];
					fbp1 = fbp0 + 1;

					if (off >= (INT16U)dpbp->dpb_secsize * 2 - 2) {
						bp1 = getFATblock(dpbp, (INT16U)idxsec);
						if (bp1 == 0) {
							return LONG_LAST_CLUSTER;
						}
						fbp1 = &bp1->b_buffer[0];
					}

					cluster = *fbp0 | (*fbp1 << 8);

					if (idx & 0x01) {
						cluster >>= 4;
					}
					cluster &= 0x0fff;
						
					if(cluster == 0)
					{
						break;
					}
				}
				if(off < (INT16U)dpbp->dpb_secsize * 2)
				{
					break;
				}
				off &= 1;
		  #endif
			} else {
				return LONG_LAST_CLUSTER;
			}
		}
		
		if (idx < size) {
			 break;
		}
		if (searchall == 1) {
			searchall = 0;
			idx = 2;
			// Tristan: Then scan all culster again? We should skip those we've checked.
		} else {
			break;
		}
	}

  #if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)||ISEXFAT(dpbp)) {
		dpbp->dpb_xcluster = idx;				// zhangzha: is it (dix + 1) ????

		if (idx >= size) {
			// No empty clusters, disk is FULL!
			dpbp->dpb_xcluster = UNKNCLUSTER;
			idx = LONG_LAST_CLUSTER;
			dpbp->dpb_xnfreeclst = 0;
		}
		if(ISFAT32(dpbp))
			dpbp->dpb_fsinfoflag |= FSINFODIRTYFLAG;
	}
	else
  #endif
	{
		dpbp->dpb_cluster = (INT16U) idx;		// zhangzha: is it (dix + 1) ????
		
		if ((INT16U) idx >= (INT16U) size) {
			// No empty clusters, disk is FULL!
			dpbp->dpb_cluster = UNKNCLUSTER;
			idx = LONG_LAST_CLUSTER;
			dpbp->dpb_nfreeclst = 0;
		}
	}

	return idx;
}


//从fat表中申请一段连续的cluster， InLength是要申请的cluster数量，OutLength是实际申请到的数量
//如果申请不到了，说明磁盘满了，返回LONG_LAST_CLUSTER，实际的cluster数量为0
CLUSTER fast_find_fat_free(f_node_ptr fnp, INT32U InLength, INT16U* OutLength)
{
	REG CLUSTER idx , size , idxsec = 0;
	struct dpb *dpbp = fnp->f_dpb;
	INT8S searchall;  
	struct buffer* bp;
	INT16U *p16;
#if WITHFAT32 == 1
	INT32U *p32;
#endif
	INT16U off = 0;
	
	CLUSTER start_cls_id;
	*OutLength = 0;

	/* Start from optimized lookup point for start of FAT   */
	idx = 2;
	size = dpbp->dpb_size;  
	
	searchall = 0;
  
#if WITHFAT32 == 1 || WITHEXFAT == 1
	 if (ISFAT32(dpbp)||ISEXFAT(dpbp)) 
	 {
		 if(dpbp->dpb_xnfreeclst == 0)
		 {
			 return LONG_LAST_CLUSTER;
		 }
	 }
	 else
#endif
	 {
		 if(dpbp->dpb_nfreeclst == 0)
		 {
			 return LONG_LAST_CLUSTER;
		 }
	 }

#if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)||ISEXFAT(dpbp))
	{
		if (dpbp->dpb_xcluster != UNKNCLUSTER)
		{
			idx = dpbp->dpb_xcluster;
			searchall = 1;
		}
		size = dpbp->dpb_xsize;
	}
	else
#endif
	if (dpbp->dpb_cluster != UNKNCLUSTER)
	{
		idx = dpbp->dpb_cluster;
		searchall = 1;
	}


	size += 0x02;	// new code [2/13/2006] 簇总数 ＝ 实际簇数 + 0x02

	/* Search the FAT table looking for the first free      */
	/* entry.                                               */
	while(1)
	{
		if(ISFAT16(dpbp))
		{
			idxsec =  (idx >> 8) + dpbp->dpb_fatstrt;
			off = (INT16U)idx & 0xff;
		}
#if WITHFAT32 == 1
		else if(ISFAT32(dpbp))
		{
			idxsec =  (idx >> 7) + dpbp->dpb_fatstrt;
			off = (INT16U)idx & 0x7f;
			if(dpbp->dpb_xflags & FAT_NO_MIRRORING)
			{
				idxsec += (dpbp->dpb_xflags & 0xf) * dpbp->dpb_xfatsize;   //for link active fat
			}
		}
#endif
#if WITHEXFAT == 1
		else if(ISEXFAT(dpbp))
		{

			idxsec=((idx-2) >> 12) + dpbp->dpb_exmtable;			// 2^12=4096
			off = (INT16U) ((idx-2) & 0xFFF);
			if (dpbp->dpb_xflags & EX_FAT_NO_MIRRORING) {
				idxsec += (dpbp->dpb_xflags & 0x1) * dpbp->dpb_exmlength;	// for link active fat
			}
		}
#endif
#if WITHFAT12 == 1
		else if(ISFAT12(dpbp))
		{
			idxsec =  (INT16U)idx * 3;
			off = (INT16U)(idxsec & 0x3ff);
			idxsec = (idxsec >> 10) + dpbp->dpb_fatstrt;
		}
#endif
		
		for (; idx < size;)
		{

		 #if WITHEXFAT == 1
		    if(ISEXFAT(dpbp))
		    {	
			    bp = getMapblock(dpbp, idxsec++);
  		    }
  		    else
  		  #endif
				bp = getFATblock(dpbp , idxsec++);
  			if(bp == NULL)
				return LONG_LAST_CLUSTER; //no other errno used

			if(ISFAT16(dpbp))
			{
				p16 = (INT16U *)bp->b_buffer;
				for(; (off < 256 && idx < size); off++, idx++)
				{					 
					if(p16[off] == 0)
					{
						//找到空闲块了，开始计数
						if(*OutLength == 0)
						{
							//第一次找到，记录起始cluster id
							start_cls_id = idx;
						}
						InLength--;
						(*OutLength)++;
						if(InLength == 0)
						{
							break;
						}
					}
					else
					{
						if(*OutLength)
							break;
					}
				}
				//if(off < 256)
				//	break;
				if(*OutLength)
					break;
				
				off = 0;
			}
#if WITHFAT32 == 1
			else if(ISFAT32(dpbp))
			{
				p32 = (INT32U *)bp->b_buffer;
				for(; (off < 128 && idx < size); off++, idx++)
				{
					if(p32[off] == 0)
					{
						//找到空闲块了，开始计数
						if(*OutLength == 0)
						{
							//第一次找到，记录起始cluster id
							start_cls_id = idx;
						}
						InLength--;
						(*OutLength)++;
						if(InLength == 0)
						{
							break;
						}
					}
					else
					{
						if(*OutLength)
							break;
					}
				}
				//if(off < 128)
				//	break;
				if(*OutLength)
					break;
				
				off = 0;
			}
#endif
#if WITHEXFAT == 1
			else if(ISEXFAT(dpbp))
			{
				INT8U  BitPos=off&0x1F,i;
				INT32U Valuebit=1;
				Valuebit=Valuebit<<BitPos;
				off=off>>5;
				
				p32 = (INT32U *)bp->b_buffer;				
				for(; off < 128;  off++)
				{
					for(i=BitPos;((i<32)&& (idx < size));i++, idx++)
					{
						if((p32[off]&Valuebit) == 0)
						{
							//找到空闲块了，开始计数
							if(*OutLength == 0)
							{
								//第一次找到，记录起始cluster id
								start_cls_id = idx;
							}
							InLength--;
							(*OutLength)++;
							if(InLength == 0)
							{
								break;
							}
						}
						else
						{
							if(*OutLength)
								break;
						}	
						Valuebit<<=1;
					}					
					if(*OutLength)
						break;
					Valuebit=1;
				}
				//if(off < 128)
				//	break;
				if(*OutLength)
					break;
				
				off = 0;
			}
#endif
#if WITHFAT12 == 1
			else if(ISFAT12(dpbp))
			{
				REG INT8U *fbp0, *fbp1;
				struct buffer *bp1;
				INT16U cluster;
				
				for(; (off < 1024 && idx < size); off += 3, idx++)
				{
					fbp0 = &bp->b_buffer[off >> 1];
					// pointer to next byte, will be overwritten, if not valid
					fbp1 = fbp0 + 1;
					
					if (off >= (INT16U)(dpbp->dpb_secsize - 1) * 2) {
						/* blockio.c LRU logic ensures that bp != bp1 */
						bp1 = getFATblock(dpbp, (INT16U)idxsec);
						if (bp1 == 0) {
							return LONG_LAST_CLUSTER;
						}
						
						fbp1 = &bp1->b_buffer[0];
					}

					cluster = *fbp0 | (*fbp1 << 8);
					
					// Now to unpack the contents of the FAT entry. Odd and even bytes are packed differently.
					if (idx & 0x01) {
						cluster >>= 4;
					}
					cluster &= 0x0fff;
						
					if(cluster == 0)
					{
						//找到空闲块了，开始计数
						if(*OutLength == 0)
						{
							//第一次找到，记录起始cluster id
							start_cls_id = idx;
						}
						InLength--;
						(*OutLength)++;
						if(InLength == 0)
						{
							break;
						}
					}
					else
					{
						if(*OutLength)
							break;
					}
				}
				if(*OutLength)
					break;
				
				off -= 1024;
			}
#endif
			else
			{
				return LONG_LAST_CLUSTER;
			}
		}
		
		if(idx < size)
		{
			break;
		}
		
		if(searchall==1)
		{
			if(*OutLength)
				break;
			
			searchall = 0;
			idx = 2;
		}
		else
		{
			break;
		}
	}

#if WITHFAT32 == 1 || WITHEXFAT == 1 
	if (ISFAT32(dpbp)||ISEXFAT(dpbp))
	{
		dpbp->dpb_xcluster = idx;
		//dpbp->dpb_xcluster = start_cls_id + *OutLength;

		if ((idx >= size)&&(*OutLength == 0))				// 2008.11.03 modified for dpb_xcluster large then data size 
		//if (start_cls_id >= size)
		{
			/* No empty clusters , disk is FULL!                     */
			dpbp->dpb_xcluster = UNKNCLUSTER;
			idx = LONG_LAST_CLUSTER;
			dpbp->dpb_xnfreeclst = 0; 		 //20051027 add by zhangyan
		}
	#if 0
		else if(start_cls_id + *OutLength - 1 >= size)
		{
			*OutLength = (INT16U)(size - start_cls_id);
		}
	#endif
		/* return the free entry                                */
		if(!ISEXFAT(dpbp))
		{
			dpbp->dpb_fsinfoflag |= FSINFODIRTYFLAG; //add by matao
		}
	}
	else
#endif
	{
		dpbp->dpb_cluster = idx;
		//dpbp->dpb_cluster = (INT16U)start_cls_id + *OutLength;
		
		//if (start_cls_id >= size)
		if ((idx >= size)&&(*OutLength == 0))				// 2008.11.03 modified for dpb_xcluster large then data size 
		{
		/* No empty clusters , disk is FULL!                     */
			dpbp->dpb_cluster = UNKNCLUSTER;
			idx = LONG_LAST_CLUSTER;
			dpbp->dpb_nfreeclst = 0; 		
		}
	#if 0
		else if(start_cls_id + *OutLength - 1 >= size)
		{
			*OutLength = size - start_cls_id;
		}
	#endif
	}
	/* return the free entry                                */
	//返回找到的起始cluster id
	if(*OutLength == 0)
		return LONG_LAST_CLUSTER;

	return start_cls_id;
}
#endif	//#ifndef READ_ONLY
