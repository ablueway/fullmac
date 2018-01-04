#include <stdlib.h>
#include "drv_l1_cache.h"
#include "drv_l1_i2s.h"
#include "drv_l1_adc.h"
#include "drv_l1_scaler.h"
#include "drv_l2_scaler.h"
#include "drv_l1_jpeg.h"
#include "drv_l1_csi.h"
#include "drv_l1_cdsp.h"
#include "drv_l2_cdsp.h"
#include "drv_l2_sensor.h"
#include "gplib_jpeg.h"
#include "gplib_jpeg_encode.h"
#include "avi_encoder_app.h"
#include "jpeg_header.h"
#include "font.h"
#include "drv_l2_nand_ext.h"

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
AviEncPara_t AviEncPara, *pAviEncPara;
AviEncAudPara_t AviEncAudPara, *pAviEncAudPara;
AviEncVidPara_t AviEncVidPara, *pAviEncVidPara;
AviEncPacker_t AviEncPacker0, *pAviEncPacker0;
AviEncPacker_t AviEncPacker1, *pAviEncPacker1;
catpure_t *pCap;

GP_AVI_AVISTREAMHEADER	avi_aud_stream_header;
GP_AVI_AVISTREAMHEADER	avi_vid_stream_header;
GP_AVI_BITMAPINFO		avi_bitmap_info;
GP_AVI_PCMWAVEFORMAT	avi_wave_info;

static INT8U g_csi_index;
static INT8U g_pcm_index;


// function
void avi_encode_init(void)
{
    pAviEncPara = &AviEncPara;
    gp_memset((INT8S *)pAviEncPara, 0, sizeof(AviEncPara_t));
    
    pAviEncAudPara = &AviEncAudPara;
    gp_memset((INT8S *)pAviEncAudPara, 0, sizeof(AviEncAudPara_t));
	pAviEncVidPara = &AviEncVidPara;
    gp_memset((INT8S *)pAviEncVidPara, 0, sizeof(AviEncVidPara_t));

    pAviEncPacker0 = &AviEncPacker0;
    gp_memset((INT8S *)pAviEncPacker0, 0, sizeof(AviEncPacker_t));   
    pAviEncPacker1 = &AviEncPacker1;
    gp_memset((INT8S *)pAviEncPacker1, 0, sizeof(AviEncPacker_t));
    
	pAviEncPacker0->file_handle = -1;
	pAviEncPacker0->index_handle = -1;
	pAviEncPacker1->file_handle = -1;
	pAviEncPacker1->index_handle = -1;
	pAviEncVidPara->scaler_zoom_ratio = 1;
}

static void avi_encode_sync_timer_isr(void)
{
	if(pAviEncPara->AviPackerCur->p_avi_wave_info) {
		if((pAviEncPara->tv - pAviEncPara->ta) < pAviEncPara->delta_ta) {
			pAviEncPara->tv += pAviEncPara->tick;
		}	 
	} else {
		pAviEncPara->tv += pAviEncPara->tick;
	}
	
	if((pAviEncPara->tv - pAviEncPara->Tv) > 0) {
		if(pAviEncPara->post_cnt == pAviEncPara->pend_cnt) {
			if(avi_enc_one_frame() >= 0) {
				pAviEncPara->post_cnt++;
			}
		}
	}
}

void avi_encode_video_timer_start(void)
{
	INT32U freq_hz, preload;
	
	//pAviEncPara->ta = 0;
	//pAviEncPara->tv = 0;
	//pAviEncPara->Tv = 0;
	pAviEncPara->pend_cnt = 0;
	pAviEncPara->post_cnt = 0;
#if MIC_INPUT_SRC == C_ADC_LINE_IN 
	if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_C) {
		preload = R_TIMERC_PRELOAD;
	} else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_D) {
		preload = R_TIMERD_PRELOAD;
	} else if(AVI_AUDIO_RECORD_TIMER == ADC_AS_TIMER_E) {
		preload = R_TIMERE_PRELOAD;
	} else {//timerf 
		preload = R_TIMERF_PRELOAD;	
	} 
	
	if(pAviEncPara->AviPackerCur->p_avi_wave_info) {
		INT32U temp;
		
		//temp = 0x10000 -((0x10000 - (R_TIMERE_PRELOAD & 0xFFFF)) * p_vid_dec_para->n);
		temp = (0x10000 - (preload & 0xFFFF)) * pAviEncPara->freq_div;
		freq_hz = MCLK/2/temp;
		if(MCLK %(2*temp)) {
			freq_hz++;
		}
	} else {
		freq_hz = AVI_ENCODE_TIME_BASE;
	}
	
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN || MIC_INPUT_SRC == C_LINE_IN_LR
	freq_hz = preload = AVI_ENCODE_TIME_BASE; 
	
#endif 	

	timer_freq_setup(AVI_ENCODE_VIDEO_TIMER, freq_hz, 0, avi_encode_sync_timer_isr);
}

void avi_encode_video_timer_stop(void)
{
	timer_stop(AVI_ENCODE_VIDEO_TIMER);
}

// file handle
INT32S avi_encode_set_file_handle_and_caculate_free_size(AviEncPacker_t *pAviEncPacker, INT16S FileHandle)
{
	INT16S disk_no;
	INT64U disk_free;
#if AVI_ENCODE_FAST_SWITCH_EN == 1	
	struct stat_t statbuf;
#endif
	
	//SOURCE_TYPE_USER_DEFINE
	if(FileHandle > 100) {
		pAviEncPara->disk_free_size = (INT64U)FileHandle << 48;
   		pAviEncPara->record_total_size = 2*32*1024 + 16; 
		return STATUS_OK;
	}
	
	//create index file handle
    disk_no = GetDiskOfFile(FileHandle);
    if(disk_no == 0) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"A:\\");
    } else if(disk_no == 1) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"B:\\");
    } else if(disk_no == 2) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"C:\\");
    } else if(disk_no == 3) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"D:\\");
    } else if(disk_no == 4) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"E:\\");
    } else if(disk_no == 5) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"F:\\");
    } else if(disk_no == 6) {
    	gp_strcpy((INT8S*)pAviEncPacker->index_path, (INT8S*)"G:\\");
    } else {
    	return STATUS_FAIL;
    }
    
    chdir((CHAR*)pAviEncPacker->index_path);
#if AVI_ENCODE_FAST_SWITCH_EN == 1
    if(stat("index0.tmp", &statbuf) < 0) {
    	gp_strcat((INT8S*)pAviEncPacker->index_path, (INT8S*)"index0.tmp");
    } else { 
    	gp_strcat((INT8S*)pAviEncPacker->index_path, (INT8S*)"index1.tmp");
	}
