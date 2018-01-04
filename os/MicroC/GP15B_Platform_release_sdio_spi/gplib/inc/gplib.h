#ifndef __GPLIB_H__
#define __GPLIB_H__

#include "driver_l2.h"
#include "gplib_cfg.h"


/* A decode call can eat up to FAAD_MIN_STREAMSIZE bytes per decoded channel,
   so at least so much bytes per channel should be available in this stream */
#define FAAD_MIN_STREAMSIZE	 768 /* 6144 bits/channel */
#define AAC_DEC_FRAMESIZE	1024
#define AAC_DEC_BITSTREAM_BUFFER_SIZE 4096

//#define SUPPORT_5_1_CHANNELS

#ifdef SUPPORT_5_1_CHANNELS
#define MAX_CHANNELS		 6
#else
#define MAX_CHANNELS		 2
#endif


#ifdef SUPPORT_5_1_CHANNELS
#define	AAC_DEC_MEMORY_BLOCK_SIZE	20816//24956
#else
#define	AAC_DEC_MEMORY_BLOCK_SIZE	12672//16700
#endif



// ***************************************************************************
#define AAC_OK														0x0


#define UNSUPPORTED_FILE_FORMAT_MP4									0x80000001
#define NOT_MONO_OR_STEREO											0x80000002

#define NOT_LC_OBJECT_TYPE											0x80000011
#define UNABLE_TO_FIND_ADTS_SYNCWORD								0x80000012

#define GAIN_CONTROL_NOT_YET_IMPLEMENTED                            0x80000021
#define PULSE_CODING_NOT_ALLOWED_IN_SHORT_BLOCKS                    0x80000022
#define SCALEFACTOR_OUT_OF_RANGE                                    0x80000023
#define CHANNEL_COUPLING_NOT_YET_IMPLEMENTED                        0x80000024
#define ERROR_DECODING_HUFFMAN_CODEWORD					            0x80000025
#define NON_EXISTENT_HUFFMAN_CODEBOOK_NUMBER_FOUND                  0x80000026
#define INVALID_NUMBER_OF_CHANNELS                                  0x80000027
#define MAXIMUM_NUMBER_OF_BITSTREAM_ELEMENTS_EXCEEDED               0x80000028
#define INPUT_DATA_BUFFER_TOO_SMALL                                 0x80000029
#define ARRAY_INDEX_OUT_OF_RANGE                                    0x8000002A
#define MAXIMUM_NUMBER_OF_SCALEFACTOR_BANDS_EXCEEDED                0x8000002B
#define QUANTISED_VALUE_OUT_OF_RANGE								0x8000002C
#define UNEXPECTED_CHANNEL_CONFIGURATION_CHANGE                     0x8000002D
#define ERROR_IN_PROGRAM_CONFIG_ELEMENT                             0x8000002E
#define PCE_SHALL_BE_THE_FIRST_ELEMENT_IN_A_FRAME                   0x8000002F
#define BITSTREAM_VALUE_NOT_ALLOWED_BY_SPECIFICATION                0x80000030
// ***************************************************************************
/* object types for AAC */
#define MAIN       1
#define LC         2
#define SSR        3
#define LTP        4
#define HE_AAC     5
#define ER_LC     17
#define ER_LTP    19
#define LD        23
#define DRM_ER_LC 27 /* special object type for DRM */
// ***************************************************************************



int aac_dec_init(void *pWorkMem, int downMatrix, const unsigned char *bs_buf);

int aac_dec_parsing(void *pWorkMem, int wi);

/*static const uint32_t sample_rates[] =
{
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025,  8000
};*/

int aac_dec_set_param(void *pWorkMem,
					  unsigned char objectTypeIndex,			// object type (only support LC profile)
					  unsigned char samplingFrequencyIndex,		// sample rate index (refer to sample_rates[])
					  unsigned char channelsConfiguration);		// channels

int aac_dec_run(void *pWorkMem, int wi, short *pcm_out);

unsigned long aac_dec_get_samplerate(void *pWorkMem);
unsigned char aac_dec_get_channel(void *pWorkMem);
int	aac_dec_get_bitspersample(void *pWorkMem);

#ifdef SUPPORT_5_1_CHANNELS
int aac_dec_if_lfe_channel_exists(void *pWorkMem);
#endif

int aac_dec_get_mem_block_size(void);

int aac_dec_get_read_index(void *pWorkMem);

int aac_dec_get_errno(void *pWorkMem);
const char *aac_dec_get_version(void);
int aac_dec_SetRingBufferSize(void *pWorkMem, int size);
void aac_dec_set_ri(void *pWorkMem, int ri_addr);

// Component layer functions
extern void gplib_init(INT32U free_memory_start, INT32U free_memory_end);

// Memory management
extern void gp_mm_init(INT32U free_memory_start, INT32U free_memory_end);
extern void * gp_malloc_align(INT32U size, INT32U align);		// SDRAM allocation. Align value must be power of 2, eg: 2, 4, 8, 16, 32...
extern void * gp_malloc(INT32U size);							// SDRAM allocation
extern void * gp_iram_malloc_align(INT32U size, INT32U align);	// IRAM allocation. Align value must be power of 2, eg: 2, 4, 8, 16, 32...
extern void * gp_iram_malloc(INT32U size);						// IRAM allocation
extern void gp_free(void *ptr);									// Both SDRAM and IRAM can be freed by this function
extern INT32U mm_free_get(void); /*mm debug mode only*/

// File System
#define FAT16_Type	        		0x01
#define FAT32_Type	        		0x02
#define FAT12_Type	        		0x03
#define EXFAT_Type					0x04
#define FORCE_FAT32_Type	        0x12
#define FORCE_FAT16_Type	        0x11
#define FORCE_EXFAT_Type			0x14

