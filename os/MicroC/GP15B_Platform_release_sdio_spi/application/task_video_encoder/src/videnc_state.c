#include <stdio.h>
#include "define.h"
#include "videnc_state.h"
#include "videnc_api.h"
#include "drv_l1_scaler.h"
#include "drv_l1_wrap.h"
#include "drv_l2_nand_ext.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define FILE_WRITE_BUF_SIZE		(512*1024)		//avi packer file buffer
#define INDEX_WRITE_BUF_SIZE	(64*1024)		//avi packer index buffer

#define AVI_FILE_MAX_SIZE		2000000000		// 2GB
#define DISK_MIN_FREE_SIZE		512*1024		// 512K

#define FRAME_MODE_EN			0				// 0: conv422 fifo mode, 1: conv422 frame mode

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MSG_VIDENC_PREVIEW_START = 0x100,
	MSG_VIDENC_PREVIEW_STOP,
	MSG_VIDENC_ENCODE_START,
	MSG_VIDENC_ENCODE_STOP,
	MSG_VIDENC_CAPTURE,
	MSG_VIDENC_ONE_FRAME,
	MSG_VIDENC_STORAGE_FULL,
	MSG_VIDENC_STORAGE_ERR,
	MSG_VIDENC_EXIT,
	MSG_VIDENC_CAPTURE_UP	
} MSG_VIDENC_STATE;

typedef enum
{
	MSG_DISPLAY_ONCE = 0x200,
	MSG_DISPLAY_EXIT	
} MSG_DISP;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void videnc_state_entry(void *p_arg);
static void videnc_disp_task_entry(void *p_arg);

