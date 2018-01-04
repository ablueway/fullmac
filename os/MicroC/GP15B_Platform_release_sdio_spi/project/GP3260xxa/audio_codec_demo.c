#include <stdio.h>
#include "application.h"
#include "drv_l1_power.h"
#include "drv_l2_ad_key.h"
#include "audio_decoder.h"
#include "audio_encoder.h"

void audio_decode_simple_demo(void)
{
	INT32S ret, key;
	struct f_info file_info;
	MEDIA_SOURCE src;
	CODEC_START_STATUS status;
	AUDIO_ARGUMENT audio_arg;
	AUDIO_CODEC_STATUS audio_status;
	AUDIO_INFO info;
	
	// adc dac power on
	drv_l1_power_ldo_codec_ctrl(ENABLE, LDO_CODEC_3P3V);
	
	// Mount device   
	while(1) {   
		if(_devicemount(FS_SD1)) {      
			DBG_PRINT("Mount Disk Fail[%d]\r\n", FS_SD1);         							                            
		} else {      
			DBG_PRINT("Mount Disk success[%d]\r\n", FS_SD1);         	               
			break;         
		}      
	} 
	
	chdir("F:\\");	
	ret = _findfirst("*.*", &file_info, D_ALL);
	if(ret < 0) {
		DBG_PRINT("_findfirst fail...\r\n");
		goto __exit;
	}
	
	DBG_PRINT("File Name = %s\r\n", file_info.f_name);
	
	audio_decode_entrance();
	
	drv_l2_ad_key_init();
	while(1) 
	{
		key = drv_l2_ad_key_scan();
		
		if(key == ADKEY_IO1) {
			ret = _findnext(&file_info);
			if(ret < 0) {
				ret = _findfirst("*.*", &file_info, D_ALL);
				if(ret < 0) {
					DBG_PRINT("_findfirst fail...\r\n");
					goto __exit;
				}
			}
		
			DBG_PRINT("File Name = %s\r\n", file_info.f_name);
		}
		else if(key == ADKEY_IO2) {
			audio_status = audio_decode_status(audio_arg);
			if(audio_status != AUDIO_CODEC_PROCESS_END) {
				audio_decode_stop(audio_arg);
				audio_decode_parser_stop(audio_arg);
				close(src.type_ID.FileHandle);
				src.type_ID.FileHandle = -1;
			}
			
			src.type = SOURCE_TYPE_FS;
			src.Format.AudioFormat = MP3;
			src.type_ID.FileHandle = open((char *)file_info.f_name, O_RDONLY);	
			if(src.type_ID.FileHandle < 0) {
				continue;
			}
			
			audio_arg.Main_Channel = 1;		// Use DAC Channel A+B
			audio_arg.L_R_Channel = 3;		// Left + Right Channel
			audio_arg.mute = 0;
			audio_arg.volume = 32;			// volume level = 0~63

			status = audio_decode_parser_start(audio_arg, src, &info);
			if(status != START_OK) {
				DBG_PRINT("audio paser fail !!!\r\n");
				while(1);
			}
			
			DBG_PRINT("format = %d\r\n", info.Format);
			DBG_PRINT("channel = %d\r\n", info.Channel);
			DBG_PRINT("sample_rate = %d\r\n", info.SampleRate);
			DBG_PRINT("bit_rate = %d\r\n", info.DataRate);
			DBG_PRINT("duration = %d\r\n", info.Duration);
			
			status = audio_decode_start(audio_arg);
			if(status != START_OK) {
				DBG_PRINT("audio start fail !!!\r\n");
				while(1);
			}
		}
		else if(key == ADKEY_IO3) {
			audio_status = audio_decode_status(audio_arg);
			if(audio_status != AUDIO_CODEC_PROCESS_END) {
				audio_decode_stop(audio_arg);
			}
			
			audio_decode_parser_stop(audio_arg);
			close(src.type_ID.FileHandle);
			src.type_ID.FileHandle = -1;
		}
		else if(key == ADKEY_IO4) {
			audio_status = audio_decode_status(audio_arg);
			if(audio_status == AUDIO_CODEC_PROCESSING) {
				audio_decode_pause(audio_arg);
			} else if(audio_status == AUDIO_CODEC_PROCESS_PAUSED) {
				audio_decode_resume(audio_arg);
			}
		}
		else if(key == ADKEY_IO5 || key == ADKEY_IO5_C) {
			audio_arg.volume++;
			if(audio_arg.volume > 63) {
				audio_arg.volume = 0;
			}
			
			DBG_PRINT("Volunme = %d\r\n", audio_arg.volume);
			audio_decode_volume_set(audio_arg, audio_arg.volume);
		}
		else if(key == ADKEY_IO6) {
			ret = audio_decode_get_cur_time(audio_arg);
			DBG_PRINT("CurTime = %d\r\n", ret);
			
			ret += 30;
			if(ret > audio_decode_get_total_time(audio_arg)) {
				continue;
			}
			
			audio_decode_seek_play(audio_arg, ret);
		}
		else if(key == ADKEY_IO7) {
			DBG_PRINT("audio_decode_exit\r\n"); 
			goto __exit;
		}
	}
	
__exit:	
	audio_decode_stop(audio_arg);
	audio_decode_parser_stop(audio_arg);
	close(src.type_ID.FileHandle);
	audio_decode_exit();
	drv_l2_ad_key_uninit();		
}

