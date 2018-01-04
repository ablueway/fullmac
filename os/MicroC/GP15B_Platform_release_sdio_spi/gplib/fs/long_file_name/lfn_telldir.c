#include "fsystem.h"
#include "globals.h"


#ifndef READ_ONLY
#if LFN_FLAG == 1
/**************************************************************************/
void rewinddir(void)
{
  	REG dmatch *dmp = &sda_tmp_dm;
	dmp->dm_entry =0; 	
#if ADD_ENTRY == 1
	dmp->dm_currentfileentry =0; 		//2006.7.12 add
#endif
}

INT16U telldir(void)
{
	REG dmatch *dmp = &sda_tmp_dm;
	return (dmp->dm_entry);
}

#if ADD_ENTRY == 1
INT16U tellcurrentfiledir(void)			//2006.7.12
{
	REG dmatch *dmp = &sda_tmp_dm;
	return (dmp->dm_currentfileentry);
}
#endif

/**************************************************************************/
#endif // LFN_FLAG
#endif