/*-----------------  seek flags  ------------------*/
#define SEEK_SET     	0 		/* offset from beginning of the file*/
#define SEEK_CUR     	1 		/* offset from current location     */
#define SEEK_END     	2 		/* offset from eof  */

/***************** open flags (the 2nd parameter)**********************/
#define O_RDONLY        0x0000
#define O_WRONLY        0x0001
#define O_RDWR          0x0002
#define O_ACCMODE       0x0003

#define O_TRUNC         0x0200 	/*    both */
#define O_CREAT         0x0400
#define O_EXCL		    0x4000	/* not fcntl */

/* File attribute constants for _findfirst() */
#define _A_NORMAL       0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY       0x01    /* Read only file */
#define _A_HIDDEN       0x02    /* Hidden file */
#define _A_SYSTEM       0x04    /* System file */
#define _A_SUBDIR       0x10    /* Subdirectory */
#define _A_ARCH         0x20    /* Archive file */

/* FAT file system attribute bits                               */
#define D_NORMAL        0       /* normal                       */
#define D_RDONLY        0x01    /* read-only file               */
#define D_HIDDEN        0x02    /* hidden                       */
#define D_SYSTEM        0x04    /* system                       */
#define D_VOLID         0x08    /* volume id                    */
#define D_DIR           0x10    /* subdir                       */
#define D_ARCHIVE       0x20    /* archive bit                  */

#define D_FILE			(0x40)	/* all attribute but D_DIR		*/
#define D_FILE_1		(0x80)	/* contain D_NORMAL,D_RDONLY,D_ARCHIVE */
#define D_ALL (D_FILE | D_RDONLY | D_HIDDEN | D_SYSTEM | D_DIR | D_ARCHIVE)

#define UNI_GBK			0
#define UNI_BIG5		1
#define UNI_SJIS		2

#define UNI_ENGLISH		0x8003
#define UNI_ARABIC		0x8004
#define UNI_UNICODE		0x8100

// file system error code
/* Internal system error returns                                */
#define SUCCESS         0       /* Function was successful      */
#define DE_INVLDFUNC    -1      /* Invalid function number      */
#define DE_FILENOTFND   -2      /* File not found               */
#define DE_PATHNOTFND   -3      /* Path not found               */
#define DE_TOOMANY      -4      /* Too many open files          */
#define DE_ACCESS       -5      /* Access denied                */
#define DE_INVLDHNDL    -6      /* Invalid handle               */
#define DE_MCBDESTRY    -7      /* Memory control blocks shot   */
#define DE_NOMEM        -8      /* Insufficient memory          */
#define DE_INVLDMCB     -9      /* Invalid memory control block */
#define DE_INVLDENV     -10     /* Invalid enviornement         */
#define DE_INVLDFMT     -11     /* Invalid format               */
#define DE_INVLDACC     -12     /* Invalid access               */
#define DE_INVLDDATA    -13     /* Invalid data                 */
#define DE_INVLDDRV     -15     /* Invalid drive                */
#define DE_RMVCUDIR     -16     /* Attempt remove current dir   */
#define DE_DEVICE       -17     /* Not same device              */
#define DE_MAX_FILE_NUM       -18     /* No more files                */
#define DE_WRTPRTCT     -19     /* No more files                */
#define DE_BLKINVLD     -20     /* invalid block                */
#define DE_INVLDBUF     -24     /* invalid buffer size, ext fnc */
#define DE_SEEK         -25     /* error on file seek           */
#define DE_HNDLDSKFULL  -28     /* handle disk full (?)         */
#define DE_INVLDPARM    -87     /* invalid parameter			*/
#define DE_UNITABERR    -88     /* unitab error					*/
#define DE_TOOMANYFILES	-89		/* to many files				*/

#define DE_DEADLOCK	-36
#define DE_LOCK		-39

#define DE_INVLDCDFILE	-48		/* invalid cd file name */
#define DE_NOTEMPTY		-49		/* DIR NOT EMPTY */
#define DE_ISDIR		-50		/* Is a directory name          */
#define DE_FILEEXISTS   -80		/* File exists                  */
#define DE_DEVICEBUSY	-90
#define DE_NAMETOOLONG	-100	/* specified path name too long */
#define DE_FILENAMEINVALID -110	/* Invalid */


#define LFN_FLAG			1
#define WITHFAT32			1
#define WITHFAT12			1
#define WITHEXFAT           1

struct stat_t
{
	INT16U	st_mode;
  #if WITHEXFAT == 1
	INT64S	st_size;
  #else
    INT32S	st_size;
  #endif  	
	INT32U	st_mtime;
};

struct _diskfree_t {
	INT32U total_clusters;
	INT32U avail_clusters;
	INT32U sectors_per_cluster;
	INT32U bytes_per_sector;
};

struct deviceinfo 	 {
	INT8S device_name[16]; 		 // device name
	INT8S device_enable; 			 // device enable status
	INT8S device_typeFAT; 		 // device FAT type
	INT32U device_availspace; 	 // device available space
	INT32U device_capacity; 		 // device capacity
};

// data structure for _setftime()
struct timesbuf 	 {
	INT16U modtime;
	INT16U moddate;
	INT16U accdate;
};

struct f_info {
	INT8U	f_attrib;
	INT16U	f_time;
	INT16U	f_date;
  #if WITHEXFAT == 1
	INT64U	f_size;  
  #else
	INT32U	f_size;
  #endif
	INT16U	entry;
	INT8S	f_name[256];
	INT8S	f_short_name[8 + 3 + 1];
};

