/************************************************************************/
/* 
	find file from disk ignore dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

INT32U paperDir;

f_ppos sfn_paperchangedisk(STDiskFindControl *pstDiskFindControl, INT16S dsk)
{
	pstDiskFindControl->cur_pos.f_dsk = dsk;
	pstDiskFindControl->cur_pos.f_entry = paperDir;
	pstDiskFindControl->cur_pos.f_offset = 0;
	pstDiskFindControl->cur_pos.f_is_root = 0;
	
	return(&pstDiskFindControl->cur_pos);
}




f_ppos fs_getpaperfirstfile(STDiskFindControl *pstDiskFindControl, CHAR *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S speedup,INT32S (*filter)(INT8S *str))
{

	INT16S			i;
	f_node_ptr		fnp;
	
	//INT8S			*Dir;
#if MALLOC_USE == 1
	INT8S			*path_max;
	tFatFind		*psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S			*path_max = gFS_path1;
	tFatFind		*psFind = &gFS_sFindData;
#else
	STATIC INT8S		path_max[MAX_CDSPATH];
	STATIC tFatFind	sFindData;
	tFatFind		*psFind = &sFindData;
#endif
	struct cds		*cdsp;	
	INT8S 			*path_max_tmp;
	INT16S 			disk;
//	INT16S 			ret = SUCCESS;
	
/*------------------------------------------------------------------*/
	pstDiskFindControl->level = 0;
	pstDiskFindControl->find_begin_file = 0;
	pstDiskFindControl->find_begin_dir = 0;	
	
	pstDiskFindControl->sfn_cluster = 0;
	pstDiskFindControl->sfn_cluster_offset = 0;
	pstDiskFindControl->dir_change_flag = 0;
	pstDiskFindControl->root_dir_flag = 0;
	
	memset(pstDiskFindControl->dir_offset, 0, 16 * sizeof(INT16U));
	memset(pstDiskFindControl->cluster, 0, 16 * sizeof(INT32U));
	memset(pstDiskFindControl->cluster_offset, 0, 16 * sizeof(INT32U));
	

	sfn_setcurdirpos(pstDiskFindControl, &pstDiskFindControl->cur_pos);
/*-----------------------------------------------------------------*/	
//���л�·��
	if(path==0)
		return NULL;
	{	
		// �����ж�·�����Ƿ񳬳�
		i = check_path_length((INT8S *) path, FD32_LFNPMAX);
		if (i<0)
			return NULL;

	#if MALLOC_USE == 1
		path_max = (INT8S *)FS_MEM_ALLOC(i+sizeof(tFatFind)); 
		if (path_max == NULL)
			return NULL;
		
		psFind = (tFatFind*)&path_max[i];
	#endif
		
		//��·��ת����utf8����
		if(gUnicodePage == UNI_UNICODE)
		{
			if (DE_UNITABERR == UnicodeToUTF8((INT8S *) path, (UTF8 *) path_max))
			{
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)path_max); 	
			#endif
				return NULL;
			}
		}
		else
		{
			if (DE_UNITABERR == ChartToUTF8((INT8S *) path, (UTF8 *) path_max))
			{
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)path_max); 	
			#endif
				return NULL;
			}
		}
		
		//���ַ�'/'ת����'\'�������̷�ת���ɴ�д��
		pre_truename(path_max);
		
		//�л����̵�·���ƶ��Ĵ���
		path_max_tmp = path_max;
		if (path_max[1] == ':')
		{
			if (path_max[0] - 'A' != default_drive)
			{
				disk = path_max[0];
				if (disk >= 'a' && disk <= 'z')
					disk -= 'a' - 'A';
			
				if (fs_changedisk(disk-'A')!=0)
				{
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *)path_max); 	
				#endif
					return NULL;
				}
			}
			path_max_tmp +=2;     
		}
		
		//��ȡ���̵Ľṹ�弰�жϴ��̵�ǰ�Ƿ���Ч
		cdsp = get_cds(default_drive); 	 
		if (cdsp == NULL)				//guili 2006.5.16
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif	
			return NULL;			//not check return value
		}						
		if ((media_check(cdsp->cdsDpb) < 0))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 
		#endif
			return NULL;
		}
		
		/* now test for its existance. If it doesn't, return an error.  */
		if ((fnp = descend_path((CHAR *) path_max_tmp, psFind)) == NULL)
		{	
			dir_close(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 
		#endif
			return NULL;
		}

		if (!(fnp->f_dir.dir_attrib & D_DIR))
		{
			dir_close(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 
		#endif
			return NULL; 	
	  	}
		/* problem: RBIL table 01643 does not give a FAT32 field for the
		 CDS start cluster. But we are not using this field ourselves */
		//cdsp->cdsStrtClst = (CLUSTER)fnp->f_dirstart;  				//Huajun 

//��¼�����ļ�Ŀ¼��Ϣ
		paperDir=(CLUSTER)fnp->f_dirstart;
		sfn_paperchangedisk(pstDiskFindControl, fnp->f_dpb->dpb_unit);
//

		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 
	#endif
	}	
/*--------------------------------------------------------------------------------*/
	return fs_getpapernextfile(pstDiskFindControl, extname, f_info, attr, speedup,filter);
}

