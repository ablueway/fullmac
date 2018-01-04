#include "fsystem.h"
#include "globals.h"
#ifndef READ_ONLY

#if LFN_FLAG == 0
f_node_ptr dir_open(const INT8S *dirname)
{
  	f_node_ptr fnp;
  	INT16U i;
  	INT8S fcbname[FNAME_SIZE + FEXT_SIZE];

  	// Allocate an fnode if possible - error return (0) if not.
  	if ((fnp = get_f_node()) == (f_node_ptr) 0) {
    	return (f_node_ptr) 0;
  	}

  	// Force the fnode into read-write mode
  	fnp->f_mode = RDWR;

  	// determine what drive and dpb we are using...
  	fnp->f_dpb = get_dpb(fs_toupper(dirname[0])-'A');
  	// Perform all directory common handling after all special handling has been performed.
	if (media_check(fnp->f_dpb) < 0) {
    	release_f_node(fnp);
    	return (f_node_ptr) 0;
  	}
  
  	// Walk the directory tree to find the starting cluster
  	// Start from the root directory (dirstart = 0)
  	// The CDS's cdsStartCls may be used to shorten the search beginning at the CWD, see mapPath() and CDS.H in order to enable this behaviour there.
  	dir_init_fnode(fnp, 0);
  	while (*dirname != '\0') {
    	// skip all path seperators
    	while (*dirname == '\\') {
      		++dirname;
      	}
    	// don't continue if we're at the end
    	if (*dirname == '\0') {
      		break;
      	}

    	// Convert the name into an absolute name for comparison...
    	fmemset(fcbname, ' ', FNAME_SIZE + FEXT_SIZE);

    	for (i=0; i<FNAME_SIZE+FEXT_SIZE; i++, dirname++) {
      		INT8S c = *dirname;
      		
      		if (c == '.') {
        		i = FNAME_SIZE - 1;
        	} else if (c!='\0' && c!='\\') {
        		fcbname[i] = c;
      		} else {
        		break;
        	}
    	}

    	// Now search through the directory to find the entry...
    	i = FALSE;
    	while (dir_read(fnp) == 1) {
      		if (!(fnp->f_dir.dir_attrib & D_VOLID) && fcbmatch(fcbname, fnp->f_dir.dir_name)) {
        		i = TRUE;
        		break;
      		}
      		fnp->f_diroff++;
    	}

    	if (!i || !(fnp->f_dir.dir_attrib & D_DIR)) {
      		release_f_node(fnp);
      		return (f_node_ptr) 0;
    	} else {
      		// make certain we've moved off root
      		dir_init_fnode(fnp, getdstart(fnp->f_dpb, &fnp->f_dir));
    	}
  	}
  	
  	return fnp;
}
#endif
#endif	//#ifndef READ_ONLY
