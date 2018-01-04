/*
* Description: This task encodes YCbCr or YUYV data into JPEG variable length coded data
*
* Author: Cater Chen
*
* Date: 2008/10/28
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*/
#include "msg_manager.h"
#include "image_encoder.h"
#include "gplib_jpeg.h"
#include "gplib_jpeg_encode.h"
#include "drv_l1_scaler.h"
#include "drv_l1_jpeg.h"
#include "drv_l1_cache.h"
#include "drv_l2_scaler.h"

#define quant_table_user_define      				(((INT32U)1)<<27)
#define LINE_OFFSET_NUM								2
#define C_USE_SCALER_NUMBER							SCALER_0 

#if _OPERATING_SYSTEM == _OS_UCOS2
static void image_block_write_task_entry(void *p_arg);
#endif

#if _OPERATING_SYSTEM == _OS_UCOS2
static INT16S encode_fd;
static INT32U use_disk;
#endif

//TASK define for encode block write
static MSG_Q_ID encode_msg_queue_id = NULL;
static OS_EVENT *image_enc_sw_sem = NULL;
extern MSG_Q_ID ApQ;
#define ENCODE_QUEUE_MAX_NUMS   		    		16
#define ENCODE_MAXIMUM_MESSAGE_NUM					16
#define ENCODE_MAX_MSG_SIZE                			0x20
#define BLOCK_WRITE_START                    		1
#define BlockWriteStackSize                 		512
#define TSK_ENCODE_BLOCK_WRITE              		24
static INT32U block_write_para[ENCODE_MAX_MSG_SIZE/4 + 1];
static INT32U BlockWriteStack[BlockWriteStackSize];

static INT32U (*pfunc_encode_end)(INT32U encode_buf,INT32U encode_size);
static INT32U (*pfunc_encode_block_read)(IMAGE_ENCODE_ARGUMENT *encode_info,IMAGE_ENCODE_PTR *encode_ptr);

static IMAGE_SCALE_ARGUMENT image_scaler_info;
static INT8U header_ck,ck_count,block_wr_num;
static INT32U task_create = 0; 
static INT32U encode_status,process_status,BLOCK_READ;

static INT8U image_encode_jpeg_422_header[624] = {
	0xFF, 0xD8, 0xFF, 0xFE, 0x00, 0x0B, 0x47, 0x50, 0x45, 0x6E, 0x63, 0x6F, 0x64, 0x65, 0x72,
	// Quality 50
	0xFF, 0xDB, 0x00, 0x43, 0x00,  //Luminance
	0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A,	0x10, 0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 
	0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 
	0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 
	0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 
	0xFF, 0xDB, 0x00, 0x43, 0x01, //Chrominance 
	0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A, 0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63, 
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
	0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0xF0, 
    // Quality 50
	0x01, 0x40, 0x03, 0x01, 0x21, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xFF, 0xC4, 0x00, 0x1F, 
	0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 
	0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 
	0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 
	0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 
	0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 
	0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 
	0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 
	0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 
	0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 
	0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 
	0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 
	0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 
	0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
	0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 
	0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 
	0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 
	0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 
	0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 
	0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 
	0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 
	0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 
	0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 
	0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 
	0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 
	0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDD, 0x00, 0x04, 
	0x00, 0x00, 0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3F, 0x00
};

// extern function
extern IMAGE_CODEC_STATUS image_scale_status(void);
extern CODEC_START_STATUS image_scale_start(IMAGE_SCALE_ARGUMENT arg);

extern INT16U quant20_luminance_table[];
extern INT16U quant20_chrominance_table[];

extern INT16U quant25_luminance_table[];
extern INT16U quant25_chrominance_table[];

extern INT16U quant30_luminance_table[];
extern INT16U quant30_chrominance_table[];

extern INT16U quant40_luminance_table[];
extern INT16U quant40_chrominance_table[];

extern INT16U quant50_luminance_table[];
extern INT16U quant50_chrominance_table[];

extern INT16U quant70_luminance_table[];
extern INT16U quant70_chrominance_table[];

extern INT16U quant80_luminance_table[];
extern INT16U quant80_chrominance_table[];

extern INT16U quant85_luminance_table[];
extern INT16U quant85_chrominance_table[];

extern INT16U quant90_luminance_table[];
extern INT16U quant90_chrominance_table[];

extern INT16U quant93_luminance_table[];
extern INT16U quant93_chrominance_table[];

extern INT16U quant95_luminance_table[];
extern INT16U quant95_chrominance_table[];

extern INT16U quant98_luminance_table[];
extern INT16U quant98_chrominance_table[];

extern const INT8U zigzag_scan[];
//=============================================================================
// register encode end function.
void image_encode_end_func_register(INT32U (*func)(INT32U encode_buf, INT32U encode_size))
{
	if(func) {
		pfunc_encode_end = func;
	}
}

// register encode block read function.
void image_encode_block_read_func_register(INT32U (*func)(IMAGE_ENCODE_ARGUMENT *encode_info,IMAGE_ENCODE_PTR *encode_ptr))
{
	if(func) {
		pfunc_encode_block_read = func;
	}
}

//============================== Image Encoder ================================
//
// including JPEG422 420.....
//
//=============================================================================
static void image_enc_lock(void)
{
	INT8U err;
	
	if(image_enc_sw_sem)
		OSSemPend(image_enc_sw_sem, 0, &err);
}

static void image_enc_unlock(void)
{
	if(image_enc_sw_sem)
		OSSemPost(image_enc_sw_sem);
}

