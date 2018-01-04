#ifndef __WAV_ENC_H__
#define __WAV_ENC_H__

/**************************************************************************/
// Wav encoder Head File
// v1031 (140408)
/**************************************************************************/

// Constant Definition //
#define WAV_ENC_MEMORY_BLOCK_SIZE		40 
#define DVRWAVE_FRAME_SIZE	512

// wave format tag //
#define	WAVE_FORMAT_PCM				(0x0001) 
#define	WAVE_FORMAT_ADPCM			(0x0002)
#define	WAVE_FORMAT_ALAW			(0x0006)
#define	WAVE_FORMAT_MULAW			(0x0007)
#define WAVE_FORMAT_IMA_ADPCM		(0x0011)

// wave decoder error code //
#define WAV_ENC_NO_ERROR    			0
#define WAV_ENC_UNKNOWN_FORMAT			0x80000100
#define WAV_ENC_CHANNEL_ERROR			0x80000101

// Function Definition //
//========================================================
// Function Name : wav_enc_init
// Syntax : int wav_enc_init(void *p_workmem);
// Purpose :  initial kernel
// Parameters : void *p_workmem : allocated working memory
// Return : "0" for initial sucessfully
//          "other value" for fail (check error code)
//========================================================
int wav_enc_init(void *p_workmem);

//========================================================
// Function Name : wav_enc_run
// Syntax : int wav_enc_run(void *p_workmem,short *p_pcmbuf,unsigned char *p_bsbuf);
// Purpose : main decode process
// Parameters : void *p_workmem : allocated working memory
//              short *p_pcmbuf : pointer to input pcm buffer
//              unsigned char *p_bsbuf : pointer to encode out data buffer
// Return : samples of encode out data
//========================================================
int wav_enc_run(void *p_workmem,short *p_pcmbuf,unsigned char *p_bsbuf);

//========================================================
// Function Name : wav_enc_get_header
// Syntax : int wav_enc_get_header(void *p_workmem, void *p_header,int length, int numsamples);
// Purpose : write infomation, file-length and decode out samples into wav-header
// Parameters : void *p_workmem : allocated working memory
//              void *p_header : Wave Header buffer
//              int length : full file size of encoded wav-file
//              int numsamples : full sample size of decoded wav-file
// Return : "0" for write sucessfully
//          "other value" for fail (check error code)
//========================================================
int wav_enc_get_header(void *p_workmem, void *p_header,int length, int numsamples);

//========================================================
// Function Name : wav_enc_get_mem_block_size
// Syntax : int wav_enc_get_mem_block_size(void);		
// Purpose : get size of working memory
// Parameters :none
// Return : size of working memory
//========================================================
int wav_enc_get_mem_block_size(void);

//========================================================
// Function Name : wav_enc_get_SamplePerFrame
// Syntax : int wav_enc_get_SamplePerFrame(void *p_workmem);		
// Purpose : get blocksize
// Parameters : void *p_workmem : allocated working memory
// Return : value of blocksize
//========================================================
int wav_enc_get_SamplePerFrame(void *p_workmem);

//========================================================
// Function Name : wav_enc_get_BytePerPackage
// Syntax : int wav_enc_get_BytePerPackage(void *p_workmem);		
// Purpose : get Block Align
// Parameters : void *p_workmem : allocated working memory
// Return : value of Block Align
//========================================================
int wav_enc_get_BytePerPackage(void *p_workmem);

//========================================================
// Function Name : wav_enc_get_HeaderLength
// Syntax : int wav_enc_get_HeaderLength(void *p_workmem);		
// Purpose : get Header Length
// Parameters : void *p_workmem : allocated working memory
// Return : length of header
//========================================================
int wav_enc_get_HeaderLength(void *p_workmem);

//========================================================
// Function Name : wav_enc_Set_Parameter
// Syntax : int wav_enc_Set_Parameter(void *p_workmem, int Channel,int SampleRate,int FormatTag);
// Purpose : set parameters into kernel
// Parameters : void *p_workmem : working memory pointer
//              int Channel : channel of input PCM
//              int SampleRate : SampleRate of input PCM
//              int FormatTag : wave format to be encode(check wave format tag)
// Return : "0" : set sucessfully
//           "others": error code for fail
//========================================================
int wav_enc_Set_Parameter(void *p_workmem, int Channel,int SampleRate,int FormatTag);


#endif //__WAV_ENC_H__