#else
   	gp_strcat((INT8S*)pAviEncPacker->index_path, (INT8S*)"index0.tmp");
#endif
    
    pAviEncPacker->file_handle = FileHandle;
    pAviEncPacker->index_handle = open((char*)pAviEncPacker->index_path, O_RDWR|O_CREAT|O_TRUNC);
    if(pAviEncPacker->index_handle < 0) {
    	return STATUS_FAIL;
   	}
   	
   	//caculate storage free size 
    disk_free = vfsFreeSpace(disk_no);
    DEBUG_MSG("DISK FREE SIZE = %dMByte\r\n", disk_free/1024/1024);
    if(disk_free < C_MIN_DISK_FREE_SIZE) {
    	avi_encode_close_file(pAviEncPacker);
  		return STATUS_FAIL;
  	}
    	
    pAviEncPara->disk_free_size = disk_free - C_MIN_DISK_FREE_SIZE;
   	pAviEncPara->record_total_size = 2*32*1024 + 16; //avi header + data is 32k align + index header
	return STATUS_OK;
}

INT16S avi_encode_close_file(AviEncPacker_t *pAviEncPacker)
{
	INT32S nRet = 0;
	
#if AVI_PACKER_LIB_EN == 1
	// final version
	nRet = close(pAviEncPacker->file_handle);
	nRet = close(pAviEncPacker->index_handle);
	nRet = unlink2((CHAR*)pAviEncPacker->index_path);
	
	pAviEncPacker->file_handle = -1;
	pAviEncPacker->index_handle = -1;
#if (NAND1_EN || NAND2_EN)
	nRet = GetDiskOfFile(pAviEncPacker->file_handle);	
	if(nRet == FS_NAND1 || nRet == FS_NAND2) {
		DrvNand_flush_allblk(); 
		DEBUG_MSG("Nand_flush_allblk.\r\n");	
	}
	nRet = 0;
#endif
#endif
	pAviEncPacker->file_handle = -1;
	pAviEncPacker->index_handle = -1;
	return nRet;
}

void avi_encode_fail_handle_close_file(AviEncPacker_t *pAviEncPacker)
{	
    close(pAviEncPacker->file_handle);
	close(pAviEncPacker->index_handle);
    unlink((CHAR*)pAviEncPacker->index_path);
	pAviEncPacker->file_handle = -1;
	pAviEncPacker->index_handle = -1;
}

