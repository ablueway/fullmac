#include "fsystem.h"
#include "globals.h"

#if _USE_EXTERNAL_MEMORY == 1
	// ����4K WORD �Ŀռ䣬�൱�� 8K BYTE
	#define COPY_FILE_BUFFER_SIZE	(0x1000)
	#define MINI_BUFFER_SIZE		(0x100)
#else
	#define COPY_FILE_BUFFER_SIZE	(0x100)
#endif

#ifndef READ_ONLY
/************************************************************************/
/*		�����ļ��Ŀ���
input: 
		srcfile		Դ�ļ����ַ���ָ��
		destfile	Ŀ���ļ����ַ���ָ��
output:

function:	
		[1/3/2006]
			�����ⲿ�ռ䣬�������ݵ���ʱ�洢

                                                                   */
/************************************************************************/
//Yongliang MD,20050223
//Zhangyan MD,20050316
//-----------------------------------------------------
INT16S _copy (CHAR *srcfile, CHAR *destfile) 	 
{
	INT16S fd1,fd2;
	INT16S ret,ret2,olderr;
	INT16U data_size;

#if MALLOC_USE == 1
	#if _USE_EXTERNAL_MEMORY == 1
	INT32U	buf;
	#else
	INT8S *	buf;	 	 
	#endif
#elif USE_GLOBAL_VAR == 1
	INT8S *	buf = gFS_path1;
#else
	INT8S buf[COPY_FILE_BUFFER_SIZE];
#endif
	
	olderr = gFS_errno;
	
	ret = -1;
	ret2 = 0;
	
	if (!srcfile || !destfile) 	 
	{ 
		gFS_errno = DE_INVLDPARM;
		return -1;
	}

	data_size = COPY_FILE_BUFFER_SIZE;
		
#if MALLOC_USE == 1
	#if _USE_EXTERNAL_MEMORY == 1
	for ( buf = NULL; ; )
	{
		buf = FS_EXTMEM_ALLOC(data_size);
		if (buf == NULL)
			data_size >>= 0x01;	//������벻���㹻�Ŀռ䣬��С����ĳߴ硣

		// ����ռ䴦�ڽ���״̬������ռ䲻�ף�ֱ�ӷ���, ��ֹ������ѭ��
		if (data_size < MINI_BUFFER_SIZE)
		{
			if (buf != NULL)
			{
				FS_EXTMEM_FREE(buf);	// ����Ѿ����뵽�ռ��ˣ����пռ���ͷ�!
			}
			buf = NULL;	// ֱ�ӽ�ָ�븳ֵΪ��
			break;
		}
		
		if (buf != NULL)
			break;	
	}
	#else
	buf = (INT8S *)FS_MEM_ALLOC(data_size); 
	#endif

	if (buf==NULL)
	{
		gFS_errno = DE_NOMEM;			
		return -1;
	}
#endif
 
	fd1 = open ((CHAR *) srcfile, O_RDONLY);
	if (fd1 == -1){
		goto Label_Exit;
	}
	
	fd2 = open ((CHAR *) destfile, O_RDWR);
	if ((fd2 != -1) || ((fd2 == -1) && (gFS_errno == DE_ISDIR)))
	{
		//gFS_errno = DE_FILENOTFND;
		gFS_errno = DE_FILEEXISTS;	// zurong modify 2009.6.11
		
		close (fd1);
		close (fd2);
		
		goto Label_Exit;
	}
	
	fd2 = open ((CHAR *) destfile, O_RDWR | O_CREAT); 
	if (fd2 == -1)
	{
		close (fd1);
		goto Label_Exit;
	}
	
	gFS_errno = olderr;

	FS_OS_LOCK();
	
	do
	{
		ret = fat_read(fd1, (INT32U)buf<<WORD_PACK_VALUE, data_size<<WORD_PACK_VALUE);
		if (ret <0 )
		{
			break;			 
		}
		ret2 = fat_write(fd2, (INT32U)buf<<WORD_PACK_VALUE, ret);	

		if (ret != ret2) 	 
		{
			if (ret >= 0){
				ret = DE_HNDLDSKFULL;				 
			}
			break;
		}
    }
	while (ret > 0);

	FS_OS_UNLOCK();

	close (fd1);
	close (fd2);
	
	if (ret < 0) 	 
	{
		if (ret == DE_HNDLDSKFULL) 	 
		{ 	
			unlink ((CHAR *) destfile);
		}
		handle_error_code(ret);
		ret = -1;
	}
	else 	 {
		ret = 0;
	}
	
Label_Exit:

#if MALLOC_USE == 1
#if _USE_EXTERNAL_MEMORY == 1
	FS_EXTMEM_FREE(buf);
#else
	FS_MEM_FREE((INT16U *)buf);
#endif
#endif
	
	return ret;
}
#endif
