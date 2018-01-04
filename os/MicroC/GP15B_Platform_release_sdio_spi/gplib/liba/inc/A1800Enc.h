#ifndef __A1800ENC_H__
#define __A1800ENC_H__

/**************************************************************************/
// A1800 encoder Head File 
// v1011(141022)
/**************************************************************************/

// Constant definition //
#define	A18_ENC_FRAMESIZE			320		// input pcm size per frame
#define	A18_ENC_MEMORY_SIZE			5784//660		// 20 + 320*2

// Error Code //
#define A18_ENC_NO_ERROR			0
#define A18_E_MODE_ERR				0x80000004

// Function Definition //
//========================================================
// Function Name : A18_enc_run
// Syntax : int A18_enc_run(unsigned char *p_workmem, const short *p_pcmbuf, unsigned char *p_bsbuf);
// Purpose : main encode process
// Parameters : unsigned char *p_workmem : allocated working memory
//              short * p_pcmbuf : pointer to DecodeOut PCM buffer
//              unsigned char *p_bsbuf : pointer to bitstream input buffer
// Return : "0" for encode sucessfully
//========================================================
extern int A18_enc_run(unsigned char *p_workmem, const short *p_pcmbuf, unsigned char *p_bsbuf);

//========================================================
// Function Name : A18_enc_init
// Syntax : int A18_enc_init(unsigned char *p_workmem);
// Purpose : initial kernel
// Parameters : unsigned char *p_workmem : allocated working memory
// Return :  "0" : initial sucessfully
//           "others": error code for fail
//========================================================
extern int A18_enc_init(unsigned char *p_workmem);

//========================================================
// Function Name : A18_enc_get_BitRate
// Syntax : int A18_enc_get_BitRate(unsigned char *p_workmem);
// Purpose : get bit-rate
// Parameters : unsigned char *p_workmem : allocated working memory
// Return : bit-rate in bps
//========================================================
extern int A18_enc_get_BitRate(unsigned char *p_workmem);

//========================================================
// Function Name : A18_enc_get_PackageSize
// Syntax : int A18_enc_get_PackageSize(unsigned char *p_workmem);
// Purpose : get the package size
// Parameters : unsigned char *p_workmem : allocated working memory
// Return : package size
//========================================================
extern int A18_enc_get_PackageSize(unsigned char *p_workmem);

//========================================================
// Function Name : A18_enc_get_errno
// Syntax : int A18_enc_get_errno(char *A18enc_workmem);
// Purpose : get error number
// Parameters : char *A18enc_workmem : allocated working memory
// Return : error code
//========================================================
extern int A18_enc_get_errno(char *A18enc_workmem);

//========================================================
// Function Name : A18_enc_get_version
// Syntax : const char* A18_enc_get_version(void);
// Purpose : get version
// Parameters : none
// Return : version of library
//========================================================
extern const char* A18_enc_get_version(void);

//========================================================
// Function Name : A18_enc_get_mem_block_size
// Syntax : A18_enc_get_mem_block_size(void);
// Purpose : Get the size of Working-memory
// Parameters : none
// Return : size of Working-memory
//========================================================
extern int A18_enc_get_mem_block_size(void);

//========================================================
// Function Name : A18_enc_set_BitRate
// Syntax : void A18_enc_set_BitRate(unsigned char *p_workmem, int BitRate);
// Purpose :  set encode bit-rate
// Parameters : unsigned char *p_workmem : allocated working memory
//              int BitRate : bit rate in bps
// Return : none
//========================================================
extern void A18_enc_set_BitRate(unsigned char *p_workmem, int BitRate);

#endif //!__A1800ENC_H__