INT32S avi_encode_set_avi_header(AviEncPacker_t *pAviEncPacker)
{
	INT16U sample_per_block, package_size;
	
	pAviEncPacker->p_avi_aud_stream_header = &avi_aud_stream_header;
	pAviEncPacker->p_avi_vid_stream_header = &avi_vid_stream_header;
	pAviEncPacker->p_avi_bitmap_info = &avi_bitmap_info;
	pAviEncPacker->p_avi_wave_info = &avi_wave_info;
	gp_memset((INT8S*)pAviEncPacker->p_avi_aud_stream_header, 0, sizeof(GP_AVI_AVISTREAMHEADER));
	gp_memset((INT8S*)pAviEncPacker->p_avi_vid_stream_header, 0, sizeof(GP_AVI_AVISTREAMHEADER));
	gp_memset((INT8S*)pAviEncPacker->p_avi_bitmap_info, 0, sizeof(GP_AVI_BITMAPINFO));
	gp_memset((INT8S*)pAviEncPacker->p_avi_wave_info, 0, sizeof(GP_AVI_PCMWAVEFORMAT));
	
	//audio
	avi_aud_stream_header.fccType[0] = 'a';
	avi_aud_stream_header.fccType[1] = 'u';
	avi_aud_stream_header.fccType[2] = 'd';
	avi_aud_stream_header.fccType[3] = 's';
	
	switch(pAviEncAudPara->audio_format) 
	{
	case WAV:
		pAviEncPacker->wave_info_cblen = 16;
		avi_aud_stream_header.fccHandler[0] = 1;
		avi_aud_stream_header.fccHandler[1] = 0;
		avi_aud_stream_header.fccHandler[2] = 0;
		avi_aud_stream_header.fccHandler[3] = 0;
			
		avi_wave_info.wFormatTag = WAVE_FORMAT_PCM;
		avi_wave_info.nChannels = pAviEncAudPara->channel_no;	
		avi_wave_info.nSamplesPerSec =  pAviEncAudPara->audio_sample_rate;
		avi_wave_info.nAvgBytesPerSec =  pAviEncAudPara->channel_no * pAviEncAudPara->audio_sample_rate * 2; 
		avi_wave_info.nBlockAlign = pAviEncAudPara->channel_no * 2;
		avi_wave_info.wBitsPerSample = 16; //16bit
			
		avi_aud_stream_header.dwScale = avi_wave_info.nBlockAlign;
		avi_aud_stream_header.dwRate = avi_wave_info.nAvgBytesPerSec;
		avi_aud_stream_header.dwSampleSize = avi_wave_info.nBlockAlign;;	
		break;
		
	case MICROSOFT_ADPCM:
		pAviEncPacker->wave_info_cblen = 50;
		avi_aud_stream_header.fccHandler[0] = 0;
		avi_aud_stream_header.fccHandler[1] = 0;
		avi_aud_stream_header.fccHandler[2] = 0;
		avi_aud_stream_header.fccHandler[3] = 0;

		package_size = 0x100;
		if(pAviEncAudPara->channel_no == 1) {
			sample_per_block = 2 * package_size - 12;
		} else if(pAviEncAudPara->channel_no == 2) {
			sample_per_block = package_size - 12;
		} else {
			sample_per_block = 1;
		} 
		avi_wave_info.wFormatTag = WAVE_FORMAT_ADPCM;
		avi_wave_info.nChannels = pAviEncAudPara->channel_no;	
		avi_wave_info.nSamplesPerSec =  pAviEncAudPara->audio_sample_rate;
		avi_wave_info.nAvgBytesPerSec =  package_size * pAviEncAudPara->audio_sample_rate / sample_per_block; // = PackageSize * FrameSize / fs
		avi_wave_info.nBlockAlign = package_size; //PackageSize
		avi_wave_info.wBitsPerSample = 4; //4bit
		avi_wave_info.cbSize = 32;
		// extension ...
		avi_wave_info.ExtInfo[0] = 0x01F4;	avi_wave_info.ExtInfo[1] = 0x0007;	
		avi_wave_info.ExtInfo[2] = 0x0100;	avi_wave_info.ExtInfo[3] = 0x0000;
		avi_wave_info.ExtInfo[4] = 0x0200;	avi_wave_info.ExtInfo[5] = 0xFF00;
		avi_wave_info.ExtInfo[6] = 0x0000;	avi_wave_info.ExtInfo[7] = 0x0000;
		
		avi_wave_info.ExtInfo[8] =  0x00C0;	avi_wave_info.ExtInfo[9] =  0x0040;
		avi_wave_info.ExtInfo[10] = 0x00F0; avi_wave_info.ExtInfo[11] = 0x0000;
		avi_wave_info.ExtInfo[12] = 0x01CC; avi_wave_info.ExtInfo[13] = 0xFF30;
		avi_wave_info.ExtInfo[14] = 0x0188; avi_wave_info.ExtInfo[15] = 0xFF18;
		break;
		
	case IMA_ADPCM:
		pAviEncPacker->wave_info_cblen = 20;
		avi_aud_stream_header.fccHandler[0] = 0;
		avi_aud_stream_header.fccHandler[1] = 0;
		avi_aud_stream_header.fccHandler[2] = 0;
		avi_aud_stream_header.fccHandler[3] = 0;
		
		package_size = 0x100;
		if(pAviEncAudPara->channel_no == 1) {
			sample_per_block = 2 * package_size - 7;
		} else if(pAviEncAudPara->channel_no == 2) {
			sample_per_block = package_size - 7;
		} else {
			sample_per_block = 1;
		} 
		avi_wave_info.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
		avi_wave_info.nChannels = pAviEncAudPara->channel_no;	
		avi_wave_info.nSamplesPerSec =  pAviEncAudPara->audio_sample_rate;
		avi_wave_info.nAvgBytesPerSec =  package_size * pAviEncAudPara->audio_sample_rate / sample_per_block;
		avi_wave_info.nBlockAlign = package_size;	//PackageSize
		avi_wave_info.wBitsPerSample = 4; //4bit
		avi_wave_info.cbSize = 2;
		// extension ...
		avi_wave_info.ExtInfo[0] = sample_per_block;
		break;
		
	default:
		pAviEncPacker->wave_info_cblen = 0;
		pAviEncPacker->p_avi_aud_stream_header = NULL; 
		pAviEncPacker->p_avi_wave_info = NULL;
	}
	
	avi_aud_stream_header.dwScale = avi_wave_info.nBlockAlign;
	avi_aud_stream_header.dwRate = avi_wave_info.nAvgBytesPerSec;
	avi_aud_stream_header.dwSampleSize = avi_wave_info.nBlockAlign;
	
	//video
	avi_vid_stream_header.fccType[0] = 'v';
	avi_vid_stream_header.fccType[1] = 'i';
	avi_vid_stream_header.fccType[2] = 'd';
	avi_vid_stream_header.fccType[3] = 's';
	avi_vid_stream_header.dwScale = pAviEncVidPara->dwScale;
	avi_vid_stream_header.dwRate = pAviEncVidPara->dwRate;
	avi_vid_stream_header.rcFrame.right = pAviEncVidPara->encode_width;
	avi_vid_stream_header.rcFrame.bottom = pAviEncVidPara->encode_height;
	switch(pAviEncVidPara->video_format)
	{
	case C_MJPG_FORMAT:
		avi_vid_stream_header.fccHandler[0] = 'm';
		avi_vid_stream_header.fccHandler[1] = 'j';
		avi_vid_stream_header.fccHandler[2] = 'p';
		avi_vid_stream_header.fccHandler[3] = 'g';
		
		avi_bitmap_info.biSize = pAviEncPacker->bitmap_info_cblen = 40;
		avi_bitmap_info.biWidth = pAviEncVidPara->encode_width;
		avi_bitmap_info.biHeight = pAviEncVidPara->encode_height;
		avi_bitmap_info.biBitCount = 24;
		avi_bitmap_info.biCompression[0] = 'M';
		avi_bitmap_info.biCompression[1] = 'J';
		avi_bitmap_info.biCompression[2] = 'P';
		avi_bitmap_info.biCompression[3] = 'G';
		avi_bitmap_info.biSizeImage = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height * 3;
		break;
		
	case C_XVID_FORMAT:
		avi_vid_stream_header.fccHandler[0] = 'x';
		avi_vid_stream_header.fccHandler[1] = 'v';
		avi_vid_stream_header.fccHandler[2] = 'i';
		avi_vid_stream_header.fccHandler[3] = 'd';
	
		avi_bitmap_info.biSize = pAviEncPacker->bitmap_info_cblen = 68;
		avi_bitmap_info.biWidth = pAviEncVidPara->encode_width;
		avi_bitmap_info.biHeight = pAviEncVidPara->encode_height;
		avi_bitmap_info.biBitCount = 24;
		avi_bitmap_info.biCompression[0] = 'X';
		avi_bitmap_info.biCompression[1] = 'V';
		avi_bitmap_info.biCompression[2] = 'I';
		avi_bitmap_info.biCompression[3] = 'D';
		avi_bitmap_info.biSizeImage = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height * 3;
		break; 
	} 
	
	return STATUS_OK;
}

void avi_encode_set_curworkmem(void *pAviEncPacker)
{
	 pAviEncPara->AviPackerCur = pAviEncPacker;
}

// status
void avi_encode_set_status(INT32U bit)
{
	pAviEncPara->avi_encode_status |= bit;
}  

void avi_encode_clear_status(INT32U bit)
{
	pAviEncPara->avi_encode_status &= ~bit;
}  

INT32S avi_encode_get_status(void)
{
    return pAviEncPara->avi_encode_status;
}

//memory
INT32U avi_encode_post_empty(OS_EVENT *event, INT32U frame_addr)
{
	INT8U err;
	
	err = OSQPost(event, (void *) frame_addr);
	return err;
}

INT32U avi_encode_get_empty(OS_EVENT *event)
{
	INT8U err;
	INT32U frame_addr;
	
	frame_addr = (INT32U) OSQAccept(event, &err);
	if(err != OS_NO_ERR || frame_addr == 0) {
    	return 0; 
    }	 

	return frame_addr;
}

INT32U avi_encode_get_csi_frame(void)
{
	INT32U addr;
	
	addr =  pAviEncVidPara->csi_frame_addr[g_csi_index++];
	if(g_csi_index >= AVI_ENCODE_CSI_BUFFER_NO) {
		g_csi_index = 0;  
	}
	return addr;
}