f_ppos fs_getpapernextfile(STDiskFindControl *pstDiskFindControl, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S speedup,INT32S (*filter)(INT8S *str))
{
	struct	sfn_info dir_info;
	f_pos	*p_file_pos, *p_dir_pos;
	f_pos	cur_pos, *p_cur_pos;
	INT16S dsk=pstDiskFindControl->cur_pos.f_dsk;
	
	
	while (1)		//find file
	{
		if (pstDiskFindControl->find_begin_file == 0)
		    p_file_pos=sfn_findfirst(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);
		else
		    p_file_pos=sfn_findnext(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);

		if (p_file_pos != NULL)	
		{
		    if (pstDiskFindControl->level == 0)
		    {
		    	pstDiskFindControl->root_dir_flag = 1;			// the root folder have found file
		    }
		    pstDiskFindControl->find_begin_file = 1;			//��һ�ε���findnext
		    if (f_info->f_name[0] == '.')		//070423 �����ҵ�����ΪD_ALLʱ��������"."��".."�ڵ�
		    	continue;
		    
		    return p_file_pos; 
		}
		else
		//��Ŀ¼���ļ��������ˣ���ʼ����Ŀ¼
		{
			while (1)		//find dir
			{
			    if (pstDiskFindControl->find_begin_dir == 0)  
			    	p_dir_pos=sfn_findfirst(pstDiskFindControl, dsk, "*", &dir_info, D_DIR, NULL);
			    else
			    	p_dir_pos=sfn_findnext(pstDiskFindControl, dsk, "*", &dir_info, D_DIR, NULL);
			      
			    //����ҵ�Ŀ¼��������Ŀ¼
			    if (p_dir_pos != NULL)
			    {
			        //�ų�Ŀ¼"."��".."�����
			        if ( dir_info.f_name[0] == '.')
			        {
			        	pstDiskFindControl->find_begin_dir = 1;
			        	continue;
			        }

					//�����ڳ����޶���Ŀ¼���ʱ�����������Ŀ¼
					if(pstDiskFindControl->level >= C_MAX_DIR_LEVEL - 1)
						continue;			        

					//��ȡ��ǰĿ¼��handle
					sfn_getcurpos(pstDiskFindControl, &cur_pos);
					sfn_setcurdirpos(pstDiskFindControl, p_dir_pos);				//���浱ǰ�ļ����ڵ�Ŀ¼��f_pos

					p_dir_pos = sfn_changedir(pstDiskFindControl, p_dir_pos);		//������Ŀ¼��Ӧ�жϷ���ֵ
					//������Ŀ¼ʧ�ܣ������ˣ����ҽ���
					if (p_dir_pos == NULL)
					{
						//gFS_errno = EFLG_DIR; 					//Ŀ¼������
						//return NULL;
						pstDiskFindControl->find_begin_dir = 1;
			        	continue;
					}
			        
			        pstDiskFindControl->dir_change_flag = 1;
			        //������Ŀ¼�ɹ��������ϲ�Ŀ¼��ƫ��ֵ��
			        //�Ա�֤�»ط����ϲ�Ŀ¼ʱ���Լ������²���
					pstDiskFindControl->dir_offset[pstDiskFindControl->level] = cur_pos.f_offset;
					if(speedup)
					{
						pstDiskFindControl->cluster[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster;
						pstDiskFindControl->cluster_offset[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster_offset;
					}
					pstDiskFindControl->level++;
			        
			        // 080321 add for disk find speedup
				  	if(speedup)
				  	{
				  		pstDiskFindControl->sfn_cluster = pstDiskFindControl->cluster[pstDiskFindControl->level];
						pstDiskFindControl->sfn_cluster_offset = pstDiskFindControl->cluster_offset[pstDiskFindControl->level];
				  	}
				  	else
				  	{
				  		pstDiskFindControl->sfn_cluster = 0;
				  		pstDiskFindControl->sfn_cluster_offset = 0;
				  	}
				  	// 080321 add end
			        		        
			        //���ſ�ʼ������Ŀ¼�µ��ļ�
			        pstDiskFindControl->find_begin_file = 0;
			        pstDiskFindControl->find_begin_dir = 0;
			        break;
			    }    
			    else
			    //Ŀ¼�������ˣ��򷵻���һ��Ŀ¼
			    {
					//�����ǰ�Ǹ�Ŀ¼�����ʾȫ������������
					if (pstDiskFindControl->level == 0)
					{
						gFS_errno = 0;
						return NULL;
					}
					
					//��ȡ��ǰĿ¼��handle
			        sfn_getcurpos(pstDiskFindControl, &cur_pos);			        
					p_cur_pos = sfn_fatherdir(pstDiskFindControl, &cur_pos);	//from current pos to father dir
					//�޷������ϲ�Ŀ¼�������ˣ����ҽ���
					if (p_cur_pos == NULL)
					{
						gFS_errno = 0;
						return NULL;
					}
					
					//�ָ���һ���Ŀ¼���
					pstDiskFindControl->level--;
					sfn_getcurpos(pstDiskFindControl, &cur_pos);
					cur_pos.f_offset = pstDiskFindControl->dir_offset[pstDiskFindControl->level];
					sfn_setcurpos(pstDiskFindControl, &cur_pos);
					pstDiskFindControl->dir_change_flag = 0;
					
					// 080321 add for disk find speedup
					pstDiskFindControl->sfn_cluster = 0;
					pstDiskFindControl->sfn_cluster_offset = 0;
				  	// 080321 add end
					
					pstDiskFindControl->find_begin_dir = 1;
			    }
			}//while (1)		//find dir
		}
	}//while (1)		//find file
}
#endif
