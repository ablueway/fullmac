/* Description.
 *    Finds the cluster which contains byte at the fnp->f_offset offset and
 *  stores its number to the fnp->f_cluster. The search begins from the start of
 *  a file or a directory depending whether fnp->f_ddir is FALSE or TRUE
 *  and continues through the FAT chain until the target cluster is found.
 *  The mode can have only XFR_READ or XFR_WRITE values.
 *    In the XFR_WRITE mode map_cluster extends the FAT chain by creating
 *  new clusters upon necessity.
 * Return value.
 *  DE_HNDLDSKFULL - [XFR_WRITE mode only] unable to find free cluster
 *                   for extending the FAT chain, the disk is full.
 *                   The fnode is released from memory.
 *  DE_SEEK        - [XFR_READ mode only] byte at f_offset lies outside of
 *                   the FAT chain. The fnode is not released.
 * Notes.
 *  If we are moving forward, then use the relative cluster number offset
 *  that we are at now (f_cluster_offset) to start, instead of starting
 *  at the beginning. */
#include "fsystem.h"
#include "globals.h"

INT16S map_cluster(REG f_node_ptr fnp, INT16S mode)
{
	CLUSTER relcluster, cluster;
  #if WITHEXFAT == 1
    CLUSTER AvidCluslong;
  #endif  
    
	if (fnp->f_cluster == FREE) {
		// If this is a read but the file still has zero bytes return immediately....
		if (mode == XFR_READ) {
			return DE_SEEK;
		}
	#ifndef READ_ONLY	
		// If someone did a seek, but no writes have occured, we will need to initialize the fnode.
		// (mode == XFR_WRITE)
		cluster = extend(fnp);
		if (cluster == LONG_LAST_CLUSTER) {		// If there are no more free fat entries, then we are full!
			return DE_HNDLDSKFULL;
		}
		fnp->f_cluster = cluster;
	#else
		return DE_ACCESS;
	#endif		
	}
  
	relcluster = (CLUSTER)((fnp->f_offset / fnp->f_dpb->dpb_secsize) >> fnp->f_dpb->dpb_shftcnt);

	if (relcluster < fnp->f_cluster_offset) {
		// Set internal index and cluster size
		fnp->f_cluster = (fnp->f_flags & F_DDIR) ? fnp->f_dirstart : getdstart(fnp->f_dpb, &fnp->f_dir);
		fnp->f_cluster_offset = 0;
	}
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
	  INT32U clucount;
	  clucount=fnp->f_dpb->dpb_secsize;
	  clucount=(clucount<<(fnp->f_dpb->dpb_shftcnt))-1;
	  clucount=((fnp->ext_dir->DataLength)+clucount)/(fnp->f_dpb->dpb_secsize);
	  AvidCluslong=clucount>>(fnp->f_dpb->dpb_shftcnt); 			  
	}
  #endif				  

	/* Now begin the linear search. The relative cluster is         */
	/* maintained as part of the set of physical indices. It is     */
	/* also the highest order index and is mapped directly into     */
	/* physical cluster. Our search is performed by pacing an index */
	/* up to the relative cluster position where the index falls    */
	/* within the cluster.                                          */

	while (fnp->f_cluster_offset != relcluster) {
		// get next cluster in the chain
	  #if _SEEK_SPEED_UP == 1
		if (fnp->f_link_attribute) {
			cluster = fast_next_cluster(fnp, relcluster);		// Tristan: fast_next_cluster is in API layer, should not be called by file system layer
		  #if WITHEXFAT == 1
			if((relcluster>=AvidCluslong)&&(ISEXFAT(fnp->f_dpb)))
			{
				cluster=LONG_LAST_CLUSTER;
			}
		  #endif
			if (cluster == 1) {
				cluster = next_cluster(fnp->f_dpb, fnp->f_cluster);
			} else {
				fnp->f_cluster = cluster;		 
				fnp->f_cluster_offset = relcluster;
			}
		} else {
			cluster = next_cluster(fnp->f_dpb, fnp->f_cluster);
		}
	  #else
		cluster = next_cluster(fnp->f_dpb, fnp->f_cluster); 	  
	  #endif
	
		if (cluster == 1) {
			return DE_SEEK;
		} else if (cluster == LONG_LAST_CLUSTER) {
			// If this is a read and the next is a LAST_CLUSTER, then we are going to read past EOF, return zero read
			if (mode == XFR_READ) {
				return DE_SEEK;
			}
		#ifndef READ_ONLY	
			// Expand the list if we're going to write and have run into the last cluster marker.
			cluster = extend(fnp);
			if (cluster == LONG_LAST_CLUSTER) {
				return DE_HNDLDSKFULL;
			}
		#else
			return DE_ACCESS;
		#endif	
		}
		
		if (fnp->f_cluster_offset == relcluster) {
			break;
		} else {
			fnp->f_cluster = cluster;
			fnp->f_cluster_offset++;
		}
	}
	
	return SUCCESS;
}