static INT32S sensor_mem_alloc(void)
{
	INT32U buffer_addr;
	INT32S i, buffer_size, nRet;
	
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
	buffer_size = pAviEncVidPara->sensor_capture_width * pAviEncVidPara->sensor_capture_height << 1;
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	buffer_size = pAviEncVidPara->sensor_capture_width * SENSOR_FIFO_LINE << 1;
#endif
	
	buffer_addr = (INT32U) gp_malloc_align(buffer_size * AVI_ENCODE_CSI_BUFFER_NO, 32);
	if(buffer_addr == 0) {
		RETURN(STATUS_FAIL);	
	}
	
	for(i=0; i<AVI_ENCODE_CSI_BUFFER_NO; i++) {
		pAviEncVidPara->csi_frame_addr[i] = buffer_addr + buffer_size * i;
		DEBUG_MSG("sensor_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->csi_frame_addr[i]);
	}	
	nRet = STATUS_OK;	
Return:
	return nRet;
}

static INT32S scaler_mem_alloc(void)
{
	INT32U buffer_addr;
	INT32S i, buffer_size, nRet;
	
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
	if((AVI_ENCODE_DIGITAL_ZOOM_EN == 1) || pAviEncVidPara->scaler_flag) {
		buffer_size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height << 1;
		buffer_addr = (INT32U) gp_malloc_align(buffer_size * AVI_ENCODE_SCALER_BUFFER_NO, 32);
		if(buffer_addr == 0) {
			RETURN(STATUS_FAIL);	
		}
		
		for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++) {
			pAviEncVidPara->scaler_output_addr[i] = (INT32U)buffer_addr + buffer_size * i;
			DEBUG_MSG("scaler_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->scaler_output_addr[i]);
		}
	}
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	if(pAviEncVidPara->scaler_flag) {
		nRet = pAviEncVidPara->encode_height /(pAviEncVidPara->sensor_capture_height / SENSOR_FIFO_LINE);
		nRet += 2; //+2 is to fix block line
		buffer_size = pAviEncVidPara->encode_width * nRet << 1; 
		buffer_addr = (INT32U) gp_malloc_align(buffer_size * AVI_ENCODE_SCALER_BUFFER_NO, 32);
		if(buffer_addr == 0) {
			RETURN(STATUS_FAIL);
		}
		
		for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++) {
			pAviEncVidPara->scaler_output_addr[i] = buffer_addr + buffer_size * i;
			DEBUG_MSG("scaler_frame_addr[%d] = 0x%x\r\n", i, pAviEncVidPara->scaler_output_addr[i]);
		}
	}
#endif
	nRet = STATUS_OK;
Return:
	return nRet;
}

static INT32S display_mem_alloc(void)
{
#if AVI_ENCODE_PREVIEW_DISPLAY_EN == 1
	INT32U buffer_addr;
	INT32S i, buffer_size, nRet;
	
	if(pAviEncVidPara->dispaly_scaler_flag) {
		buffer_size = pAviEncVidPara->display_buffer_width * pAviEncVidPara->display_buffer_height << 1;
		
		buffer_addr = (INT32U) gp_malloc_align(buffer_size * AVI_ENCODE_DISPALY_BUFFER_NO, 32);
		if(buffer_addr == 0) {
			RETURN(STATUS_FAIL);
		}
		
		for(i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
			pAviEncVidPara->display_output_addr[i] = buffer_addr + buffer_size * i;
			DEBUG_MSG("display_frame_addr[%d]= 0x%x\r\n", i, pAviEncVidPara->display_output_addr[i]);
		}
	}	

	nRet = STATUS_OK;
Return:
	return nRet;
#else
	return STATUS_OK;
#endif
}

static INT32S video_mem_alloc(void)
{
#if AVI_ENCODE_VIDEO_ENCODE_EN == 1
	INT32U buffer_addr;
	INT32S i, buffer_size, nRet;
	
	buffer_size = pAviEncVidPara->encode_width * pAviEncVidPara->encode_height;
	buffer_addr = (INT32U) gp_malloc_align(buffer_size * AVI_ENCODE_VIDEO_BUFFER_NO, 32);	
	if(buffer_addr == 0) {
		RETURN(STATUS_FAIL);
	}
	
	for(i=0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++) {
		pAviEncVidPara->video_encode_addr[i] = buffer_addr + buffer_size * i;
		DEBUG_MSG("jpeg_frame_addr[%d]   = 0x%x\r\n", i, pAviEncVidPara->video_encode_addr[i]);
	}	
	
	nRet = STATUS_OK;
Return:
	return nRet;
#else
	return STATUS_OK;
#endif
} 

static INT32S AviPacker_mem_alloc(AviEncPacker_t *pAviEncPacker)
{
#if AVI_PACKER_LIB_EN == 1
	INT32S nRet;
	INT32U buffer_addr, buffer_size;
	
	pAviEncPacker->file_buffer_size = FileWriteBuffer_Size;
	pAviEncPacker->index_buffer_size = IndexBuffer_Size;
	buffer_size = AviPackerV3_GetWorkMemSize() + FileWriteBuffer_Size + IndexBuffer_Size;
	buffer_addr = (INT32U)gp_malloc_align(buffer_size, 32);
	if(buffer_addr == 0) {
		 RETURN(STATUS_FAIL);
	}
	
	pAviEncPacker->file_write_buffer = (void *)buffer_addr;
	pAviEncPacker->index_write_buffer = (void *)(buffer_addr + FileWriteBuffer_Size);
	pAviEncPacker->avi_workmem = (void *)(buffer_addr + FileWriteBuffer_Size + IndexBuffer_Size);

	DEBUG_MSG("file_write_buffer = 0x%x\r\n", pAviEncPacker->file_write_buffer);
	DEBUG_MSG("index_write_buffer= 0x%x\r\n", pAviEncPacker->index_write_buffer);
	DEBUG_MSG("avi_workmem_buffer= 0x%x\r\n", pAviEncPacker->avi_workmem);

	nRet = STATUS_OK;
Return:
	return nRet;
#else
	pAviEncPacker->file_write_buffer = 0;
	return STATUS_OK;
#endif
} 

static void sensor_mem_free(void) 
{
	INT32S i;
	
	if(pAviEncVidPara->csi_frame_addr[0]) {
		gp_free((void*)pAviEncVidPara->csi_frame_addr[0]);
	}
	
	for(i=0; i<AVI_ENCODE_CSI_BUFFER_NO; i++) {
		pAviEncVidPara->csi_frame_addr[i] = 0;
	}
}

static void scaler_mem_free(void) 
{
	INT32S i;
	
	if(pAviEncVidPara->scaler_output_addr[0]) {
		gp_free((void*)pAviEncVidPara->scaler_output_addr[0]);
	}
	
	for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++) {
		pAviEncVidPara->scaler_output_addr[i] = 0;
	}
}

static void display_mem_free(void) 
{
	INT32S i;
	
	if(pAviEncVidPara->display_output_addr[0]) {
		gp_free((void*)pAviEncVidPara->display_output_addr[0]);	
	}
	
	for(i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
		pAviEncVidPara->display_output_addr[i] = 0;
	}
}

