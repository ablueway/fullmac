#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
//-----------------------------------------------------
INT16S _move (CHAR *srcfile, CHAR * destpath) 	 
{
	return _rename(srcfile, destpath);
}
#endif