typedef struct {
	INT32U  f_entry;
	INT16U  f_offset;
	INT8S   f_dsk;
	INT8S	f_is_root;		// to differentiate the root folder and the first folder in root folder, in disk with FAT16/FAT12 format
} f_pos, *f_ppos;

struct sfn_info {
    INT8U   f_attrib;
	INT16U  f_time;
	INT16U  f_date;
#if WITHEXFAT == 1
	INT64U	f_size;
#else
	INT32U	f_size;
#endif
    INT8S    f_name[9];
    INT8S    f_extname[4];
    f_pos	f_pos;
};

struct nls_table {
	CHAR			*charset;
	INT16U			Status;
	INT16S			(*init_nls)(void);
	INT16S			(*exit_nls)(void);
	INT16U			(*uni2char)(INT16U uni);
	INT16U			(*char2uni)(INT8U **rawstring);
};

typedef struct
{
	f_pos		cur_pos;
	f_pos		cur_dir_pos;
	INT16U		level;
	INT16S		find_begin_file;
	INT16S		find_begin_dir;
	INT16U		index;

	INT32U		sfn_cluster;
	INT32U		sfn_cluster_offset;

	INT8U		dir_change_flag;
	INT8U		root_dir_flag;			// if the root folder have found the file, this flag is setted
	INT16U		dir_offset[16];
	INT32U		cluster[16];
	INT32U	 	cluster_offset[16];
#if WITHEXFAT == 1
	INT32U 		FatherClu[16];			// 没有. .. 造成要记录父地址	
#endif
} STDiskFindControl;

struct STFileNodeInfo
{
	INT8S		disk;					//disk
	INT16U		attr;					//set the attribute to be found
	INT8S		extname[4];				//extension
	INT8S		*pExtName;				//for extend disk find funtion, support find multi extend name
	INT8S		*path;					//the file in the the path to be found
	INT16U		*FileBuffer;			//buffer point for file nodes
	INT16U		FileBufferSize;			//buffer size, every 20 words contain a file node, so must be multiple of 20
	INT16U		*FolderBuffer;			//buffer point for folder nodes
	INT16U		FolderBufferSize;		//buffer size, every 20 words contain a file node, so must be multiple of 20

	// the following parameter user do not care
	INT8S		flag;
	INT8U		root_dir_flag;			// if the root folder have found the file, this flag is setted
	// 08.02.27 add for search more then one kinds extern name of file
	INT16U		MarkDistance;
	INT32S		MaxFileNum;
	INT32S		MaxFolderNum;
	// 08.02.27 add end
	
	INT32S		(*filter)(INT8S *name);
};

typedef struct
{
	INT8U	name[11 + 1];
	INT16U	f_time;
	INT16U	f_date;
} STVolume;

// file system global variable
extern OS_EVENT *gFS_sem;
extern INT16U			gUnicodePage;
extern const struct nls_table	nls_ascii_table;
//extern const struct nls_table	nls_arabic_table;
extern const struct nls_table	nls_cp936_table;
extern const struct nls_table	nls_cp950_table;
extern const struct nls_table	nls_cp932_table;
extern const struct nls_table	nls_cp1252_table;

/***************************************************************************/
/*        F U N C T I O N    D E C L A R A T I O N S	     			   */
/***************************************************************************/
//========================================================
//Function Name:	fs_get_version
//Syntax:		const char *fs_get_version(void);
//Purpose:		get file system library version
//Note:			the return version string like "GP$xyzz" means ver x.y.zz
//Parameters:   void
//Return:		the library version
//=======================================================
extern const char *fs_get_version(void);

//========================================================
//Function Name:	file_search_start
//Syntax:		INT32S file_search_start(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl)
//Purpose:		search all the files of disk start
//Note:
//Parameters:   stFNodeInfo
//				pstDiskFindControl
//Return:		0 means SUCCESS, -1 means faile
//=======================================================
extern INT32S file_search_start(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl);

//========================================================
//Function Name:	file_search_continue
//Syntax:		INT32S file_search_continue(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl)
//Purpose:		search all the files of disk continue
//Note:
//Parameters:   stFNodeInfo
//				pstDiskFindControl
//Return:		0 means SUCCESS, 1 means search end, -1 means faile
//=======================================================
extern INT32S file_search_continue(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl);

extern INT32S file_search_in_folder_start(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl);
extern INT32S file_search_in_folder_continue(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl);

//========================================================
//Function Name:	getfirstfile
//Syntax:		f_ppos getfirstfile(INT16S dsk, CHAR *extname, struct sfn_info* f_info, INT16S attr);
//Purpose:		find the first file of the disk(will find into the folder)
//Note:
//Parameters:   dsk, extname, f_info, attr
//Return:		f_ppos
//=======================================================
extern f_ppos getfirstfile(INT16S dsk, CHAR *extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S* str));

//========================================================
//Function Name:	getnextfile
//Syntax:		f_ppos getnextfile(INT16S dsk, CHAR *extname, struct sfn_info* f_info, INT16S attr);
//Purpose:		find the next file of the disk(will find into the folder)
//Note:
//Parameters:   dsk, extname, f_info, attr
//Return:		f_ppos
//=======================================================
extern f_ppos getnextfile(INT16S dsk, CHAR * extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S* str));

//Function Name:	getpaperfirstfile
//Syntax:		f_ppos getpaperfirstfile(CHAR *path, CHAR *extname, struct sfn_info* f_info, INT16S attr);

//Purpose:		find the next file of the Folder(will find into the folder)
//Note:
//Parameters:   dsk, extname, f_info, attr
//Return:		f_ppos
//=======================================================
extern f_ppos getpaperfirstfile(CHAR *path, CHAR *extname, struct sfn_info* f_info, INT16S attr, INT32S (*filter)(INT8S *str));