static void video_mem_free(void) 
{
	INT32S i;
	
	if(pAviEncVidPara->video_encode_addr[0]) {
		gp_free((void*)pAviEncVidPara->video_encode_addr[0]);
	}
	
	for(i=0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++) {
		pAviEncVidPara->video_encode_addr[i] = 0;
	}
}

static void AviPacker_mem_free(AviEncPacker_t *pAviEncPacker)
{
	
	if(pAviEncPacker->file_write_buffer) {
		gp_free((void*)pAviEncPacker->file_write_buffer);
	}
			
	pAviEncPacker->file_write_buffer = 0;
	pAviEncPacker->index_write_buffer = 0;
	pAviEncPacker->avi_workmem = 0;
}

INT32S avi_encode_memory_alloc(void)
{
	INT32S nRet;
	
	//inti index
	g_csi_index = 0;
	g_pcm_index = 0;
	if(sensor_mem_alloc() < 0) {
		RETURN(STATUS_FAIL);
	}
	
	if(scaler_mem_alloc() < 0) {
		RETURN(STATUS_FAIL); 
	}
	
	if(display_mem_alloc() < 0) {
		RETURN(STATUS_FAIL);
	}
	     
	if(video_mem_alloc() < 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S avi_packer_memory_alloc(void)
{
	INT32S nRet;
	
	if(AviPacker_mem_alloc(pAviEncPacker0) < 0) {
		RETURN(STATUS_FAIL);
	}
#if AVI_ENCODE_FAST_SWITCH_EN == 1	
	if(AviPacker_mem_alloc(pAviEncPacker1) < 0) {
		RETURN(STATUS_FAIL);
	}
#endif
	nRet = STATUS_OK;
Return:	
	return nRet;
}

void avi_encode_memory_free(void)
{
	sensor_mem_free();
	scaler_mem_free();
	display_mem_free();
	video_mem_free();
}

void avi_packer_memory_free(void)
{
	AviPacker_mem_free(pAviEncPacker0);
#if AVI_ENCODE_FAST_SWITCH_EN == 1		
	AviPacker_mem_free(pAviEncPacker1);
#endif
}

// format
void avi_encode_set_sensor_format(INT32U format)
{
	switch(format)
	{
	case V4L2_PIX_FMT_YUYV:
		//pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_VYUY;
		pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_UYVY;
		break;
	case V4L2_PIX_FMT_YVYU:
		pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_UYVY;
		break;	
	case V4L2_PIX_FMT_UYVY:
		pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_UYVY;
		break;
	case V4L2_PIX_FMT_VYUY:
		pAviEncVidPara->sensor_output_format = C_SCALER_CTRL_IN_YUYV;
		break;	
	}
} 

void avi_encode_set_display_format(INT32U format)
{
	if(format == IMAGE_OUTPUT_FORMAT_RGB565) {
		pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_RGB565;
	} else if(format == IMAGE_OUTPUT_FORMAT_YUYV) {
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_YUYV;
    } else if(format == IMAGE_OUTPUT_FORMAT_UYVY) {
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_UYVY;	
    } else {
    	pAviEncVidPara->display_output_format = C_SCALER_CTRL_OUT_YUYV;	
	} 
}

void avi_encode_set_display_scaler(void)
{
	if((pAviEncVidPara->encode_width != pAviEncVidPara->display_buffer_width)||
		(pAviEncVidPara->encode_height != pAviEncVidPara->display_buffer_height) ||
		(pAviEncVidPara->display_width != pAviEncVidPara->display_buffer_width) ||
		(pAviEncVidPara->display_height != pAviEncVidPara->display_buffer_height) || 
		(pAviEncVidPara->display_output_format != C_SCALER_CTRL_OUT_YUYV)) {
		pAviEncVidPara->dispaly_scaler_flag = 1;
	 } else {
		pAviEncVidPara->dispaly_scaler_flag = 0;
	}
			
	if((pAviEncVidPara->sensor_capture_width != pAviEncVidPara->encode_width) || 
		(pAviEncVidPara->sensor_capture_height != pAviEncVidPara->encode_height)) {
		pAviEncVidPara->scaler_flag = 1;
	} else {
		pAviEncVidPara->scaler_flag = 0;
	}
}

INT32S avi_encode_set_jpeg_quality(INT8U quality_value)
{
	INT32U i, video_frame;
	
	pAviEncVidPara->quality_value = jpeg_header_generate(JPEG_IMG_FORMAT_422, quality_value, 
														pAviEncVidPara->encode_width, 
														pAviEncVidPara->encode_height);
	//copy to video buffer
	for(i = 0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++) {	
		video_frame = pAviEncVidPara->video_encode_addr[i];
		gp_memcpy((INT8S*)video_frame, (INT8S*)jpeg_header_get_addr(), jpeg_header_get_size());
	}
	
	return jpeg_header_get_size();
}

// check disk free size
BOOLEAN avi_encode_disk_size_is_enough(INT32S cb_write_size)
{
#if AVI_ENCODE_CAL_DISK_SIZE_EN	== 1
	INT32U temp;
	INT32S nRet;
	INT64U disk_free_size;
	
	temp = pAviEncPara->record_total_size;
	disk_free_size = pAviEncPara->disk_free_size;
	temp += cb_write_size;
	if(temp >= AVI_FILE_MAX_RECORD_SIZE) {
		RETURN(FALSE);
	}
	
	if(temp >= disk_free_size) {
		RETURN(FALSE);
	}
	pAviEncPara->record_total_size = temp;
#endif

	nRet = TRUE;
Return:
	return nRet;
}

// csi frame mode switch buffer
void csi_eof_isr(INT32U event)
{
	if(event == CSI_SENSOR_FRAME_END_EVENT) {
		INT8U err;
		INT32U frame, ready_frame;
	
		frame = (INT32U) OSQAccept(cmos_frame_q, &err);
		if((err != OS_NO_ERR) || (frame == 0)) {
			DEBUG_MSG("L");	
			return;
		}
		
		ready_frame = *P_CSI_TG_FBSADDR;
		*P_CSI_TG_FBSADDR = frame;
		OSQPost(scaler_task_q, (void *)ready_frame);
	}
	
	if(event == CSI_SENSOR_FIFO_OVERFLOW_EVENT) {
		DEBUG_MSG("UnderRun\r\n");
	}
}

void csi_capture_isr(INT32U event)
{
	if(event == CSI_SENSOR_FRAME_END_EVENT) {
		INT32U frame;
	
		pAviEncPara->skip_cnt--;
		if(pAviEncPara->skip_cnt > 0) {
			return;
		}
		
		R_CSI_TG_CTRL0 &= ~0x01;
		frame = *P_CSI_TG_FBSADDR;
		OSMboxPost(pCap->Sensor_m, (void *)frame);
	}
	
	if(event == CSI_SENSOR_FIFO_OVERFLOW_EVENT) {
		DEBUG_MSG("UnderRun\r\n");
	}
}

// csi fifo mode switch buffer
static void csi_fifo_end(void)
{
	INT8U err;
	INT32U ready_frame;
	
	//R_IOC_O_DATA ^= 0x04;
	pAviEncPara->fifo_irq_cnt++;
	if((pAviEncPara->fifo_irq_cnt - pAviEncPara->vid_pend_cnt) >= 2) {
		pAviEncPara->fifo_enc_err_flag = 1;
		return;
	}

	if(((pAviEncPara->avi_encode_status & C_AVI_ENCODE_PRE_START) == 0) || pAviEncPara->fifo_enc_err_flag) {
		return;
	}

	ready_frame = pAviEncVidPara->csi_fifo_addr[pAviEncPara->fifo_irq_cnt - 1];
	if(pAviEncPara->fifo_irq_cnt >= pAviEncPara->vid_post_cnt) {
		ready_frame |= C_AVI_ENCODE_FRAME_END;
	}
	
	err = OSQPost(vid_enc_task_q, (void *)ready_frame);	
	if(err != OS_NO_ERR) {
		DEBUG_MSG("L");
		pAviEncPara->fifo_enc_err_flag = 1;
		return;
	}
}

// csi fifo mode frame end, take time: post 30 times 80us
static void csi_fifo_frame_end(void)
{
	INT8U i;
	
	if(pAviEncPara->fifo_irq_cnt != pAviEncPara->vid_post_cnt) {
		DEBUG_MSG("[%x]\r\n", pAviEncPara->fifo_irq_cnt);
		pAviEncPara->fifo_enc_err_flag = 1;
	}
			
	if(pAviEncPara->vid_pend_cnt && pAviEncPara->fifo_enc_err_flag) {
		OSQFlush(vid_enc_task_q);
		OSQPost(vid_enc_task_q, (void *)C_AVI_ENCODE_FIFO_ERR);
	}

	pAviEncPara->fifo_enc_err_flag = 0;
	pAviEncPara->fifo_irq_cnt = 0;
	for(i = 0; i<pAviEncPara->vid_post_cnt; i++) {
		pAviEncVidPara->csi_fifo_addr[i] = avi_encode_get_csi_frame();		
	}
}

void csi_fifo_isr(INT32U event)
{
	if(event == CSI_SENSOR_OUTPUT_FIFO_EVENT) {
		csi_fifo_end();
	}
	
	if(event == CSI_SENSOR_FRAME_END_EVENT) {
		csi_fifo_frame_end();
	}
	
	if(event == CSI_SENSOR_FIFO_OVERFLOW_EVENT) {
		DEBUG_MSG("UnderRun\r\n");
	}
}

void cdsp_eof_isr(void)
{
	INT8U err;
	INT32U frame, ready_frame;

#if C_DMA_CH == 0	
	pAviEncPara->abBufFlag ^= 0x01;
#elif C_DMA_CH == 1
	pAviEncPara->abBufFlag = 1;
#elif C_DMA_CH == 2
	pAviEncPara->abBufFlag = 0;
#endif
	
	frame = (INT32U) OSQAccept(cmos_frame_q, &err);
	if((err != OS_NO_ERR) || (frame == 0)) {
    	DEBUG_MSG("L");	
    	return;
	}  

    if(pAviEncPara->abBufFlag) {
    	ready_frame = R_CDSP_DMA_YUVABUF_SADDR;
    	R_CDSP_DMA_YUVABUF_SADDR = frame;
	} else {
    	ready_frame = R_CDSP_DMA_YUVBBUF_SADDR;
    	R_CDSP_DMA_YUVBBUF_SADDR = frame;
	}
    
    OSQPost(scaler_task_q, (void *)ready_frame);
}

void cdsp_capture_isr(void)
{
	INT32U ready_frame;

#if C_DMA_CH == 0	
	pAviEncPara->abBufFlag ^= 0x01;
#elif C_DMA_CH == 1
	pAviEncPara->abBufFlag = 1;
#elif C_DMA_CH == 2
	pAviEncPara->abBufFlag = 0;
#endif
	
	pAviEncPara->skip_cnt--;
	if(pAviEncPara->skip_cnt > 0) {
		return;
	}
	
	if(pAviEncPara->abBufFlag) {
		ready_frame = R_CDSP_DMA_YUVABUF_SADDR;
	} else {
		ready_frame = R_CDSP_DMA_YUVBBUF_SADDR;
	}
	
	OSMboxPost(pCap->Sensor_m, (void *)ready_frame);	
}

void cdsp_fifo_isr(void)
{
	

}

// jpeg once encode
INT32S jpeg_encode_once(JpegPara_t *pJpegEnc)
{
	INT32S nRet;
	
	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(pJpegEnc->quality_value);	// Load default qunatization table(quality)
	gplib_jpeg_default_huffman_table_load();								// Load default huffman table 

	nRet = jpeg_encode_input_size_set(pJpegEnc->width, pJpegEnc->height);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	jpeg_encode_input_format_set(pJpegEnc->input_format);					// C_JPEG_FORMAT_YUYV / C_JPEG_FORMAT_YUV_SEPARATE
	nRet = jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = jpeg_encode_output_addr_set((INT32U) pJpegEnc->output_buffer);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = jpeg_encode_once_start(pJpegEnc->input_buffer_y, pJpegEnc->input_buffer_u, pJpegEnc->input_buffer_v);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	while(1) {
		pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			// Get encode length
			nRet = jpeg_encode_vlc_cnt_get();
			cache_invalid_range(pJpegEnc->output_buffer, nRet);
			break;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODING) {
			continue;
		} else {
			DEBUG_MSG("JPEG encode error!\r\n");
			RETURN(STATUS_FAIL);
		}
	}
	
Return:	
	jpeg_encode_stop();	
	return nRet;
}

// jpeg fifo encode
INT32S jpeg_encode_fifo_start(JpegPara_t *pJpegEnc)
{
	INT32S nRet;
	
	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(pJpegEnc->quality_value);		// Load default qunatization table(quality=50)
	gplib_jpeg_default_huffman_table_load();			        // Load default huffman table 
	nRet = jpeg_encode_input_size_set(pJpegEnc->width, pJpegEnc->height);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	jpeg_encode_input_format_set(pJpegEnc->input_format);
	nRet = jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = jpeg_encode_output_addr_set((INT32U)pJpegEnc->output_buffer);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}

	nRet = jpeg_encode_on_the_fly_start(pJpegEnc->input_buffer_y, 
										pJpegEnc->input_buffer_u, 
										pJpegEnc->input_buffer_v, 
										(pJpegEnc->input_y_len + pJpegEnc->input_uv_len + pJpegEnc->input_uv_len) << 1);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	if(pJpegEnc->wait_done == 0) {
		RETURN(STATUS_OK);
	}
	
	nRet = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done 
	if(nRet & (C_JPG_STATUS_TIMEOUT | C_JPG_STATUS_INIT_ERR |C_JPG_STATUS_RST_MARKER_ERR)) {
		nRet = STATUS_FAIL;
	}
Return:	
	return nRet;
}

