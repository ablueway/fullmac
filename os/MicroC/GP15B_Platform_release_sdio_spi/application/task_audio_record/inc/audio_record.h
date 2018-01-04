#ifndef __AUDIO_RECORD_H__
#define __AUDIO_RECORD_H__
#include "application.h"
#include "drv_l1_adc.h"

// source type
#define C_GP_FS				0
#define C_USER_DEFINE		1

// audio encode status 
#define C_STOP_RECORD		0x00000000
#define C_START_RECORD		0x00000001

// input device
typedef enum
{
	ADC_LINE_IN = 0,
	MIC_LINE_IN,
	LR_LINE_IN,
	LINE_IN_MAX
} AUDIO_RECORD_TYPE;

typedef struct AudEncInfo_s
{
	INT32U source_type;		// encode file type
	INT16S fd;				// file handle
	INT8U  input_device;	// signal source
	INT8U  channel_no;		// 1: mono, 2: stereo
	INT32U audio_format;	// audio format
	INT32U sample_rate;		// sample rate
	INT32U bit_rate;		// bit rate
} AudEncInfo_t;

// callback function
extern INT32S (*audenc_user_write)(INT32U device, INT8U bHeader, INT32U buffer_addr, INT32U cbLen);

// api function
extern void *audio_record_task_create(INT8U priority);
extern INT32S audio_record_task_delete(void *work_mem);
extern INT32S adc_record_task_start(void *work_mem, AudEncInfo_t *pInfo);
extern INT32S adc_record_task_stop(void *work_mem);
extern INT32U audio_record_get_status(void *work_mem);
extern INT32S audio_record_set_down_sample(void *work_mem, INT32U down_sample_en, INT32U down_sample_factor);
#endif