//========================================================
//Function Name:	getpapernextfile
//Syntax:		f_ppos getpapernextfile(CHAR *extname,struct sfn_info* f_info, INT16S attr);
//Purpose:		find the next file of the Folder(will find into the folder)
//Note:
//Parameters:   dsk, extname, f_info, attr
//Return:		f_ppos
//=======================================================
extern f_ppos getpapernextfile(CHAR *extname,struct sfn_info* f_info, INT16S attr, INT32S (*filter)(INT8S *str));

//========================================================
//Function Name:	sfn_open
//Syntax:		INT16S sfn_open(f_ppos ppos);
//Purpose:		open the file that getfirstfile/getnextfile find
//Note:
//Parameters:   ppos
//Return:		file handle
//=======================================================
extern INT16S sfn_open(f_ppos ppos);

//========================================================
//Function Name:	sfn_stat
//Syntax:		INT16S sfn_stat(INT16S fd, struct sfn_info *sfn_info);
//Purpose:		get file attribute of an opened file
//Note:
//Parameters:   fd, sfn_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S sfn_stat(INT16S fd, struct sfn_info *sfn_info);

//========================================================
//Function Name:	GetFileInfo
//Syntax:		INT16S FsgetFileInfo(INT16S fd, struct f_info *f_info);
//Purpose:		get long file name infomation through file handle
//Note:
//Parameters:   fd, f_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S FsgetFileInfo(INT16S fd, struct f_info *f_info);

//========================================================
//Function Name:	GetFileInfo
//Syntax:		INT16S GetFileInfo(f_ppos ppos, struct f_info *f_info);
//Purpose:		get long file name infomation that getfirstfile/getnextfile find
//Note:
//Parameters:   ppos, f_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S GetFileInfo(f_ppos ppos, struct f_info *f_info);

//========================================================
//Function Name:	GetFolderInfo
//Syntax:		INT16S GetFolderInfo(f_ppos ppos, struct f_info *f_info);
//Purpose:		get long folder name infomation that getfirstfile/getnextfile find
//Note:
//Parameters:   ppos, f_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S GetFolderInfo(f_ppos ppos, struct f_info *f_info);

//========================================================
//Function Name:	sfn_unlink
//Syntax:		INT16S sfn_unlink(f_ppos ppos);
//Purpose:		delete the file that getfirstfile/getnextfile find
//Note:
//Parameters:   ppos
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S sfn_unlink(f_ppos ppos);

//========================================================
//Function Name:	StatFileNumByExtName
//Syntax:		INT16S StatFileNumByExtName(INT16S dsk, CHAR *extname, INT32U *filenum);
//Purpose:		get the file number of the disk that have the same extend name
//Note:
//Parameters:   dsk, extname, filenum
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S StatFileNumByExtName(INT16S dsk, CHAR *extname, INT32U *filenum);

//========================================================
//Function Name:	GetFileNumEx
//Syntax:		INT16S GetFileNumEx(struct STFileNodeInfo *stFNodeInfo, INT32U *nFolderNum, INT32U *nFileNum);
//Purpose:		get the file number and the folder number of the disk that have the same extend name
//Note:
//Parameters:   stFNodeInfo, nFolderNum, nFileNum
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S GetFileNumEx(struct STFileNodeInfo *stFNodeInfo, INT32U *nFolderNum, INT32U *nFileNum);

//========================================================
//Function Name:	GetFileNodeInfo
//Syntax:		f_ppos GetFileNodeInfo(struct STFileNodeInfo *stFNodeInfo, INT32U nIndex, struct sfn_info* f_info);
//Purpose:		get the file node infomation
//Note:			before run this function, ensure you have execute the function "GetFileNumEx()"
//				0 <= nIndex < nMaxFileNum
//Parameters:   stFNodeInfo, nIndex, f_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern f_ppos GetFileNodeInfo(struct STFileNodeInfo *stFNodeInfo, INT32U nIndex, struct sfn_info* f_info);

//========================================================
//Function Name:	GetFolderNodeInfo
//Syntax:		f_ppos GetFolderNodeInfo(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, struct sfn_info* f_info);
//Purpose:		get the folder node infomation
//Note:			before run this function, ensure you have execute the function "GetFileNumEx()"
//				0 <= nIndex < nMaxFileNum
//Parameters:   stFNodeInfo, nFolderIndex, f_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern f_ppos GetFolderNodeInfo(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, struct sfn_info* f_info);

//========================================================
//Function Name:	GetFileNumOfFolder
//Syntax:		INT16S GetFileNumOfFolder(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, INT16U *nFile);
//Purpose:		get the file number of a folder
//Note:			before run this function, ensure you have execute the function "GetFileNumEx()"
//Parameters:   stFNodeInfo, nFolderIndex, nFile
//Return:		0, SUCCESS
//				-1, FAILE
//================================== =====================
extern INT16S GetFileNumOfFolder(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, INT16U *nFile);

//========================================================
//Function Name:	FolderIndexToFileIndex
//Syntax:		INT16S FolderIndexToFileIndex(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, INT32U *nFileIndex);
//Purpose:		convert folder id to file id(the index number of first file in this folder)
//Note:			before run this function, ensure you have execute the function "GetFileNumEx()"
//Parameters:   stFNodeInfo, nFolderIndex, nFileIndex
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S FolderIndexToFileIndex(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, INT32U *nFileIndex);

