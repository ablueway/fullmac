#include "fsystem.h"
#include "globals.h"

INT16S get_dir_from_str(CHAR *in_str, CHAR *out_str);

#define THE_LAST_DIR	(0x00)
#define PATH_LEN_MAX	(0x100)	

#ifndef READ_ONLY
/************************************************************************/
/*	fs_smartchdir
func:	进行多级的目录切换动作，目录存在进入目录，目录不存在建立目录
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

	// offset 初始化为0
	offset = 0x00;	

	// 跳过开头的'\\'
	while (path[offset] == '\\')
	{
		offset++;
	}

	for (i=THE_LAST_DIR+1;  i!= THE_LAST_DIR;  )
	{
		// 取出一级目录名 
		i = get_dir_from_str((CHAR *) path+offset, now_path);

		// 指向下一级目录，注意跳过了一个'\\'
		offset += strlen(now_path);
		
		while (path[offset] == '\\') {
			offset++;
		}

		j = fat_chdir(now_path);
		
		if (j) {		// 进入失败，需要建立			
			j = fat_mkdir(now_path);	// 建立该级目录			
			if (j) { 
				break;	// 建立失败直接返回!
			}
			else{
				fat_chdir(now_path);	// 进入刚建立的目录
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
//input:	PSTR in_str			输入的字符串的指针
//			PSTR out_str		得到的目录的存放的指针
//output:	THE_LAST_DIR		已经是最后的目录，没有后续目录了
//			THE_LAST_DIR+1 		还有后续目录，还需要继续进行
//note:
//
//example:	
//
//
//note:		2005-03-17 对函数进行调整，处理如此的情形 "C:\\123\\\\DEBUG\\9876"	
//
//			在目录名之间有多于一个 '\\'出现的情形	
//
//
//change:	7/22/2005 需要考虑繁体汉字 BIG5 编码中出现 0x5c 的情形
//
//
//////////////////////////////////////////////////////////////////////////
INT16S get_dir_from_str(CHAR *in_str, CHAR *out_str)
{
	INT16S i,j;
	out_str[0] = '\0';
	
	// 只有"\\" 的情形
	if ((in_str[0] == '\\') && (in_str[1] == '\0')){
		strcpy(out_str, in_str);
		return THE_LAST_DIR;
	}
	
	// 根目录的情形
	if ((in_str[0] == '\\') && (in_str[1] != '\0')){
		strcpy(out_str, "\\");
		return THE_LAST_DIR+1;	
	}
	
	// "C:\\ABCD\\998HGF" 有":"的情形
	if ((in_str[1] == ':') && (in_str[2] == '\\')){
		memcpy(out_str, in_str, 0x03);
		out_str[3] = '\0';
		if (in_str[3] == '\0'){
			return THE_LAST_DIR;
		}
		return   THE_LAST_DIR+1;
	}
	
	// 获得需要复制的长度
	// 需要处理如此的情形	"C:\\123\\\\\\DEBUG\\987"
	for (i=0x00; i<PATH_LEN_MAX-1; i++)
	{
		// 如果不是最后一个'\\'， 还有继续
		if ((in_str[i] == '\\') && (in_str[i+1] == '\\')){
			continue;
		}
		if (in_str[i] == '\\' || in_str[i] == '\0'){
			break;
		}
	}
	
	// 复制字符串
	for (j=0x00; j<i; j++)
	{
		if (in_str[j] != '\\'){
			out_str[j] = in_str[j];
		}
		else{
			// 考虑 繁体汉字 编码为 0xZZ 0x5C 的情形
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
	
	// 给得到的字符串补零
	out_str[i] = '\0';
	
	// 判断需要返回的值
	if ((in_str[i] == '\\') && (in_str[i+1] != '\0'))
	{
		return THE_LAST_DIR+1;
	}
	return THE_LAST_DIR;
}
#endif //#ifndef READ_ONLY
