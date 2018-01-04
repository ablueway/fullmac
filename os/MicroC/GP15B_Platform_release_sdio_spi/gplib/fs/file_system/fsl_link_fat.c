#include "fsystem.h"
#include "globals.h"

/************************************************************************/
/*                                                                      */
/*                      cluster/sector routines                         */
/*                                                                      */
/************************************************************************/

/* special "impossible" "Cluster2" value of 1 denotes reading the
   cluster number rather than overwriting it */
#define READ_CLUSTER 1

/*                                                              */
/* The FAT file system is difficult to trace through FAT table. */
/* There are two kinds of FAT's, 12 bit and 16 bit. The 16 bit  */
/* FAT is the easiest, since it is nothing more than a series   */
/* of INT16U's. The 12 bit FAT is difficult, because it packs 3  */
/* FAT entries into two BYTE's. These are packed as follows:    */
/*                                                              */
/*      0x0003 0x0004 0x0005 0x0006 0x0007 0x0008 0x0009 ...    */
/*                                                              */
/*      are packed as                                           */
/*                                                              */
/*      0x03 0x40 0x00 0x05 0x60 0x00 0x07 0x80 0x00 0x09 ...   */
/*                                                              */
/*      12 bytes are compressed to 9 bytes                      */
/*                                                              */