static INT32S image_encode_jpeg_header_generate(INT32U q, INT16U w, INT16U h)
{
	INT16U *plum=NULL, *pchr=NULL;
	INT32S i;
	IMAGE_ENCODE_ARGUMENT *image_encode_q;
	
	switch(q)
	{
	case 20: 
		plum = (INT16U *)quant20_luminance_table; 
		pchr = (INT16U *)quant20_chrominance_table;
		break;
		
	case 25: 
		plum = (INT16U *)quant25_luminance_table; 
		pchr = (INT16U *)quant25_chrominance_table;
		break;
	
	case 30:
		plum = (INT16U *)quant30_luminance_table; 
		pchr = (INT16U *)quant30_chrominance_table;
		break;
		
	case 40: 
		plum = (INT16U *)quant40_luminance_table; 
		pchr = (INT16U *)quant40_chrominance_table;
		break;
	
	case 50: 
		plum = (INT16U *)quant50_luminance_table; 
		pchr = (INT16U *)quant50_chrominance_table;
		break;
	
	case 70: 
		plum = (INT16U *)quant70_luminance_table; 
		pchr = (INT16U *)quant70_chrominance_table;
		break;
	
	case 80: 
		plum = (INT16U *)quant80_luminance_table; 
		pchr = (INT16U *)quant80_chrominance_table;
		break;
	
	case 85: 
		plum = (INT16U *)quant85_luminance_table; 
		pchr = (INT16U *)quant85_chrominance_table;
		break;
	
	case 90: 
		plum = (INT16U *)quant90_luminance_table; 
		pchr = (INT16U *)quant90_chrominance_table;
		break;
	
	case 93: 
		plum = (INT16U *)quant93_luminance_table; 
		pchr = (INT16U *)quant93_chrominance_table;
		break;
	
	case 95: 
		plum = (INT16U *)quant95_luminance_table; 
		pchr = (INT16U *)quant95_chrominance_table;
		break;
	
	case 98: 
		plum = (INT16U *)quant98_luminance_table; 
		pchr = (INT16U *)quant98_chrominance_table;
		break;
	
	case 100:
		break;
	
	default: 
		image_encode_q = (IMAGE_ENCODE_ARGUMENT *)q;
		q = image_encode_q->QuantizationQuality;
		plum = (INT16U *)image_encode_q->QuantLuminanceTable; 
		pchr = (INT16U *)image_encode_q->QuantChrominanceTable;
		break;
	}
	
	if(q == 100) {
		//Luminance
		for(i=0; i<64; i++) {
			image_encode_jpeg_422_header[i+0x14] = 0x01; 
		}

		//Chrominance 
		for(i=0; i<64; i++) {
			image_encode_jpeg_422_header[i+0x59] = 0x01;
		}
	} else {
		//Luminance
		for(i=0; i<64; i++) {
			image_encode_jpeg_422_header[i+0x14] = (INT8U) (*(plum + zigzag_scan[i]) & 0xFF); 
		}

		//Chrominance 
		for(i=0; i<64; i++) {
			image_encode_jpeg_422_header[i+0x59] = (INT8U) (*(pchr + zigzag_scan[i]) & 0xFF); 
		}
	}
	
	image_encode_jpeg_422_header[0x9E] = h >> 8;
	image_encode_jpeg_422_header[0x9F] = h & 0xFF;
	image_encode_jpeg_422_header[0xA0] = w >> 8;
	image_encode_jpeg_422_header[0xA1] = w & 0xFF; 
		
	return q;
}

