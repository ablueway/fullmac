#ifndef __mp3_dec_h__
#define __mp3_dec_h__

/**************************************************************************/
// MP3 decoder Head File 
// v1200 (131016) (decode by using HW)
/**************************************************************************/

// Constant definition //
#define MP3_DEC_FRAMESIZE					1152	// ???
#define MP3_DEC_BITSTREAM_BUFFER_SIZE   	2048    // size in bytes
#define MP3_DEC_MEMORY_SIZE 				14020	//(20632-8)	// 18456

// Error Code //
#define MP3_DEC_ERR_NONE			0x00000000	/* no error */

#define MP3_DEC_ERR_BUFLEN	   	   	0x80000001	/* input buffer too small (or EOF) */
#define MP3_DEC_ERR_BUFPTR	   	   	0x80000002	/* invalid (null) buffer pointer */

#define MP3_DEC_ERR_NOMEM	   	   	0x80000031	/* not enough memory */

#define MP3_DEC_ERR_LOSTSYNC	   	0x80000101	/* lost synchronization */
#define MP3_DEC_ERR_BADLAYER	   	0x80000102	/* reserved header layer value */
#define MP3_DEC_ERR_BADBITRATE	   	0x80000103	/* forbidden bitrate value */
#define MP3_DEC_ERR_BADSAMPLERATE  	0x80000104	/* reserved sample frequency value */
#define MP3_DEC_ERR_BADEMPHASIS	   	0x80000105	/* reserved emphasis value */
#define MP3_DEC_ERR_BADMPEGID		0x80000106	//for error mpegid add by zgq on 20080508

#define MP3_DEC_ERR_BADCRC	   	   	0x80000201	/* CRC check failed */
#define MP3_DEC_ERR_BADBITALLOC	   	0x80000211	/* forbidden bit allocation value */
#define MP3_DEC_ERR_BADSCALEFACTOR  0x80000221	/* bad scalefactor index */
#define MP3_DEC_ERR_BADMODE         0x80000222	/* bad bitrate/mode combination */
#define MP3_DEC_ERR_BADFRAMELEN	    0x80000231	/* bad frame length */
#define MP3_DEC_ERR_BADBIGVALUES    0x80000232	/* bad big_values count */
#define MP3_DEC_ERR_BADBLOCKTYPE    0x80000233	/* reserved block_type */
#define MP3_DEC_ERR_BADSCFSI	    0x80000234	/* bad scalefactor selection info */
#define MP3_DEC_ERR_BADDATAPTR	    0x80000235	/* bad main_data_begin pointer */
#define MP3_DEC_ERR_BADPART3LEN	    0x80000236	/* bad audio data length */
#define MP3_DEC_ERR_BADHUFFTABLE    0x80000237	/* bad Huffman table select */
#define MP3_DEC_ERR_BADHUFFDATA	    0x80000238	/* Huffman data overrun */
#define MP3_DEC_ERR_BADSTEREO	    0x80000239	/* incompatible block_type for JS */


// Function Definition //
//========================================================
// Function Name : mp3_dec_get_version
// Syntax : char * mp3_dec_get_version(void);
// Purpose : get library version
// Parameters : none
// Return : version of library
//========================================================
const char * mp3_dec_get_version(void);

//========================================================
// Function Name : mp3_dec_init
// Syntax : int mp3_dec_init(void *p_workmem, void *p_bsbuf);
// Purpose : initial kernel
// Parameters : void *p_workmem: allocated working memory
//              void* p_bsbuf : pointer to input bitstream buffer
// Return : "0" if init successfully
//========================================================
int mp3_dec_init(void *p_workmem, void *p_bsbuf);

//========================================================
// Function Name : mp3_dec_set_ring_size
// Syntax : int mp3_dec_set_ring_size(void *p_workmem, int size);
// Purpose : set the size of bitstream buffer
// Parameters : void *p_workmem: pointer to working memory
//              int size: size of bitstream
// Return : size of bitstream buffer, it must be the mutiple of 512
//========================================================
int mp3_dec_set_ring_size(void *p_workmem, int size);

//========================================================
// Function Name : mp3_dec_parsing
// Syntax : int mp3_dec_parsing(void *p_workmem, int wi);
// Purpose : parsing header information
// Parameters : void *p_workmem: allocated working memory
//              int wi: write index of bitstream buffer
// Return : error code(Check error code)
//========================================================
int mp3_dec_parsing(void *p_workmem, int wi);