INT32S jpeg_encode_fifo_once(JpegPara_t *pJpegEnc)
{
	INT32S nRet;

	while(1) {
		if(pJpegEnc->wait_done == 0) {
			pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);
		}
			 
	   	if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
	    	RETURN(C_JPG_STATUS_ENCODE_DONE);
	    } else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
   	    	nRet = jpeg_encode_on_the_fly_start(pJpegEnc->input_buffer_y, 
	    										pJpegEnc->input_buffer_u, 
	    										pJpegEnc->input_buffer_v, 
	    										(pJpegEnc->input_y_len + pJpegEnc->input_uv_len + pJpegEnc->input_uv_len) << 1);
	    	if(nRet < 0) {
	    		RETURN(nRet);
	    	}
	    	
	    	if(pJpegEnc->wait_done == 0) {
	    		RETURN(STATUS_OK);
	    	}
	    	
	    	pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done 
	    	RETURN(pJpegEnc->jpeg_status);
	    } else if(pJpegEnc->jpeg_status & C_JPG_STATUS_STOP) {
	        DEBUG_MSG("\r\njpeg encode is not started!\r\n");
	    	RETURN(C_JPG_STATUS_STOP);
	    } else if(pJpegEnc->jpeg_status & C_JPG_STATUS_TIMEOUT) {
	        DEBUG_MSG("\r\njpeg encode execution timeout!\r\n");
	        RETURN(C_JPG_STATUS_TIMEOUT);
	    } else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INIT_ERR) {
	         DEBUG_MSG("\r\njpeg encode init error!\r\n");
	         RETURN(C_JPG_STATUS_INIT_ERR);
	    } else {
	    	DEBUG_MSG("\r\nJPEG status error = 0x%x!\r\n", pJpegEnc->jpeg_status);
	        RETURN(STATUS_FAIL);
	    }
    }

