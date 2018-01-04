#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 0
BOOLEAN fcbmatch(const INT8S *fcbname1, const INT8S *fcbname2)
{
	return memcmp(fcbname1, fcbname2, FNAME_SIZE + FEXT_SIZE) == 0;
}
#endif
#endif
