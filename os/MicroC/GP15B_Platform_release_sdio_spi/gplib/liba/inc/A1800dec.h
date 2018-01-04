#ifndef __A1800DEC_H__
#define __A1800DEC_H__
/**************************************************************************/
// A1800 decoder Head File 
// v1021(141022)
/**************************************************************************/
// Constant definition //
#define A1800DEC_MEMORY_BLOCK_SIZE		5824//1676
#define A18_DEC_FRAMESIZE        320
#define A18_DEC_BITSTREAM_BUFFER_SIZE 4096

// Error Code //
#define A18_OK						0x00000001
#define A18_E_NO_MORE_SRCDATA		0x80040005

// Function Definition //
//========================================================
// Function Name : A18_dec_SetRingBufferSize
// Syntax : int A18_dec_SetRingBufferSize(void *obj, int size);
// Purpose : set ring-buffer size in kernel
// Parameters : void *obj : allocated working memory
//              int size : ring-buffer size
// Return : final ring-buffer size in kernel
//========================================================
extern int  A18_dec_SetRingBufferSize(void *obj, int size);

//========================================================
// Function Name : a1800dec_run
// Syntax : int a1800dec_run(void *obj, int write_index, short * pcm_out);
// Purpose : main decode process
// Parameters : void *obj : allocated working memory
//              int write_index : write index of bitstream buffer
//              short * pcm_out : pointer to DecodeOut PCM buffer
// Return : number of decoded PCM sample, or "0" for error
//========================================================
extern int  a1800dec_run(void *obj, int write_index, short * pcm_out);

//========================================================
// Function Name : a1800dec_init
// Syntax : int a1800dec_init(void *obj, const unsigned char* bs_buf);
// Purpose :  initial kernel
// Parameters : void *obj : allocated working memory
//              char* bs_buf : pointer to the header of in-buffer
// Return : "0" for initial sucessfully
//========================================================
extern int  a1800dec_init(void *obj, const unsigned char* bs_buf);

//========================================================
// Function Name : a1800dec_parsing
// Syntax : int a1800dec_parsing(void *obj, int write_index);
// Purpose : parsing header information
// Parameters : void *obj : allocated working memory
//              int write_index : write index of bitstream buffer
// Return : "1" : parsing successfully
//          "others": parsing fail
//========================================================
extern int  a1800dec_parsing(void *obj, int write_index);

//========================================================
// Function Name : a1800dec_read_index
// Syntax : a1800dec_read_index(void *obj);
// Purpose : get read index of bitstream ring buffer
// Parameters : void *obj : allocated working memory
// Return : read-index of bit stream ring-buffer
//========================================================
extern int  a1800dec_read_index(void *obj);

//========================================================
// Function Name : a1800dec_GetMemoryBlockSize
// Syntax : a1800dec_GetMemoryBlockSize(void);
// Purpose : Get the size of A6400 Decoder Working memory
// Parameters : none
// Return : size of Decoder Working-memory
//========================================================
extern int  a1800dec_GetMemoryBlockSize(void);

//========================================================
// Function Name : a1800dec_errno
// Syntax : int a1800dec_errno(void *obj);
// Purpose : get error number
// Parameters : void *obj : allocated working memory
// Return : error code
//========================================================
extern int  a1800dec_errno(void *obj);

//========================================================
// Function Name : A18_dec_get_version
// Syntax : const char* A18_dec_get_version(void);
// Purpose : get version
// Parameters : none
// Return : version of library
//========================================================
extern const char* A18_dec_get_version(void);

//========================================================
// Function Name : A18_dec_get_bitrate
// Syntax : int A18_dec_get_bitrate(void *obj);
// Purpose :  get bit rate
// Parameters : void *obj : allocated working memory
// Return : bit rate in kbps
//========================================================
extern int  A18_dec_get_bitrate(void *obj);

//========================================================
// Function Name :  A18_dec_get_samplerate
// Syntax : int A18_dec_get_samplerate(void *obj);
// Purpose :  get sample rate
// Parameters : void *obj : allocated working memory
// Return : sample rate in Hz
//========================================================
extern int  A18_dec_get_samplerate(void *obj);

//========================================================
// Function Name : A18_dec_get_channel
// Syntax : int A18_dec_get_channel(void *obj);
// Purpose :  get channel
// Parameters : void *obj : allocated working memory
// Return : "1": mono; "2": stereo
//========================================================
extern int	A18_dec_get_channel(void *obj);

//========================================================
// Function Name :  A18_dec_get_bitspersample
// Syntax : int A18_dec_get_bitspersample(void *obj);
// Purpose :  get bits per sample
// Parameters : void *obj : allocated working memory
// Return : bits per sample
//========================================================
extern int	A18_dec_get_bitspersample(void *obj);

#endif //__A1800DEC_H__