Return:	
	return nRet;
}

INT32S jpeg_encode_fifo_stop(JpegPara_t *pJpegEnc)
{
	INT32S nRet;
	
	if(pJpegEnc->wait_done == 0) {
		pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);
	}
			
	while(1) {	
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			nRet = jpeg_encode_vlc_cnt_get();	// get jpeg encode size
			RETURN(nRet);
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			nRet = jpeg_encode_on_the_fly_start(pJpegEnc->input_buffer_y, 
												pJpegEnc->input_buffer_u, 
												pJpegEnc->input_buffer_v, 
												(pJpegEnc->input_y_len + pJpegEnc->input_uv_len + pJpegEnc->input_uv_len)<<1);
			if(nRet < 0) {
				RETURN(STATUS_FAIL);
			}
			pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done 
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_STOP) {
	        DEBUG_MSG("\r\njpeg encode is not started!\r\n");
	    	RETURN(STATUS_FAIL);
	    } else if(pJpegEnc->jpeg_status & C_JPG_STATUS_TIMEOUT) {
	        DEBUG_MSG("\r\njpeg encode execution timeout!\r\n");
	        RETURN(STATUS_FAIL);
	    } else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INIT_ERR) {
	         DEBUG_MSG("\r\njpeg encode init error!\r\n");
	         RETURN(STATUS_FAIL);
	    } else {
	    	DEBUG_MSG("\r\nJPEG status error = 0x%x!\r\n", pJpegEnc->jpeg_status);
	        RETURN(STATUS_FAIL);
	    }
	}
	
Return:
	jpeg_encode_stop();	
	return nRet;
}

INT32S save_jpeg_file(INT16S fd, INT32U encode_frame, INT32U encode_size)
{
	INT32S nRet;
	
	if(pAviEncPara->source_type == SOURCE_TYPE_FS) {	
		nRet = write(fd, encode_frame, encode_size);
		if(nRet != encode_size) {
			RETURN(STATUS_FAIL);
		}
			
		nRet = close(fd);
		if(nRet < 0) {
			RETURN(STATUS_FAIL);
		}
		
		fd = -1;
	} else {
		if(pAviEncPara->memptr == 0) {
			RETURN(STATUS_FAIL);
		}
		
		gp_memcpy((INT8S *)pAviEncPara->memptr, (INT8S *)encode_frame, encode_size);	
	}
	
	nRet = STATUS_OK;	
Return:
	return nRet;
}

// audio function
INT32S avi_audio_memory_allocate(INT32U	cblen)
{
	INT16U *ptr;
	INT32S i, j, size, nRet;

	for(i=0; i<AVI_ENCODE_PCM_BUFFER_NO; i++) {
		pAviEncAudPara->pcm_input_addr[i] = (INT32U) gp_malloc_align(cblen, 4);
		if(!pAviEncAudPara->pcm_input_addr[i]) {
			RETURN(STATUS_FAIL);
		}
		
		ptr = (INT16U*)pAviEncAudPara->pcm_input_addr[i];
		for(j=0; j<(cblen/2); j++) {
			*ptr++ = 0x8000;
		}
	}
	
	size = pAviEncAudPara->pack_size * C_WAVE_ENCODE_TIMES;
	pAviEncAudPara->pack_buffer_addr = (INT32U) gp_malloc_align(size, 4);
	if(!pAviEncAudPara->pack_buffer_addr) {
		RETURN(STATUS_FAIL);
	}
	nRet = STATUS_OK;
Return:	
	return nRet;
}

void avi_audio_memory_free(void)
{
	INT32U i;
	
	for(i=0; i<AVI_ENCODE_PCM_BUFFER_NO; i++) {
		gp_free((void *)pAviEncAudPara->pcm_input_addr[i]);
		pAviEncAudPara->pcm_input_addr[i] = 0;
	}
	
	gp_free((void *)pAviEncAudPara->pack_buffer_addr);
	pAviEncAudPara->pack_buffer_addr = 0;
}

INT32U avi_audio_get_next_buffer(void)
{
	INT32U addr;
	
	addr = pAviEncAudPara->pcm_input_addr[g_pcm_index++];
	if(g_pcm_index >= AVI_ENCODE_PCM_BUFFER_NO) {
		g_pcm_index = 0;
	}
	return addr;
}

