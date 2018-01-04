#include "audio_encoder.h"
#include "audio_record.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static void *AudEnc0_workmem;


/**
 * @brief   audio encode entrance.
 * @param   none
 * @return 	none
 */
void audio_encode_entrance(void)
{
	if(AudEnc0_workmem == 0) {
		AudEnc0_workmem = audio_record_task_create(AUD_ENC_PRIORITY);
	}
}

/**
 * @brief   audio encode exit.
 * @param   none
 * @return 	none
 */
void audio_encode_exit(void)
{
	if(AudEnc0_workmem) {
		audio_record_task_delete(AudEnc0_workmem);
		AudEnc0_workmem = 0;
	}
}

/**
 * @brief   start audio encode.
 * @param   src[in]: audio file source
 * @param   channle_no[in]: channel no, 0: mono use ADC, 1: mono use MIC, 2: stereo use line in LR
 * @param   sample_rate[in]: sample rate 
 * @param   bit_rate[in]: bit rate 
 * @return 	status, see CODEC_START_STATUS
 */
CODEC_START_STATUS audio_encode_start(MEDIA_SOURCE src, INT32U channle_no, INT16U sample_rate, INT32U bit_rate)
{	
	AudEncInfo_t info;
	
	if(AudEnc0_workmem == 0) {
		return REENTRY_ERROR;
	}
	
	if(audio_record_get_status(AudEnc0_workmem) == C_START_RECORD) {
		return REENTRY_ERROR;
	}
		
	if((src.type != SOURCE_TYPE_FS) && 
		(src.type != SOURCE_TYPE_USER_DEFINE)) {
		return RESOURCE_NO_FOUND_ERROR;
	}
	
	if(channle_no == 0) {
		info.input_device = ADC_LINE_IN;
		info.channel_no = 1;
	} else if(channle_no == 1) {
		info.input_device = MIC_LINE_IN;
		info.channel_no = 1;
	} else {
		info.input_device = LR_LINE_IN;
		info.channel_no = 2;
	}
	
	info.audio_format = src.Format.AudioFormat;
	info.sample_rate = sample_rate;
	info.bit_rate = bit_rate;
	
	if(src.type == SOURCE_TYPE_FS) {
		if(src.type_ID.FileHandle < 0) {	
			return RESOURCE_NO_FOUND_ERROR;
		}
		
		info.source_type = C_GP_FS;
		info.fd = src.type_ID.FileHandle;		
	} else {
		info.source_type = C_USER_DEFINE;
		info.fd = (-1);
	}
		
	if(src.Format.AudioFormat == A1800) {
		audio_record_set_down_sample(AudEnc0_workmem, ENABLE, 2);	
	}
	
	if(adc_record_task_start(AudEnc0_workmem, &info) < 0) {
		return CODEC_START_STATUS_ERROR_MAX;
	}

	return START_OK;
}

/**
 * @brief   stop audio encode.
 * @param   none
 * @return 	none
 */
void audio_encode_stop(void)
{
	if(AudEnc0_workmem) {
		adc_record_task_stop(AudEnc0_workmem);
	}
}

/**
 * @brief   get audio encode status.
 * @param   none
 * @return 	status, see AUDIO_CODEC_STATUS
 */
AUDIO_CODEC_STATUS audio_encode_status(void)
{
	INT32S status;

	if(AudEnc0_workmem == 0) {
		return REENTRY_ERROR;
	}
	
	status = audio_record_get_status(AudEnc0_workmem);
	if(status == C_START_RECORD) {
		return AUDIO_CODEC_PROCESSING;
	} else if(status == C_STOP_RECORD) {
		return AUDIO_CODEC_PROCESS_END;
	} else {
		return AUDIO_CODEC_STATUS_MAX;
	}
	
	return AUDIO_CODEC_PROCESS_END;
}

/**
 * @brief   get audio encode status.
 * @param   bEnable[in]: enable or disable
 * @param   DownSampleFactor[in]: down sample factor 2, 3, 4
 * @return 	status, see CODEC_START_STATUS
 */
CODEC_START_STATUS audio_encode_set_downsample(INT8U bEnable, INT8U DownSampleFactor)
{
	INT32S nRet;
	
	if(DownSampleFactor > 4) {
		DownSampleFactor = 4;
	}
	
	if(DownSampleFactor < 2) {
		DownSampleFactor = 2;
	}
	
	if(AudEnc0_workmem == 0) {
		return REENTRY_ERROR;
	}
	
	nRet = audio_record_set_down_sample(AudEnc0_workmem, bEnable, DownSampleFactor);
	if(nRet < 0) {
		return  RESOURCE_NO_FOUND_ERROR;
	}
	
	return START_OK;
}	

/**
 * @brief   audio encode register user define write callback function.
 * @param   user_write[in]: user define write function
 * @return 	none
 */
void audio_encode_register_user_write_callback(INT32S (*user_write)(INT32U, INT8U , INT32U , INT32U))
{
	if(user_write) {
		audenc_user_write = user_write;
	}
}