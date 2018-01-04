#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1

/*************************************************************************/

#define OEMCP_UNCONVERTABLE (1 << 0)
#define OEMCP_WAS_LONGER    (1 << 1)

/*************************************************************************/
/* Converts a up to SourceSize bytes of the Source UTF-8 string to the */
/* OEM character set, and puts the result in the Dest strings, writing */
/* at most DestSize bytes. If SourceSize is -1 the Source string is    */
/* supposed to be null terminated.                                     */
/* WARNING: Does not append the null terminator to Dest.               */
/*************************************************************************/
INT16S utf8_to_oemcp(UTF8 *Source, INT16S SourceSize,
                  INT8S *Dest,   INT16S DestSize)
{
	 UTF8  *SourceLast = NULL;
	 INT8S  *DestLast   = Dest + DestSize - 1;
	 INT16S   Res = 0;
	 UTF32  Scalar;
	
	 if (SourceSize != -1) 
	 	SourceLast = Source + SourceSize - 1;

	 while (*Source)
	 {
		 INT16S Skip2;
		
		 if (Dest > DestLast)
		 {
			 if (*Source) 
			 	Res |= OEMCP_WAS_LONGER;
			 break;
		 }
		 
		 if ((SourceLast) && (Source > SourceLast)) 
		 	break;
		 
		 /* Get the Unicode scalar value from Source */
		 if (!(*Source & 0x80)) 
		 	Scalar = *Source++;
		 else
		 {
			 INT16S Skip = fd32_utf8to32(Source, &Scalar);
			
			 if (Scalar==0xFEFF) 	 //add by zhangyan 051025
			 {
				 Source += Skip;
				 Skip = fd32_utf8to32(Source, &Scalar);
			 }
			
			 if (Skip < 0) 
			 	return FD32_EUTF8;
			 Source += Skip;
		 }

		 /* Otherwise we have to scan the code page to find the scalar */
		 Skip2 = uni2char((INT16U) Scalar, (INT8U *) Dest);
		
		 if (Skip2 == DE_UNITABERR)
		 {
			 *Dest = '_';
			 Dest += 1;
		 }
		 else
			 Dest += Skip2;
	 }

	 if (DestSize>11)
	 {
		if(gUnicodePage == UNI_UNICODE)
		{
		     *(INT16U*)Dest = 0;
		}
		else
		{ 
		     *Dest = 0;
		}
	 }

	 return Res;
}

#ifndef READ_ONLY
//guili 2006.5.25 start
/*************************************************************************/
/* 检查名字是否是长文件名 */
//只检查长度,对文件名中有空格且全部是ASCII的情况做一个补充,下面还有其它的拦截的函数
/*************************************************************************/
INT16S check_ifis_lfn(UTF8 *Source, INT16S SourceSize, INT16S DestSize)
{
	UTF8  *SourceLast = NULL;
	INT16S Res = 0;
	INT16U CharCount;
	
	CharCount = 0;

	if (SourceSize != -1) SourceLast = Source + SourceSize - 1;
	
	while (*Source)
	{
		if (!(*Source & 0x80))
		{
			CharCount += 1;
			if (CharCount > DestSize)
			{
				Res |=  OEMCP_WAS_LONGER;
				break;
			}
			Source += 1;
			if ((SourceLast) && (Source > SourceLast)) break;
		}
		else
		{
			Res |=  OEMCP_WAS_LONGER;
			break;
		}
	}

	return Res;
}
//guili 2006.5.25 end
#endif

/*************************************************************************/
#endif // LFN_FLAG