void avi_adc_hw_start(INT32U sample_rate)
{
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	drv_l1_adc_fifo_clear();
	drv_l1_adc_user_line_in_en(AVI_AUDIO_RECORD_ADC_CH, ENABLE);
	drv_l1_adc_auto_ch_set(AVI_AUDIO_RECORD_ADC_CH);
	drv_l1_adc_fifo_level_set(4);
	drv_l1_adc_auto_sample_start();	
	drv_l1_adc_sample_rate_set(AVI_AUDIO_RECORD_TIMER, sample_rate);

#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN
	drv_l1_i2s_rx_set_input_path(MIC_INPUT);
	drv_l1_i2s_rx_init();
	drv_l1_i2s_rx_sample_rate_set(sample_rate);
	drv_l1_i2s_rx_mono_ch_set();
	drv_l1_i2s_rx_start();
	
#elif MIC_INPUT_SRC == C_LINE_IN_LR	
	drv_l1_i2s_rx_set_input_path(LINE_IN_LR); 
	drv_l1_i2s_rx_init();
	drv_l1_i2s_rx_sample_rate_set(sample_rate);
	drv_l1_i2s_rx_start();	
#endif
}

void avi_adc_hw_stop(void)
{
#if MIC_INPUT_SRC == C_ADC_LINE_IN
	drv_l1_adc_auto_sample_stop();
#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN || MIC_INPUT_SRC == C_LINE_IN_LR
	drv_l1_i2s_rx_stop();
	drv_l1_i2s_rx_exit();
#endif	
}

#if	AVI_ENCODE_SHOW_TIME == 1
const INT8U *number[] = {	
	acFontHZArial01700030, 
	acFontHZArial01700031, 
	acFontHZArial01700032,
	acFontHZArial01700033, 
	acFontHZArial01700034,
	acFontHZArial01700035,
	acFontHZArial01700036,
	acFontHZArial01700037, 
	acFontHZArial01700038,
	acFontHZArial01700039 
};

//  Draw OSD function
void cpu_draw_osd(const INT8U *source_addr, INT32U target_addr, INT16U offset, INT16U res)
{	
	const INT8U* chptr;
	INT8U i,j,tmp;
	INT8U *ptr;
	
	ptr = (INT8U*) target_addr;
	ptr+= offset;
	chptr = source_addr;
	for(i=0; i<19; i++) {
		tmp = *chptr++;
		for(j=0; j<8; j++) {
			if(tmp & 0x80) {	
				*ptr++ =0x80;
				*ptr++ = 0xff;
			} else {
				ptr += 2;
			}	
			tmp = tmp<<1;
		}
 		ptr += (res-8)*2;
 	}
} 

void cpu_draw_time_osd(TIME_T current_time, INT32U target_buffer, INT16U resolution)
{
	INT8U  data;
	INT16U offset, space, wtemp;
	INT32U line;
	
	if(resolution == 320) {
		line = target_buffer + 220*resolution*2;//QVGA
		offset = 160*2;
	} else {
		line = target_buffer + 440*resolution*2;//VGA	
		offset = 480*2;
	}
	space = 16;
	//Arial 17
	//year
	if (current_time.tm_year > 2008) {
		wtemp = current_time.tm_year - 2000;
		cpu_draw_osd(number[2], line, offset, resolution);
		cpu_draw_osd(number[0],line,offset+space*1,resolution);
		data = wtemp/10;
		wtemp -= data*10;
		cpu_draw_osd(number[data],line,offset+space*2,resolution);
		data = wtemp;
		cpu_draw_osd(number[data],line,offset+space*3,resolution);
	} else {
		cpu_draw_osd(number[2], line, offset, resolution);
		cpu_draw_osd(number[0],line,offset+space*1,resolution);
		cpu_draw_osd(number[0],line,offset+space*2,resolution);
		cpu_draw_osd(number[8],line,offset+space*3,resolution);
	}
	
	// :
	cpu_draw_osd(acFontHZArial017Slash,line,offset+space*4,resolution);
	
	//month
	wtemp = current_time.tm_mon; 
	cpu_draw_osd(number[wtemp/10],line,offset+space*5,resolution);
	cpu_draw_osd(number[wtemp%10],line,offset+space*6,resolution);
	
	//:
	cpu_draw_osd(acFontHZArial017Slash,line,offset+space*7,resolution);
	
	//day
	wtemp = current_time.tm_mday;
	cpu_draw_osd(number[wtemp/10],line,offset+space*8,resolution);
	cpu_draw_osd(number[wtemp%10],line,offset+space*9,resolution);
	
	//hour
	wtemp = current_time.tm_hour;
	cpu_draw_osd(number[wtemp/10],line,offset+space*11,resolution);
	cpu_draw_osd(number[wtemp%10],line,offset+space*12,resolution);
	
	// :
	cpu_draw_osd(acFontHZArial017Comma, line,offset+space*13,resolution);
	
	//minute
	wtemp = current_time.tm_min;
	cpu_draw_osd(number[wtemp/10],line,offset+space*14,resolution);
	cpu_draw_osd(number[wtemp%10],line,offset+space*15,resolution);
	
	// :
	cpu_draw_osd(acFontHZArial017Comma,line,offset+space*16,resolution);
	
	//second
	wtemp = current_time.tm_sec;
	cpu_draw_osd(number[wtemp/10], line,offset+space*17,resolution);
	cpu_draw_osd(number[wtemp%10],line,offset+space*18,resolution);	
	
	//drain cache
	if(resolution == 320) {
		cache_drain_range((target_buffer + (resolution*440*2)), resolution*(480-440)*2);
	} else {
		cache_drain_range((target_buffer + (resolution*220*2)), resolution*(240-220)*2);
	}
}
#if 0
extern INT16U ad_value;
extern INT16U ad_18_value;
void cpu_draw_advalue_osd(INT16U value, INT32U target_buffer, INT16U resolution, INT16U aa)
{
	INT8U  data;
	INT16U offset, space, wtemp;
	INT32U line;
	if(resolution == 320){
		line = target_buffer + 220*resolution*2;//QVGA
		offset = 250*2;
	} else {
		line = target_buffer + aa*resolution*2;//VGA	
		offset = 480*2;
	}
	space = 16;
	//Arial 17
	//year
	wtemp = value; 
	data = wtemp/10000;
	wtemp -= data*10000;
	cpu_draw_osd(number[data], line, offset, resolution);
	data = wtemp/1000;
	wtemp -= data*1000;
	cpu_draw_osd(number[data],line,offset+space*1,resolution);
	data = wtemp/100;
	wtemp -= data*100;
	cpu_draw_osd(number[data],line,offset+space*2,resolution);
	data = wtemp/10;
	wtemp -= data*10;
	cpu_draw_osd(number[data],line,offset+space*3,resolution);
	data = wtemp;
	cpu_draw_osd(number[data],line,offset+space*4,resolution);
}
#endif
#endif