extern INT32S videnc_aud_task_start(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_aud_task_stop(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_vid_task_exit(VidEncCtrl_t *pVidEnc);
extern void videnc_vid_task_entry(void *p_arg);

extern INT32S videnc_vid_task_start(VidEncCtrl_t *pVidEnc, INT32U Qvalue);
extern INT32S videnc_vid_task_stop(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_aud_task_exit(VidEncCtrl_t *pVidEnc);
extern void videnc_aud_task_entry(void *p_arg);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U (*display_callback)(INT16U disp_w, INT16U disp_h, INT32U disp_buf);
INT32S (*packer_put_data)(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);


/**
 * @brief   open video encode with wrap path
 * @param   none
 * @return  work memory
 */
void *videnc_open(void)
{
	INT8U err;
	VidEncCtrl_t *pVidEnc;

	pVidEnc = (VidEncCtrl_t *)gp_malloc_align(sizeof(VidEncCtrl_t), 4);
	if(pVidEnc == 0) {
		return 0;
	}
	
	MSG("videnc_workmem = 0x%x\r\n", pVidEnc);
	gp_memset((INT8S *)pVidEnc, 0x00, sizeof(VidEncCtrl_t));
	pVidEnc->state = STATE_STOP;
	
	// video encode state
	pVidEnc->VidEncQ = OSQCreate(pVidEnc->videnc_q_buf, VIDENC_QUEUE_MAX);
	if(pVidEnc->VidEncQ == 0) {
		goto __fail;
	}

	pVidEnc->VidEncM = OSMboxCreate(NULL);
	if(pVidEnc->VidEncM == 0) {
		goto __fail;
	}
	
	// display task
	pVidEnc->DispQ = OSQCreate(pVidEnc->disp_q_buf, VIDENC_QUEUE_MAX);
	if(pVidEnc->DispQ == 0) {
		goto __fail;
	}
	
	pVidEnc->DispM = OSMboxCreate(NULL);
	if(pVidEnc->DispM == 0) {
		goto __fail;
	}
	
	// video encode
	pVidEnc->VideoQ = OSQCreate(pVidEnc->video_q_buf, VIDENC_QUEUE_MAX);
	if(pVidEnc->VideoQ == 0) {
		goto __fail;
	}

	pVidEnc->VideoM = OSMboxCreate(NULL);
	if(pVidEnc->VideoM == 0) {
		goto __fail;
	}	
	
	// audio encode
	pVidEnc->AudioQ = OSQCreate(pVidEnc->audio_q_buf, VIDENC_QUEUE_MAX);
	if(pVidEnc->AudioQ == 0) {
		goto __fail;
	}

	pVidEnc->AudioM = OSMboxCreate(NULL);
	if(pVidEnc->AudioM == 0) {
		goto __fail;
	}
	
	// buffer q
	pVidEnc->DispBufQ = OSQCreate(pVidEnc->dispbuf_q_buf, DISP_BUF_NUM);
	if(pVidEnc->DispBufQ == 0) {
		goto __fail;
	}
	
	pVidEnc->FifoBufQ = OSQCreate(pVidEnc->fifobuf_q_buf, FIFO_BUF_NUM);
	if(pVidEnc->FifoBufQ == 0) {
		goto __fail;
	}
	
	pVidEnc->VidEmptyBufQ = OSQCreate(pVidEnc->vidempty_q_buf, VIDEO_BUF_NUM);
	if(pVidEnc->VidEmptyBufQ == 0) {
		goto __fail;
	}
	
	pVidEnc->VidReadyBufQ = OSQCreate(pVidEnc->vidready_q_buf, VIDEO_BUF_NUM);
	if(pVidEnc->VidReadyBufQ == 0) {
		goto __fail;
	}
	
	// task create
	err = OSTaskCreate(videnc_state_entry, (void *)pVidEnc, &pVidEnc->state_task_stack[VidEncTaskStackSize - 1], AVI_ENC_PRIORITY); 
	if(err != OS_NO_ERR) {
		goto __fail;
	}
	
	pVidEnc->state_task_running = 1;
	err = OSTaskCreate(videnc_disp_task_entry, (void *)pVidEnc, &pVidEnc->disp_task_stack[VidEncTaskStackSize - 1], SCALER_PRIORITY); 
	if(err != OS_NO_ERR) {
		goto __fail;
	}
	
	pVidEnc->disp_task_running = 1;
	err = OSTaskCreate(videnc_vid_task_entry, (void *)pVidEnc, &pVidEnc->video_task_stack[VidEncTaskStackSize - 1], JPEG_ENC_PRIORITY); 
	if(err != OS_NO_ERR) {
		goto __fail;
	}
	
	pVidEnc->video_task_running = 1;
	err = OSTaskCreate(videnc_aud_task_entry, (void *)pVidEnc, &pVidEnc->audio_task_stack[VidEncTaskStackSize - 1], AUD_ENC_PRIORITY); 
	if(err != OS_NO_ERR) {
		goto __fail;
	}
	
	pVidEnc->audio_task_running = 1;
	return (void *)pVidEnc;

__fail:
	videnc_close(pVidEnc);
	return 0;
}

/**
 * @brief   close video encode with wrap path
 * @param   workmem[in]: work memory
 * @return  none
 */
void videnc_close(void *workmem)
{
	INT8U err;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return;
	}
	
	if(pVidEnc->audio_task_running) {
		videnc_aud_task_exit(pVidEnc);
	}
	
	if(pVidEnc->video_task_running) {
		videnc_vid_task_exit(pVidEnc);
	}
	
	if(pVidEnc->disp_task_running) {
		videnc_send_msg(pVidEnc->DispQ, pVidEnc->DispM, MSG_DISPLAY_EXIT);
	}
	
	if(pVidEnc->state_task_running) {
		videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_EXIT);
	}
	
	// video encode state
	if(pVidEnc->VidEncQ) {
		OSQDel(pVidEnc->VidEncQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->VidEncM) {
		OSMboxDel(pVidEnc->VidEncM, OS_DEL_ALWAYS, &err);
	}
	
	// display
	if(pVidEnc->DispQ) {
		OSQDel(pVidEnc->DispQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->DispM) {
		OSMboxDel(pVidEnc->DispM, OS_DEL_ALWAYS, &err);
	}
	
	// video encode
	if(pVidEnc->VideoQ) {
		OSQDel(pVidEnc->VideoQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->VideoM) {
		OSMboxDel(pVidEnc->VideoM, OS_DEL_ALWAYS, &err);
	}
	
	// audio encode
	if(pVidEnc->AudioQ) {
		OSQDel(pVidEnc->AudioQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->AudioM) {
		OSMboxDel(pVidEnc->AudioM, OS_DEL_ALWAYS, &err);
	}
	
	// buffer q
	if(pVidEnc->DispBufQ) {
		OSQDel(pVidEnc->DispBufQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->FifoBufQ) {
		OSQDel(pVidEnc->FifoBufQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->VidEmptyBufQ) {
		OSQDel(pVidEnc->VidEmptyBufQ, OS_DEL_ALWAYS, &err);
	}
	
	if(pVidEnc->VidReadyBufQ) {
		OSQDel(pVidEnc->VidReadyBufQ, OS_DEL_ALWAYS, &err);
	}
		
	gp_free((void *)pVidEnc);
}

/**
 * @brief   register display function
 * @param   disp[in]: display callback function
 * @return  none
 */
void videnc_register_disp_fun(INT32U (*disp)(INT16U, INT16U, INT32U))
{
	display_callback = disp;
}

/**
 * @brief   start avi packer
 * @param   workmem[in]: video encode work memory
 * @param   fd[in]: video file handle
 * @param   task_prio[in]: avi packer priority
 * @return  packer work memory
 */
void *videnc_packer_start(void *workmem, INT16S fd, INT32U task_prio)
{
	INT32S ret;
	INT32U package_size, sample_per_block;
	PackerCtrl_t *pPackerCtrl;
	GP_AVI_AVISTREAMHEADER *pAviAud, *pAviVid;
	GP_AVI_PCMWAVEFORMAT *pAviWave;
	GP_AVI_BITMAPINFO *pAviBmi;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	MSG("%s\r\n", __func__);
	if(pVidEnc == 0) {
		return 0;
	}
	
	if(pPackerCtrl->state == STATE_PACKER_START) {
		return 0;
	}
	
	// allocate avi packer mwmeory	
	pPackerCtrl = (PackerCtrl_t *)gp_malloc_align(sizeof(PackerCtrl_t), 4);
	if(pPackerCtrl == 0) {
		return 0;
	}

	MSG("PackerCtrl = 0x%x\r\n", pPackerCtrl);
	gp_memset((INT8S *)pPackerCtrl, 0x00, sizeof(PackerCtrl_t));
	pPackerCtrl->FileWriteBufSize = FILE_WRITE_BUF_SIZE;
	pPackerCtrl->IndexWriteBufSize = INDEX_WRITE_BUF_SIZE;
	ret = AviPackerV3_GetWorkMemSize() + pPackerCtrl->FileWriteBufSize + pPackerCtrl->IndexWriteBufSize;
	pPackerCtrl->WorkMem = (void *)gp_malloc_align(ret, 4);
	if(pPackerCtrl->WorkMem == 0) {
		goto __fail;
	}
	
	gp_memset((INT8S *)pPackerCtrl->WorkMem, 0x00, ret);
	pPackerCtrl->fd = -1;
	pPackerCtrl->index_fd = -1;
	pPackerCtrl->FileWriteBuf = (void *)((INT32U)pPackerCtrl->WorkMem + AviPackerV3_GetWorkMemSize());
	pPackerCtrl->IndexWriteBuf = (void *)((INT32U)pPackerCtrl->FileWriteBuf + pPackerCtrl->FileWriteBufSize);
	MSG("AviWorkMem = 0x%x (%d)\r\n", pPackerCtrl->FileWriteBuf, AviPackerV3_GetWorkMemSize());
	MSG("FileWriteBuf = 0x%x (%d)\r\n", pPackerCtrl->WorkMem, pPackerCtrl->FileWriteBufSize);
	MSG("IndexWriteBuf = 0x%x (%d)\r\n", pPackerCtrl->IndexWriteBuf, pPackerCtrl->IndexWriteBufSize);
	
	// check disk size
	if(pVidEnc->source_type == SOURCE_TYPE_FS) {
		INT64U disk_free;
		INT16S disk_no = GetDiskOfFile(fd);
		
		// caculate storage free size 
		disk_free = vfsFreeSpace(disk_no);
		if(disk_free < DISK_MIN_FREE_SIZE) {
		   goto __fail;
		}
		
		MSG("DISK FREE SIZE = %dMByte\r\n", disk_free/1024/1024);
		pVidEnc->disk_free_size = disk_free - DISK_MIN_FREE_SIZE;
		pVidEnc->record_total_size = (2* 32 * 1024) + 16; //avi header + data is 32k align + index header
		
		if(disk_no == 0) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"A:\\index_%d.tmp", OSTimeGet());
		} else if(disk_no == 1) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"B:\\index_%d.tmp", OSTimeGet());
		} else if(disk_no == 2) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"C:\\index_%d.tmp", OSTimeGet());
		} else if(disk_no == 3) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"D:\\index_%d.tmp", OSTimeGet());
		} else if(disk_no == 4) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"E:\\index_%d.tmp", OSTimeGet());
		} else if(disk_no == 5) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"F:\\index_%d.tmp", OSTimeGet());
		} else if(disk_no == 6) {
			sprintf((char *)pPackerCtrl->index_path, (const char *)"G:\\index_%d.tmp", OSTimeGet());
		} else {
			goto __fail;
		}
		
		MSG("index file = %s\r\n", pPackerCtrl->index_path);
		pPackerCtrl->fd = fd;
		pPackerCtrl->index_fd = open((char*)pPackerCtrl->index_path, O_RDWR|O_CREAT|O_TRUNC);
		if(pPackerCtrl->index_fd < 0) {
			MSG("index_fd open fail\r\n");
			goto __fail;
		}
	} else {
		// need to implement
		while(1);
	}
	
	// video information init
	pAviVid = &pPackerCtrl->VidStreamHeader;
	pAviBmi = &pPackerCtrl->BitmapInfo;
	pAviVid->fccType[0] = 'v';
	pAviVid->fccType[1] = 'i';
	pAviVid->fccType[2] = 'd';
	pAviVid->fccType[3] = 's';
	pAviVid->dwScale = pVidEnc->dwScale;
	pAviVid->dwRate = pVidEnc->dwRate;
	pAviVid->rcFrame.right = pVidEnc->target_w;
	pAviVid->rcFrame.bottom = pVidEnc->target_h;
	switch(pVidEnc->video_format)
	{
	case C_MJPG_FORMAT:
		pPackerCtrl->BitmapInfoLen = 40;
		pAviVid->fccHandler[0] = 'm';
		pAviVid->fccHandler[1] = 'j';
		pAviVid->fccHandler[2] = 'p';
		pAviVid->fccHandler[3] = 'g';
		
		pAviBmi->biSize = pPackerCtrl->BitmapInfoLen;
		pAviBmi->biWidth = pVidEnc->target_w;
		pAviBmi->biHeight = pVidEnc->target_h;
		pAviBmi->biBitCount = 24;
		pAviBmi->biCompression[0] = 'M';
		pAviBmi->biCompression[1] = 'J';
		pAviBmi->biCompression[2] = 'P';
		pAviBmi->biCompression[3] = 'G';
		pAviBmi->biSizeImage = pAviBmi->biWidth * pAviBmi->biHeight * 3;
		break;
		
	case C_XVID_FORMAT:
		pPackerCtrl->BitmapInfoLen = 68;
		pAviVid->fccHandler[0] = 'x';
		pAviVid->fccHandler[1] = 'v';
		pAviVid->fccHandler[2] = 'i';
		pAviVid->fccHandler[3] = 'd';
		
		pAviBmi->biSize = pPackerCtrl->BitmapInfoLen;
		pAviBmi->biWidth = pVidEnc->target_w;
		pAviBmi->biHeight = pVidEnc->target_h;
		pAviBmi->biBitCount = 24;
		pAviBmi->biCompression[0] = 'X';
		pAviBmi->biCompression[1] = 'V';
		pAviBmi->biCompression[2] = 'I';
		pAviBmi->biCompression[3] = 'D';
		pAviBmi->biSizeImage = pAviBmi->biWidth * pAviBmi->biHeight * 3;
		break; 
	} 
	
	// audio information init
	pAviAud = &pPackerCtrl->AudStreamHeader;
	pAviWave = &pPackerCtrl->WaveInfo;
	pAviAud->fccType[0] = 'a';
	pAviAud->fccType[1] = 'u';
	pAviAud->fccType[2] = 'd';
	pAviAud->fccType[3] = 's';
	
	switch(pVidEnc->audio_format) 
	{
	case WAV:
		pPackerCtrl->WaveInfoLen = 16;
		pAviAud->fccHandler[0] = 1;
		pAviAud->fccHandler[1] = 0;
		pAviAud->fccHandler[2] = 0;
		pAviAud->fccHandler[3] = 0;
			
		pAviWave->wFormatTag = WAVE_FORMAT_PCM;
		pAviWave->nChannels = pVidEnc->channel_no;	
		pAviWave->nSamplesPerSec = pVidEnc->sample_rate;
		pAviWave->nAvgBytesPerSec = pAviWave->nChannels * pAviWave->nSamplesPerSec * 2; 
		pAviWave->nBlockAlign = pAviWave->nChannels * 2;
		pAviWave->wBitsPerSample = 16; //16bit
		break;
		
	case MICROSOFT_ADPCM:
		pPackerCtrl->WaveInfoLen = 50;
		pAviAud->fccHandler[0] = 0;
		pAviAud->fccHandler[1] = 0;
		pAviAud->fccHandler[2] = 0;
		pAviAud->fccHandler[3] = 0;

		package_size = 0x100;
		if(pVidEnc->channel_no == 1) {
			sample_per_block = 2 * package_size - 12;
		} else if(pVidEnc->channel_no == 2) {
			sample_per_block = package_size - 12;
		} else {
			sample_per_block = 1;
		}
		 
		pAviWave->wFormatTag = WAVE_FORMAT_ADPCM;
		pAviWave->nChannels = pVidEnc->channel_no;	
		pAviWave->nSamplesPerSec = pVidEnc->sample_rate;
		pAviWave->nAvgBytesPerSec = package_size * pAviWave->nSamplesPerSec / sample_per_block; // = PackageSize * FrameSize / fs
		pAviWave->nBlockAlign = package_size; //PackageSize
		pAviWave->wBitsPerSample = 4; //4bit
		pAviWave->cbSize = 32;
		
		// extension ...
		pAviWave->ExtInfo[0] = 0x01F4;	
		pAviWave->ExtInfo[1] = 0x0007;	
		pAviWave->ExtInfo[2] = 0x0100;	
		pAviWave->ExtInfo[3] = 0x0000;
		pAviWave->ExtInfo[4] = 0x0200;	
		pAviWave->ExtInfo[5] = 0xFF00;
		pAviWave->ExtInfo[6] = 0x0000;	
		pAviWave->ExtInfo[7] = 0x0000;
		pAviWave->ExtInfo[8] = 0x00C0;	
		pAviWave->ExtInfo[9] = 0x0040;
		pAviWave->ExtInfo[10] = 0x00F0; 
		pAviWave->ExtInfo[11] = 0x0000;
		pAviWave->ExtInfo[12] = 0x01CC; 
		pAviWave->ExtInfo[13] = 0xFF30;
		pAviWave->ExtInfo[14] = 0x0188; 
		pAviWave->ExtInfo[15] = 0xFF18;
		break;
		
	case IMA_ADPCM:
		pPackerCtrl->WaveInfoLen = 20;
		pAviAud->fccHandler[0] = 0;
		pAviAud->fccHandler[1] = 0;
		pAviAud->fccHandler[2] = 0;
		pAviAud->fccHandler[3] = 0;
		
		package_size = 0x100;
		if(pVidEnc->channel_no == 1) {
			sample_per_block = 2 * package_size - 7;
		} else if(pVidEnc->channel_no == 2) {
			sample_per_block = package_size - 7;
		} else {
			sample_per_block = 1;
		} 
		
		pAviWave->wFormatTag = WAVE_FORMAT_IMA_ADPCM;
		pAviWave->nChannels = pVidEnc->channel_no;	
		pAviWave->nSamplesPerSec = pVidEnc->sample_rate;
		pAviWave->nAvgBytesPerSec = package_size * pAviWave->nSamplesPerSec / sample_per_block;
		pAviWave->nBlockAlign = package_size;	//PackageSize
		pAviWave->wBitsPerSample = 4; //4bit
		pAviWave->cbSize = 2;
		
		// extension ...
		pAviWave->ExtInfo[0] = sample_per_block;
		break;
	}
	
	pAviAud->dwScale = pAviWave->nBlockAlign;
	pAviAud->dwRate = pAviWave->nAvgBytesPerSec;
	pAviAud->dwSampleSize = pAviWave->nBlockAlign;
	
	// avi packer init
	pPackerCtrl->task_prio = task_prio;
	ret = AviPackerV3_Open(pPackerCtrl->WorkMem,
							pPackerCtrl->fd, 
							pPackerCtrl->index_fd,
							&pPackerCtrl->VidStreamHeader,
							pPackerCtrl->BitmapInfoLen,
							&pPackerCtrl->BitmapInfo,
							&pPackerCtrl->AudStreamHeader,
							pPackerCtrl->WaveInfoLen,
							&pPackerCtrl->WaveInfo, 
							pPackerCtrl->task_prio,
							pPackerCtrl->FileWriteBuf, 
							pPackerCtrl->FileWriteBufSize, 
							pPackerCtrl->IndexWriteBuf, 
							pPackerCtrl->IndexWriteBufSize);
	if(ret < 0) {
		goto __fail;
	}
							
	AviPackerV3_SetErrHandler(pPackerCtrl->WorkMem, videnc_packer_err_handle);
	if(pVidEnc->source_type == SOURCE_TYPE_FS) {
		packer_put_data = AviPackerV3_PutData;
	} else {
		// need to implement
		while(1);
	}
	
	pPackerCtrl->state = STATE_PACKER_START;
	pVidEnc->PackerCtrl = (void *)pPackerCtrl;
	return (void *)pPackerCtrl;

