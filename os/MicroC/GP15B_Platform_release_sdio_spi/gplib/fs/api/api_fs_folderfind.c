/************************************************************************/
/* 
	find file from floder ignore dir
	
	zhangzha creat @2009.08.26
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
f_ppos fs_get_first_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir,INT32S (*filter)(INT8S *str))
{
	REG f_node_ptr fnp;
	INT16S i;
	INT16S dsk;
#if MALLOC_USE == 1
	INT8S* Dir;
	tFatFind* psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S		*Dir = gFS_path1;
	tFatFind	*psFind = &gFS_sFindData;
#else
	STATIC INT8S		Dir[FD32_LFNPMAX];
	STATIC tFatFind	sFindData;
	tFatFind* psFind = &sFindData;
#endif

	i = check_path_length(path ,FD32_LFNPMAX);
	if(i<0)
	{
		gFS_errno = DE_NAMETOOLONG;
		return NULL;
	}

#if MALLOC_USE == 1
	Dir = (INT8*)FS_MEM_ALLOC(i + sizeof(tFatFind));
	if(Dir == NULL)
	{
		gFS_errno = DE_NOMEM;
		return NULL;
	}
	
	psFind = (tFatFind*)&Dir[i];
#endif
	
	if(DE_UNITABERR == ChartToUTF8((INT8S *)path , (UTF8 *)Dir) )
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16*)Dir); 
	#endif
		gFS_errno = DE_UNITABERR;
		return NULL;
	}
	pre_truename(Dir);     //change '/' to'\'
		
	if ((fnp = descend_path((CHAR *)Dir , psFind)) == NULL)
    {
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16*)Dir); 
	#endif
		return NULL;
	}

	/* Now initialize the dirmatch structure.            */
	if ((Dir[0] == '\0') || (Dir[0] == '\\'))
	{
		dsk = default_drive; 
	}
	else
	{
		if (Dir[1] == ':')
			dsk = Dir[0] - 'A';
	}
	pstDiskFindControl->cur_pos.f_dsk = dsk;
	pstDiskFindControl->cur_pos.f_entry = fnp->f_dirstart;
	pstDiskFindControl->cur_pos.f_offset = 0;

	pstDiskFindControl->level = 0;
	pstDiskFindControl->find_begin_file = 0;
	pstDiskFindControl->find_begin_dir = 0;	
	
	pstDiskFindControl->sfn_cluster = 0;
	pstDiskFindControl->sfn_cluster_offset = 0;
	
	memset(pstDiskFindControl->dir_offset, 0, 16 * sizeof(INT16U));
	memset(pstDiskFindControl->cluster, 0, 16 * sizeof(INT32U));
	memset(pstDiskFindControl->cluster_offset, 0, 16 * sizeof(INT32U));
	
	//sfn_changedisk(pstDiskFindControl, dsk);
	sfn_setcurdirpos(pstDiskFindControl, &pstDiskFindControl->cur_pos);
	return fs_getnextfile(pstDiskFindControl, dsk, extname, f_info, attr, find_child_dir,filter);
}

f_ppos fs_get_next_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir,INT32S (*filter)(INT8S *str))
{
	INT16S	dsk;
	struct	sfn_info dir_info;
	f_pos	*p_file_pos, *p_dir_pos;
	f_pos	cur_pos, *p_cur_pos;
	
	dsk = pstDiskFindControl->cur_pos.f_dsk;
	while (1)		//find file
	{
		if (pstDiskFindControl->find_begin_file == 0)
		    p_file_pos=sfn_findfirst(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);
		else
		    p_file_pos=sfn_findnext(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);

		if (p_file_pos != NULL)	
		{    
		    pstDiskFindControl->find_begin_file = 1;			//��һ�ε���findnext
		    if (f_info->f_name[0] == '.')		//070423 �����ҵ�����ΪD_ALLʱ��������"."��".."�ڵ�
		    	continue;
		    
		    return p_file_pos; 
		}
		else
		//��Ŀ¼���ļ��������ˣ���ʼ����Ŀ¼
		{
			if(!find_child_dir)
			{
				return NULL;
			}
			
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
						gFS_errno = EFLG_DIR; 					//Ŀ¼������
						return NULL;
					}
			        
			        //������Ŀ¼�ɹ��������ϲ�Ŀ¼��ƫ��ֵ��
			        //�Ա�֤�»ط����ϲ�Ŀ¼ʱ���Լ������²���
					pstDiskFindControl->dir_offset[pstDiskFindControl->level] = cur_pos.f_offset;
					if(0)//if(speedup)
					{
						pstDiskFindControl->cluster[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster;
						pstDiskFindControl->cluster_offset[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster_offset;
					}
					pstDiskFindControl->level++;	
			        
			        // 080321 add for disk find speedup
				  	if(0)//if(speedup)
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
