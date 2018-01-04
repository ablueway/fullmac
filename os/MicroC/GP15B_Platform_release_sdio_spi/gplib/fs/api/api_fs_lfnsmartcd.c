#include "fsystem.h"
#include "globals.h"

INT16S get_dir_from_str(CHAR *in_str, CHAR *out_str);

#define THE_LAST_DIR	(0x00)
#define PATH_LEN_MAX	(0x100)	

#ifndef READ_ONLY
/************************************************************************/
/*	fs_smartchdir
func:	���ж༶��Ŀ¼�л�������Ŀ¼���ڽ���Ŀ¼��Ŀ¼�����ڽ���Ŀ¼
input:
output:	
 */
/************************************************************************/
INT16S fs_smartchdir(INT32U org_path)
{
	INT16S	i, j, offset, ret;
	INT8S *	path;
#if USE_GLOBAL_VAR == 1
	CHAR	*now_path = (CHAR *) gFS_path1;
#else	
	CHAR	now_path[0x80];
#endif

	path = (INT8S *)org_path;

	// offset ��ʼ��Ϊ0
	offset = 0x00;	

	// ������ͷ��'\\'
	while (path[offset] == '\\')
	{
		offset++;
	}

	for (i=THE_LAST_DIR+1;  i!= THE_LAST_DIR;  )
	{
		// ȡ��һ��Ŀ¼�� 
		i = get_dir_from_str((CHAR *) path+offset, now_path);

		// ָ����һ��Ŀ¼��ע��������һ��'\\'
		offset += strlen(now_path);
		
		while (path[offset] == '\\') {
			offset++;
		}

		j = fat_chdir(now_path);
		
		if (j) {		// ����ʧ�ܣ���Ҫ����			
			j = fat_mkdir(now_path);	// �����ü�Ŀ¼			
			if (j) { 
				break;	// ����ʧ��ֱ�ӷ���!
			}
			else{
				fat_chdir(now_path);	// ����ս�����Ŀ¼
			}
		}	
	}

	if (i == THE_LAST_DIR)
	{
		ret = SUCCESS;
	}
	else
	{
		ret = -1;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////
//			get_dir_from_str
//input:	PSTR in_str			������ַ�����ָ��
//			PSTR out_str		�õ���Ŀ¼�Ĵ�ŵ�ָ��
//output:	THE_LAST_DIR		�Ѿ�������Ŀ¼��û�к���Ŀ¼��
//			THE_LAST_DIR+1 		���к���Ŀ¼������Ҫ��������
//note:
//
//example:	
//
//
//note:		2005-03-17 �Ժ������е�����������˵����� "C:\\123\\\\DEBUG\\9876"	
//
//			��Ŀ¼��֮���ж���һ�� '\\'���ֵ�����	
//
//
//change:	7/22/2005 ��Ҫ���Ƿ��庺�� BIG5 �����г��� 0x5c ������
//
//
//////////////////////////////////////////////////////////////////////////
INT16S get_dir_from_str(CHAR *in_str, CHAR *out_str)
{
	INT16S i,j;
	out_str[0] = '\0';
	
	// ֻ��"\\" ������
	if ((in_str[0] == '\\') && (in_str[1] == '\0')){
		strcpy(out_str, in_str);
		return THE_LAST_DIR;
	}
	
	// ��Ŀ¼������
	if ((in_str[0] == '\\') && (in_str[1] != '\0')){
		strcpy(out_str, "\\");
		return THE_LAST_DIR+1;	
	}
	
	// "C:\\ABCD\\998HGF" ��":"������
	if ((in_str[1] == ':') && (in_str[2] == '\\')){
		memcpy(out_str, in_str, 0x03);
		out_str[3] = '\0';
		if (in_str[3] == '\0'){
			return THE_LAST_DIR;
		}
		return   THE_LAST_DIR+1;
	}
	
	// �����Ҫ���Ƶĳ���
	// ��Ҫ������˵�����	"C:\\123\\\\\\DEBUG\\987"
	for (i=0x00; i<PATH_LEN_MAX-1; i++)
	{
		// ����������һ��'\\'�� ���м���
		if ((in_str[i] == '\\') && (in_str[i+1] == '\\')){
			continue;
		}
		if (in_str[i] == '\\' || in_str[i] == '\0'){
			break;
		}
	}
	
	// �����ַ���
	for (j=0x00; j<i; j++)
	{
		if (in_str[j] != '\\'){
			out_str[j] = in_str[j];
		}
		else{
			// ���� ���庺�� ����Ϊ 0xZZ 0x5C ������
			if (j && (in_str[j] == '\\') && ((INT8U)in_str[j-1] & 0x80 )) {
				out_str[j] = in_str[j];				
				continue;
			}
			else{
				i = j;
				break;				
			}
		}
	}
	
	// ���õ����ַ�������
	out_str[i] = '\0';
	
	// �ж���Ҫ���ص�ֵ
	if ((in_str[i] == '\\') && (in_str[i+1] != '\0'))
	{
		return THE_LAST_DIR+1;
	}
	return THE_LAST_DIR;
}
#endif //#ifndef READ_ONLY
