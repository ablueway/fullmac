#include <stdio.h>
#include "application.h"
#include "drv_l1_power.h"
#include "drv_l2_display.h"
#include "drv_l2_ad_key.h"
#include "video_decoder.h"
#include "video_encoder.h"
#include "videnc_state.h"
#include "drv_l1_sfr.h"
#include "drv_l2_cdsp.h"

#define DISPLAY_DEVICE	DISDEV_TFT

#define CAL_MODE 0

void save_a_file(INT16S fd)
{
	INT32U cdsp_buf,ret;
	
	
	cdsp_buf = R_CDSP_DMA_RAWBUF_SADDR;
	ret = write(fd,cdsp_buf,1280*720*2);
	if(ret != 1280*720*2)	
		DBG_PRINT("write length fail \r\n");
	close(fd);	
}
static INT32U display_function(INT16U w, INT16U h, INT32U disp_buf)
{
	drv_l2_display_update(DISPLAY_DEVICE, disp_buf);
	return disp_buf;
}

void video_decode_simple_demo(void)
{
	INT8U speed=0, reverse=0;
	INT16U disp_width, disp_height;
	INT32S key, ret;
	struct f_info file_info;
	VIDEO_CODEC_STATUS status;
	VIDEO_INFO information;
	VIDEO_ARGUMENT arg;
	MEDIA_SOURCE src;
	
	// adc dac power on
	drv_l1_power_ldo_codec_ctrl(ENABLE, LDO_CODEC_3P3V);
	
	// Mount device   
	while(1) {   
		if(_devicemount(FS_SD1)) {      
			DBG_PRINT("Mount Disk Fail[%d]\r\n", FS_SD1);         							                            
		} else {      
			DBG_PRINT("Mount Disk success[%d]\r\n", FS_SD1);         	               
			chdir("F:\\");
			break;         
		}      
	}  

	ret = _findfirst("*.avi", &file_info, D_ALL);
	if(ret < 0) {
		DBG_PRINT("_findfirst fail...\r\n");
		goto __exit;
	}
	DBG_PRINT("File Name = %s\r\n", file_info.f_name);
		
	drv_l2_display_init();
	drv_l2_display_get_size(DISPLAY_DEVICE, &disp_width, &disp_height);
	drv_l2_display_start(DISPLAY_DEVICE, DISP_FMT_YUYV);	

	video_decode_entrance();
	video_decode_register_display_callback(display_function);

	arg.bScaler = 0x01;								//scaler output size or not
	arg.bUseDefBuf = FALSE;							//auto alloc buffer size  
	arg.AviDecodeBuf1 = NULL;
	arg.AviDecodeBuf2 = NULL;	
	arg.DisplayWidth = disp_width;					//display width
	arg.DisplayHeight = disp_height;				//display height
	arg.DisplayBufferWidth = disp_width;			//display buffer width
	arg.DisplayBufferHeight = disp_height;			//display buffer height	
	arg.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;	//set output format  
	
	drv_l2_ad_key_init();
	while(1) 
	{
		key = drv_l2_ad_key_scan();
		
		if(key == ADKEY_IO1) {
			ret = _findnext(&file_info);
			if(ret < 0) {
				ret = _findfirst("*.avi", &file_info, D_ALL);
				if(ret < 0) {
					DBG_PRINT("_findfirst fail...\r\n");
					goto __exit;
				}
			}
	
			DBG_PRINT("File Name = %s\r\n", file_info.f_name);
		}
		else if(key == ADKEY_IO2) {
			if(video_decode_status() != VIDEO_CODEC_PROCESS_END) {
				video_decode_stop();
			}
			
			speed = 1;
			reverse = 0;
			
			src.Format.VideoFormat = MJPEG;	
			src.type = SOURCE_TYPE_FS;
			src.type_ID.FileHandle = open((char *)file_info.f_name, O_RDONLY);	
			if(src.type_ID.FileHandle < 0) {
				DBG_PRINT("file open fail\r\n");
				continue;;
			}
			
			DBG_PRINT("video_decode_paser_header\r\n");	
			status = video_decode_paser_header(&information, arg, src);
			if(status != VIDEO_CODEC_STATUS_OK) {
				DBG_PRINT("paser header fail !!!\r\n");
				continue;
			}
			
			DBG_PRINT("Aud SampleRate = %d\r\n", information.AudSampleRate);
			DBG_PRINT("Vid FrameRate = %d\r\n", information.VidFrameRate);
			DBG_PRINT("resolution = %d x %d\r\n", information.Width, information.Height);
			DBG_PRINT("Total Time = %d seconds\r\n", information.TotalDuration);
			
			DBG_PRINT("video_decode_start\r\n");			
//			status = video_decode_start(arg, src);
			if(video_decode_start(arg, src) != START_OK) {
				DBG_PRINT("video start fail!!!\r\n");
				continue;
			}
			
			audio_decode_volume(0x20);
		}
		else if(key == ADKEY_IO3) {
			if(video_decode_status() != VIDEO_CODEC_PROCESS_END) {
				video_decode_stop();
			}
		}
		else if(key == ADKEY_IO4) {
			if(video_decode_status() == VIDEO_CODEC_PROCESSING) {
				video_decode_pause();
			} else if(video_decode_status() == VIDEO_CODEC_PROCESS_PAUSE) {
				video_decode_resume();
			}
		}
		else if(key == ADKEY_IO5) {
			if(video_decode_status() == VIDEO_CODEC_PROCESSING) {
				ret = video_decode_get_current_time();
				ret += 3;
				if(ret > information.TotalDuration) {
					ret = 1;
				}
				
				DBG_PRINT("seek_time = %d\r\n", video_decode_set_play_time(ret));
			}
		}
		else if(key == ADKEY_IO6) {
			if(video_decode_status() == VIDEO_CODEC_PROCESSING) {
				speed++;
				if(speed > 4) {
					speed = 1;
				}
				
				ret = video_decode_set_play_speed(speed);
			}
		}
		else if(key == ADKEY_IO7) {
			if(video_decode_status() == VIDEO_CODEC_PROCESSING) {
				if(reverse == 0) {
					reverse = 1;
				} else {
					reverse = 0;
				}
				ret = video_decode_set_reverse_play(reverse);
			}
		}
		else if(key == ADKEY_IO8) {
			DBG_PRINT("video_decode_exit\r\n");   
			goto __exit;
		}
	}	
		
__exit:	
	if(video_decode_status() != VIDEO_CODEC_PROCESS_END) {
		video_decode_stop();
	}
			
	video_decode_exit();
	drv_l2_ad_key_uninit();	
	drv_l2_display_uninit();
}