static INT32U image_encode_block_read(IMAGE_ENCODE_ARGUMENT *encode_info, IMAGE_ENCODE_PTR *encode_ptr)
{
	INT32U y_len,scale_state=0;
	ScalerFormat_t src;
	
	if(BLOCK_READ == 0)
	{
	  	image_scaler_info.InputWidth = encode_info->ScalerInfoPtr->InputWidth; 
		image_scaler_info.InputHeight = encode_info->ScalerInfoPtr->InputHeight;
		image_scaler_info.InputVisibleWidth = encode_info->ScalerInfoPtr->InputVisibleWidth;
	    image_scaler_info.InputVisibleHeight = encode_info->ScalerInfoPtr->InputVisibleHeight;
		image_scaler_info.OutputWidth = encode_info->ScalerInfoPtr->OutputWidth;
		image_scaler_info.OutputHeight = (encode_info->ScalerInfoPtr->OutputHeight + LINE_OFFSET_NUM);
	    image_scaler_info.OutputWidthFactor = encode_info->ScalerInfoPtr->OutputWidthFactor;
	    image_scaler_info.OutputHeightFactor = encode_info->ScalerInfoPtr->OutputHeightFactor;
	    image_scaler_info.InputOffsetX = 0;	    
	    image_scaler_info.InputOffsetY = encode_info->ScalerInfoPtr->InputOffsetY;
	    image_scaler_info.InputBufPtr = (INT8U *)encode_info->ScalerInfoPtr->InputBufPtr;
		switch(encode_info->ScalerInfoPtr->InputFormat)
		{ 
		      case SCALER_INPUT_FORMAT_RGB565:
		           scale_state = (INT8U)C_SCALER_CTRL_IN_RGB565;
		           break;                     
		      case SCALER_INPUT_FORMAT_YUYV:
		           scale_state = (INT8U)C_SCALER_CTRL_IN_YUYV;
		           break;           
		      case SCALER_INPUT_FORMAT_UYVY:
		           scale_state = (INT8U)C_SCALER_CTRL_IN_UYVY;
		           break;                 
		      default:
		           DBG_PRINT("scaler input format error!\r\n");
		           break;         
		}
		image_scaler_info.InputFormat = scale_state;
		switch(encode_info->ScalerInfoPtr->OutputFormat)
		{                  
		      case IMAGE_OUTPUT_FORMAT_RGB565:
		           scale_state = (INT8U)C_SCALER_CTRL_OUT_RGB565;
		           break;                      
		      case IMAGE_OUTPUT_FORMAT_YUYV:
		           scale_state = (INT8U)C_SCALER_CTRL_OUT_YUYV;
		           break;           
		      case IMAGE_OUTPUT_FORMAT_UYVY:
		           scale_state = (INT8U)C_SCALER_CTRL_OUT_UYVY;
		           break;          
		      default:
		           DBG_PRINT("scaler output format error!\r\n");
		           break;          
		}			
		image_scaler_info.OutputFormat = scale_state;	    
		image_scaler_info.OutputBufWidth = encode_info->ScalerInfoPtr->OutputWidth;
		image_scaler_info.OutputWidth = encode_info->ScalerInfoPtr->OutputWidth;
		image_scaler_info.OutputBufHeight = encode_info->ScalerInfoPtr->OutputHeight;
		image_scaler_info.OutputHeight = encode_info->ScalerInfoPtr->OutputHeight;
		image_scaler_info.OutBoundaryColor = encode_info->ScalerInfoPtr->OutBoundaryColor;	    	    
	    BLOCK_READ = encode_info->ScalerInfoPtr->OutputHeight;	
	}
	else
	{
	    y_len = (image_scaler_info.OutputHeightFactor * BLOCK_READ);
	    image_scaler_info.InputOffsetY += y_len;		
	}

	src.input_format = image_scaler_info.InputFormat;
	src.input_width = image_scaler_info.InputWidth;
	src.input_height = image_scaler_info.InputHeight;
	src.input_visible_width = image_scaler_info.InputVisibleWidth;
	src.input_visible_height = image_scaler_info.InputVisibleHeight;	
	src.input_x_offset = image_scaler_info.InputOffsetX;
	src.input_y_offset = image_scaler_info.InputOffsetY;	
	src.input_y_addr = (INT32U)image_scaler_info.InputBufPtr;
	src.input_u_addr = 0;
	src.input_v_addr = 0;
	src.output_format = image_scaler_info.OutputFormat;
	src.output_width = image_scaler_info.OutputWidth;
	src.output_height = image_scaler_info.OutputHeight;
	src.output_buf_width = image_scaler_info.OutputWidth;
	src.output_buf_height = image_scaler_info.OutputHeight;	
	src.output_x_offset = 0;
	src.output_y_addr = (INT32U)encode_ptr->yaddr;
	src.output_u_addr = 0;
	src.output_v_addr = 0;	
	src.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;	
	src.scale_mode = C_SCALER_RATIO_USER;
	src.digizoom_m = 10;	
	src.digizoom_n = 10;
	scale_state = drv_l2_scaler_trigger(C_USE_SCALER_NUMBER, 1, (ScalerFormat_t *)&src, image_scaler_info.OutBoundaryColor);	
	
	return scale_state; 	
}

void image_encode_entrance(void)
{
   INT8S *pimage;
   INT32S ret;
   
   if(task_create == 0){
		#if _OPERATING_SYSTEM == _OS_UCOS2
			//Create a Encode Block Write task
			ret = OSTaskCreate(image_block_write_task_entry, (void *) 0, &BlockWriteStack[BlockWriteStackSize - 1], TSK_ENCODE_BLOCK_WRITE);
			if(ret == OS_NO_ERR) {
				task_create = 1;
				DBG_PRINT("image_block_write_task_entry [%d]\r\n", IMAGE_PRIORITY);
			} else {
				DBG_PRINT("image_block_write_task_entry fail[%d]\r\n", IMAGE_PRIORITY);	
			}
			
			if(image_enc_sw_sem == NULL)
			{
				image_enc_sw_sem = OSSemCreate(1);
				if(image_enc_sw_sem == 0)
					DBG_PRINT("Failed to allocate obj_sw_sem memory\r\n");	
			}			
		#endif     
   }
   
   encode_status = 0;
   process_status = 0;
   BLOCK_READ = 0;
   
   pimage=(INT8S *)&image_scaler_info;
   gp_memset(pimage,0,sizeof(IMAGE_SCALE_ARGUMENT));   
   
   image_encode_block_read_func_register(image_encode_block_read);
}						

void image_encode_exit(void)
{	
	INT8U err;
	
	#if _OPERATING_SYSTEM == _OS_UCOS2
		if(task_create)
			OSTaskDel(TSK_ENCODE_BLOCK_WRITE);
		if(encode_msg_queue_id)
			msgQDelete(encode_msg_queue_id);
		encode_msg_queue_id = NULL;	
		if(image_enc_sw_sem)
			OSSemDel(image_enc_sw_sem, OS_DEL_ALWAYS, &err);
		image_enc_sw_sem = NULL;		
	#endif
    task_create = 0;
}	

