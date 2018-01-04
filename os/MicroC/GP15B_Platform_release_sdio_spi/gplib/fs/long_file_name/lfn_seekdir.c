#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
/**************************************************************************/
void seekdir(INT16U pos)     //the parameter "pos" must be the return value of "telldir"
{
	 REG dmatch *dmp = &sda_tmp_dm; 	
	 dmp->dm_entry = pos;
	 return;
}
/**************************************************************************/
#endif // LFN_FLAG