__fail:	
	if(pPackerCtrl) {
		if(pPackerCtrl->fd) {
			close(pPackerCtrl->fd);
		}
		
		if(pPackerCtrl->index_fd) {
			close(pPackerCtrl->index_fd);
			unlink((CHAR*)pPackerCtrl->index_path);
		}
	
		if(pPackerCtrl->WorkMem) {
			gp_free((void *)pPackerCtrl->WorkMem);
		}
	
		gp_free((void *)pPackerCtrl);
	}
	
	return 0;
}

/**
 * @brief   stop avi packer
 * @param   packer_workmem[in]: packer work memory
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_packer_stop(void *packer_workmem)
{
	INT32S ret;
	PackerCtrl_t *pPackerCtrl = (PackerCtrl_t *)packer_workmem;
	
	MSG("%s\r\n", __func__);
	if(pPackerCtrl == 0) {
		return STATUS_FAIL;
	}
	
	if(pPackerCtrl->state == STATE_PACKER_STOP) {
		return STATUS_OK;
	}
	
	// close packer
	if((pPackerCtrl->fd >= 0) && (pPackerCtrl->index_fd >= 0)) {
		ret = AviPackerV3_Close(pPackerCtrl->WorkMem);
		close(pPackerCtrl->fd);
		close(pPackerCtrl->index_fd);
    	unlink2((CHAR*)pPackerCtrl->index_path);
		pPackerCtrl->fd = -1;
		pPackerCtrl->index_fd = -1;
		
	#if (NAND1_EN || NAND2_EN)
		switch(GetDiskOfFile(pPackerCtrl->fd))
		{
		case FS_NAND1:
		case FS_NAND2:
			DrvNand_flush_allblk(); 
			MSG("Nand_flush_allblk.\r\n");
			break;
		}
	#endif		
	} else {
		ret = 0;
	}
	
	if(pPackerCtrl) {
		pPackerCtrl->state = STATE_PACKER_STOP;
		if(pPackerCtrl->WorkMem) {
			gp_free((void *)pPackerCtrl->WorkMem);
		}
	
		gp_free((void *)pPackerCtrl);
	}
	
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

/**
 * @brief   start preview 
 * @param   workmem[in]: video encode work memory
 * @param   param[in]: video encode paramter
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_preview_start(void *workmem, VidEncParam_t *param)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	if(pVidEnc->state == STATE_PREVIEW) {
		return STATUS_OK;	
	}
	
	pVidEnc->source_type = param->source_type;
	pVidEnc->channel_no = param->channel_no;
	pVidEnc->sample_rate = param->sample_rate;
	pVidEnc->audio_format = param->audio_format;
	
	pVidEnc->video_format = param->video_format;
	pVidEnc->dwScale = 1;
	pVidEnc->dwRate = param->frame_rate;
	
	pVidEnc->Qvalue = param->quality_value;
	if(pVidEnc->Qvalue == 0) {
		pVidEnc->Qvalue = 50;
	}
	
	// scale 0 output format for display
	if(param->display_format == IMAGE_OUTPUT_FORMAT_RGB565) {
		pVidEnc->disp_fmt = C_SCALER_CTRL_OUT_RGB565;
	} else if(param->display_format == IMAGE_OUTPUT_FORMAT_YUYV) {
		pVidEnc->disp_fmt = C_SCALER_CTRL_OUT_YUYV;
	} else if(param->display_format == IMAGE_OUTPUT_FORMAT_UYVY) {
		pVidEnc->disp_fmt = C_SCALER_CTRL_OUT_UYVY;
	} else {
		MSG("Scaler not support output format!!!\r\n");
		return STATUS_OK;
	}
	
	// conv422 output format for jpeg encode input format
	if((param->sensor_w * param->sensor_h) >= (1024*720)) {	
		MSG("JPEG_GPYUV420\r\n");
		pVidEnc->conv422_fmt = JPEG_GPYUV420;
	} else {
		MSG("JPEG_YUV422\r\n");
		pVidEnc->conv422_fmt = JPEG_YUV422;
	}
	
	pVidEnc->sensor_w = param->sensor_w;
	pVidEnc->sensor_h = param->sensor_h;
	pVidEnc->disp_w = param->disp_w;
	pVidEnc->disp_h = param->disp_h;
	pVidEnc->target_w = param->target_w;
	pVidEnc->target_h = param->target_h;
#if FRAME_MODE_EN == 0
	pVidEnc->fifo_line_num = JPEG_FIFO_LINE;
	pVidEnc->fifo_buff_num = FIFO_BUF_NUM;
#else
	pVidEnc->fifo_line_num = pVidEnc->target_h;
	pVidEnc->fifo_buff_num = 3;
#endif

	pVidEnc->snap_delay = 150;	//freeze preview a moment while capturing a photo 

	return videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_PREVIEW_START);
}

/**
 * @brief   stop preview 
 * @param   workmem[in]: video encode work memory
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_preview_stop(void *workmem)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	if(pVidEnc->state == STATE_STOP) {
		return STATUS_OK;	
	}
	
	return videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_PREVIEW_STOP);
}

/**
 * @brief   start video encode 
 * @param   workmem[in]: video encode work memory
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_encode_start(void *workmem)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	if(pVidEnc->state == STATE_ENCODE) {
		return STATUS_OK;	
	}
	
	return videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_ENCODE_START);
}

/**
 * @brief   stop video encode 
 * @param   workmem[in]: video encode work memory
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_encode_stop(void *workmem)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	if(pVidEnc->state == STATE_PREVIEW) {
		return STATUS_OK;	
	}
	
	return videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_ENCODE_STOP);
}
/**
 * @brief   capture a photo in jpeg 
 * @param   workmem[in]: video encode work memory
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_scaler_up_capture_start(void *workmem,INT16S fd)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	if(pVidEnc->state == STATE_ENCODE) {
		return STATUS_OK;	
	}
	
	pVidEnc->fd = fd;
	
	return videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_CAPTURE_UP);
}

/**
 * @brief   capture a photo in jpeg 
 * @param   workmem[in]: video encode work memory
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_capture_start(void *workmem,INT16S fd)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	if(pVidEnc->state == STATE_ENCODE) {
		return STATUS_OK;	
	}
	
	pVidEnc->fd = fd;
	
	return videnc_send_msg(pVidEnc->VidEncQ, pVidEnc->VidEncM, MSG_VIDENC_CAPTURE);
}
/**
 * @brief   viddec get status 
 * @param   workmem[in]: video encode work memory
 * @return  currect status / STATUS_FAIL
 */