CLUSTER link_fat(struct dpb *dpbp, CLUSTER Cluster1, REG CLUSTER Cluster2,INT8U SecoendFlag)
{
	struct buffer *bp;
	INT16U idx;
	INT8U wasfree;
	CLUSTER clussec = Cluster1;
	CLUSTER max_cluster = dpbp->dpb_size + 1;	// Maximum cluster index = cluster number + 2 - 1

  #if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)||ISEXFAT(dpbp)) {
		max_cluster = dpbp->dpb_xsize + 1;		// Maximum cluster index = cluster number + 2 - 1
	}
  #endif
 
	if (clussec <= 1 || clussec > max_cluster) {
		return 1;
	}

	// idx is an index which is the offset of the FAT entry within the sector
	if (ISFAT12(dpbp)) {					// FAT12
		clussec = (INT16U) clussec * 3;
		idx = (INT16U) (clussec & 0x3ff);
		clussec >>= 10;					
	} else if (ISFAT16(dpbp)) {				// FAT16
		idx = (INT16U) (clussec & 0xff);
		clussec >>= 8;	
  #if WITHFAT32 == 1
	} else if (ISFAT32(dpbp)) {				// FAT32
		idx = (INT16U) (clussec & 0x7f);
		clussec >>= 7;			
  #endif		
  #if WITHEXFAT == 1
	}else if (ISEXFAT(dpbp)) {				// EXFAT
		idx = (INT16U) (clussec & 0x7f);
		clussec >>= 7;
  #endif
  	}
  else {
		return 1;
	}

	clussec += dpbp->dpb_fatstrt;
  #if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)&& (dpbp->dpb_xflags & FAT_NO_MIRRORING)) {
		// we must modify the active fat, it's number is in the 0-3 bits of dpb_xflags
		clussec += (dpbp->dpb_xflags & 0xf) * dpbp->dpb_xfatsize;
	}
	else if(ISEXFAT(dpbp)&& (dpbp->dpb_xflags & EX_FAT_NO_MIRRORING)){
		clussec += (dpbp->dpb_xflags & 0x1) * dpbp->dpb_xfatsize;
	}	
	
  #endif

  	if(ISEXFAT(dpbp)&& NOFatChain(SecoendFlag))
  	{
		;
  	}
	else{
		// Get the block that this cluster is in
		bp = getFATblock(dpbp, clussec);
		if (bp == NULL) {
			return 1; 		// the only error code possible here
		}

		if (ISFAT12(dpbp)) {
			REG INT8U *fbp0, *fbp1;
			struct buffer *bp1;
			INT16U cluster, cluster2;
			
			// form an index so that we can read the block as a byte array
			idx >>= 1;

			/* Test to see if the cluster straddles the block. If   */
			/* it does, get the next block and use both to form the */
			/* the FAT word. Otherwise, just point to the next      */
			/* block.                                               */
			fbp0 = &bp->b_buffer[idx >> (WORD_PACK_VALUE)];

			// pointer to next byte, will be overwritten, if not valid
			fbp1 = fbp0 + 1;

			if (idx >= (INT16U)dpbp->dpb_secsize - 1) {
				/* blockio.c LRU logic ensures that bp != bp1 */
				bp1 = getFATblock(dpbp, (INT16U)clussec + 1);
				if (bp1 == 0) {
					return 1; /* the only error code possible here */
				}
				
				if (Cluster2 != READ_CLUSTER) {
					bp1->b_flag |= BFR_DIRTY | BFR_VALID;
				}
				
				fbp1 = &bp1->b_buffer[0];
			}

			cluster = *fbp0 | (*fbp1 << 8);
			
			{
				INT16U res = cluster;
			
				// Now to unpack the contents of the FAT entry. Odd and even bytes are packed differently.
				if (Cluster1 & 0x01) {
					cluster >>= 4;
				}
				cluster &= 0x0fff;
				
				if ((INT16U)Cluster2 == READ_CLUSTER) {
					if (cluster >= MASK12) {
						return LONG_LAST_CLUSTER;
					}
					if (cluster == BAD12) {
						return LONG_BAD;
					}
					return cluster;
				}
				
				wasfree = 0;
				if (cluster == FREE) {
					wasfree = 1;
				}
				
				cluster = res;
			}

			// Cluster2 may be set to LONG_LAST_CLUSTER == 0x0FFFFFFFUL or 0xFFFF
			// -- please don't remove this mask!
			cluster2 = (INT16U)Cluster2 & 0x0fff;
			
			// Now pack the value in
			if ((INT16U)Cluster1 & 0x01) {
				cluster &= 0x000f;
				cluster2 <<= 4;
			} else {
				cluster &= 0xf000;
			}
			cluster |= cluster2;
			*fbp0 = (INT8U)cluster;
			*fbp1 = (INT8U)(cluster >> 8);
		} else if (ISFAT16(dpbp)) {
		    // form an index so that we can read the block as a byte array and get the cluster number
			INT16U res = getword(&bp->b_buffer[idx << (1 - WORD_PACK_VALUE)]); 
			
			if ((INT16U) Cluster2 == READ_CLUSTER) {
				if (res >= MASK16) {
					return LONG_LAST_CLUSTER;
				}
				if (res == BAD16) {
					return LONG_BAD;
				}
				
				return res;
			}
		    // Finally, put the word into the buffer and mark the buffer as dirty.
			putword(&bp->b_buffer[idx << (1 - WORD_PACK_VALUE)], (INT16U)Cluster2); 
			wasfree = 0;
			if (res == FREE) {
				wasfree = 1;
			}
	  #if WITHFAT32 == 1 || WITHEXFAT == 1
		} else if (ISFAT32(dpbp)||ISEXFAT(dpbp)) {
			// form an index so that we can read the block as a byte array
			INT32U res = getlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)]); 
			
			if (Cluster2 == READ_CLUSTER) {
				if (res > LONG_BAD) {
					return LONG_LAST_CLUSTER;
				}
				return res;
			}
			if(ISEXFAT(dpbp))
			{
				if(Cluster2==LONG_LAST_CLUSTER)
				{
					Cluster2=0xFFFFFFFF;
				}
			}
			// Finally, put the word into the buffer and mark the buffer as dirty.
			putlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)], Cluster2); 
			wasfree = 0;
			if (res == FREE) {
				wasfree = 1;
			}
	  #endif
		} else {
			return 1;
		}
  	}
	bp->b_flag |= BFR_DIRTY | BFR_VALID;	
#if WITHEXFAT == 1
	if(ISEXFAT(dpbp)&&(Cluster2!=READ_CLUSTER))	
	{
		INT32U mapadr,value,res;
		INT16U smalladr,bitadr;
		mapadr=((Cluster1-2)>>12)+dpbp->dpb_exmtable;
		smalladr=((Cluster1-2)&0xFFF)>>5;
		bitadr=(Cluster1-2)&0x1F;
		value=1;
		value=value<<bitadr;
		
		bp = getMapblock(dpbp, mapadr);
		res = getlong(&bp->b_buffer[smalladr << (2 - WORD_PACK_VALUE)]);
		if(Cluster2==FREE)
		{
			res=res&(~value); 			
		}
		else
		{
			res=res|value;
		}
		putlong(&bp->b_buffer[smalladr << (2 - WORD_PACK_VALUE)], res);
	}			
	// update the free space count
	bp->b_flag |= BFR_DIRTY | BFR_VALID;
