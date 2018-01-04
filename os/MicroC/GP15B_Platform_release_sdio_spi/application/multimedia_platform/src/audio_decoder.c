#include "turnkey_audio_decoder_task.h"
#include "turnkey_audio_dac_task.h"
#include "audio_decoder.h"
#include "msg_manager.h"
#include "drv_l1_dac.h"
#include "wav_dec.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define PLAY_SPU_EN		1
#define PLAY_DAC_EN		1
#define PLAY_I2S_EN		1
#define PLAY_MIDI_EN	1

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static void *MainCh_Workmem[AUDIO_CHANNEL_MAX];


static void audio_decode_end(INT32U main_ch)
{
	switch(main_ch)
	{
	case AUDIO_CHANNEL_SPU:
		DBG_PRINT("main channel 0 finsih\r\n");
		break;

	case AUDIO_CHANNEL_DAC:
		DBG_PRINT("main channel 1 finsih\r\n");
		break;
	
	case AUDIO_CHANNEL_I2S:
		DBG_PRINT("main channel 2 finsih\r\n");
		break;	
		
	case AUDIO_CHANNEL_MIDI:
		DBG_PRINT("main channel 3 finsih\r\n");
		break;
	}
}

/**
 * @brief   audio decode entrance.
 * @param   none
 * @return 	none
 */
void audio_decode_entrance(void)
{
	audio_decode_register_end_callback(audio_decode_end);
#if PLAY_SPU_EN == 1	
	if(MainCh_Workmem[AUDIO_CHANNEL_SPU] == 0) {
		MainCh_Workmem[AUDIO_CHANNEL_SPU] = AudDecOpen(AUDIO_CHANNEL_SPU);
	}
#else 
	MainCh_Workmem[AUDIO_CHANNEL_SPU] = 0;
#endif

#if PLAY_DAC_EN == 1	
	if(MainCh_Workmem[AUDIO_CHANNEL_DAC] == 0) {
		MainCh_Workmem[AUDIO_CHANNEL_DAC] = AudDecOpen(AUDIO_CHANNEL_DAC);
	}
#else
	MainCh_Workmem[AUDIO_CHANNEL_DAC] = 0;
#endif

#if PLAY_I2S_EN == 1	
	if(MainCh_Workmem[AUDIO_CHANNEL_I2S] == 0) {
		MainCh_Workmem[AUDIO_CHANNEL_I2S] = AudDecOpen(AUDIO_CHANNEL_I2S);
	}
#else
	MainCh_Workmem[AUDIO_CHANNEL_I2S] = 0;
#endif

#if PLAY_MIDI_EN == 1	
	MainCh_Workmem[AUDIO_CHANNEL_MIDI] = MidiDecOpen();
#else
	MainCh_Workmem[AUDIO_CHANNEL_MIDI] = 0;
#endif
}

/**
 * @brief   audio decode exit.
 * @param   none
 * @return 	none
 */
void audio_decode_exit(void)							
{
#if PLAY_SPU_EN == 1	
	if(MainCh_Workmem[AUDIO_CHANNEL_SPU]) {
		AudDecClose(MainCh_Workmem[AUDIO_CHANNEL_SPU]);
		MainCh_Workmem[AUDIO_CHANNEL_SPU] = 0;
	}
#endif

#if PLAY_DAC_EN == 1	
	if(MainCh_Workmem[AUDIO_CHANNEL_DAC]) {
		AudDecClose(MainCh_Workmem[AUDIO_CHANNEL_DAC]);
		MainCh_Workmem[AUDIO_CHANNEL_DAC] = 0;
	}
#endif

#if PLAY_I2S_EN == 1	
	if(MainCh_Workmem[AUDIO_CHANNEL_I2S]) {
		AudDecClose(MainCh_Workmem[AUDIO_CHANNEL_I2S]);
		MainCh_Workmem[AUDIO_CHANNEL_I2S] = 0;
	}
#endif 
	
#if PLAY_MIDI_EN == 1	
	MidiDecClose(MainCh_Workmem[AUDIO_CHANNEL_MIDI]); 
	MainCh_Workmem[AUDIO_CHANNEL_MIDI] = 0;
#endif
}

/**
 * @brief   start parser audio file.
 * @param   arg[in]: audio file argument
 * @param   src[in]: audio file source
 * @param   audio_info[out]: audio file information
 * @return 	status, see CODEC_START_STATUS
 */
