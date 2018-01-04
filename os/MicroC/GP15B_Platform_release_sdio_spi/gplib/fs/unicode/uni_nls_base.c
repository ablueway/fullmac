#include "fsystem.h"
#include "globals.h"

INT16S init_nls(void)
{
	INT16S	ret;
		
	ret = g_nls_table.init_nls();
	if (ret == 0)
		g_nls_table.Status = C_NLS_SUCCESS;
	else 
		g_nls_table.Status = C_NLS_FAIL;
	
	return ret;
}

INT16S exit_nls(void)
{
	if (g_nls_table.Status == C_NLS_SUCCESS)
	{
		g_nls_table.Status = C_NLS_FAIL;
		return g_nls_table.exit_nls();
	}
	return -1;
}

INT16S uni2char(INT16U uni, INT8U *out) 
{                 
	INT8U cl;
	INT8U ch;
	INT16U c;
	INT16S n;
	
	if(gUnicodePage == UNI_UNICODE)
	{
		if(uni <= 0x80)
		{
			out[0] = uni;
			return 1;
		}
		else
		{
			*(INT16U*)out = uni;
			return 2;
		}
	}
	else
	{
		if (uni < 0x80 )
		{
			out[0] = uni;
			return 1;
		}
	
		if (g_nls_table.Status == C_NLS_SUCCESS)
		{
			c = g_nls_table.uni2char(uni);
			cl = c & 0xFF;
			ch = (c >> 8) & 0xFF;
			if (ch && cl)
			{
				out[0] = cl;
				out[1] = ch;
				n = 2;
			}
			else if ((ch == 0) && cl)
			{
				out[0] = cl;
				n = 1;
			}
			else
			{
				return DE_UNITABERR;
			}
		}
		else
		{
			return DE_UNITABERR;
		}
	}
	return n;
}

INT16S char2uni( INT8U **rawstring, INT16U *uni)
{
	if (g_nls_table.Status == C_NLS_SUCCESS)
	{
		*uni = g_nls_table.char2uni(rawstring);
		if (*uni != 0)
			return 0;						//success
	}
	return DE_UNITABERR;					//fail
}

INT16S ChangeUnitab(struct nls_table *st_nls_table)
{
	exit_nls();
	g_nls_table = *st_nls_table;
	return init_nls();
}
