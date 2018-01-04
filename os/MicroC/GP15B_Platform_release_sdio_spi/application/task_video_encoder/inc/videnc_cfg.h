#ifndef __VIDENC_CFG_H__
#define __VIDENC_CFG_H__

#define VID_ENC_SYMC_TIMER		TIMER_C 		// timer, A,B,C
#define VID_ENC_TIME_BASE		50				// timer frequency must >= frame rate

#define DISP_BUF_NUM			3				// display buffer number
#define VIDEO_BUF_NUM			3				// video buffer number
#define JPEG_FIFO_LINE			16				// fifo line size
#define FIFO_BUF_NUM			6				// fifo buffer number
#define SCALOR_UP_W			3264
#define SCALOR_UP_H			2448
#define SCA_FRAME_FIFO_LEN		1280*JPEG_FIFO_LINE*2	

#endif //__VIDENC_CFG_H__