//========================================================
//Function Name:	FileIndexToFolderIndex
//Syntax:		INT16S FileIndexToFolderIndex(struct STFileNodeInfo *stFNodeInfo, INT32U nFileIndex, INT32U *nFolderIndex);
//Purpose:		convert file id to folder id(find what folder id that the file is in)
//				fileindex玫file诘folderindex
//Note:			before run this function, ensure you have execute the function "GetFileNumEx()"
//Parameters:   stFNodeInfo, nFolderIndex, nFileIndex
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S FileIndexToFolderIndex(struct STFileNodeInfo *stFNodeInfo, INT32U nFileIndex, INT32U *nFolderIndex);

//========================================================
//Function Name:	get_fnode_pos
//Syntax:		INT16S get_fnode_pos(f_pos *fpos);
//Purpose:		get the file node position after findfirst/findnext, and then you can open this file by sfn_open
//Note:			before run this function, ensure you have execute the function "_findfirst()/_findnext()"
//Parameters:   fpos
//Return:		0, SUCCESS
//=======================================================
extern INT16S get_fnode_pos(f_pos *fpos);

//f_ppos getfirstfileEx(INT8S *path, CHAR *extname, struct sfn_info *f_info, INT16S attr);
//f_ppos getnextfileEx(CHAR * extname, struct sfn_info* f_info, INT16S attr);

//========================================================
//Function Name:	dosdate_decode
//Syntax:		void dosdate_decode(INT16U dos_date, INT16U *pyear, INT8U *pmonth, INT8U *pday);
//Purpose:		convert the dos_data to year, month, day
//Note:
//Parameters:   dos_date, pyear, pmonth, pday
//Return:		void
//=======================================================
extern void dosdate_decode(INT16U dos_date, INT16U *pyear, INT8U *pmonth, INT8U *pday);

//========================================================
//Function Name:	dostime_decode
//Syntax:		void dostime_decode(INT16U dos_time, INT8U *phour, INT8U *pminute, INT8U *psecond);
//Purpose:		convert the dos_time to hour, minute, second
//Note:
//Parameters:   dos_time, phour, pminute, psecond
//Return:		void
//=======================================================
extern void dostime_decode(INT16U dos_time, INT8U *phour, INT8U *pminute, INT8U *psecond);

//========================================================
//Function Name:	time_decode
//Syntax:		INT8S *time_decode(INT16U *tp, CHAR *timec);
//Purpose:		convert *tp to a string like "hh:mm:ss"
//Note:
//Parameters:   tp, timec
//Return:		the point of string
//=======================================================
extern INT8S *time_decode(INT16U *tp, CHAR *timec);

//========================================================
//Function Name:	date_decode
//Syntax:		INT8S *date_decode(INT16U *dp, CHAR *datec);
//Purpose:		convert *dp to a string like "yyyy-mm-dd"
//Note:
//Parameters:   dp, datec
//Return:		the point of string
//=======================================================
extern INT8S *date_decode(INT16U *dp, CHAR *datec);

//========================================================
//Function Name:	fs_safexit
//Syntax:		void fs_safexit(void);
//Purpose:		close all the opened files except the registed file
//Note:
//Parameters:   NO
//Return:		void
//=======================================================
extern void fs_safexit(void);

//========================================================
//Function Name:	fs_registerfd
//Syntax:		void fs_registerfd(INT16S fd);
//Purpose:		regist opened file so when you call function fs_safexit() this file will not close
//Note:
//Parameters:   fd
//Return:		void
//=======================================================
extern void fs_registerfd(INT16S fd);

//========================================================
//Function Name:	disk_safe_exit
//Syntax:		void disk_safe_exit(INT16S dsk);
//Purpose:		close all the opened files of the disk
//Note:
//Parameters:   dsk
//Return:		void
//=======================================================
extern void disk_safe_exit(INT16S dsk);

//========================================================
//Function Name:	open
//Syntax:		INT16S open(CHAR *path, INT16S open_flag);
//Purpose:		open/creat file
//Note:
//Parameters:   path, open_flag
//Return:		file handle
//=======================================================
extern INT16S open(CHAR *path, INT16S open_flag);

//========================================================
//Function Name:	close
//Syntax:		INT16S close(INT16S fd);
//Purpose:		close file
//Note:
//Parameters:   fd
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S close(INT16S fd);

//========================================================
//Function Name:	read
//Syntax:		INT32S read(INT16S fd, INT32U buf, INT32U size);
//Purpose:		read data
//Note:			the buffer is BYTE address, the size is BYTE size
//Parameters:   fd, buf, size
//Return:		really read size
//=======================================================
extern INT32S read(INT16S fd, INT32U buf, INT32U size);

//========================================================
//Function Name:	write
//Syntax:		INT32S write(INT16S fd, INT32U buf, INT32U size);
//Purpose:		write data
//Note:			the buffer is BYTE address, the size is BYTE size
//Parameters:   fd, buf, size
//Return:		really write size
//=======================================================
extern INT32S write(INT16S fd, INT32U buf, INT32U size);

//========================================================
//Function Name:	lseek64
//Syntax:		INT64S lseek64(INT16S handle, INT64S offset0,INT16S fromwhere);
//Purpose:		change data point of file
//Note:			use lseek(fd, 0, SEEK_CUR) can get current offset of file.
//Parameters:   fd, offset, fromwhere
//Return:		data point
//=======================================================
extern INT64S lseek64(INT16S handle, INT64S offset0,INT16S fromwhere);

//========================================================
//Function Name:	lseek
//Syntax:		INT32S lseek(INT16S fd,INT32S offset,INT16S fromwhere);
//Purpose:		change data point of file
//Note:			use lseek(fd, 0, SEEK_CUR) can get current offset of file.
//Parameters:   fd, offset, fromwhere
//Return:		data point
//=======================================================
extern INT32S lseek(INT16S handle, INT32S offset0,INT16S fromwhere);