INT32U g_cdsp_rawcal_mode;
#if CAPTURE_RAW_MODE
INT32U flag_update;
INT32U update_value;
INT32U g_frame_addr1;
#endif
void video_encode_simple_demo(void)
{
	CHAR file_name[64];
	INT16U disp_width, disp_height;
	INT32S key, index = 0;
	INT64U disk_free;
	CODEC_START_STATUS status;
	VIDEO_ARGUMENT arg;   
	MEDIA_SOURCE src;  
	
	// adc dac power on
	drv_l1_power_ldo_codec_ctrl(ENABLE, LDO_CODEC_3P3V);
	
	// Mount device
	while(1) {   
		if(_devicemount(FS_SD1)) { 
			DBG_PRINT("Mount Disk Fail[%d]\r\n", FS_SD1);         							                            
		} else {      
			DBG_PRINT("Mount Disk success[%d]\r\n", FS_SD1);         
			DBG_PRINT("StartTime = %d\r\n", OSTimeGet());         
			disk_free = vfsFreeSpace(FS_SD1);         
			DBG_PRINT("Disk Free Size = %dMbyte\r\n", disk_free/1024/1024);         
			DBG_PRINT("EndTime = %d\r\n", OSTimeGet());			               
			chdir("F:\\");
			break;         
		}      
	} 
	
	drv_l2_display_init();
	drv_l2_display_get_size(DISPLAY_DEVICE, &disp_width, &disp_height);
	drv_l2_display_start(DISPLAY_DEVICE, DISP_FMT_YUYV);
	
	video_encode_entrance();
	video_encode_register_display_callback(display_function);
	
	drv_l2_set_raw_yuv_path(1);
	g_cdsp_rawcal_mode = 0;
	
	index = 0;
	arg.bScaler = 1;							// must be 1
#if 1
	arg.TargetWidth = 1280;						//encode width   
	arg.TargetHeight = 720;							//encode height   
	arg.SensorWidth = 1280;						//sensor input width    
	arg.SensorHeight = 720;						//sensor input height  
#else
	arg.TargetWidth = 640; 						//encode width   
	arg.TargetHeight = 480;						//encode height   
	arg.SensorWidth = 640;						//sensor input width    
	arg.SensorHeight = 480;						//sensor input height  
#endif	 
	arg.DisplayWidth = disp_width;				//display width      
	arg.DisplayHeight = disp_height;			//display height     
	arg.DisplayBufferWidth = disp_width;		//display buffer width    
	arg.DisplayBufferHeight = disp_height;		//display buffer height   
	arg.VidFrameRate = 30;						//video encode frame rate    
	arg.AudSampleRate = 22050;					//audio record sample rate   
	arg.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;//display output format    
	drv_l2_set_target_sensor_size(arg.TargetWidth, arg.TargetHeight);
	
	video_encode_preview_start(arg);
	
	drv_l2_ad_key_init();
	while(1) 
	{
		key = drv_l2_ad_key_scan();
		if(key == ADKEY_IO1) {
			if(video_encode_status() != VIDEO_CODEC_PROCESS_END) {
				video_encode_stop();
			}
			
			src.type = SOURCE_TYPE_FS;         
			src.Format.VideoFormat = MJPEG;
				
			sprintf((char *)file_name, (const char *)"avi_rec%03d.avi", index++);
			DBG_PRINT("file name = %s\r\n", file_name);
			 
			src.type_ID.FileHandle = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);         
			if(src.type_ID.FileHandle < 0) {         
				DBG_PRINT("file open fail\r\n");            
				continue;
			}
			
			DBG_PRINT("video_encode_start\r\n");         
			status = video_encode_start(src);         
			if(status != START_OK) {
				DBG_PRINT("video start fail!!!\r\n");
				continue;
			}
		}
		else if(key == ADKEY_IO2) {
			DBG_PRINT("video_encode_stop\r\n");   
			if(video_encode_status() != VIDEO_CODEC_PROCESS_END) {
				video_encode_stop();
			}
		}
		else if(key == ADKEY_IO3) { 
		
			src.type = SOURCE_TYPE_FS;         
			src.Format.VideoFormat = MJPEG;
				
			sprintf((char *)file_name, (const char *)"pic%03d.jpg", index++);
			DBG_PRINT("file name = %s\r\n", file_name);
			 
			src.type_ID.FileHandle = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);         
			if(src.type_ID.FileHandle < 0) {         
				DBG_PRINT("file open fail\r\n");            
				continue;
			}
			
			DBG_PRINT("video_encode_capture_picture\r\n");  
			status = video_encode_capture_picture(src);
			if(status != START_OK) {
				DBG_PRINT("video_encode_capture_picture fail!!!\r\n");
				continue;
			}
		}
		else if(key == ADKEY_IO4) { 
		
			src.type = SOURCE_TYPE_FS;         
			src.Format.VideoFormat = MJPEG;
				
			sprintf((char *)file_name, (const char *)"pic%03d.jpg", index++);
			DBG_PRINT("file name = %s\r\n", file_name);
			 
			src.type_ID.FileHandle = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);         
			if(src.type_ID.FileHandle < 0) {         
				DBG_PRINT("file open fail\r\n");            
				continue;
			}
			
			DBG_PRINT("video_encode_capture_size\r\n");  
			status = video_encode_capture_size(src, 90, 640, 480);
			if(status != START_OK) {
				DBG_PRINT("video start fail!!!\r\n");
				continue;
			}
		}
		else if(key == ADKEY_IO7) {
			DBG_PRINT("video_encode_exit\r\n");   
			goto __exit;
		}
	}
	
