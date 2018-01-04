#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 0

const CHAR _DirWildNameChars[] = "*?./\\\":|<>[]+=;,";

#define DirChar(c)  (((INT8U) (c))>=' ' && !strchr(_DirWildNameChars+10, (CHAR) (c)))
#define WildChar(c) (((INT8U) (c))>=' ' && !strchr(_DirWildNameChars+2, (CHAR) (c)))
#define NameChar(c) (((INT8U) (c))>=' ' && !strchr(_DirWildNameChars, (CHAR) (c)))


/*
    MSD durring an FindFirst search string looks like this;
    (*), & (.)  == Current directory *.*
    (\)         == Root directory *.*
    (..)        == Back one directory *.*

    This always has a "truename" as input, so we may do some shortcuts

    returns number of characters in the directory component (up to the
    last backslash, including d:) or negative if error
 */
INT16S ParseDosName(const INT8S *filename, INT8S *fcbname, BOOLEAN bAllowWildcards)
{
  	INT16U nDirCnt, nFileCnt, nExtCnt;
  	INT8S *lpszLclDir, *lpszLclFile, *lpszLclExt;

  	// Initialize the users data fields
  	nDirCnt = nFileCnt = nExtCnt = 0;

  	// NB: this code assumes ASCII
  #ifndef _555_FS_
  	_5c_to_ff((CHAR *) filename);
  #endif

  	// Now see how long a directory component we have.
  	lpszLclDir = lpszLclFile = filename;
  	filename += 2;
  
  	while (DirChar(*filename)) {
    	if (*filename == '\\') {
      		lpszLclFile = filename + 1;
      	}
    	++filename;
  	}
  	nDirCnt = lpszLclFile - lpszLclDir;

  	// Parse out the file name portion.
  	filename = lpszLclFile;
  	while (bAllowWildcards ? WildChar(*filename) : NameChar(*filename)) {
    	++nFileCnt;
    	++filename;
  	}

  	if (nFileCnt == 0) {
    	INT16S err = DE_PATHNOTFND;
    	
    	if (bAllowWildcards && *filename=='\0' && (nDirCnt==3 || filename[-1]!='\\')) {
        	// D:\ or D:\DOS but not D:\DOS\  
      		err = DE_MAX_FILE_NUM;
      	}
    	return err;
  	}

  	// Now we have pointers set to the directory portion and the file portion. Determine the existance of an extension.
  	lpszLclExt = filename;
  	if ('.' == *filename) {
    	lpszLclExt = ++filename;
    	while (*filename) {
      		if (bAllowWildcards ? WildChar(*filename) : NameChar(*filename)) {
        		++nExtCnt;
        		++filename;
      		} else {
        		return DE_FILENOTFND;
      		}
    	}
  	} else if (*filename) {
    	return DE_FILENOTFND;
    }

  #ifndef _555_FS_
    _ff_to_5c((CHAR *) lpszLclDir);
  #endif

  	// Finally copy whatever the user wants extracted to the user's buffers.
  	fmemset(fcbname, ' ', FNAME_SIZE + FEXT_SIZE);
  	memcpy(fcbname, lpszLclFile, nFileCnt);
  	memcpy(&fcbname[FNAME_SIZE], lpszLclExt, nExtCnt);

  	// Clean up before leaving
  	return nDirCnt;
}

#else

const CHAR _invld_char[] = "\\/:*?\"<>|";

#define LFN_CHAR(c) (((INT8U) (c))>=' ' && !strchr(_invld_char, (c)))


INT8S check_lfn(CHAR *name)
{
	INT16U i;
	INT8U blankNum = 0;
	CHAR *name1;
	CHAR *nametmp;
	
	if (!name || *name=='\0' || !strcmp(name, ".") || !strcmp(name, "..")) {
		return -1;
	}
	
	nametmp = name;
	i=0;
	while (name[i] == ' ') {
		blankNum++;
		i++;
	}
	while (name[i] != '\0')	{				// 果空格后面就是结束符,之前的做法会判断不出来
		if (!LFN_CHAR(name[i])) {
			return -1;
		}
		i++;
	}

	i--;
	while ((name[i]==' ') || (name[i]=='.')) {	// Cut ' ' and '.'at the ent of name
		name[i] = '\0';
		if (!i) {
			if (nametmp == &name[i]) {		// file name is empty
				return -1;		
			}
		}
		i--;
	}

	name1 = name + blankNum;   				// Ignore the leading spaces in the name
	strcpy(name, name1);
	return 0;
}

INT8S check_lfn_dir(INT8U *name)   // check dir name='.xxxx'
{
	if (*name == '.') {
		return -1;
	}
	return 0;
}

#endif