CODEC_START_STATUS audio_decode_parser_start(AUDIO_ARGUMENT arg, MEDIA_SOURCE src, AUDIO_INFO *audio_info)
{
	INT32S ret;
	ParserInfo_t param;
	struct sfn_info temp_sfn_file;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			return CHANNEL_ASSIGN_ERROR;
		}
	
		ret = AudDecGetStatus(MainCh_Workmem[arg.Main_Channel]);	
		if(ret != AUDIO_PLAY_STOP) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
	
		param.fd = src.type_ID.FileHandle;

		switch(src.type)
		{
		case SOURCE_TYPE_FS:
			param.source_type = AUDIO_SRC_TYPE_FS;
			sfn_stat(param.fd, &temp_sfn_file);
			param.file_len = temp_sfn_file.f_size;
			param.data_offset = 0;
			break;
			
		case SOURCE_TYPE_NVRAM:
			param.source_type = AUDIO_SRC_TYPE_APP_RS; 
			//Aud_Para.file_len = nv_rs_size_get(param.fd);
			break;
			
		case SOURCE_TYPE_USER_DEFINE:
			param.source_type = AUDIO_SRC_TYPE_USER_DEFINE;
			param.file_len = arg.data_len;
			param.data_start_addr = arg.data_start_addr;
			param.data_offset = arg.data_offset;
			break;
		
		case SOURCE_TYPE_FS_RESOURCE_IN_FILE:
			param.source_type = AUDIO_SRC_TYPE_FS_RESOURCE_IN_FILE;
			param.data_offset = arg.data_offset;
			param.file_len = arg.data_len;
			break;
		default:
			return CODEC_START_STATUS_ERROR_MAX;
		}
			
		switch(src.Format.AudioFormat)
		{
		#if APP_S880_DECODE_EN == 1
		case S880:
			param.audio_format = AUDIO_TYPE_S880;
			break;
		#endif	
		#if APP_A1600_DECODE_EN == 1
		case A1600:
			param.audio_format = AUDIO_TYPE_A1600;
			break;
		#endif
		#if APP_A1800_DECODE_EN == 1
		case A1800:
			param.audio_format = AUDIO_TYPE_A1800;
			break;
		#endif
		#if APP_A6400_DECODE_EN == 1
		case A6400:
			param.audio_format = AUDIO_TYPE_A6400;
			break;
		#endif
		#if APP_WAV_CODEC_EN == 1
		case WAV:
			param.audio_format = AUDIO_TYPE_WAV;
			break;
		#endif
		#if APP_MP3_DECODE_EN == 1
		case MP3:
			param.audio_format = AUDIO_TYPE_MP3;
			break;
		#endif
		#if APP_WMA_DECODE_EN == 1
		case WMA:
			param.audio_format = AUDIO_TYPE_WMA;
			break;
		#endif	
		#if APP_AAC_DECODE_EN == 1
		case AAC:
			param.audio_format  = AUDIO_TYPE_AAC;
			break;
		#endif
		#if APP_OGG_DECODE_EN == 1
		case OGG:
			param.audio_format  = AUDIO_TYPE_OGG;
			break;
		#endif
		default:
			return AUDIO_ALGORITHM_NO_FOUND_ERROR;
		}
		
		// parser start 
		ret = AudDecParserStart(MainCh_Workmem[arg.Main_Channel], &param);
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		
		audio_info->Format = param.audio_format;
		audio_info->DataRate =  param.bit_rate;
		audio_info->SampleRate = param.sample_rate;
		audio_info->Channel = param.channel_no;
		audio_info->Duration = AudDecGetTotalTime(MainCh_Workmem[arg.Main_Channel]);
		break;

	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			return CHANNEL_ASSIGN_ERROR;
		}
		
		switch(src.type)
		{
		case SOURCE_TYPE_FS:
			ret = MidiDecParserStart(MainCh_Workmem[AUDIO_CHANNEL_MIDI], AUDIO_SRC_TYPE_FS, src.type_ID.FileHandle, 0x00);
			break;
			
		case SOURCE_TYPE_USER_DEFINE:
			ret = MidiDecParserStart(MainCh_Workmem[AUDIO_CHANNEL_MIDI], SOURCE_TYPE_USER_DEFINE, -1, (INT32U)src.type_ID.memptr);
			break;
		default:
			return CODEC_START_STATUS_ERROR_MAX;
		}
		
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		
		audio_info->Format = MIDI;
		break;

	default:
		return CHANNEL_ASSIGN_ERROR;
	}
	
	return START_OK;	
}

