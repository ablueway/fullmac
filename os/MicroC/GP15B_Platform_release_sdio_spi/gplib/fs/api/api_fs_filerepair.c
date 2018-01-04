#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _DIR_UPDATA == 1
INT16S fs_FileRepair(INT16S fd)
{
	CLUSTER			curcls, nextcls;
	CLUSTER			clsnum;
	f_node_ptr		fnp;
	INT16U			i;
	INT32U 			clssize;
#if WITHEXFAT == 1
	INT64U			size;
#else
	INT32U			size;
#endif
	/* Translate the fd into a useful pointer                       */
	fnp = xlt_fd(fd);
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return DE_INVLDHNDL;
	
	if (!fnp ->f_dpb ->dpb_mounted)
		return DE_INVLDHNDL;
	
	//������Ŀ¼
	if (fnp->f_dir.dir_attrib & D_DIR)
		return DE_INVLDHNDL;
#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		size = fnp->ext_dir->ValidDataLength;
	}
	else
#endif
	{
		size = fnp->f_dir.dir_size;
	}
	clssize = fnp->f_dpb->dpb_secsize << fnp->f_dpb->dpb_shftcnt;
	clsnum = (size + clssize - 1) / clssize;
	
	//û��ָ��fat�����ļ���С��Ϊ0���޸��ļ���СΪ0
#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		curcls=fnp->ext_dir->FirstCluster;
	}
	else
#endif
	{
		curcls = fnp->f_dir.dir_start;
	}

	if (curcls == 0)
	{
		if (size != 0)
		{
		  #if WITHEXFAT == 1
			if(ISEXFAT(fnp->f_dpb))
			{
				fnp->ext_dir->ValidDataLength=0;
			}
			else
		  #endif
		  	{
			  	fnp->f_dir.dir_size = 0;
		  	}
			fnp->f_flags |= F_DMOD;       /* mark file as modified */
		}
		return 0;
	}
	
	//�ļ���С��0������ָ��fat��������fat��ָ��Ϊ0
	if ( (clsnum == 0) && !(fnp->f_dir.dir_attrib & D_DIR) && (curcls != 0) )
	{
	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			fnp->ext_dir->FirstCluster=0;
		}
		else
	  #endif
		{
			fnp->f_dir.dir_start = 0;	
		}	
		fnp->f_flags |= F_DMOD;       /* mark file as modified */
		return 0;
	}


#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb)&&(NOFatChain(fnp->ext_dir->SecondaryFlags)))
	{
	  	return 0;	
	}
#endif				   

	i = 0;	
	while (1)
	{
		i++;
			
		nextcls = next_cluster(fnp->f_dpb, curcls);
			
		if (clsnum == 1) 
		{
			//���һ��clusterΪ������־���ļ�һ������
			if (nextcls == LONG_LAST_CLUSTER)
			{
				break;
			}
			else
			//fatָ�����ļ���С����Ŀ¼��ָ�����ļ���С������fat���СΪĿ¼��ָ���Ĵ�С
			{
			  #if WITHEXFAT == 1	
			  	if(ISEXFAT(fnp->f_dpb))
			  	{
					link_fat(fnp->f_dpb, curcls, LONG_LAST_CLUSTER,fnp->ext_dir->SecondaryFlags);
			  	}
			  	else
			  	{
			  		link_fat(fnp->f_dpb, curcls, LONG_LAST_CLUSTER,NULL);
			  	}
			  #else
			    link_fat(fnp->f_dpb, curcls, LONG_LAST_CLUSTER,NULL);
			  #endif
				break;
			}
		}
		 
		//��ʱclsnumһ������1
		if ( (nextcls == LONG_LAST_CLUSTER) || ((nextcls == 0)) )
		{
			//����fat��
			if (nextcls == 0)
			{
			  #if WITHEXFAT == 1
			  	if(ISEXFAT(fnp->f_dpb))
			  	{
					link_fat(fnp->f_dpb, curcls, LONG_LAST_CLUSTER,fnp->ext_dir->SecondaryFlags);
			  	}
			  	else
			  	{
			  		link_fat(fnp->f_dpb, curcls, LONG_LAST_CLUSTER,NULL);
			  	}
			  #else
			    link_fat(fnp->f_dpb, curcls, LONG_LAST_CLUSTER,NULL);
			  #endif
			}
			//�����ļ���С
		  #if WITHEXFAT == 1
			if(ISEXFAT(fnp->f_dpb))
			{
				fnp->ext_dir->ValidDataLength=i * clssize;
			}
			else
		  #endif
			{
				fnp->f_dir.dir_size = i * clssize;
			}			  
			fnp->f_flags |= F_DMOD;       /* mark file as modified */
			break;
		}		
		clsnum--;
		curcls = nextcls;
	}
	return 0;
}
#endif
#endif	//#ifndef READ_ONLY
