#include "fsystem.h"
#include "globals.h"

/* swap internal and external delete flags */
void swap_deleted(INT8U *name)
{
  	if (name[0]==(INT8U)(DELETED) || name[0]==(INT8U)(EXT_DELETED)) {
    	name[0] ^= EXT_DELETED - DELETED; /* 0xe0 */
    }
}
