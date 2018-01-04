#ifndef AVI_ENCODER_SCALER_JPEG_H_
#define AVI_ENCODER_SCALER_JPEG_H_

#include "application.h"
#include "drv_l1_timer.h"
#include "drv_l1_adc.h"

//video encode mode
#define C_VIDEO_ENCODE_FRAME_MODE		0
#define C_VIDEO_ENCODE_FIFO_MODE		1 
#define VIDEO_ENCODE_MODE				C_VIDEO_ENCODE_FRAME_MODE

//video  
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE 
	#define AVI_ENCODE_DIGITAL_ZOOM_EN		0	//0: disable digital zoom in/out 1: enable digital zoom in/out 
	#define AVI_ENCODE_PREVIEW_DISPLAY_EN	1	//0: disable display preview	 1: enable display preview
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	#define AVI_ENCODE_DIGITAL_ZOOM_EN		0	//fix to 0 when use fifo mode
	#define AVI_ENCODE_PREVIEW_DISPLAY_EN	0	//fix to 0 when use fifo mode
#endif
#define AVI_PACKER_LIB_EN				1		//0: disable, 1:enable avi packer
#define AVI_ENCODE_PRE_ENCODE_EN		0		//0: disable, 1:enable audio and video pre-encode 
#define AVI_ENCODE_VIDEO_ENCODE_EN		1		//0: disable, 1:enable; video record enable
#define AVI_ENCODE_FAST_SWITCH_EN		0		//0: disable, 1:enable; video encode fast stop and start 
#define AVI_ENCODE_VIDEO_TIMER			TIMER_C //timer, A,B,C
#define AVI_ENCODE_TIME_BASE			50		//timer frequency must >= frame rate
#define AVI_ENCODE_SHOW_TIME			0		//0: disable, 1:enable

//buffer number 
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE 
#define AVI_ENCODE_CSI_BUFFER_NO		2//3	//sensor use buffer number
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	#define AVI_ENCODE_CSI_BUFFER_NO	2  	//fix to 2 when use fifo mode
#endif
#define AVI_ENCODE_CSI_FIFO_NO			32	//sensor fifo mode use buffer number		
#define AVI_ENCODE_SCALER_BUFFER_NO		3	//scaler use buffer number
#define AVI_ENCODE_DISPALY_BUFFER_NO	2	//display use buffer number
#define AVI_ENCODE_VIDEO_BUFFER_NO		2//3	//jpeg encode use buffer number
#define AVI_ENCODE_PCM_BUFFER_NO		3	//audio record pcm use buffer number

//audio format
#define AVI_AUDIO_ENCODE_EN				1	//0: disable, 1:enable audio encode 	
#define AVI_ENCODE_AUDIO_FORMAT			WAV //0: no audio, IMA_ADPCM, MICROSOFT_ADPCM

//audio encode buffer size
#define C_WAVE_ENCODE_TIMES				15	//audio encode times

//mic phone input source  
#define C_ADC_LINE_IN					0	//use adc hardware 
#define C_BUILDIN_MIC_IN				1	//use build-in mic
#define C_LINE_IN_LR					2	//use line in lr 
#define MIC_INPUT_SRC					C_BUILDIN_MIC_IN

#define AVI_AUDIO_RECORD_TIMER			ADC_AS_TIMER_F  //adc use timer, C ~ F
#define AVI_AUDIO_RECORD_ADC_CH			ADC_LINE_1		//adc use channel, 0 ~ 3

//avi file max size
#define AVI_ENCODE_CAL_DISK_SIZE_EN		1				//0: disable, 1: enable
#define AVI_FILE_MAX_RECORD_SIZE		2000000000		//2GB
#define C_MIN_DISK_FREE_SIZE			512*1024		//512K

//avi packer buffer
#define FileWriteBuffer_Size			16*32*1024		//avi pack buffer size, must multiple 32k 
#define IndexBuffer_Size				64*1024			//fix to 64k 		

//video encode fifo mode, fifo line set
#define SENSOR_FIFO_8_LINE				(1<<3)
#define SENSOR_FIFO_16_LINE				(1<<4)
#define SENSOR_FIFO_32_LINE				(1<<5)
#define SENSOR_FIFO_LINE				SENSOR_FIFO_32_LINE

//video format
#define C_XVID_FORMAT					0x44495658
#define	C_MJPG_FORMAT					0x47504A4D
#define AVI_ENCODE_VIDEO_FORMAT			C_MJPG_FORMAT //only support mjpeg 

//pause/resume method
#define AUDIO_RESTART_EN				1

// function configure
#define AUDIO_SFX_HANDLE				0
#define APP_QRCODE_BARCODE_EN			0
#endif //AVI_ENCODER_SCALER_JPEG_H_