#endif		

	if (Cluster2==FREE || wasfree) {
		INT16S adjust = 0;
		
		if (!wasfree) {
			adjust = 1;
		} else if (Cluster2 != FREE) {
			adjust = -1;
		}
		
	  #if WITHFAT32 == 1 || WITHEXFAT ==1 
		if ((ISFAT32(dpbp)||ISEXFAT(dpbp))&& dpbp->dpb_xnfreeclst!=XUNKNCLSTFREE) {
			// update the free space count for returned cluster
			dpbp->dpb_xnfreeclst += adjust;
			if(!ISEXFAT(dpbp))
			{
				dpbp->dpb_fsinfoflag |= FSINFODIRTYFLAG;
			}	
		} else
	  #endif
		if (dpbp->dpb_nfreeclst != UNKNCLSTFREE) {
			dpbp->dpb_nfreeclst += adjust;
		}
	}
	
	return SUCCESS;
}

#ifndef READ_ONLY
//快速给fat表赋值
//注意length+start_cls一定小于一个sector能存放的cluster id的数量
CLUSTER fast_link_fat( struct dpb * dpbp, CLUSTER start_cls, INT16U length,INT8U SecoendFlag)
{
	struct buffer *bp;
	INT16U idx;
//	UINT8 wasfree;
	CLUSTER clussec = start_cls;
	CLUSTER max_cluster;
	INT16U	i;

#if WITHEXFAT == 1
	CLUSTER ExFatClu=start_cls;
#endif

	

	//最大簇号 = (有效簇总数 + 2 -1 )
#if WITHFAT32 == 1 || WITHEXFAT == 1
	if(ISFAT32(dpbp)||ISEXFAT(dpbp))
		max_cluster = dpbp->dpb_xsize + 1;
	else
#endif
		max_cluster = dpbp->dpb_size + 1;

	if (clussec <= 1 || clussec > max_cluster)
		return 1;

/* idx is a pointer to an index which is the nibble offset of the FAT
   entry within the sector for FAT12 , or word offset for FAT16 , or
   dword offset for FAT32 */
	if(ISFAT16(dpbp))/* FAT16 or FAT32 */
	{
		idx = (INT16U)(clussec & 0xff);
		clussec >>= 8;
	}	
#if WITHFAT32 == 1 || WITHEXFAT == 1
	else if (ISFAT32(dpbp)||ISEXFAT(dpbp))
	{
		idx = (INT16U)(clussec & 0x7f);
		clussec >>= 7;
	}
#endif
#if WITHFAT12 == 1
	else if (ISFAT12(dpbp)) 
	{					// FAT12
		clussec = (INT16U) clussec * 3;
		idx = (INT16U) (clussec & 0x3ff);
		clussec >>= 10;
	}
#endif
	else
	{
		return -1;
	}

////////////////////////////////////////////////////////////
	clussec += dpbp->dpb_fatstrt;
#if WITHFAT32 == 1 || WITHEXFAT == 1
	if(ISFAT32(dpbp) && (dpbp->dpb_xflags & FAT_NO_MIRRORING))
	{
		if((dpbp->dpb_xflags & 0xf) == 2) 
			clussec += dpbp->dpb_xfatsize<<1;
		else
			clussec += (dpbp->dpb_xflags & 0xf) * dpbp->dpb_xfatsize;
	}
	else if(ISEXFAT(dpbp) && ((dpbp->dpb_xflags) & EX_FAT_NO_MIRRORING))
	{
		clussec += (dpbp->dpb_xflags & 0x1) * dpbp->dpb_xfatsize;	
	}
#endif
	
 	if(ISEXFAT(dpbp)&& NOFatChain(SecoendFlag))
  	{
		;
  	}
	else{
		/* Get the block that this cluster is in                */
		bp = getFATblock(dpbp , clussec);
		if (bp == NULL)
			return 1; /* the only error code possible here */

		start_cls++;
		if (ISFAT16(dpbp))
		{
			for(i = 0; i < length - 1; i++, idx++)
			{
				putword(&bp->b_buffer[idx << (1 - WORD_PACK_VALUE)] , (INT16U)start_cls++);
			}
			putword(&bp->b_buffer[idx << (1 - WORD_PACK_VALUE)] , 0xffff);
		}
	  #if WITHFAT32 == 1|| WITHEXFAT == 1
		else if (ISFAT32(dpbp)||ISEXFAT(dpbp))
		{
			for(i = 0; i < length - 1; i++, idx++)
			{
				putlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)], start_cls++); 
			}
			if(ISFAT32(dpbp))
				putlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)], LONG_LAST_CLUSTER); 
			else
			 	putlong(&bp->b_buffer[idx << (2 - WORD_PACK_VALUE)], 0xFFFFFFFF); 
		}
	  #endif
	  #if WITHFAT12 == 1
		else if (ISFAT12(dpbp)) 
		{
			REG INT8U *fbp0, *fbp1;
			struct buffer *bp1;
			INT16U cluster, cluster2;
			
			// form an index so that we can read the block as a byte array
			//idx >>= 1;
			for(i = 0; i < length; i++, idx += 3)
			{
				/* Test to see if the cluster straddles the block. If   */
				/* it does, get the next block and use both to form the */
				/* the FAT word. Otherwise, just point to the next      */
				/* block.                                               */
				fbp0 = &bp->b_buffer[idx >> 1];

				// pointer to next byte, will be overwritten, if not valid
				fbp1 = fbp0 + 1;

				if (idx >= (INT16U)(dpbp->dpb_secsize - 1) * 2) {
					/* blockio.c LRU logic ensures that bp != bp1 */
					bp1 = getFATblock(dpbp, (INT16U)clussec + 1);
					if (bp1 == 0) {
						return 1; /* the only error code possible here */
					}
					bp1->b_flag |= BFR_DIRTY | BFR_VALID;
					fbp1 = &bp1->b_buffer[0];
				}

				cluster = *fbp0 | (*fbp1 << 8);

				if(i == length - 1)
				{
					//start_cls = 0xffff;
					cluster2 = 0x0fff;
				}
				else
				{
					cluster2 = (INT16U)start_cls & 0x0fff;
				}
				
				// Now pack the value in
				if ((INT16U)start_cls & 0x01) 
				{
					cluster &= 0xf000;
				}
				else
				{
					cluster &= 0x000f;
					cluster2 <<= 4;
				}
				cluster |= cluster2;
				*fbp0 = (INT8U)cluster;
				*fbp1 = (INT8U)(cluster >> 8);
				
				start_cls++;
			}
		}
	#endif
		else
		{
			return 1;
		}
	}
	bp->b_flag |= BFR_DIRTY | BFR_VALID;
