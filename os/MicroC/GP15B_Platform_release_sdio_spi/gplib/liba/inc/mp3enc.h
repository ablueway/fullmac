#ifndef __MP3ENC_H__
#define __MP3ENC_H__

/**************************************************************************/
// MP3 encoder Head File
// v1191 (141022)
/**************************************************************************/
// Constant definition //
#define  MP3_ENC_WORKMEM_SIZE  31704

// Error Code //
#define MP3ENC_ERR_INVALID_CHANNEL		0x80000001
#define MP3ENC_ERR_INVALID_SAMPLERATE	0x80000002
#define MP3ENC_ERR_INVALID_BITRATE		0x80000003

// Function Definition //
//========================================================
// Function Name : mp3enc_init
// Syntax : int mp3enc_init(void *pWorkMem,	void *pWorkMem, int nChannels, int nSamplesPreSec,
//                          int nKBitsPreSec, int Copyright, char *Ring, int RingBufSize, int RingWI);
// Purpose :  initial kernel
// Parameters : void *pWorkMem : allocated working memory
//              int nChannels : input PCM Channel
//              int nSamplesPreSec : input PCM SampleRate
//              int nKBitsPreSec : output MP3 BitRate
//              int Copyright : useless
//              char *Ring : pointer of RingBuffer
//              int RingBufSize : size of RingBuffer
//              int RingWI : Write-index of RingBuffer
// Return : "Positive" : number of Framesize
//          "Negative" : error code for init fail
//========================================================
extern int mp3enc_init(
	void *pWorkMem,
	int nChannels,
	int nSamplesPreSec,
	int nKBitsPreSec,
	int Copyright,
	char *Ring,
	int RingBufSize,
	int RingWI);

//========================================================
// Function Name : mp3enc_encframe
// Syntax : int mp3enc_encframe(void *pWorkMem, const short *PCM);
// Purpose : main encode process
// Parameters : void *pWorkMem : allocated working memory
//              short *PCM : pointer of DecodeOut PCM buffer
// Return : Write index
//========================================================
extern int mp3enc_encframe(void *pWorkMem, const short *PCM);

//========================================================
// Function Name : mp3enc_end
// Syntax : int mp3enc_end(void *pWorkMem);
// Purpose : fill "0" to the remainder frame at the end of encode process
// Parameters : void *pWorkMem : allocated working memory
// Return : Write index
//========================================================
extern int mp3enc_end(void *pWorkMem);

//========================================================
// Function Name : mp3enc_GetErrString
// Syntax : char *mp3enc_GetErrString(int ErrCode);
// Purpose : get error string
// Parameters : int ErrCode : error code
// Return : error string
//========================================================
const char *mp3enc_GetErrString(int ErrCode);

//========================================================
// Function Name : mp3enc_GetVersion
// Syntax : char *mp3enc_GetVersion(void);
// Purpose : get version
// Parameters : none
// Return : version of library
//========================================================
const char *mp3enc_GetVersion(void);

//========================================================
// Function Name : mp3enc_GetWorkMemSize
// Syntax : int mp3enc_GetWorkMemSize(void);
// Purpose : Get the size of Working memory in kernel
// Parameters : none
// Return : size of kernel Working-memory
//========================================================
int mp3enc_GetWorkMemSize(void);


#endif	// __MP3ENC_H__