//========================================================
//Function Name:	unlink
//Syntax:		INT16S unlink(CHAR *filename);
//Purpose:		delete the file
//Note:
//Parameters:   filename
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S unlink(CHAR *filename);

//========================================================
//Function Name:	rename
//Syntax:		INT16S _rename(CHAR *oldname, CHAR *newname);
//Purpose:		change file name
//Note:
//Parameters:   oldname, newname
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _rename(CHAR *oldname, CHAR *newname);

//========================================================
//Function Name:	mkdir
//Syntax:		INT16S mkdir(CHAR *pathname);
//Purpose:		cread a folder
//Note:
//Parameters:   pathname
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S mkdir(CHAR *pathname);

//========================================================
//Function Name:	rmdir
//Syntax:		INT16S rmdir(CHAR *pathname);
//Purpose:		delete a folder
//Note:			the folder must be empty
//Parameters:   pathname
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S rmdir(CHAR *pathname);

//========================================================
//Function Name:	chdir
//Syntax:		INT16S chdir(CHAR *path);
//Purpose:		change current path to new path
//Note:
//Parameters:   path
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S chdir(CHAR *path);

//========================================================
//Function Name:	getcwd
//Syntax:		INT32U getcwd(CHAR *buffer, INT16S maxlen );
//Purpose:		get current path
//Note:
//Parameters:   buffer, maxlen
//Return:		the path name string point
//=======================================================
extern INT32U getcwd(CHAR *buffer, INT16S maxlen );

//========================================================
//Function Name:	fstat
//Syntax:		INT16S fstat(INT16S handle, struct stat_t *statbuf);
//Purpose:		get file infomation
//Note:			the file must be open
//Parameters:   handle, statbuf
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S fstat(INT16S handle, struct stat_t *statbuf);

//========================================================
//Function Name:	stat
//Syntax:		INT16S stat(CHAR *path, struct stat_t *statbuf);
//Purpose:		get file infomation
//Note:
//Parameters:   path, statbuf
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S stat(CHAR *path, struct stat_t *statbuf);

//========================================================
//Function Name:	_findfirst
//Syntax:		INT16S _findfirst(CHAR *name, struct f_info *f_info, INT16U attr);
//Purpose:		find the first file in one folder
//Note:
//Parameters:   name, f_info, attr
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _findfirst(CHAR *name, struct f_info *f_info, INT16U attr);

//========================================================
//Function Name:	_findnext
//Syntax:		INT16S _findnext(struct f_info *f_info);
//Purpose:		find next file in one folder
//Note:
//Parameters:   f_info
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _findnext(struct f_info *f_info);

//========================================================
//Function Name:	_getdiskfree
//Syntax:		INT16S _getdiskfree(INT16S dsk, struct _diskfree_t *st_free);
//Purpose:		get disk total space and free space
//Note:
//Parameters:   dsk, st_free
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _getdiskfree(INT16S dsk, struct _diskfree_t *st_free);

//========================================================
//Function Name:	vfsFreeSpace
//Syntax:		INT32S vfsFreeSpace(INT16S driver);
//Purpose:		get disk free space
//Note:
//Parameters:   dsk, st_free
//Return:		the free space of the disk
//=======================================================
//INT32S vfsFreeSpace(INT16S driver);
extern INT64U vfsFreeSpace(INT16S driver);

//========================================================
//Function Name:	_changedisk
//Syntax:		INT16S _changedisk(INT8U disk);
//Purpose:		change current disk to another disk
//Note:
//Parameters:   disk
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
#define _changedisk     fs_changedisk
extern INT16S _changedisk(INT8U disk);

//========================================================
//Function Name:	_copy
//Syntax:		INT16S _copy(CHAR *path1, CHAR *path2);
//Purpose:		copy file
//Note:
//Parameters:   srcfile, destfile
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
INT16S _copy(CHAR *path1, CHAR *path2);

//========================================================
//Function Name:	fs_init
//Syntax:		void fs_init(void);
//Purpose:		initial all file system global variable
//Note:
//Parameters:   NO
//Return:		void
//=======================================================
extern void fs_init(void);

//========================================================
//Function Name:	fs_uninit
//Syntax:		INT16S fs_uninit(void);
//Purpose:		free file system resource, and unmount all disk
//Note:
//Parameters:   NO
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S fs_uninit(void);

//========================================================
//Function Name:	tellcurrentfiledir
//Syntax:		INT16U tellcurrentfiledir(void);
//Purpose:		get current directory entry point
//Note:
//Parameters:   NO
//Return:		directory entry point
//=======================================================
extern INT16U tellcurrentfiledir(void);

//========================================================
//Function Name:	telldir
//Syntax:		INT16U telldir(void);
//Purpose:		get next directory entry point
//Note:
//Parameters:   NO
//Return:		directory entry point
//=======================================================
extern INT16U telldir(void);

//========================================================
//Function Name:	seekdir
//Syntax:		void seekdir(INT16U pos);
//Purpose:		set directory entry point, and next time, if you call _findnext(),
//				the function will find file from this point
//Note:
//Parameters:   directory entry point
//Return:		NO
//=======================================================
extern void seekdir(INT16U pos);     //the parameter "pos" must be the return value of "telldir"

//========================================================
//Function Name:	rewinddir
//Syntax:		void rewinddir(void);
//Purpose:		reset directory entry point to 0
//Note:
//Parameters:   NO
//Return:		NO
//=======================================================
extern void rewinddir(void);

//========================================================
//Function Name:	_setfattr
//Syntax:		INT16S _setfattr(CHAR *filename, INT16U attr);
//Purpose:		set file attribute
//Note:
//Parameters:   filename, attr
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _setfattr(CHAR *filename, INT16U attr);