//添加BitMap 部分更新
#if WITHEXFAT == 1
	if(ISEXFAT(dpbp)&&(length!=0))
	{
		INT32U mapadr,value,res;
		INT16U smalladr,bitadr;

		for(i=0;i<length;i++)
		{
			mapadr=((ExFatClu-2)>>12)+(dpbp->dpb_exmtable);
			smalladr=((ExFatClu-2)&0xFFF)>>5;
			bitadr=(ExFatClu-2)&0x1F;
			value=1;
			value=value<<bitadr;
			
			bp = getMapblock(dpbp, mapadr);
			res = getlong(&bp->b_buffer[smalladr << (2 - WORD_PACK_VALUE)]);
			res=res|value;
			putlong(&bp->b_buffer[smalladr << (2 - WORD_PACK_VALUE)], res);
			ExFatClu++;
		}
	}
	/* update the free space count                          */
	bp->b_flag |= BFR_DIRTY | BFR_VALID;
#endif




	
#if WITHFAT32 == 1
	if (ISFAT32(dpbp) && dpbp->dpb_xnfreeclst != XUNKNCLSTFREE)
	{
		/* update the free space count for returned     */
		/* cluster                                      */
		dpbp->dpb_xnfreeclst -= length;
		dpbp->dpb_fsinfoflag |= FSINFODIRTYFLAG;
	}
	else
#endif
	if (dpbp->dpb_nfreeclst != UNKNCLSTFREE)
		dpbp->dpb_nfreeclst -= length;

	return SUCCESS;
}
#endif