INT32U image_encode_start(IMAGE_ENCODE_ARGUMENT arg)
{
	INT8U source_format, input_mode=0, sampling_mode;
	INT16U image_valid_width, image_valid_height;
	INT32U fly_y_addr, fly_u_addr, fly_v_addr, y_len, uv_len,i,buf_ptr_ck,sum_encode_size,blocklength;
	INT32U encode_output_ptr=0,encode_size=0,malloc_error,msg_id,OutputBufPtr_1,OutputBufPtr_2=0,buffer_chage,quality;
	INT32U y_addr,u_addr,v_addr,encode_count;
	INT32S jpeg_status;
	INT64U disk_size;   
	IMAGE_ENCODE_ARGUMENT image_encode;
	ENCODE_BLOCK_WRITE_ARGUMENT image;
	IMAGE_ENCODE_PTR block_encode_ptr;
	
	image_enc_lock();
	if(process_status == 0){
		//global value init
		process_status = 1;
		encode_count = 0;
		
		//local value init
		buf_ptr_ck = 0;
		sum_encode_size = 0;
		
		//encode struct ptr
		image_encode = arg;
		malloc_error = 0;
		
		//encode information
		image_valid_width = image_encode.InputWidth;
		image_valid_height = image_encode.InputHeight;
		
		if(image_encode.InputFormat == IMAGE_ENCODE_INPUT_FORMAT_YUYV) {
		    source_format = C_JPEG_FORMAT_YUYV;
		} else {
			DBG_PRINT("InputFormat Error!!!\r\n");
			malloc_error = 1;
			goto JPEG_END;
		}
		  	
		input_mode = image_encode.EncodeMode;				// 0 = read once, 1 = read on-the-fly
		
		if(image_encode.OutputFormat == IMAGE_ENCODE_OUTPUT_FORMAT_YUV422) {
		    sampling_mode = C_JPG_CTRL_YUV422;		        // C_JPG_CTRL_YUV422 or C_JPG_CTRL_YUV420
			image_encode_jpeg_422_header[0xA4] = 0x21;
		} else if(image_encode.OutputFormat == IMAGE_ENCODE_OUTPUT_FORMAT_YUV420) {
	    	sampling_mode = C_JPG_CTRL_YUV420;
			image_encode_jpeg_422_header[0xA4] = 0x22;	    	
		} else {
			DBG_PRINT("OutputFormat Error!!!\r\n");
			malloc_error = 1;
			goto JPEG_END;
		}
		   
		switch(image_encode.QuantizationQuality) 
		{
			case 100:
				quality = 100;
				break;

			case 98:
				quality = 98;
				break;	       

			case 95:
				quality = 95;
				break;

			case 93:
				quality = 93;
				break;	       

			case 90:
				quality = 90;
				break;

			case 85:
				quality = 85;
				break;	       

			case 80:
				quality = 80;
				break;

			case 70:
				quality = 70;
				break;	

			case 50:
				quality = 50;
				break;	

			default:
				quality = (INT32U)&image_encode;
				quality |= quant_table_user_define;
				break;           
		}   	        
		
		if(quality > 100)
			image_encode_jpeg_header_generate((INT32U)&image_encode,image_valid_width,image_valid_height);
		else
			image_encode_jpeg_header_generate(quality,image_valid_width,image_valid_height);
		
		if(input_mode == IMAGE_ENCODE_BLOCK_READ_WRITE)
		{
			if(image_encode.OutputFormat == IMAGE_ENCODE_OUTPUT_FORMAT_YUV422){
				 y_addr = image_valid_width / 16;
				 u_addr = (image_encode.BlockLength / 8);
				 v_addr = y_addr*u_addr;
			}else{
				 y_addr = image_valid_width / 16;
				 u_addr = (image_encode.BlockLength / 16);
			     v_addr = y_addr*u_addr;
			}
			image_encode_jpeg_422_header[0x260] = (v_addr >> 8);
			image_encode_jpeg_422_header[0x261] = (v_addr & 0xFF);		
		}
				
		/* Copy header to output buffer */
		gp_memcpy((INT8S *)image_encode.OutputBufPtr, (INT8S *)&image_encode_jpeg_422_header[0], sizeof(image_encode_jpeg_422_header));	
		
		#if _OPERATING_SYSTEM == _OS_UCOS2    
			if(image_encode.FileHandle != -1)
				encode_fd = image_encode.FileHandle;
			else
				encode_fd = -1;	
			use_disk = image_encode.UseDisk;
		#endif
		
		if(input_mode == IMAGE_ENCODE_BLOCK_READ_WRITE)
		{
			if(encode_fd != -1)
			{
				jpeg_status = write(encode_fd, (INT32U)image_encode.OutputBufPtr, sizeof(image_encode_jpeg_422_header));
				if(jpeg_status == sizeof(image_encode_jpeg_422_header))	
					DBG_PRINT("image encode save ok\r\n");			
				else
					DBG_PRINT("image encode save fail\r\n");
			}
			else
				DBG_PRINT("image encode FileHandle fail\r\n");
		}
		
		blocklength = image_encode.BlockLength;
		
		if(blocklength == 0){
		   blocklength = image_encode.ScalerInfoPtr->OutputHeight;
		}
		
 
		if (source_format == C_JPEG_FORMAT_YUYV) {		// YUYV
			y_addr = (INT32U)image_encode.InputBufPtr.yaddr;
			u_addr = 0;
			v_addr = 0;
			if (!y_addr){
			    if(input_mode == IMAGE_ENCODE_ONCE_READ)
			    	y_addr = (INT32U) gp_malloc_align((image_valid_width*image_valid_height)*2, 32); 
			    else
			    	y_addr = (INT32U) gp_malloc_align((image_valid_width*(blocklength + LINE_OFFSET_NUM))*2, 32); 
			    buf_ptr_ck = 1;
			}    
			if (!y_addr) {
				gp_free((void *) y_addr);
				DBG_PRINT("JPEG FORMAT YUYV malloc buffer error!\r\n");
				malloc_error = 1;
			}
		} else if (sampling_mode == C_JPG_CTRL_YUV422) {
            y_addr = (INT32U)image_encode.InputBufPtr.yaddr;
			u_addr = (INT32U)image_encode.InputBufPtr.uaddr;
			v_addr = (INT32U)image_encode.InputBufPtr.vaddr;	
            // Allocate memory for JPEG encode output ptr
            if (!y_addr || !u_addr || !v_addr) {
			   y_addr = (INT32U) gp_malloc_align((image_valid_width * blocklength), 32);
			   u_addr = (INT32U) gp_malloc_align((image_valid_width * blocklength)>>1, 32);
			   v_addr = (INT32U) gp_malloc_align((image_valid_width * blocklength)>>1, 32);
			   buf_ptr_ck = 1;
            }
			if (!y_addr || !u_addr || !v_addr) {
				gp_free((void *) y_addr);
				gp_free((void *) u_addr);
				gp_free((void *) v_addr);
				DBG_PRINT("YUV422 malloc buffer error!\r\n");
				malloc_error = 1;
			}
		} else {		// C_JPG_CTRL_YUV420
			y_addr = (INT32U)image_encode.InputBufPtr.yaddr;
			u_addr = (INT32U)image_encode.InputBufPtr.uaddr;
			v_addr = (INT32U)image_encode.InputBufPtr.vaddr;
			// Allocate memory for JPEG encode output ptr
			if (!y_addr || !u_addr || !v_addr) {
			    y_addr = (INT32U) gp_malloc_align((image_valid_width * blocklength), 32);
			    u_addr = (INT32U) gp_malloc_align((image_valid_width * blocklength)>>2, 32);
			    v_addr = (INT32U) gp_malloc_align((image_valid_width * blocklength)>>2, 32);
			    buf_ptr_ck = 1;
			}
			if (!y_addr || !u_addr || !v_addr) {
				gp_free((void *) y_addr);
				gp_free((void *) u_addr);
				gp_free((void *) v_addr);
				DBG_PRINT("YUV420 malloc buffer error!\r\n");
				malloc_error = 1;
			}
		}
		
		// Allocate memory for JPEG encode output ptr
		encode_output_ptr = (INT32U)(image_encode.OutputBufPtr + sizeof(image_encode_jpeg_422_header));
		
		if(encode_output_ptr == 0){
		    if(input_mode == IMAGE_ENCODE_ONCE_READ)
	      	   encode_output_ptr = (INT32U) gp_malloc_align((image_valid_width * image_valid_height)*2, 32);
	      	else 
	      	   encode_output_ptr = (INT32U) gp_malloc_align((image_valid_width * (blocklength + LINE_OFFSET_NUM)), 32);
	    }  	   
		
		if (encode_output_ptr == 0) {
			DBG_PRINT("encode_output_ptr malloc buffer error!\r\n");
			malloc_error = 1;
		}
		   
		if(malloc_error == 0) {
			if((input_mode == IMAGE_ENCODE_ONCE_READ) || (input_mode == IMAGE_ENCODE_BLOCK_READ)){
			     // Setup JPEG encode engine
			    jpeg_encode_init();			    
			    // gplib jpeg
			    gplib_jpeg_default_quantization_table_load(quality);		// Load default qunatization table(quality=50)
			    gplib_jpeg_default_huffman_table_load();			        // Load default huffman table 
			    jpeg_encode_input_size_set(image_valid_width, image_valid_height);
			    jpeg_encode_input_format_set(source_format);
			    jpeg_encode_yuv_sampling_mode_set(sampling_mode);
			    jpeg_encode_output_addr_set((INT32U) encode_output_ptr);	
		    }
			
			if (input_mode == IMAGE_ENCODE_ONCE_READ) {	// Enocde once
				jpeg_encode_once_start(y_addr, u_addr, v_addr);
				while(1){
					jpeg_status = jpeg_encode_status_query(1);
				  	if (jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
				    	encode_size = jpeg_encode_vlc_cnt_get();		// Get encode length
				      	cache_invalid_range(encode_output_ptr, encode_size);
				      	//image_encode_end(encode_size);
				    	if(pfunc_encode_end) {
				    		pfunc_encode_end(encode_output_ptr,encode_size);
				    	}				                
				      	encode_status = 1;
						if(encode_fd != -1)
						{
							disk_size = vfsFreeSpace(use_disk);
	   						if(disk_size > (encode_size + sizeof(image_encode_jpeg_422_header))){
								jpeg_status = write(encode_fd, (INT32U)image_encode.OutputBufPtr, (encode_size + sizeof(image_encode_jpeg_422_header)));
								if(jpeg_status == (encode_size + sizeof(image_encode_jpeg_422_header)))	
									DBG_PRINT("image encode save ok\r\n");			
								else
									DBG_PRINT("image encode save fail\r\n");
								close(encode_fd);
								encode_fd = -1;
							}
							else
								DBG_PRINT("Disk size is not enough for jpeg encode\r\n");
						}
						else
							DBG_PRINT("image encode FileHandle fail\r\n");
						image_encode.FileHandle = -1;				      	
				      	break;
				  	}else if (jpeg_status & C_JPG_STATUS_STOP) {
				      	DBG_PRINT("JPEG is not started!\r\n");
				      	break;
				  	}else if (jpeg_status & C_JPG_STATUS_TIMEOUT) {
				      	DBG_PRINT("JPEG execution timeout!\r\n");
				      	break;
				  	}else if (jpeg_status & C_JPG_STATUS_INIT_ERR) {
				      	DBG_PRINT("JPEG init error!\r\n");
				      	break;
				  	}else {
				      	DBG_PRINT("jpeg_status = 0x%08x!\r\n", (INT32U)jpeg_status);
				      	DBG_PRINT("JPEG status error!\r\n");
				  	}
				}	
			} else if(input_mode == IMAGE_ENCODE_BLOCK_READ){		// Encode on the fly
	            if(image_encode.ScalerInfoPtr == 0)
	            {
		            y_len = (image_valid_width*blocklength);		  	// Encode 16 lines each time
	  	            if (source_format == C_JPEG_FORMAT_YUYV) {
	  		            y_len <<= 1;
	  		            uv_len = 0;
	  	            } else if (sampling_mode == C_JPG_CTRL_YUV422) {
	  		            uv_len = y_len >> 1;
	  	            } else {		// YUV420
	  		            uv_len = y_len >> 2;
		            }
	  	            
	  	            fly_y_addr = y_addr;
	  	            fly_u_addr = u_addr;
	  	            fly_v_addr = v_addr;
	    			jpeg_encode_on_the_fly_start(fly_y_addr, fly_u_addr, fly_v_addr, y_len+uv_len+uv_len);
	  	            
	  	            while (1) {
	  		            jpeg_status = jpeg_encode_status_query(1);
		                if (jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			                encode_size = jpeg_encode_vlc_cnt_get();	// Get encode length
			                cache_invalid_range(encode_output_ptr, encode_size);
					    	if(pfunc_encode_end) {
					    		pfunc_encode_end(encode_output_ptr,encode_size);
					    	}				                
			                encode_status = 1;
			                BLOCK_READ = 0;
							if(encode_fd != -1)
							{
								disk_size = vfsFreeSpace(use_disk);
		   						if(disk_size > (encode_size + sizeof(image_encode_jpeg_422_header))){
									jpeg_status = write(encode_fd, (INT32U)image_encode.OutputBufPtr, (encode_size + sizeof(image_encode_jpeg_422_header)));
									if(jpeg_status == (encode_size + sizeof(image_encode_jpeg_422_header)))	
										DBG_PRINT("image encode save ok\r\n");			
									else
										DBG_PRINT("image encode save fail\r\n");
									close(encode_fd);
									encode_fd = -1;
								}
								else
									DBG_PRINT("Disk size is not enough for jpeg encode\r\n");
							}
							else
								DBG_PRINT("image encode FileHandle fail\r\n");
							image_encode.FileHandle = -1;						                
			                break;
		                }				
  		                if (jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			                // Prepare new data for JPEG encoding
			                fly_y_addr += y_len;
			                fly_u_addr += uv_len;
			                fly_v_addr += uv_len;
			                encode_count++;
			                DBG_PRINT("encode_count = 0x%08x!\r\n", (INT32U)encode_count);
			                // Now restart JPEG encoding on the fly
	  		                if (jpeg_encode_on_the_fly_start(fly_y_addr, fly_u_addr, fly_v_addr,  y_len+uv_len+uv_len)) {
	  		                  	DBG_PRINT("Failed to call jpeg_encode_on_the_fly_start()\r\n");
	  			                break;
	  		                }      
		                }		// if (jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
		                if (jpeg_status & C_JPG_STATUS_STOP) {
			                DBG_PRINT("JPEG is not started!\r\n");
			                break;
		                }
		                if (jpeg_status & C_JPG_STATUS_TIMEOUT) {
			                DBG_PRINT("JPEG execution timeout!\r\n");
			                break;
		                }
		                if (jpeg_status & C_JPG_STATUS_INIT_ERR) {
		 	                DBG_PRINT("JPEG init error!\r\n");
			                break;
		                }
		            }
				}else{
					y_len = (image_valid_width * blocklength);		  		// Encode 16 lines each time
	  	            if (source_format == C_JPEG_FORMAT_YUYV) {
	  		            y_len <<= 1;
	  		            uv_len = 0;
	  	            } else if (sampling_mode == C_JPG_CTRL_YUV422) {
	  		            uv_len = y_len >> 1;
	  	            } else {		// YUV420
	  		            uv_len = y_len >> 2;
		            }
	  	           	
	  	            block_encode_ptr.yaddr = (INT8U *)y_addr;
		            block_encode_ptr.uaddr = (INT8U *)u_addr;
		            block_encode_ptr.vaddr = (INT8U *)v_addr;
	  	            
	  	            if(pfunc_encode_block_read) {
						pfunc_encode_block_read((IMAGE_ENCODE_ARGUMENT *)&image_encode,(IMAGE_ENCODE_PTR *)&block_encode_ptr);
				    }
				    else
				    {
				    	malloc_error = 1;
				    	goto JPEG_END;
		            }
		            
		            encode_count++;
		            DBG_PRINT("encode_count = 0x%08x!\r\n", (INT32U)encode_count);  	            
	  	            fly_y_addr = (INT32U)block_encode_ptr.yaddr;
	  	            fly_u_addr = (INT32U)block_encode_ptr.uaddr;
	  	            fly_v_addr = (INT32U)block_encode_ptr.vaddr;
		            jpeg_encode_on_the_fly_start(fly_y_addr, fly_u_addr, fly_v_addr, (y_len+uv_len+uv_len));
	  	            
	  	            while (1) {
						jpeg_status = jpeg_encode_status_query(1);
						if (jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
							encode_size = jpeg_encode_vlc_cnt_get();	// Get encode length
							cache_invalid_range(encode_output_ptr, encode_size);
					    	if(pfunc_encode_end) {
					    		pfunc_encode_end(encode_output_ptr,encode_size);
					    	}
							encode_status = 1;
							BLOCK_READ = 0;
							if(encode_fd != -1)
							{
								disk_size = vfsFreeSpace(use_disk);
		   						if(disk_size > (encode_size + sizeof(image_encode_jpeg_422_header))){
									jpeg_status = write(encode_fd, (INT32U)image_encode.OutputBufPtr, (encode_size + sizeof(image_encode_jpeg_422_header)));
									if(jpeg_status == (encode_size + sizeof(image_encode_jpeg_422_header)))	
										DBG_PRINT("image encode save ok\r\n");			
									else
										DBG_PRINT("image encode save fail\r\n");
									close(encode_fd);
									encode_fd = -1;
								}
								else
									DBG_PRINT("Disk size is not enough for jpeg encode\r\n");
							}
							else
								DBG_PRINT("image encode FileHandle fail\r\n");
							image_encode.FileHandle = -1;								
							break;
						}
						if (jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
							// Prepare new data for JPEG encoding
			  	            if(pfunc_encode_block_read) {
								pfunc_encode_block_read((IMAGE_ENCODE_ARGUMENT *)&image_encode,(IMAGE_ENCODE_PTR *)&block_encode_ptr);
						    }
						    else
						    {
						    	malloc_error = 1;
						    	goto JPEG_END;
		  		            }						
							
							encode_count++;
							DBG_PRINT("encode_count = 0x%08x!\r\n", (INT32U)encode_count);
							// Now restart JPEG encoding on the fly
							if (jpeg_encode_on_the_fly_start(fly_y_addr, fly_u_addr, fly_v_addr, (y_len+uv_len+uv_len))) {
								DBG_PRINT("Failed to call jpeg_encode_on_the_fly_start()\r\n");
								break;
							}
						}		// if (jpeg_status & C_JPG_STATUS_INPUT_EMPTY)
						if (jpeg_status & C_JPG_STATUS_STOP) {
							DBG_PRINT("JPEG is not started!\r\n");
							break;
						}
						if (jpeg_status & C_JPG_STATUS_TIMEOUT) {
							DBG_PRINT("JPEG execution timeout!\r\n");
							break;
						}
						if (jpeg_status & C_JPG_STATUS_INIT_ERR) {
							DBG_PRINT("JPEG init error!\r\n");
							break;
						}
		            }	
	            }	                
  			} else if(input_mode == IMAGE_ENCODE_BLOCK_READ_WRITE && task_create) {
				block_wr_num = (image_valid_height / blocklength) + 1;
				header_ck = 0xD0;
				ck_count = 0;
				buffer_chage = 0;
				OutputBufPtr_1 = encode_output_ptr;
				OutputBufPtr_2 = (INT32U) gp_malloc_align((image_valid_width * blocklength),32);
				if (OutputBufPtr_2 == 0) {
				 	malloc_error = 1;
				}
				if(malloc_error == 0) {
	        		for(i=0;i<block_wr_num;i++) { 	
	  		            // Setup JPEG encode engine 
		                jpeg_encode_init();
					    gplib_jpeg_default_quantization_table_load(quality);		// Load default qunatization table(quality=50)
		                gplib_jpeg_default_huffman_table_load();			        // Load default huffman table 
	                    jpeg_encode_input_size_set(image_valid_width, blocklength);
		                jpeg_encode_input_format_set(source_format);	
		                jpeg_encode_yuv_sampling_mode_set(sampling_mode);
		                jpeg_encode_output_addr_set((INT32U) encode_output_ptr);
	  		            block_encode_ptr.yaddr = (INT8U *)y_addr;
			            block_encode_ptr.uaddr = (INT8U *)u_addr;
			            block_encode_ptr.vaddr = (INT8U *)v_addr;
		  	            
		  	            if(pfunc_encode_block_read) {
							pfunc_encode_block_read((IMAGE_ENCODE_ARGUMENT *)&image_encode,(IMAGE_ENCODE_PTR *)&block_encode_ptr);
					    }
					    else
					    {
					    	malloc_error = 1;
					    	goto JPEG_END;
	  		            }
	  		            
	  		            jpeg_encode_once_start(y_addr, u_addr, v_addr);
			            while(1){
							jpeg_status = jpeg_encode_status_query(1);
							if (jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
							   encode_size = jpeg_encode_vlc_cnt_get();		// Get encode length
							   cache_invalid_range(encode_output_ptr, encode_size);
							   jpeg_encode_stop();
							   encode_count++;
							   DBG_PRINT("encode_count = 0x%08x!\r\n", (INT32U)encode_count);
							   break;
							} else if (jpeg_status & C_JPG_STATUS_STOP) {
							   DBG_PRINT("JPEG is not started!\r\n");
							   break;
							} else if (jpeg_status & C_JPG_STATUS_TIMEOUT) {
							   DBG_PRINT("JPEG execution timeout!\r\n");
							   break;
							} else if (jpeg_status & C_JPG_STATUS_INIT_ERR) {
							   DBG_PRINT("JPEG init error!\r\n");
							   break;
							} else {
							   DBG_PRINT("jpeg_status = 0x%08x!\r\n", (INT32U)jpeg_status);
							   DBG_PRINT("JPEG status error!\r\n");
							}
		                }
		                image.encode_buffer_ptr = encode_output_ptr;
		                image.encode_size = encode_size;
	  	              #if _OPERATING_SYSTEM == _OS_UCOS2 
	  	                if(encode_msg_queue_id)
	  	                	msgQSend(encode_msg_queue_id, BLOCK_WRITE_START, (void *)&image,  sizeof(ENCODE_BLOCK_WRITE_ARGUMENT), MSG_PRI_NORMAL);   
		                sum_encode_size += encode_size;
		                if(ApQ)
		                	msgQReceive(ApQ, &msg_id, (void *) &block_write_para, sizeof(ENCODE_BLOCK_WRITE_ARGUMENT));  
		              #else
					    malloc_error = 1;
					    goto JPEG_END;		                
		              #endif
		                if(msg_id == START_OK) {
		                   if(buffer_chage == 0) {
		                      encode_output_ptr = OutputBufPtr_2;
		                      buffer_chage = 1;
		                   } else {
		                      encode_output_ptr = OutputBufPtr_1;
		                      buffer_chage = 0;
		                   } 
		                } else {
		                   DBG_PRINT("encode block write status error!\r\n");
		                   break;
		                }      
		        	}
		  	    }else{
		  	    	DBG_PRINT("OutputBufPtr_2 malloc buffer error!\r\n");
		  	    }
			}
			else
				DBG_PRINT("encode state error!\r\n");
		}
	JPEG_END:
		if((input_mode == IMAGE_ENCODE_ONCE_READ) || (input_mode==IMAGE_ENCODE_BLOCK_READ)) {
			jpeg_encode_stop();
		}
		
		if(input_mode == IMAGE_ENCODE_BLOCK_READ_WRITE) {
	    	if(pfunc_encode_end) {
	    		pfunc_encode_end(encode_output_ptr,sum_encode_size);
	    	}			
			encode_status = 1;
		   	BLOCK_READ = 0;	   	
		   	gp_free((void *)OutputBufPtr_2);
		}
		
		if(buf_ptr_ck == 1) {
			if(y_addr) 
				gp_free((void *) y_addr);
		   	if(u_addr) 
		   		gp_free((void *) u_addr);
		   	if(v_addr) 
		   		gp_free((void *) v_addr);
		}
		
		process_status = 0;
		
		if(malloc_error == 1) {
		    jpeg_status = RESOURCE_NO_FOUND_ERROR; 
		} else {    
		    if(input_mode == IMAGE_ENCODE_BLOCK_READ_WRITE) {
		       jpeg_status = sum_encode_size;
		    } else {
		       jpeg_status = encode_size;
		    }   
		} 
		 
	} else {
		jpeg_status = REENTRY_ERROR;
  	}
  	
  	image_enc_unlock();
  	
  	return jpeg_status;	 
}

