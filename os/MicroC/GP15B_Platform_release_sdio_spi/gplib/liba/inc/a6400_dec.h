#ifndef __a6400_dec_h__
#define __a6400_dec_h__

/**************************************************************************/
// A6400 decoder Head File
// v2030 (131016)(decode by using HW)
/**************************************************************************/

// Constant definition //
#define A6400_DEC_FRAMESIZE					1152	// ???
#define A6400_DEC_BITSTREAM_BUFFER_SIZE   	4096    // size in bytes
#define A6400_DEC_MEMORY_SIZE 				14016//14004	//(20632-8)	// 18456

// Error Code //
#define A6400_DEC_ERR_NONE			0x00000000	/* no error */

#define A6400_DEC_ERR_BUFLEN	   	   	0x80000001	/* input buffer too small (or EOF) */
#define A6400_DEC_ERR_BUFPTR	   	   	0x80000002	/* invalid (null) buffer pointer */

#define A6400_DEC_ERR_NOMEM	   	   	0x80000031	/* not enough memory */

#define A6400_DEC_ERR_LOSTSYNC	   	0x80000101	/* lost synchronization */
#define A6400_DEC_ERR_BADLAYER	   	0x80000102	/* reserved header layer value */
#define A6400_DEC_ERR_BADBITRATE	   	0x80000103	/* forbidden bitrate value */
#define A6400_DEC_ERR_BADSAMPLERATE  	0x80000104	/* reserved sample frequency value */
#define A6400_DEC_ERR_BADEMPHASIS	   	0x80000105	/* reserved emphasis value */
#define A6400_DEC_ERR_BADMPEGID		0x80000106	//for error mpegid add by zgq on 20080508

#define A6400_DEC_ERR_BADCRC	   	   	0x80000201	/* CRC check failed */
#define A6400_DEC_ERR_BADBITALLOC	   	0x80000211	/* forbidden bit allocation value */
#define A6400_DEC_ERR_BADSCALEFACTOR  0x80000221	/* bad scalefactor index */
#define A6400_DEC_ERR_BADMODE         0x80000222	/* bad bitrate/mode combination */
#define A6400_DEC_ERR_BADFRAMELEN	    0x80000231	/* bad frame length */
#define A6400_DEC_ERR_BADBIGVALUES    0x80000232	/* bad big_values count */
#define A6400_DEC_ERR_BADBLOCKTYPE    0x80000233	/* reserved block_type */
#define A6400_DEC_ERR_BADSCFSI	    0x80000234	/* bad scalefactor selection info */
#define A6400_DEC_ERR_BADDATAPTR	    0x80000235	/* bad main_data_begin pointer */
#define A6400_DEC_ERR_BADPART3LEN	    0x80000236	/* bad audio data length */
#define A6400_DEC_ERR_BADHUFFTABLE    0x80000237	/* bad Huffman table select */
#define A6400_DEC_ERR_BADHUFFDATA	    0x80000238	/* Huffman data overrun */
#define A6400_DEC_ERR_BADSTEREO	    0x80000239	/* incompatible block_type for JS */


// Function Definition //
//========================================================
// Function Name : a6400_dec_init
// Syntax : int a6400_dec_init(char *p_workmem, unsigned char *p_bsbuf);
// Purpose :  initial kernel
// Parameters : char *p_workmem: allocated working memory
//              unsigned char* p_bsbuf : pointer to the header of in-buffer
// Return : "0" if init successfully
//========================================================
int a6400_dec_init(char *p_workmem, unsigned char *p_bsbuf);

//========================================================
// Function Name : a6400_dec_parsing
// Syntax : int a6400_dec_parsing(char *p_workmem, unsigned int wi);
// Purpose : parsing header information
// Parameters : char *p_workmem: allocated working memory
//              int wi: write index of bitstream buffer
// Return : error code(Check error code)
//========================================================
int a6400_dec_parsing(char *p_workmem, unsigned int wi);

//========================================================
// Function Name : a6400_dec_run
// Syntax : int a6400_dec_run(char *p_workmem, short *p_pcmbuf, unsigned int wi, short granule);
// Purpose : main decode process
// Parameters : char *p_workmem: allocated working memory
//              short *p_pcmbuf: input pcm buffer address
//              int wi: write index of bitstream buffer
//              short granule : decode to the end of file, 
//                              set the granul as "1" let HW output last granule(half-frame) data.
// Return : "Positive" : number of decoded PCM sample
//          "0" : decode fail
//========================================================
int a6400_dec_run(char *p_workmem, short *p_pcmbuf, unsigned int wi, short granule);

//========================================================
// Function Name : a6400_dec_get_ri
// Syntax : int a6400_dec_get_ri(char *p_workmem);
// Purpose :  get read index of bitstream ring buffer
// Parameters : char *p_workmem: allocated working memory pointer
// Return : read index of bit stream ring buffer
//========================================================
int a6400_dec_get_ri(char *p_workmem);

//========================================================
// Function Name : a6400_dec_get_mem_block_size
// Syntax : int a6400_dec_get_mem_block_size (void);
// Purpose : Get the size of A6400 Decoder Working memory
// Parameters : none
// Return : size of A6400 Decoder Working memory
//========================================================
int a6400_dec_get_mem_block_size (void);

//========================================================
// Function Name : a6400_dec_get_errno
// Syntax : int a6400_dec_get_errno(char *p_workmem);
// Purpose : get error number
// Parameters : char *p_workmem: allocated working memory pointer
// Return : return error code
//========================================================
int a6400_dec_get_errno(char *p_workmem);

//========================================================
// Function Name :  a6400_dec_get_channel
// Syntax : int a6400_dec_get_channel(char *p_workmem);
// Purpose :  get channel
// Parameters : char *p_workmem: allocated working memory pointer
// Return : "1": mono; "2": stereo
//========================================================
int a6400_dec_get_channel(char *p_workmem);

//========================================================
// Function Name :  a6400_dec_get_bitrate
// Syntax : int a6400_dec_get_bitrate(char *p_workmem);
// Purpose :  get bit rate
// Parameters : char *p_workmem: allocated working memory pointer
// Return : bit rate in kbps
//========================================================
int a6400_dec_get_bitrate(char *p_workmem);

//========================================================
// Function Name :  a6400_dec_get_samplerate
// Syntax : int a6400_dec_get_samplerate(char *p_workmem);
// Purpose :  get sample rate
// Parameters : char *p_workmem: allocated working memory pointer
// Return : sample rate in Hz
//========================================================
int a6400_dec_get_samplerate(char *p_workmem);

//========================================================
// Function Name : a6400_dec_get_version
// Syntax : const unsigned char * a6400_dec_get_version(void);
// Purpose : get version
// Parameters : none
// Return : version of library
//========================================================
const char * a6400_dec_get_version(void);

//========================================================
// Function Name : a6400_dec_end
// Syntax : int a6400_dec_end(char *p_workmem,int wi);
// Purpose : check the end of decode process
// Parameters : char *p_workmem: allocated working memory pointer
//              int wi: write index of bitstream buffer
// Return : "1" means end of file, "0" means not end of file
//========================================================
int a6400_dec_end(char *p_workmem,int wi);

#endif  // __a6400_dec_h__

