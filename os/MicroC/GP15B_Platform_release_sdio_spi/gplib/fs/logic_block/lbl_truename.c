#include "fsystem.h"
#include "globals.h"

// �ж� ASCII ��� ���ֵĽ����ַ�
#define FS_BANJIAO_CHAR		(0x80)
// �������255��ASCII�����127�������ַ���127������ת��ΪUNICODE ��ռ�õĳ���361���ֽ�
#define FS_CHAR_MAX_LIMIT	(255)


static BOOLEAN _5c_;    
void _5c_to_ff(CHAR *str) 	  //tranfer 0x5c('\') in HZ to 0xff(not HZ encode byte)
{
	 INT16S i;
	 
	 _5c_ = 0;
	 for (i=0; str[i]!='\0'; i++)
	 {
   		 if (str[i] >= 0x80) //
  		 { 
   			 if (str[i+1] == 0x5c) //
   			 {
    			 str[i+1] = 0xff;
    			 _5c_ = 1;
   			 }
   			 i++;  
  		 }
	 }
}

void _ff_to_5c(CHAR *str) 					   //restore 0xff to 0x5c
{
	 INT16S i;
	 
	 if (_5c_) {
		 for (i=0; str[i]; i++) {
		 	 if (str[i] == 0xff) {
			   str[i] = 0x5c;
			}
		 }
	 }
}
#ifndef READ_ONLY
/////////////////////////////////////////////////////////////////////////////////
#if WITH_CDS_PATHSTR == 1
//deleted by wanghuidi
//assume name is legal
//first write by matao
//regulate ..\ and .\ path to resolute path
//called by chdir function
void pre_truename_dir(CHAR *dest);


INT16S true_current_path(CHAR *path) 
{
#if MALLOC_USE == 1
	CHAR	*tpath;
#elif USE_GLOBAL_VAR == 1
	CHAR	*tpath = (CHAR *) gFS_path1;
#else
	CHAR	tpath[MAX_CDSPATH];
#endif
	CHAR	*str, *dest;
	
#if MALLOC_USE == 1
	tpath = (CHAR *) FS_MEM_ALLOC(MAX_CDSPATH);
	if (tpath == NULL)
		return DE_NOMEM;
#endif	
	
	str = (CHAR *) path;
	dest = (CHAR *) tpath;
	
	memset(tpath, 0, MAX_CDSPATH);
#ifndef _555_FS_	// 2005-1-18 Yongliang
	_5c_to_ff(str);
#endif
	while (*str) {
		if (*str=='\\' && *(str+1) == '.') {
			if (*(str+2)=='.') {
				if (*(str+3)=='\\' || *(str+3)=='\0') {          //  "A:\..\" or "A:\.."
					str += 3;
					for (; dest>tpath+1 && *dest!='\\'; dest--) ;
					*dest = '\0';
					continue;
				}
			} else  {
				if (*(str+2)=='\\' || *(str+2)=='\0') {         // "A:\.\" or "A:\."
					str+=2;
					continue;
				}
			}
		}
		*dest++ = *str++;
	}
	dest = tpath;     //wanghuidi add to fix bug,2005.01.18
#ifndef _555_FS_	// 2005-1-18 Yongliang
	_ff_to_5c(dest);
#endif

	if (tpath[1] == '\\') 
		strcpy(path, tpath+1);
	else
		strcpy(path, tpath);

#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *) tpath); 		 //add by zhangyan
#endif

	{
		INT16S temp;
		
		temp = strlen(path);
		
		if (temp > 1) {
			pre_truename_dir(path); //cut the \ in the end of path
		} else if (temp <= 0) {
			strcat(path, "\\");
		}
	}

	 return SUCCESS;
}
#endif
#endif

/************************************************************************/
/* ���������ַ����Ƿ񳬳�
input:	p_name �ַ�����ָ��
		len		���ȵ�����

output:	 <0 ����

		 >0 ��Ҫռ�õ�ʵ�ʿռ䳤��	

note:	һ������ ת��Ϊ UTF8ʱ�� ռ�� 3 ���ֽڵĿռ�
		һ��ASCII ת��Ϊ UTF8ʱ��ռ�� 1 ���ֽڵĿռ�

*/
/************************************************************************/
INT16S check_path_length(INT8S * p_name, INT16U len) 	
{
	INT16S i;
	INT16S count;
	INT16S unicode;

	count = 0x00;

	if(gUnicodePage == UNI_UNICODE)
	{
		for(i=0x00; ;i++)
		{		
			unicode = ((INT16S *)p_name)[i];
			if(unicode==0x0000) // ����
			{
				break;
			}
			else if(unicode&0xff00)
			{
				count += 0x03;	// ��ASCII�ַ�
			}
			else
			{
				count++;		// ASCII �ַ�
			}
		}	
	}
	else
	{
	for (i=0x00; p_name[i]; i++) {
			if (p_name[i] & FS_BANJIAO_CHAR) 
			{		
			//���ǵ���˵����������ǰ������ģ�һ���ַ�ת����utf8��ʽ���п���Ҫռ��3��byte�Ŀռ�
			count += 0x03;
				if ( !(gUnicodePage & 0x8000) )
				{
				i++;
			}
			continue;
			}
			else
			{
			count++;	// ASCII �ַ���� UTF8�ַ���ֻ��ռ��1���ֽڵĿռ�			
		}
	}
	}
	
	if (count > FS_CHAR_MAX_LIMIT) {
		return DE_NAMETOOLONG;	// �ռ䳬��
	}

	return count+1;		// �����һ���ֽڵĿռ�����
}

//change '/' to'\',and change disk id toupper  
void pre_truename(INT8S *dest)
{
	INT16S i;
	
	if (dest[1] == ':') {
		if (dest[0] >= 'a' && dest[0] <= 'z') {
			dest[0] -= ('a'-'A');
		}
	}
	for (i=0; dest[i]; i++) {
		if (dest[i] == '/') {
			dest[i] = '\\';
		}
	}
}

// cut the \ in the end of path
void pre_truename_dir(CHAR *dest)
{
  	 CHAR *s; 

    _5c_to_ff(dest);	

    s = strchr(dest, '\\');  
    if (s == NULL) {
        _ff_to_5c(dest);
        return;
    }

    while (*(s+1) == '\0') {
        *s = '\0';
        s = strchr(dest, '\\');
        if (s == NULL) {
            break;      
        }
    }
    _ff_to_5c(dest);
}

