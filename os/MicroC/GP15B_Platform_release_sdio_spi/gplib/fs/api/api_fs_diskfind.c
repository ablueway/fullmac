/************************************************************************/
/* 
	find file from disk ignore dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

f_ppos fs_getfirstfile(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S speedup,INT32S (*filter)(INT8S *str))
{
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
	
	sfn_changedisk(pstDiskFindControl, dsk);
	sfn_setcurdirpos(pstDiskFindControl, &pstDiskFindControl->cur_pos);
	return fs_getnextfile(pstDiskFindControl, dsk, extname, f_info, attr, speedup,filter);
}

f_ppos fs_getnextfile(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S speedup,INT32S (*filter)(INT8S *str))
{
	struct	sfn_info dir_info;
	f_pos	*p_file_pos, *p_dir_pos;
	f_pos	cur_pos, *p_cur_pos;
	
	while (1)		//find file
	{
		if (pstDiskFindControl->find_begin_file == 0)
		    p_file_pos=sfn_findfirst(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr,filter);
		else
		    p_file_pos=sfn_findnext(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr,filter);

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
			    	p_dir_pos=sfn_findfirst(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			    else
			    	p_dir_pos=sfn_findnext(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			      
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
				#if WITHEXFAT == 1
					pstDiskFindControl->FatherClu[pstDiskFindControl->level] = cur_pos.f_entry;
				#endif
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

#if 0
f_ppos fs_getfirstfile_1(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S *extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S *str))
{
	pstDiskFindControl->level = 0;							// �Ӹ�Ŀ¼��ʼ����
	pstDiskFindControl->find_begin_file = 0;					// ��һ���ȵ���findfirst
	pstDiskFindControl->find_begin_dir = 0;						// ��һ���ȵ���findfirst
	
	pstDiskFindControl->sfn_cluster = 0;
	pstDiskFindControl->sfn_cluster_offset = 0;
	
	memset(pstDiskFindControl->dir_offset, 0, 16 * sizeof(INT16U));
	memset(pstDiskFindControl->cluster, 0, 16 * sizeof(INT32U));
	memset(pstDiskFindControl->cluster_offset, 0, 16 * sizeof(INT32U));
	
	sfn_changedisk(pstDiskFindControl, dsk);
	sfn_setcurdirpos(pstDiskFindControl, &pstDiskFindControl->cur_pos);			// ��ǰ����Ŀ¼Ϊ��Ŀ¼;		
	return fs_getnextfile_1(pstDiskFindControl, dsk, extname, f_info, attr,filter);
}

// 080321 add for disk find speedup
f_ppos fs_getnextfile_1(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S * extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S *str))
{	
	struct	sfn_info dir_info;
	f_pos	*p_file_pos, *p_dir_pos;
	f_pos	cur_pos, *p_cur_pos;
	
	while (1)		//find file
	{
		//�Ȳ����ļ�
		if (pstDiskFindControl->find_begin_file == 0)
		    p_file_pos=sfn_findfirst(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);
		else
		    p_file_pos=sfn_findnext(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);
		    
		//����ҵ��ļ����򷵻ش��ļ��ľ��
		if (p_file_pos != NULL)	
		{    
		    pstDiskFindControl->find_begin_file = 1;			//��һ�ε���findnext
		    //if ( (f_info->f_attrib & D_DIR) == D_DIR )	//070423 �����ҵ�����ΪD_ALLʱ��������Ŀ¼�ڵ�
		    if ((CHAR)f_info->f_name[0] == '.')		//070423 �����ҵ�����ΪD_ALLʱ��������"."��".."�ڵ�
		    	continue;
		    
		    return p_file_pos; 
		}
		else
		//��Ŀ¼���ļ��������ˣ���ʼ����Ŀ¼
		{
			while (1)		//find dir
			{ 
			    if (pstDiskFindControl->find_begin_dir == 0)  
			    	p_dir_pos=sfn_findfirst(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			    else
			    	p_dir_pos=sfn_findnext(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			      
			    //����ҵ�Ŀ¼��������Ŀ¼
			    if (p_dir_pos != NULL)
			    {
			        //�ų�Ŀ¼"."��".."�����
			        if ( dir_info.f_name[0] == '.' )
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
					pstDiskFindControl->cluster[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster;
					pstDiskFindControl->cluster_offset[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster_offset;
					pstDiskFindControl->level++;	
			        
			        // 080321 add for disk find speedup
				  	pstDiskFindControl->sfn_cluster = 0;
				  	pstDiskFindControl->sfn_cluster_offset = 0;
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
					pstDiskFindControl->sfn_cluster = pstDiskFindControl->cluster[pstDiskFindControl->level];
					pstDiskFindControl->sfn_cluster_offset = pstDiskFindControl->cluster_offset[pstDiskFindControl->level];
				  	// 080321 add end
					
					pstDiskFindControl->find_begin_dir = 1;
			    }
			}//while (1)		//find dir
		}
	}//while (1)		//find file
}
#endif
#endif