#ifndef READ_ONLY
INT16S fast_map_cluster(f_node_ptr fnp, INT32U InLength, INT16U* OutLength, INT16S mode)
{
	CLUSTER relcluster, cluster = 0;
	CLUSTER	next_cls;
	INT16U	ClusterSize;
	CLUSTER	total_cluster;
	INT32U	i;
	INT16U	secsize;
#if WITHEXFAT == 1	
	INT32U AvidCluslong;
#endif	
	if (fnp->f_cluster == FREE)
	{
		if(mode == XFR_READ)
			return DE_SEEK;
	#ifndef READ_ONLY		
		//申请新的cluster，输入需要申请的cluster number，返回起始cluster id和实际申请到的cluster数量
		//如果申请不到了，说明磁盘满了，返回LONG_LAST_CLUSTER，实际的cluster数量为0
	#if WITHEXFAT == 1	
		if(ISEXFAT(fnp->f_dpb))
		{
			fnp->ext_dir->SecondaryFlags|=0x02;
			if(LinkFlag[fnp->f_dpb->dpb_unit])
			{
			    fnp->ext_dir->SecondaryFlags&=~0x02;
			}
		}
	#endif
		cluster = fast_extend(fnp, InLength, OutLength);
		if (cluster == LONG_LAST_CLUSTER)
		{
			return DE_HNDLDSKFULL;
		}
		
		#if WITHEXFAT == 1
			if(ISEXFAT(fnp->f_dpb)&&(LinkFlag[fnp->f_dpb->dpb_unit]==0))
			{
				fnp->ext_dir->SecondaryFlags|=0x02;
				fnp->f_link_attribute=FLINK_ATTR_LINER;
				fnp->f_start_cluster=cluster;
			}
		#endif	
		//??????需要在哪里update fat表的buffer呢？
		//fast_updata_fat_buffer(cluster, *OutLength);
		fnp->f_cluster = cluster;		//找到的起始clsuter id，length从*OutLength返回

	#if 1		// zurong add for restore file space,为了安全起见，先flush fat buffer，再flush dir information
		flush_fat_buffers(fnp->f_dpb->dpb_unit);		
		dir_lfn_updata(fnp, (tDirEntry*)&fnp->f_dir);
		/* Clear buffers after release*/
		/* hazard: no error checking! */
		flush_buffers(fnp->f_dpb->dpb_unit);
	#endif
		
		return SUCCESS;
	#else
		return DE_ACCESS;		

	#endif		
	}
	
	relcluster = (CLUSTER)((fnp->f_offset / fnp->f_dpb->dpb_secsize) >>
				fnp->f_dpb->dpb_shftcnt);
				
	if (relcluster < fnp->f_cluster_offset)
	{
		//因为向前seek了，所以要从头开始查找该文件的fat表
		/* Set internal index and cluster size.                 */
		fnp->f_cluster = (fnp->f_flags & F_DDIR) ? fnp->f_dirstart :
							getdstart(fnp->f_dpb , &fnp->f_dir);
		fnp->f_cluster_offset = 0;
	}
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		INT32U clucount;
		clucount=fnp->f_dpb->dpb_secsize;
		clucount=(clucount<<(fnp->f_dpb->dpb_shftcnt))-1;
		clucount=((fnp->ext_dir->DataLength)+clucount)/(fnp->f_dpb->dpb_secsize);
		AvidCluslong=clucount>>(fnp->f_dpb->dpb_shftcnt);				
	}
  #endif	 			
	//先定位到当前offset对应的cluster
	while (fnp->f_cluster_offset != relcluster)
	{
	/* get next cluster in the chain */
	#if _SEEK_SPEED_UP == 1
		if(fnp->f_link_attribute)
		{
			cluster = fast_next_cluster(fnp , relcluster);
		  #if WITHEXFAT == 1
		    if((relcluster>=AvidCluslong)&&(ISEXFAT(fnp->f_dpb)))
		    {
				cluster=LONG_LAST_CLUSTER;
			}
		  #endif
			if(cluster == 1)
			{
				//如果当前的offset没有落在可加速的范围内，则使用未加速的查找cluster函数
				cluster = next_cluster(fnp->f_dpb, fnp->f_cluster);
			}
			else
			{
				if(cluster!=LONG_LAST_CLUSTER)
				{
					fnp->f_cluster = cluster;		 
					fnp->f_cluster_offset = relcluster;
				}
			}
		}
		else
		{
			cluster = next_cluster(fnp->f_dpb, fnp->f_cluster);
		}
	#else
		cluster = next_cluster(fnp->f_dpb, fnp->f_cluster); 	  
	#endif
	
		if (cluster == 1)
			return DE_SEEK;
		
		/* If this is a read and the next is a LAST_CLUSTER,               */
		/* then we are going to read past EOF, return zero read            */
		/* or expand the list if we're going to write and have run into    */
		/* the last cluster marker.                                        */
		if (cluster == LONG_LAST_CLUSTER)
		{
			if (mode == XFR_READ)
				return DE_SEEK;
			
			break;
		}
		
		if (fnp->f_cluster_offset == relcluster) {
			break;
		} else {
			fnp->f_cluster = cluster;
			fnp->f_cluster_offset++;
		}

	}
	
	if(cluster == 0)
	{
		//还是原来那个cluster
		cluster = fnp->f_cluster;
	}
	//如果已经到文件尾了，则扩展该文件的fat表
	else if(cluster == LONG_LAST_CLUSTER)
	{
	#ifndef READ_ONLY
		//申请新的cluster，输入需要申请的cluster number，返回起始cluster id和实际申请到的cluster数量
		//如果申请不到了，说明磁盘满了，返回LONG_LAST_CLUSTER，实际的cluster数量为0
		cluster = fast_extend(fnp, InLength, OutLength);
		if (cluster == LONG_LAST_CLUSTER)
		{
			return DE_HNDLDSKFULL;
		}
		
		//??????需要在哪里update fat表的buffer呢？
		//fast_updata_fat_buffer(cluster, *OutLength);
		fnp->f_cluster = cluster;		//找到的起始clsuter id，length从*OutLength返回
		//fnp->f_cluster_offset += *OutLength - 1;		//?????fnp->f_cluster_offset为结束offset吗？有点怪怪的
		fnp->f_cluster_offset++;
		return SUCCESS;
	#else
		return -1;
	#endif //#ifndef READ_ONLY				
	}
	
	//如果还没有到文件尾，则先查找从当前点到文件尾是否有连续的cluster		
	//计算从relcluster到end cluster的cluster数目
	secsize = fnp->f_dpb->dpb_secsize;	// get sector size	
	ClusterSize = secsize << fnp->f_dpb->dpb_shftcnt;
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		total_cluster = (fnp->ext_dir->DataLength+ ClusterSize - 1) >> (fnp->f_dpb->dpb_shftcnt + 9);		
	}
	else
  #endif
		total_cluster = (fnp->f_dir.dir_size + ClusterSize - 1) >> (fnp->f_dpb->dpb_shftcnt + 9);	

	if(total_cluster - relcluster >= InLength)
	{
		total_cluster = InLength;
	}
	else
	{
		total_cluster -= relcluster;
	}
	
	*OutLength = 1;
	if(total_cluster == 0)
	{
		return SUCCESS;
	}
	total_cluster -= 1;	
	for(i = 0; i < total_cluster; i++)
	{
	/* get next cluster in the chain */
//	#ifdef _SEEK_SPEED_UP
//		if(fnp->f_link_attribute)
//		{
//			cluster = fast_next_cluster(fnp , relcluster);
//			if(cluster == 1)
//			{
//				//如果当前的offset没有落在可加速的范围内，则使用未加速的查找cluster函数
//				cluster = next_cluster(fnp->f_dpb, next_cls);
//			}
//			else
//			{
//				fnp->f_cluster = cluster;		 
//				fnp->f_cluster_offset = relcluster;
//			}
//		}
//		else
//		{
//			cluster = next_cluster(fnp->f_dpb, fnp->f_cluster);
//		}
//	#else
	  #if WITHEXFAT == 1
		if(NOFatChain(fnp->ext_dir->SecondaryFlags))
		{
			next_cls=cluster+1;
		}
		else
	  #endif
		next_cls = next_cluster(fnp->f_dpb, cluster); 	  
//	#endif
	
		if (cluster == 1)
			return DE_SEEK;
		
		if(cluster + 1 != next_cls)
		{
			//*OutLength = cluster - fnp->f_cluster + 1;
			break;
		}
		*OutLength += 1;
		cluster = next_cls;
	}
	
	return SUCCESS;
}

INT16S fast_map_cluster_end(f_node_ptr fnp, INT16U nLength)
{
	fnp->f_cluster += nLength - 1;
	fnp->f_cluster_offset += nLength - 1;
	return 0;
}
#endif