#if _OPERATING_SYSTEM == _OS_UCOS2
static CODEC_START_STATUS image_encode_block_write(ENCODE_BLOCK_WRITE_ARGUMENT *encode_info)
{
	  INT8U *ptr_ck;
	  INT32U size_ck,encode_size_ck=0,i,write_state_error;
	  INT64U disk_size;            
	  	                   
	  ptr_ck = (INT8U *)encode_info->encode_buffer_ptr;
	  size_ck = encode_info->encode_size % 4;      	                                      	                                      	                                      	                             
	  ptr_ck += (encode_info->encode_size - 2);
	  write_state_error = 0;
	  
	  if(ck_count++ < block_wr_num){
          if(size_ck == 1){
              for(i=0;i<3;i++){
                  *ptr_ck = 0x0;
                  ptr_ck++;
               }
              *ptr_ck = 0xFF;
              ptr_ck++;
              *ptr_ck = header_ck;
              encode_size_ck = encode_info->encode_size + 3;
          }else if(size_ck == 2){
              for(i=0;i<2;i++){
                  *ptr_ck = 0x0;
                  ptr_ck++;
              }
              *ptr_ck = 0xFF;
              ptr_ck++;
              *ptr_ck = header_ck;
              encode_size_ck = encode_info->encode_size + 2;
          }else{ 
              for(i=0;i<5;i++){
                  *ptr_ck=0x0;
                  ptr_ck++;
              }         
              *ptr_ck = 0xFF;
              ptr_ck++;
              *ptr_ck = header_ck;
              encode_size_ck = encode_info->encode_size + 5;
          }
      }
        
	  if (encode_fd >= 0) {
		   disk_size = vfsFreeSpace(use_disk);
		   if(disk_size > encode_size_ck){
		     write(encode_fd, encode_info->encode_buffer_ptr, encode_size_ck);
		   }else{
		     write_state_error = 1;
		     DBG_PRINT("sd card free size error!\r\n");
		   }  
	  }else{
	       DBG_PRINT("sd card write error!\r\n");
	       write_state_error = 1;
	  }
	  
	  if(header_ck++ > 0xD6)
		 header_ck = 0xD0;

	  if(ck_count == block_wr_num)
	  {
	      close(encode_fd);
	      encode_fd = -1;       	     
      }
      if(write_state_error == 0)
         return START_OK;
      else
         return RESOURCE_WRITE_ERROR; 	 
}

