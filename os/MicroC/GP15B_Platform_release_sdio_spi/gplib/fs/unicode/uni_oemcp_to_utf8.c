#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
/*************************************************************************/
/* Unicode scalar values for the CP437 character set.                  */
/* Thanks to Roman Czyborra <http://czyborra.com> for this table.      */
/* Thanks to RHIDE macros that helped me very much in writing it in C. */
/*************************************************************************/
INT16S oemcp_to_utf8(INT8S *Chart, UTF8 *UTF8Chart)
{
	INT16U Unicode_Value;
	INT8U **CharUnit = (INT8U **)&Chart;
	
	while (**CharUnit != 0x00)
	{
		if (char2uni(CharUnit, &Unicode_Value))
		{
			*UTF8Chart = 0x00; 
			return DE_UNITABERR; 		//error
		}
		UTF8Chart += fd32_utf32to8((UTF16)Unicode_Value, (UTF8 *)UTF8Chart);
	}
	*UTF8Chart = 0x00;

	return 0; 		 //success
}

// zurong add to convert unser unicode to UTF8
INT16S oemuni_to_utf8(INT8S *Chart, UTF8 *UTF8Chart)
{
	INT16U Unicode_Value;
	INT16U **CharUnicode = (INT16U **)&Chart;	

	while(**CharUnicode !=0x0000)
	{
		Unicode_Value = **CharUnicode;		
		UTF8Chart += fd32_utf32to8((UTF16)Unicode_Value, (UTF8 *)UTF8Chart);		
		(*CharUnicode)++;
	}
	*UTF8Chart = 0x00;
	
	return 0;
}

/*************************************************************************/
/* The code page 437 always has 1-byte characters */
/*************************************************************************/
INT16S oemcp_skipchar(INT8S *Dest)
{
  	 // 07.12.24 add
  	 if(*Dest == ' ')
  	 	return 0;
  	 
  	 if(*Dest & 0x80)
  	 	return 2;
  	 // 07.12.24 add end
  	 
  	 return 1;
}
/*************************************************************************/
#endif // LFN_FLAG