INT32S videnc_get_status(void *workmem)
{
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	if(pVidEnc == 0) {
		return STATUS_FAIL;
	}
	
	switch(pVidEnc->state)
	{
	case STATE_STOP:
		return C_STS_STOP;
		
	case STATE_PREVIEW:
	case STATE_CHANGE_STOP:
		return C_STS_PREVIEW;
		
	case STATE_CHANGE_ENCODE:
	case STATE_ENCODE:
	case STATE_CHANGE_PREVIEW:
		return C_STS_RECORD;
	}
	
	return STATUS_FAIL;
}

/**
 * @brief   set digital zoom 
 * @param   workmem[in]: video encode work memory
 * @param   ZoomFactor[in]: 1.0 ~ N
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S videnc_set_zoom(void *workmem, FP32 ZoomFactor)
{
#if 0
	FP32 zoom_temp, ratio;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)workmem;
	
	// scaler1 for target size	
	ratio = (FP32)pVidEnc->target_w / pVidEnc->sensor_w;
	zoom_temp = 65536 / (ratio * ZoomFactor);
	pVidEnc->xfactor[1] = (INT32U)zoom_temp;
#if 0
	ratio = (FP32)pVidEnc->target_h / pVidEnc->sensor_h;
	zoom_temp = 65536 / (ratio * ZoomFactor);
	pVidEnc->yfactor[1] = (INT32U)zoom_temp;
#else
	pVidEnc->yfactor[1] = (pVidEnc->sensor_h << 16) / pVidEnc->target_h;
#endif
	MSG("xyfactor[1] = 0x%x, 0x%x\r\n", pVidEnc->xfactor[1], pVidEnc->yfactor[1]);
	
	zoom_temp = 1 - (1 / ZoomFactor);
	pVidEnc->xoffset[1] = (int)((float)(pVidEnc->target_w/2)*zoom_temp) << 16;
#if 0	
	pVidEnc->yoffset[1] = (int)((float)(pVidEnc->target_h/2)*zoom_temp) << 16;
#else
	pVidEnc->yoffset[1] = 0;
#endif	
	MSG("offset[1] = %d, %d\r\n", pVidEnc->xoffset[1] >> 16, pVidEnc->yoffset[1] >> 16);
	
	// scaler0 for display 
	ratio = (FP32)pVidEnc->disp_w / pVidEnc->target_w;
	zoom_temp = 65536 / (ratio * ZoomFactor);
	pVidEnc->xfactor[0] = (INT32U)zoom_temp;
#if 0
	ratio = (FP32)pVidEnc->disp_h / pVidEnc->target_h;
	zoom_temp = 65536 / (ratio * ZoomFactor);
	pVidEnc->yfactor[0] = (INT32U)zoom_temp;
#else	
	pVidEnc->yfactor[0] = (pVidEnc->target_h << 16) / pVidEnc->disp_h;
#endif	
	MSG("xyfactor[0] = 0x%x, 0x%x\r\n", pVidEnc->xfactor[0], pVidEnc->yfactor[0]);
	
	zoom_temp = 1 - (1 / ZoomFactor);
	pVidEnc->xoffset[0] = (INT32U)((FP32)(pVidEnc->disp_w/2)*zoom_temp) << 16;
#if 0	
	pVidEnc->yoffset[0] = (INT32U)((FP32)(pVidEnc->disp_h/2)*zoom_temp) << 16;
#else
	pVidEnc->yoffset[0] = 0;
#endif	
	MSG("offset[0] = %d, %d\r\n", pVidEnc->xoffset[0] >> 16, pVidEnc->yoffset[0] >> 16);
	
	// digital zoom enable 
	pVidEnc->DigiZoomEn = 1;
#endif
	return STATUS_OK;
}

INT32S videnc_state_one_frame(VidEncCtrl_t *pVidEnc)
{
	INT8U err;
	
	err = OSQPost(pVidEnc->VidEncQ, (void*)MSG_VIDENC_ONE_FRAME);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S videnc_state_error(VidEncCtrl_t *pVidEnc)
{
	INT8U err;
	
	err = OSQPost(pVidEnc->VidEncQ, (void*)MSG_VIDENC_STORAGE_ERR);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

static void videnc_state_entry(void *p_arg)
{
	INT8U err, success_flag;
	INT32U msg, frame_cnt=0;
	INT32S ret;
	INT64S dwtemp;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)p_arg;
	OS_EVENT *VidEncQ = pVidEnc->VidEncQ;
	OS_EVENT *VidEncM = pVidEnc->VidEncM;
	VidBuf_t *pVidBuf;
	PackerCtrl_t *pPackerCtrl;
	OS_CPU_SR cpu_sr;
	
	while(1) 
	{
		msg = (INT32U) OSQPend(VidEncQ, 0, &err);
		if(err != OS_NO_ERR) {
			MSG("VidEncQPendFail = %d\r\n", err);
			while(1);
		}
	
		switch(msg)
		{		
		case MSG_VIDENC_PREVIEW_START:
			MSG("MSG_VIDENC_PREVIEW_START\r\n");
			ret = videnc_sensor_init(pVidEnc, NULL);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			ret = videnc_sensor_on(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			ret = videnc_memory_allocate(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			ret = videnc_preview_on(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			OSMboxPost(VidEncM, (void *)ACK_OK);
			break;
		
		case MSG_VIDENC_PREVIEW_STOP:
			MSG("MSG_VIDENC_PREVIEW_STOP\r\n");
			ret = videnc_preview_off(pVidEnc);
			videnc_sensor_off(pVidEnc);
			videnc_memory_free(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			OSMboxPost(VidEncM, (void *)ACK_OK);
			break;
		
		case MSG_VIDENC_ENCODE_START:
			MSG("MSG_VIDENC_ENCODE_START\r\n");
			frame_cnt = 0;
			pVidEnc->ta = 0;
			pVidEnc->tv = 0;
			pVidEnc->Tv = 0;
			pVidEnc->pend_cnt = 0;
			pVidEnc->post_cnt = 0;
			if(pVidEnc->audio_format) {
				pVidEnc->freq_div = pVidEnc->sample_rate / VID_ENC_TIME_BASE;
				pVidEnc->tick = (INT64S)pVidEnc->dwRate * pVidEnc->freq_div;
				pVidEnc->delta_Tv = pVidEnc->dwScale * pVidEnc->sample_rate;
			} else {
				pVidEnc->freq_div = 1;
				pVidEnc->tick = (INT64S)pVidEnc->dwRate * pVidEnc->freq_div;
				pVidEnc->delta_Tv = pVidEnc->dwScale * VID_ENC_TIME_BASE;
			}
			
			// sync start
			videnc_sync_timer_start();
			
			// video start
			ret = videnc_vid_task_start(pVidEnc, pVidEnc->Qvalue);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			// switch to encode mode
			ret = videnc_encode_on(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			// audio start
			if(pVidEnc->audio_format) {
				videnc_aud_task_start(pVidEnc);
			}
			OSMboxPost(VidEncM, (void *)ACK_OK);
			break;
			
		case MSG_VIDENC_ENCODE_STOP:
			MSG("MSG_VIDENC_ENCODE_STOP\r\n");
			// switch to preview mode
			ret = videnc_encode_off(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			// audio stop
			if(pVidEnc->audio_format) {
				ret = videnc_aud_task_stop(pVidEnc);
			}
			
			// video stop
			videnc_vid_task_stop(pVidEnc);
			
			// sync stop
			videnc_sync_timer_stop();
			OSMboxPost(VidEncM, (void *)ACK_OK);
			break;

		case MSG_VIDENC_CAPTURE:
			
			
			// video start
			ret = videnc_vid_task_start(pVidEnc, pVidEnc->Qvalue);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			// switch to encode mode
			ret = videnc_encode_on(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}

			pVidBuf = (VidBuf_t *)OSQPend(pVidEnc->VidReadyBufQ, 0, &err);
			if(pVidBuf && (err == OS_NO_ERR)) {

				ret = videnc_encode_off(pVidEnc);
				if(ret < 0) {
					OSMboxPost(VidEncM, (void *)ACK_FAIL);
					continue;
				}		

				// video stop
				videnc_vid_task_stop(pVidEnc);

				if(ret < 0) {
					OSMboxPost(VidEncM, (void *)ACK_FAIL);
					continue;
				}	

				ret = videnc_preview_off(pVidEnc);

				if(ret < 0) {
					OSMboxPost(VidEncM, (void *)ACK_FAIL);
					continue;
				}
				ret = wrap_save_jpeg_file(pVidEnc->fd, (INT32U)pVidBuf->addr, (INT32U)pVidBuf->size);
				// send back to empty buffer
				err = OSQPost(pVidEnc->VidEmptyBufQ, (void *)pVidBuf);
				if(err != OS_NO_ERR) {
					MSG("%s, VidEmptyBufQFail\r\n", __func__);
				}				
				
				if(pVidEnc->snap_delay)				//freez preview by using OS tick delay
					OSTimeDly(pVidEnc->snap_delay);
				
				ret = videnc_preview_on(pVidEnc);
			} 


			OSMboxPost(VidEncM, (void *)ACK_OK);
			break;
		case MSG_VIDENC_CAPTURE_UP:
		
			pVidEnc->target_w = SCALOR_UP_W;
			pVidEnc->target_h = SCALOR_UP_H;
			pVidEnc->conv422_fmt = JPEG_YUV422;
			ret = videnc_vid_task_start(pVidEnc, pVidEnc->Qvalue);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			pVidEnc->target_w = 1280;
			pVidEnc->target_h = 960;
			pVidEnc->conv422_fmt = JPEG_GPYUV420;
			ret = videnc_fifo_to_framebuf(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}
			
			pVidBuf = (VidBuf_t *)OSQPend(pVidEnc->VidReadyBufQ, 0, &err);
			
			
			DBG_PRINT("get frame buf = 0x%x\r\n",pVidBuf); 
			
			ret = videnc_fifo_to_framebuf_off(pVidEnc);
			if(ret < 0) {
				OSMboxPost(VidEncM, (void *)ACK_FAIL);
				continue;
			}

			if(pVidBuf && (err == OS_NO_ERR)) {
				
				ret = videnc_preview_off(pVidEnc);
				if(ret < 0) {
					OSMboxPost(VidEncM, (void *)ACK_FAIL);
					continue;
				}

				ret = videnc_scaler0up_on(pVidEnc);
				if(ret < 0) {
					OSMboxPost(VidEncM, (void *)ACK_FAIL);
					continue;
				}
				
				pVidBuf = (VidBuf_t *)OSQPend(pVidEnc->VidReadyBufQ, 0, &err);

				videnc_vid_task_stop(pVidEnc);

				ret = videnc_scaler0up_off(pVidEnc);
				if(ret < 0) {
					OSMboxPost(VidEncM, (void *)ACK_FAIL);
					continue;
				}
				
				ret = wrap_save_jpeg_file(pVidEnc->fd, (INT32U)pVidBuf->addr, (INT32U)pVidBuf->size);
				// send back to empty buffer
				err = OSQPost(pVidEnc->VidEmptyBufQ, (void *)pVidBuf);
				if(err != OS_NO_ERR) {
					MSG("%s, VidEmptyBufQFail\r\n", __func__);
				}				
				
				if(pVidEnc->snap_delay)				//freez preview by using OS tick delay
					OSTimeDly(pVidEnc->snap_delay);
				
				ret = videnc_preview_on(pVidEnc);
			}

			OSMboxPost(VidEncM, (void *)ACK_OK);
			break;
		case MSG_VIDENC_ONE_FRAME:
			success_flag = 0;
			pPackerCtrl = pVidEnc->PackerCtrl;
			
			// check time distance
			OS_ENTER_CRITICAL();
			dwtemp = (INT64S)(pVidEnc->tv - pVidEnc->Tv);
			OS_EXIT_CRITICAL();
			if(dwtemp > (pVidEnc->delta_Tv << 5)) {
				// keep first frame is not null frame
				if(frame_cnt == 0) {
					goto __one_frame_exit;
				}
				
				ret = packer_put_data(pPackerCtrl->WorkMem,
								*(long*)"00dc", 
								0, 
								(void *)NULL, 
								1, 
								0x00);
				if(ret >= 0) {
					MSG("n");
					success_flag = 1;
					pVidEnc->record_total_size += 8 + 32; 
				}
					
				goto __one_frame_exit;
			}
						
			// get ready buffer
			pVidBuf = (VidBuf_t *)OSQAccept(pVidEnc->VidReadyBufQ, &err);
			if(pVidBuf && (err == OS_NO_ERR)) {
				ret = packer_put_data(pPackerCtrl->WorkMem,
										*(long*)"00dc", 
										pVidBuf->size, 
										(void *)pVidBuf->addr, 
										1, 
										pVidBuf->keyflag);
				if(ret >= 0) {
					MSG(".");
					success_flag = 1;
					frame_cnt++;
					pVidEnc->record_total_size += pVidBuf->size + 8 + 32; 
				} 
				
				// send back to empty buffer
				err = OSQPost(pVidEnc->VidEmptyBufQ, (void *)pVidBuf);
				if(err != OS_NO_ERR) {
					MSG("%s, VidEmptyBufQFail\r\n", __func__);
				}	
			} else {
				// keep first frame is not null frame
				if(frame_cnt == 0) {
					goto __one_frame_exit;
				}
				
				ret = packer_put_data(pPackerCtrl->WorkMem,
								*(long*)"00dc", 
								0, 
								(void *)NULL, 
								1, 
								0x00);
				if(ret >= 0) {
					MSG("N");
					success_flag = 1;
					pVidEnc->record_total_size += 8 + 32; 
				}
			}
		
		__one_frame_exit:	
			// check disk size
			if(pVidEnc->record_total_size >= pVidEnc->disk_free_size) {
				err = OSQPost(VidEncQ, (void*)MSG_VIDENC_STORAGE_FULL);
				if(err != OS_NO_ERR) {
					MSG("%s, VidEncQFail\r\n", __func__);
				}
			}
			
			// success
			pVidEnc->pend_cnt++;
			if(success_flag) {
				OS_ENTER_CRITICAL();
				pVidEnc->Tv += pVidEnc->delta_Tv;
				OS_EXIT_CRITICAL();
			}		
			break;
		
		case MSG_VIDENC_STORAGE_FULL:
			MSG("MSG_VIDENC_STORAGE_FULL\r\n");
		case MSG_VIDENC_STORAGE_ERR:
			MSG("MSG_VIDENC_STORAGE_ERR\r\n");
			// sync stop
			videnc_sync_timer_stop();
			OSQFlush(VidEncQ);
			
			// switch to preview mode
			ret = videnc_encode_off(pVidEnc);
			
			// audio stop
			if(pVidEnc->audio_format) {
				ret = videnc_aud_task_stop(pVidEnc);
			}
			
			// video stop
			videnc_vid_task_stop(pVidEnc);
			
			// packer stop
			pPackerCtrl = pVidEnc->PackerCtrl;
			pVidEnc->PackerCtrl = 0;
			videnc_packer_stop((void *)pPackerCtrl);
			break;
		
		case MSG_VIDENC_EXIT:
			MSG("MSG_VIDENC_EXIT\r\n");
			OSMboxPost(VidEncM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
				
		default:
			break;
		}	
	}
	while(1);
}

void disp_send_ready_buf(VidEncCtrl_t *pVidEnc, INT32U disp_ready_buf)
{
	INT8U err;
	
	err = OSQPost(pVidEnc->DispQ, (void *)disp_ready_buf);
	if(err != OS_NO_ERR) {
		MSG("DispQPostFail.\r\n");
	}
}

static void videnc_disp_task_entry(void *p_arg)
{
	INT8U err;
	INT32U msg, disp_addr;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)p_arg;
	OS_EVENT *DispQ = pVidEnc->DispQ;
	OS_EVENT *DispM = pVidEnc->DispM;
	OS_EVENT *DispBufQ = pVidEnc->DispBufQ;
	INT16U disp_w = pVidEnc->disp_w;
	INT16U disp_h = pVidEnc->disp_h;
	
	while(1) 
	{
		msg = (INT32U) OSQPend(DispQ, 0, &err);
		if(err != OS_NO_ERR) {
			MSG("DispQPendFail = %d\r\n", err);
			while(1);
		}
		
		if(msg == 0) {
			MSG("DispQMsg = 0\r\n");
			continue;
		}
		
		switch(msg)
		{	
		case MSG_DISPLAY_EXIT:
			MSG("MSG_DISPLAY_EXIT\r\n");
			OSMboxPost(DispM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
			
		default:
			disp_addr = msg;
			if(display_callback && disp_addr) {
				display_callback(disp_w, disp_h, disp_addr);
			}
			
			err = OSQPost(DispBufQ, (void *)disp_addr);
			if(err != OS_NO_ERR) {
				MSG("DispBufQPostFail.\r\n");
			}
		}	
	}
	while(1);
}