static void image_block_write_task_entry(void *p_arg)
{
    INT32U block_write_state,msg_id;
    
    encode_msg_queue_id = msgQCreate(ENCODE_QUEUE_MAX_NUMS, ENCODE_MAXIMUM_MESSAGE_NUM, ENCODE_MAX_MSG_SIZE);
    if(encode_msg_queue_id == 0)
    	DBG_PRINT("encode_msg_queue_id malloc buffer error!\r\n");
    
    while(1)
    {
         if(encode_msg_queue_id)
         {
	         msgQReceive(encode_msg_queue_id, &msg_id, (void *) block_write_para, ENCODE_MAX_MSG_SIZE);
	         if(msg_id == BLOCK_WRITE_START){
	            block_write_state = image_encode_block_write((ENCODE_BLOCK_WRITE_ARGUMENT *)block_write_para);
	            if(ApQ)
	            	msgQSend(ApQ, block_write_state, (void *) block_write_para, sizeof(ENCODE_BLOCK_WRITE_ARGUMENT), MSG_PRI_NORMAL);
	         }
         }
         OSTimeDly(1);
    }
}
#endif

void image_encode_stop(void)
{
    image_enc_lock();
    jpeg_encode_stop(); 
    image_enc_unlock(); 
}

IMAGE_CODEC_STATUS image_encode_status(void)
{
    INT32S temp = IMAGE_CODEC_DECODE_FAIL;
    
    image_enc_lock();
	if(encode_status == 1)
	{
	   encode_status = 0;  
	   temp = IMAGE_CODEC_DECODE_END;
	}
	image_enc_unlock(); 
	  
    return temp;     
}
