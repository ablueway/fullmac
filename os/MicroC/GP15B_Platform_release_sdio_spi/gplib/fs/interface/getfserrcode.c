#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S _getfserrcode (void) 	 
{
	INT16S last_err;

	// save FS error number to temp variable		
	last_err = gFS_errno;

	// reset global variable
	gFS_errno = 0x00;

	// return last error number
	 return last_err;
}