__exit:	
	if(video_encode_status() != VIDEO_CODEC_PROCESS_END) {
		video_encode_stop();
	}
			
	video_encode_preview_stop(); 
	video_encode_exit();
	drv_l2_ad_key_uninit();	
	drv_l2_display_uninit();
}

void video_encode_wrap_path_simple_demo(void)
{
	void *work_mem;
	void *packer_work_mem;
	char file_name[64];
	INT8U index = 0;
	INT8U cap_index = 0;
	INT16U disp_w, disp_h;
	INT16S fd;
	INT32U key;
	INT64U disk_free;
	VidEncParam_t param;
	
	#if CAPTURE_RAW_MODE
	INT8U index_rawcap;
	MEDIA_SOURCE src; 
	#endif
	// adc dac power on
	drv_l1_power_ldo_codec_ctrl(ENABLE, LDO_CODEC_3P3V);
	
	// Mount device
	while(1) {   
		if(_devicemount(FS_SD1)) { 
			DBG_PRINT("Mount Disk Fail[%d]\r\n", FS_SD1);         							                            
		} else {      
			DBG_PRINT("Mount Disk success[%d]\r\n", FS_SD1);         
			DBG_PRINT("StartTime = %d\r\n", OSTimeGet());         
			disk_free = vfsFreeSpace(FS_SD1);         
			DBG_PRINT("Disk Free Size = %dMbyte\r\n", disk_free/1024/1024);         
			DBG_PRINT("EndTime = %d\r\n", OSTimeGet());			               
			chdir("F:\\");
			break;         
		}      
	}
	
	drv_l2_display_init();
	drv_l2_display_get_size(DISPLAY_DEVICE, &disp_w, &disp_h);
	drv_l2_display_start(DISPLAY_DEVICE, DISP_FMT_RGB565);
	
	#if CAPTURE_RAW_MODE
	g_frame_addr1 =(INT32U) gp_malloc_align(1280*960*2,32);
	index_rawcap = 0;
	DBG_PRINT("mode 0:RAW \r\n");
	DBG_PRINT("mode 1: +LinCorrEn\r\n");
	DBG_PRINT("mode 2: +LenCmp\r\n");
	DBG_PRINT("mode 3: +WbGainEn \r\n");
	DBG_PRINT("mode 4: +LutGammaE\r\n");
	drv_l2_set_raw_capure_setting(CAL_MODE);
	#endif
	
	param.source_type = SOURCE_TYPE_FS;
	param.channel_no = 1;
	param.sample_rate = 22050;
	param.audio_format = WAV;
	param.video_format = C_MJPG_FORMAT;
	param.frame_rate = 30;
	param.quality_value = 50;
	param.display_format = IMAGE_OUTPUT_FORMAT_RGB565;
#if 0
	param.sensor_w = 640;
	param.sensor_h = 480;
	param.disp_w = disp_w;
	param.disp_h = disp_h;
	param.target_w = 640;
	param.target_h = 480;
#else
	param.sensor_w = 1280;
	param.sensor_h = 960;
	param.disp_w = disp_w;
	param.disp_h = disp_h;
	param.target_w = 1280;
	param.target_h = 960;	
#endif	
	drv_l2_set_target_sensor_size(param.target_w, param.target_h);

	work_mem = videnc_open();
	videnc_register_disp_fun(display_function);
	
	drv_l2_set_raw_yuv_path(1);
	videnc_preview_start(work_mem, &param);
	
	drv_l2_ad_key_init();
	while(1) {
		key = drv_l2_ad_key_scan();
		if(key == ADKEY_IO1) {
			videnc_preview_start(work_mem, &param);
			DBG_PRINT("videnc_preview_start\r\n");
		}
		
		if(key == ADKEY_IO2) {
			videnc_preview_stop(work_mem);
			DBG_PRINT("videnc_preview_stop\r\n");
		}
	
		if(key == ADKEY_IO3) { 
			sprintf((char *)file_name, (const char *)"avi_rec%03d.avi", index++);
			DBG_PRINT("file name = %s\r\n", file_name);
			fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);
			if(fd < 0) {
				while(1);
			}
			
			packer_work_mem = videnc_packer_start(work_mem, fd, AVI_PACKER0_PRIORITY);
			if(packer_work_mem == 0) {
				DBG_PRINT("packer open fail\r\n");
				continue;
			}
		
			if(videnc_encode_start(work_mem) < 0) {
				DBG_PRINT("videnc_encode_start fail\r\n");
				continue;
			}
			
			DBG_PRINT("videnc_encode_start\r\n");
		}
	
		if(key == ADKEY_IO4) {
			videnc_encode_stop(work_mem);
			videnc_packer_stop(packer_work_mem);
		} 
		
		if(key == ADKEY_IO5) {
			sprintf((char *)file_name, (const char *)"capture_%03d.jpeg", cap_index++);
			DBG_PRINT("file name = %s\r\n", file_name);
			fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);
			if(fd < 0) {
				while(1);
			}
			
			videnc_capture_start(work_mem,fd);
			
		}
		if(key == ADKEY_IO7) {
			sprintf((char *)file_name, (const char *)"capture_up_%03d.jpeg", cap_index++);
			DBG_PRINT("file name = %s\r\n", file_name);
			fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);
			if(fd < 0) {
				while(1);
			}
			
			videnc_scaler_up_capture_start(work_mem,fd);
			
		}
	#if CAPTURE_RAW_MODE
		if(key == ADKEY_IO6) {
			update_value = CAL_MODE;
			flag_update = 1;
			
			sprintf((char *)file_name, (const char *)"cdsp_mode%d_raw%03d.dat",update_value, index_rawcap++);
			DBG_PRINT("file name = %s\r\n", file_name);
				
			src.type_ID.FileHandle = open(file_name, O_WRONLY|O_CREAT|O_TRUNC);         
			if(src.type_ID.FileHandle < 0) {         
				DBG_PRINT("file open fail\r\n");            
				continue;
			}
			OSTimeDly(30);
			save_a_file(src.type_ID.FileHandle);
		}
		#endif
	}
	
	videnc_close(work_mem);
}