void audio_encode_simple_demo(void)
{
	CHAR file_name[64];
	INT32S key, index;
	MEDIA_SOURCE src;
	CODEC_START_STATUS audio_status;
	
	// adc dac power on
	drv_l1_power_ldo_codec_ctrl(ENABLE, LDO_CODEC_3P3V);
	
	// Mount device   
	while(1) {   
		if(_devicemount(FS_SD1)) {      
			DBG_PRINT("Mount Disk Fail[%d]\r\n", FS_SD1);         							                            
		} else {      
			DBG_PRINT("Mount Disk success[%d]\r\n", FS_SD1);         	               
			break;         
		}      
	} 
	
	chdir("F:\\");
	audio_encode_entrance();

	index = 0;
	drv_l2_ad_key_init();
	while(1) 
	{
		key = drv_l2_ad_key_scan();
		
		if(key == ADKEY_IO1) {
			if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
				DBG_PRINT("Stop\r\n");
				audio_encode_stop();
			} else {
				DBG_PRINT("WAV\r\n");
				src.type = SOURCE_TYPE_FS;
				src.Format.AudioFormat = WAV;
				sprintf((char *)file_name, (const char *)"test_wav_%03d.wav", index++);
				DBG_PRINT("FileName = %s\r\n", file_name);
				src.type_ID.FileHandle = open(file_name, O_CREAT|O_TRUNC|O_WRONLY);	
				if(src.type_ID.FileHandle < 0) {
					continue;
				}

				audio_status = audio_encode_start(src, 1, 22050, 0);
				if(audio_status != START_OK) {
					continue;
				}
			}
		}
		else if(key == ADKEY_IO2) {
			if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
				DBG_PRINT("Stop\r\n");
				audio_encode_stop();
			} else {
				DBG_PRINT("MICROSOFT_ADPCM\r\n");
				src.type = SOURCE_TYPE_FS;
				src.Format.AudioFormat = MICROSOFT_ADPCM;
				sprintf((char *)file_name, (const char *)"test_mswav_%03d.wav", index++);
				DBG_PRINT("FileName = %s\r\n", file_name);
				src.type_ID.FileHandle = open(file_name, O_CREAT|O_TRUNC|O_WRONLY);	
				if(src.type_ID.FileHandle < 0) {
					continue;
				}

				audio_status = audio_encode_start(src, 1, 22050, 0);
				if(audio_status != START_OK) {
					continue;
				}
			}
		}
		else if(key == ADKEY_IO3) {
			if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
				DBG_PRINT("Stop\r\n");
				audio_encode_stop();
			} else {
				DBG_PRINT("IMA_ADPCM\r\n");
				src.type = SOURCE_TYPE_FS;
				src.Format.AudioFormat = IMA_ADPCM;
				sprintf((char *)file_name, (const char *)"test_imawav_%03d.wav", index++);
				DBG_PRINT("FileName = %s\r\n", file_name);
				src.type_ID.FileHandle = open(file_name, O_CREAT|O_TRUNC|O_WRONLY);		
				if(src.type_ID.FileHandle < 0) {
					continue;
				}

				audio_status = audio_encode_start(src, 1, 22050, 0);
				if(audio_status != START_OK) {
					continue;
				}
			}
		}
		else if(key == ADKEY_IO4) {
			if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
				DBG_PRINT("Stop\r\n");
				audio_encode_stop();
			} else {
				DBG_PRINT("A1800\r\n");
				src.type = SOURCE_TYPE_FS;
				src.Format.AudioFormat = A1800;
				sprintf((char *)file_name, (const char *)"test_a18_%03d.a18", index++);
				DBG_PRINT("FileName = %s\r\n", file_name);
				src.type_ID.FileHandle = open(file_name, O_CREAT|O_TRUNC|O_WRONLY);		
				if(src.type_ID.FileHandle < 0) {
					continue;
				}

				audio_status = audio_encode_start(src, 1, 16000, 32000);
				if(audio_status != START_OK) {
					continue;
				}
			}
		}
		else if(key == ADKEY_IO5) {
			if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
				DBG_PRINT("Stop\r\n");
				audio_encode_stop();
			} else {
				DBG_PRINT("MP3\r\n");
				src.type = SOURCE_TYPE_FS;
				src.Format.AudioFormat = MP3;
				sprintf((char *)file_name, (const char *)"test_mp3_%03d.mp3", index++);
				DBG_PRINT("FileName = %s\r\n", file_name);
				src.type_ID.FileHandle = open(file_name, O_CREAT|O_TRUNC|O_WRONLY);	
				if(src.type_ID.FileHandle < 0) {
					continue;
				}

				audio_status = audio_encode_start(src, 1, 44100, 320000);
				if(audio_status != START_OK) {
					continue;
				}
			}	
		}
		else if(key == ADKEY_IO6) {
			if(audio_encode_status() == AUDIO_CODEC_PROCESSING) {
				DBG_PRINT("Stop\r\n");
				audio_encode_stop();
			}
		}
		else if(key == ADKEY_IO7) {
			DBG_PRINT("audio_encode_exit\r\n"); 
			goto __exit;
		}
	}

__exit:		
	audio_encode_stop();
	close(src.type_ID.FileHandle);
	audio_encode_exit();
	drv_l2_ad_key_uninit();	
}