//========================================================
//Function Name:	_setdirattr
//Syntax:		INT16S _setdirattr(CHAR *dirname, INT16U attr);
//Purpose:		set dir attribute
//Note:
//Parameters:   filename, attr
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _setdirattr(CHAR *dirname, INT16U attr);

//========================================================
//Function Name:	_getdirattr
//Syntax:		INT16S _getdirattr(CHAR *dirname, INT16U *attr);
//Purpose:		get dir attribute
//Note:
//Parameters:   filename, attr
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _getdirattr(CHAR *dirname, INT16U *attr);

//========================================================
//Function Name:	_devicemount
//Syntax:		INT16S _devicemount(INT16S disked);
//Purpose:		mount disk, then you can use the disk
//Note:
//Parameters:   disk
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _devicemount(INT16S disked);

//========================================================
//Function Name:	_deviceunmount
//Syntax:		INT16S _deviceunmount(INT16S disked);
//Purpose:		unmount disk
//Note:
//Parameters:   disk
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _deviceunmount(INT16S disked);

//========================================================
//Function Name:	_getfserrcode
//Syntax:		INT16S _getfserrcode(void);
//Purpose:		get error code(see error.h)
//Note:
//Parameters:   NO
//Return:		error code
//=======================================================
extern INT16S _getfserrcode(void);

//========================================================
//Function Name:	_clsfserrcode
//Syntax:		void _clsfserrcode(void);
//Purpose:		clear error code to 0
//Note:
//Parameters:   NO
//Return:		void
//=======================================================
extern void _clsfserrcode(void);

//========================================================
//Function Name:	_format
//Syntax:		INT16S _format(INT8U drv, INT8U fstype);
//Purpose:		format disk to FAT32 or FAT16 type
//Note:
//Parameters:   dsk, fstype
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _format(INT8U drv, INT8U fstype);

//========================================================
//Function Name:	_deleteall
//Syntax:		INT16S _deleteall(CHAR *filename);
//Purpose:		delete all file and folder in one folder
//Note:
//Parameters:   path
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S _deleteall(CHAR *filename);

//========================================================
//Function Name:	GetSectorsPerCluster
//Syntax:		UINT16 GetSectorsPerCluster(UINT16 dsk)
//Purpose:		get Sector number per cluster
//Note:
//Parameters:   dsk
//Return:		sector number
//=======================================================
extern INT16U GetSectorsPerCluster(INT16U dsk);

//========================================================
//Function Name:	_GetCluster
//Syntax:		INT32S _GetCluster(INT16S fd);
//Purpose:		get cluster id that data point now locate
//Note:
//Parameters:   fd
//Return:		cluster id
//=======================================================
extern INT32S _GetCluster(INT16S fd);

//========================================================
//Function Name:	Clus2Phy
//Syntax:		INT32S Clus2Phy(INT16U dsk, INT32U cl_no);
//Purpose:		convert cluster id to sector address
//Note:
//Parameters:   dsk, cl_no
//Return:		sector address
//=======================================================
extern INT32S Clus2Phy(INT16U dsk, INT32U cl_no);

//========================================================
//Function Name:	DeletePartFile
//Syntax:		INT16S DeletePartFile(INT16S fd, INT32U offset, INT32U length);
//Purpose:		delete part of file, from "offset", delete "length" byte
//Note:			the file system will convert the "offset" and "length" to cluster size
//Parameters:   fd, offset, length
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S DeletePartFile(INT16S fd, INT32U offset, INT32U length);

//========================================================
//Function Name:	InserPartFile
//Syntax:		INT16S InserPartFile(INT16S tagfd, INT16S srcfd, INT32U tagoff, INT32U srclen);
//Purpose:		insert the src file to tag file
//Note:			the file system will convert the "offset" and "length" to cluster size
//Parameters:   tagfd, srcfd, tagoff, srclen
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S InserPartFile(INT16S tagfd, INT16S srcfd, INT32U tagoff, INT32U srclen);

//========================================================
//Function Name:	InserPartFile
//Syntax:		INT16 InserPartFile(INT16 tagfd, INT16 srcfd, UINT32 tagoff, UINT32 srclen)
//Purpose:		split tag file into two file, one is remain in tag file, one is in src file
//Note:			the file system will convert the "offset" and "length" to cluster size
//Parameters:   tagfd, srcfd, splitpoint
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S SplitFile(INT16S tagfd, INT16S srcfd, INT32U splitpoint);

//========================================================
//Function Name:	ChangeCodePage
//Syntax:		INT16U ChangeCodePage(INT16U wCodePage);
//Purpose:		select unicode page
//Note:			if the code page is not exsit, the file system will default change code page to "ascii"
//Parameters:   wCodePage
//Return:		code page
//=======================================================
extern INT16U ChangeCodePage(INT16U wCodePage);

//========================================================
//Function Name:	GetCodePage
//Syntax:		INT16U GetCodePage(void);
//Purpose:		get unicode page
//Note:
//Parameters:   NO
//Return:		code page
//=======================================================
extern INT16U GetCodePage(void);

//========================================================
//Function Name:	ChangeUnitab
//Syntax:		INT16S ChangeUnitab(struct nls_table *st_nls_table);
//Purpose:		change unicode convert struct
//Note:
//Parameters:   st_nls_table
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S ChangeUnitab(struct nls_table *st_nls_table);

//========================================================
//Function Name:	checkfattype
//Syntax:		INT16S checkfattype(INT8U disk);
//Purpose:		get the fat type of the disk(FAT16 or FAT32)
//Note:
//Parameters:   disk
//Return:		fat type
//=======================================================
extern INT16S checkfattype(INT8U disk);