/**
 * @brief   stop parser audio file.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
CODEC_START_STATUS audio_decode_parser_stop(AUDIO_ARGUMENT arg)
{
	INT32S ret;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			return CHANNEL_ASSIGN_ERROR;
		}
	
		ret = AudDecParserStop(MainCh_Workmem[arg.Main_Channel]);
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			return CHANNEL_ASSIGN_ERROR;
		}
		
		MidiDecParserStop(MainCh_Workmem[AUDIO_CHANNEL_MIDI]);
		break;
		
	default:
		return CHANNEL_ASSIGN_ERROR;
	}
	
	return START_OK;
}

/**
 * @brief   start audio decode.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
CODEC_START_STATUS audio_decode_start(AUDIO_ARGUMENT arg)
{	
	INT32S ret;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			return CHANNEL_ASSIGN_ERROR;
		}
	
		ret = AudDecStart(MainCh_Workmem[arg.Main_Channel]);
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		
		ret = AudDecSetVolume(MainCh_Workmem[arg.Main_Channel], arg.volume);
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		ret = MidiDecStart(MainCh_Workmem[AUDIO_CHANNEL_MIDI], arg.midi_index);
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		break;
		
	default:
		return CHANNEL_ASSIGN_ERROR;
	}
	
	return START_OK;
}

/**
 * @brief   pause audio decode.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
void audio_decode_pause(AUDIO_ARGUMENT arg)
{
	INT32S ret;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			break;
		}
	
		ret = AudDecPause(MainCh_Workmem[arg.Main_Channel]);
		if(ret < 0) {
			DBG_PRINT("AudPauseFail\r\n");
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		ret = MidiDecPause(MainCh_Workmem[AUDIO_CHANNEL_MIDI]);
		if(ret < 0) {
			DBG_PRINT("MidiPauseFail\r\n");
		}
		break;
	}
}

/**
 * @brief   resume audio decode.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
void audio_decode_resume(AUDIO_ARGUMENT arg)
{
	INT32S ret;

	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			break;
		}
	
		ret = AudDecResume(MainCh_Workmem[arg.Main_Channel]);
		if(ret < 0) {
			DBG_PRINT("AudResumeFail\r\n");
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		ret = MidiDecResume(MainCh_Workmem[AUDIO_CHANNEL_MIDI]);
		if(ret < 0) {
			DBG_PRINT("MidiResumeFail\r\n");
		}
		break;
	}	
}

/**
 * @brief   stop audio decode.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
void audio_decode_stop(AUDIO_ARGUMENT arg)
{
	INT32S ret;

	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			break;
		}
	
		ret = AudDecStop(MainCh_Workmem[arg.Main_Channel]);
		if(ret < 0) {
			DBG_PRINT("AudStopFail\r\n");
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		ret = MidiDecStop(MainCh_Workmem[AUDIO_CHANNEL_MIDI]);
		if(ret < 0) {
			DBG_PRINT("MidiStopFail\r\n");
		}
		break;
	}
}

/**
 * @brief   seek audio decode.
 * @param   arg[in]: audio file argument
 * @param   sec[in]: second
 * @return 	status, see CODEC_START_STATUS
 */
CODEC_START_STATUS audio_decode_seek_play(AUDIO_ARGUMENT arg, INT32U sec)
{
	INT32S status;
	INT32S ret;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			return CHANNEL_ASSIGN_ERROR;
		}
	
		status = AudDecGetStatus(MainCh_Workmem[arg.Main_Channel]);
		if(status != AUDIO_PLAYING) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
	
		if(sec >= audio_decode_get_total_time(arg)) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
	
		ret = AudDecSeekPlay(MainCh_Workmem[arg.Main_Channel], sec);
		if(ret < 0) {
			return CODEC_START_STATUS_ERROR_MAX;
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		break;
		
	default:
		return CHANNEL_ASSIGN_ERROR;
	}
	
	return START_OK;	
}

