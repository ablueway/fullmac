#include "fsystem.h"
#include "globals.h"

INT16S media_check(REG struct dpb *dpbp)
{
	if (dpbp == NULL) {
		return DE_INVLDDRV;
	}
	
	if (!dpbp ->dpb_mounted) {
		return -1;			 // device not ready
	}
	
	return SUCCESS;
}