//========================================================
// Function Name : mp3_dec_run
// Syntax : int mp3_dec_run(void *p_workmem, short *p_pcmbuf, int wi, int granule);
// Purpose : main decode process
// Parameters : void *p_workmem: allocated working memory
//              short *p_pcmbuf: pointer to input pcm buffer
//              int wi: write index of bitstream buffer
//              short granule : decode to the end of file, 
//                              set the granul as "1" let HW output last granule(half-frame) data.
// Return : number of decoded PCM sample, or "0" for error
//========================================================
int mp3_dec_run(void *p_workmem, short *p_pcmbuf, int wi, int granule);

//========================================================
// Function Name : mp3_dec_get_ri
// Syntax : int mp3_dec_get_ri(void *p_workmem);
// Purpose : get read index of bitstream ring buffer
// Parameters : void *p_workmem: allocated working memory pointer
// Return : read index of bitstream ring buffer
//========================================================
int mp3_dec_get_ri(void *p_workmem);

//========================================================
// Function Name : mp3_dec_get_mpegid
// Syntax : char *mp3_dec_get_mpegid(void *p_workmem);
// Purpose : get MPEG-ID (mpeg1, mpeg2, mpeg2.5)
// Parameters : void *p_workmem: allocated working memory pointer
// Return : MPEG-ID
//========================================================
const char *mp3_dec_get_mpegid(void *p_workmem);

//========================================================
// Function Name : mp3_dec_get_mem_block_size
// Syntax : int mp3_dec_get_mem_block_size (void);
// Purpose : Get the size of MP3 Decoder Working memory
// Parameters : none
// Return : size of MP3 Decoder Working memory
//========================================================
int mp3_dec_get_mem_block_size (void);

//========================================================
// Function Name : mp3_dec_get_errno
// Syntax : int mp3_dec_get_errno(void *p_workmem);
// Purpose : get error number
// Parameters : void *p_workmem: allocated working memory pointer
// Return : error code
//========================================================
int mp3_dec_get_errno(void *p_workmem);

//========================================================
// Function Name : mp3_dec_get_layer
// Syntax : int mp3_dec_get_layer(void *p_workmem);
// Purpose : get layer
// Parameters : char *p_workmem: allocated working memory pointer
// Return : number of layer
//========================================================
int mp3_dec_get_layer(void *p_workmem);

//========================================================
// Function Name :  mp3_dec_get_channel
// Syntax : int mp3_dec_get_channel(void *p_workmem);
// Purpose :  get channel
// Parameters : void *p_workmem: allocated working memory pointer
// Return : "1": mono; "2": stereo
//========================================================
int mp3_dec_get_channel(void *p_workmem);

//========================================================
// Function Name : mp3_dec_get_bitrate
// Syntax : int mp3_dec_get_bitrate(void *p_workmem);
// Purpose :  get bit rate
// Parameters : void *p_workmem: allocated working memory pointer
// Return : bit rate in kbps
//========================================================
int mp3_dec_get_bitrate(void *p_workmem);

//========================================================
// Function Name :  mp3_dec_get_samplerate
// Syntax : int mp3_dec_get_samplerate(void *p_workmem);
// Purpose :  get sample rate
// Parameters : void *p_workmem: allocated working memory pointer
// Return : sample rate in Hz
//========================================================
int mp3_dec_get_samplerate(void *p_workmem);

//========================================================
// Function Name : mp3_dec_end
// Syntax : int mp3_dec_end(void *p_workmem, int wi);
// Purpose : check the end of decode-process
// Parameters : void *p_workmem: allocated working memory pointer
//              int wi: write index of bitstream buffer
// Return : "1" means end of file, "0" means not end of file
//========================================================
int mp3_dec_end(void *p_workmem, int wi);

//========================================================
// Function Name : mp3_dec_set_eq_table
// Syntax : void mp3_dec_set_eq_table(void *p_workmem, short *eqtable);
// Purpose : set EQ table
// Parameters : void *p_workmem: allocated working memory pointer
//              short *eqtable: pointer to EQ Table (unsigned short EQ_table[12])
// Return : none
//========================================================
void mp3_dec_set_eq_table(void *p_workmem, short *eqtable);

//========================================================
// Function Name : mp3_dec_set_band_addr
// Syntax : void mp3_dec_set_band_addr(void *p_workmem, short *band);
// Purpose : set the band-buffer for output 32-bands EQ amplitude
// Parameters : void *p_workmem: allocated working memory pointer
//              short *band: pointer to EQ Band (short EQ_Band[32])
// Return : none
//========================================================
void mp3_dec_set_band_addr(void *p_workmem, short *band);


#endif  // __mp3_dec_h__