//========================================================
//Function Name:	UpdataDir
//Syntax:		INT16S UpdataDir(INT16S fd);
//Purpose:		updata dir information but not close the file
//Note:
//Parameters:   fd
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S UpdataDir(INT16S fd);

//========================================================
//Function Name:	FileRepair
//Syntax:		INT16S FileRepair(INT16S fd);
//Purpose:		if the file is destroy for some reason, this function will repair the file
//Note:			it can't deal with some complicated condition
//Parameters:   fd
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S FileRepair(INT16S fd);

//========================================================
//Function Name:	sformat
//Syntax:		INT16S sformat(INT8U drv, INT32U totalsectors, INT32U realsectors);
//Purpose:		format some disk that size is less than 16 MB
//Note:
//Parameters:   drv, totalsectors, realsectors
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S sformat(INT8U drv, INT32U totalsectors, INT32U realsectors);

//========================================================
//Function Name:	GetDiskOfFile
//Syntax:		INT16S GetDiskOfFile(INT16S fd);
//Purpose:		get the disk id of an opened file
//Note:
//Parameters:   fd
//Return:		disk id, 0 is disk "A", 1 is disk "B", and itc...
//=======================================================
extern INT16S GetDiskOfFile(INT16S fd);

//========================================================
//Function Name:	CreatFileBySize
//Syntax:		INT16S CreatFileBySize(CHAR *path, INT32U size);
//Purpose:		creat a file, and allocate "size" byte space
//Note:			size is byte size
//Parameters:   filename, size
//Return:		file handle
//=======================================================
extern INT16S CreatFileBySize(CHAR *path, INT32U size);

//========================================================
//Function Name:	get_first_file_in_folder
//Syntax:		f_ppos get_first_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir);
//Purpose:		find the first file in a folder
//Note:
//Parameters:   pstDiskFindControl:
//				path: the folder to be found
//				extname: all the file have this extend name can be found. if "*", find all file
//				f_info: the file be found
//				find_child_dir: 1 means find the file in child folder, 0 means not find
//Return:		file position
//=======================================================
extern f_ppos get_first_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir, INT32S (*filter)(INT8S *str));

//========================================================
//Function Name:	get_next_file_in_folder
//Syntax:		f_ppos get_next_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir);
//Purpose:		find the next file in a folder
//Note:
//Parameters:   pstDiskFindControl:
//				path: the folder to be found
//				extname: all the file have this extend name can be found. if "*", find all file
//				f_info: the file be found
//				find_child_dir: 1 means find the file in child folder, 0 means not find
//Return:		file position
//=======================================================
extern f_ppos get_next_file_in_folder(STDiskFindControl *pstDiskFindControl, INT8S *path, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S find_child_dir, INT32S (*filter)(INT8S *str));

//========================================================
//Function Name:	flie_cat
//Syntax:		flie_cat(INT16S file1_handle, INT16S file2_handle);
//Purpose:		cat two file to one file, file1_handle.
//Note:
//Parameters:   file1_handle, file2_handle
//Return:		0: success, other fail
//=======================================================
extern INT16S file_cat(INT16S file1_handle, INT16S file2_handle);

//========================================================
//Function Name:	unlink2
//Syntax:		INT16S unlink2(CHAR *filename);
//Purpose:		delete the file
//Note:
//Parameters:   filename
//Return:		0, SUCCESS
//				-1, FAILE
//=======================================================
extern INT16S unlink2(CHAR *filename);

//========================================================
//Function Name:	set_volume
//Syntax:		INT16S set_volume(INT8U disk_id, INT8U *p_volum);
//Purpose:		set dis volume name
//Note:			
//Parameters:   
//Return:		0: succeed   other :fail
//=======================================================
extern void set_exfatlink(INT8U disk_id,INT8U flags);//Exfat only

//========================================================
//Function Name:	get_freeclu_misc
//Syntax:		INT32U get_freeclu_misc(INT8U disk);
//Purpose:		get free cluster at info sector 
//Note:			
//Parameters:   disk unit 
//Return:		free cluster
//=======================================================
extern INT32U get_freeclu_misc(INT8U disk);

//========================================================
//Function Name:	get_freearea_misc
//Syntax:		INT32U get_freearea_misc(INT8U disk);
//Purpose:		get free area at info sector 
//Note:			
//Parameters:   disk unit 
//Return:		free area
//=======================================================
extern INT32U get_freearea_misc(INT8U disk);

//========================================================
//Function Name:	get_fatsize
//Syntax:		INT32U get_fatsize(INT8U disk); 
//Purpose:		get fat size (bytes)
//Note:			
//Parameters:   disk unit 
//Return:		fat size(bytes)
//=======================================================
extern INT32U get_fatsize_misc(INT8U disk); 

//========================================================
//Function Name:	read_fat_frist
//Syntax:		INT32U read_fat_frist(INT32U disk, INT32U buf, INT32U size);
//Purpose:		from the beginning to read fat 
//Note:			
//Parameters:   disk unit 
//Return:		fat size(bytes)
//=======================================================
extern INT32U read_fat_frist_misc(INT32U disk, INT32U buf, INT32U size);

//========================================================
//Function Name:	read_fat_next
//Syntax:		INT32U read_fat_frist(INT32U disk, INT32U buf, INT32U size);
//Purpose:		Continue to read fat 
//Note:			
//Parameters:   disk unit 
//Return:		fat size(bytes)
//=======================================================
extern INT32U read_fat_next_misc(INT32U disk, INT32U buf, INT32U size);

extern INT16S get_volume(INT8U disk_id, STVolume *pstVolume);
extern INT16S set_volume(INT8U disk_id, INT8U *p_volum);
extern INT16S handle_error_code (INT16S errcode);
extern INT16S gFS_errno;
#endif 		// __GPLIB_H__