/**
 * @brief   set audio decode volume.
 * @param   arg[in]: audio file argument
 * @param   volume[in]: volume
 * @return 	status, see CODEC_START_STATUS
 */
void audio_decode_volume_set(AUDIO_ARGUMENT arg, INT32S volume)
{
	INT32S ret;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			break;
		}
	
		ret = AudDecSetVolume(MainCh_Workmem[arg.Main_Channel], volume);
		if(ret < 0) {
			DBG_PRINT("AudVolumeSetFail\r\n");
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		ret = MidiDecSetVolume(MainCh_Workmem[AUDIO_CHANNEL_MIDI], volume);
		if(ret < 0) {
			DBG_PRINT("MidiStopFail\r\n");
		}
		break;
	}	
}

/**
 * @brief   set audio decode mute.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
void audio_decode_mute(AUDIO_ARGUMENT arg)
{
	INT32S ret;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			break;
		}
	
		ret = AudDecSetVolume(MainCh_Workmem[arg.Main_Channel], 0);
		if(ret < 0) {
			DBG_PRINT("AudVolumeSetFail\r\n");
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		ret = MidiDecSetVolume(MainCh_Workmem[AUDIO_CHANNEL_MIDI], 0);
		if(ret < 0) {
			DBG_PRINT("MidiStopFail\r\n");
		}
		break;
	}	
}

/**
 * @brief   get audio decode statue.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
AUDIO_CODEC_STATUS audio_decode_status(AUDIO_ARGUMENT arg)
{
	INT32S status;
	
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			break;
		}
	
		status = AudDecGetStatus(MainCh_Workmem[arg.Main_Channel]);
		if(status == AUDIO_PLAY_STOP) {
			return AUDIO_CODEC_PROCESS_END;
		} else if(status == AUDIO_PLAYING) {
			return AUDIO_CODEC_PROCESSING;
		} else {
			return AUDIO_CODEC_PROCESS_PAUSED;
		}
		break;
	
	case AUDIO_CHANNEL_MIDI:
		if(MainCh_Workmem[AUDIO_CHANNEL_MIDI] == 0) {
			break;
		}
		
		status = MidiDecGetStatus(MainCh_Workmem[AUDIO_CHANNEL_MIDI]);
		if(status == AUDIO_PLAY_STOP) {
			return AUDIO_CODEC_PROCESS_END;
		} else if(status == AUDIO_PLAYING) {
			return AUDIO_CODEC_PROCESSING;
		} else {
			return AUDIO_CODEC_PROCESS_PAUSED;
		}
		break;
	}
	
	return AUDIO_CODEC_PROCESS_END;
}

/**
 * @brief   get audio file total time.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
INT32U audio_decode_get_total_time(AUDIO_ARGUMENT arg)
{
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			return 0;
		}
	
		return AudDecGetTotalTime(MainCh_Workmem[arg.Main_Channel]);
	
	case AUDIO_CHANNEL_MIDI:
		break;
	}
	
	return 0;
}

/**
 * @brief   get audio decode current time.
 * @param   arg[in]: audio file argument
 * @return 	status, see CODEC_START_STATUS
 */
INT32U audio_decode_get_cur_time(AUDIO_ARGUMENT arg)
{
	switch(arg.Main_Channel) 
	{
	case AUDIO_CHANNEL_SPU:
	case AUDIO_CHANNEL_DAC:
	case AUDIO_CHANNEL_I2S:
		if(MainCh_Workmem[arg.Main_Channel] == 0) {
			return 0;
		}
	
		return AudDecGetCurTime(MainCh_Workmem[arg.Main_Channel]);
	
	case AUDIO_CHANNEL_MIDI:
		break;
	}
	
	return 0;
}

/**
 * @brief   audio decode register user define read callback function.
 * @param   user_read[in]: user define read function
 * @return 	none
 */
void audio_decode_register_user_read_callback(INT32S (*user_read)(INT32U, INT32U, INT32U, INT8U *, INT32U))
{
	if(user_read) {
		auddec_user_read = user_read;
	}
}

/**
 * @brief   audio decode register decode done callback function.
 * @param   end[in]: decode end function
 * @return 	none
 */
void audio_decode_register_end_callback(void (*end)(INT32U))
{
	if(end) {
		auddec_end = end;
	}
}

