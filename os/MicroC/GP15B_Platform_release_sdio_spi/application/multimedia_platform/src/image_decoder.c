#include "image_Decoder.h"
#include "msg_manager.h"
#include "drv_l1_scaler.h"
#include "drv_l1_jpeg.h"
#include "drv_l1_dma.h"
#include "drv_l2_scaler.h"

#define API_DO_JPEG_EN					0
#define API_SCALER_SOFTWARE_EN 			1
#define JPEG_DEC_PIECE_SIZE 			65536

#define C_USE_SCALER_NUMBER				SCALER_0 
#define SCALER_BLOCK_START				0x11
#define SCALER_SIZE_2          			2
#define SCALER_SIZE_4         			4
#define SCALER_SIZE_8          			8
#define SCALER_8_MULTIPLE     			0xF
#define SCALER_2_MULTIPLE     			0x1
#define IMAGE_DECODE_TASK_PARA 			17

extern MSG_Q_ID ApQ;
extern MSG_Q_ID image_msg_queue_id;
extern OS_EVENT *image_task_fs_queue_a;
extern OS_EVENT *image_task_fs_queue_b;
extern TK_IMAGE_TYPE_ENUM image_file_judge_type(INT16S fd);

static OS_EVENT *image_dec_sw_sem = NULL;
static INT32U image_decode_task_para[IMAGE_DECODE_TASK_PARA];
static IMAGE_DECODE_STRUCT image_decode_struct;
static IMAGE_SCALE_STRUCT  image_scale_struct;
static IMAGE_HEADER_PARSE_STRUCT header_parse_struct;
static INT32U image_decode_ck,image_scaler_ck,image_block_encode_scaler_ck,image_decode_info_ck;

#if	API_DO_JPEG_EN == 1
static INT32S video_decode_jpeg_as_gp420_by_piece(INT16S fd, INT32U file_size, INT32U output_width, INT32U output_height, INT32U output_addr);
#endif

//=============================================================================

static INT32U (*pfunc_decode_end)(INT32U decode_buffer);

// register encode end function.
void image_decode_end_register_callback(INT32U (*func)(INT32U decode_buffer))
{
	if(func) {
		pfunc_decode_end = func;
	}
}

//============================== Image Decoder ================================
//
// including JPEG, JPEG Progressive,Single Picture from Motion JPEG, BMP, GIF .....
//
//=============================================================================
static void image_dec_lock(void)
{
	INT8U err;
	
	if(image_dec_sw_sem)
		OSSemPend(image_dec_sw_sem, 0, &err);
}

static void image_dec_unlock(void)
{
	if(image_dec_sw_sem)
		OSSemPost(image_dec_sw_sem);
}

void image_decode_entrance(void)
{        
    INT8S *pimage;
    
    image_decode_ck = 0;
    image_scaler_ck = 0;
    image_block_encode_scaler_ck = 0;
    image_decode_info_ck = 0;
	
	if(image_dec_sw_sem == NULL)
	{
	  #if _OPERATING_SYSTEM == _OS_UCOS2
		image_dec_sw_sem = OSSemCreate(1);
	  #endif
		if(image_dec_sw_sem == 0)
			DBG_PRINT("Failed to allocate obj_sw_sem memory\r\n");	
	}    
    
    pimage=(INT8S *)&image_decode_struct;
    gp_memset(pimage,0,sizeof(IMAGE_DECODE_STRUCT)); 
    
    pimage=(INT8S *)&image_scale_struct;
    gp_memset(pimage,0,sizeof(IMAGE_SCALE_STRUCT)); 
    
    pimage=(INT8S *)&header_parse_struct;
    gp_memset(pimage,0,sizeof(IMAGE_HEADER_PARSE_STRUCT));             
}

void image_decode_exit(void)
{
	INT8U err;
	if(image_dec_sw_sem)
		OSSemDel(image_dec_sw_sem, OS_DEL_ALWAYS, &err);
	image_dec_sw_sem = NULL;
#if 0	
	OSTaskDel(TSK_PRI_IMAGE);
	msgQDelete(image_msg_queue_id);
	OSQDel(image_task_fs_queue_a,OS_DEL_ALWAYS, &err);
	OSQDel(image_task_fs_queue_b,OS_DEL_ALWAYS, &err);
#endif	
	
}

CODEC_START_STATUS image_decode_Info(IMAGE_INFO *info,MEDIA_SOURCE src)
{
    INT32S state;
    INT32U msg_id,image_decode_Info_state;
    IMAGE_INFO *image_info;
    MEDIA_SOURCE SOURCE;
    struct sfn_info file_info;
    IMAGE_HEADER_PARSE_STRUCT *image_header_parse;
    IMAGE_HEADER_PARSE_STRUCT image_header_Info;
    
    image_dec_lock();
    if(image_decode_info_ck == 0){
       image_decode_info_ck = 1;
	   image_info = info;
	   SOURCE = src;
	   image_header_Info = header_parse_struct;       
       switch (image_file_judge_type((INT16S)SOURCE.type_ID.FileHandle))
       {
          case TK_IMAGE_TYPE_JPEG:
               image_info->Format = JPEG;
               break; 
          
          case TK_IMAGE_TYPE_MOTION_JPEG: 
               image_info->Format = MJPEG_S;
               break;
          
          case TK_IMAGE_TYPE_BMP: 
               image_info->Format = BMP;
               break;
          
          case TK_IMAGE_TYPE_GIF: 
               image_info->Format = GIF;
               break;
          
          default: 
               image_info->Format = TK_IMAGE_TYPE_MAX;
               break;             
       }
   
       image_decode_Info_state = sfn_stat((INT16S)SOURCE.type_ID.FileHandle, &file_info);
       
       if(image_decode_Info_state == 0){
          image_info->FileSize = file_info.f_size;
          lseek(SOURCE.type_ID.FileHandle,0,0);
          image_header_Info.image_source = (INT32S)SOURCE.type_ID.FileHandle;
          image_header_Info.source_type = SOURCE.type;
          image_header_Info.cmd_id = 1;
          image_header_Info.source_size=image_info->FileSize;
         #if _OPERATING_SYSTEM == _OS_UCOS2
          msgQSend(image_msg_queue_id, MSG_IMAGE_PARSE_HEADER_REQ, (void *)&image_header_Info,  sizeof(IMAGE_HEADER_PARSE_STRUCT), MSG_PRI_NORMAL);
          msgQReceive(ApQ, &msg_id, (void *)image_decode_task_para, sizeof(IMAGE_HEADER_PARSE_STRUCT));
         #endif
          if(msg_id == MSG_IMAGE_PARSE_HEADER_REPLY)
          {
	          image_header_parse = (IMAGE_HEADER_PARSE_STRUCT *)image_decode_task_para;
	          if(image_header_parse->parse_status == 0){
	             image_info->Width = (INT32U)image_header_parse->width;
	             image_info->Height = (INT32U)image_header_parse->height;
	             lseek(SOURCE.type_ID.FileHandle,0,0);
	             image_decode_info_ck = 0;
	             state = START_OK;
	          }else{
	             image_decode_info_ck = 0;
	             state = RESOURCE_READ_ERROR;
	          }
          }else{
          	  image_decode_info_ck = 0;
          	  state = RESOURCE_READ_ERROR;
          }
       }else{
          image_decode_info_ck = 0;
          state = RESOURCE_READ_ERROR; 
       }      
    }else{
       state = REENTRY_ERROR;
    }
    image_dec_unlock();
    
    return state;    
}

CODEC_START_STATUS image_decode_start(IMAGE_ARGUMENT arg,MEDIA_SOURCE src)
{
    INT32U fs_state,temp_state;
    IMAGE_ARGUMENT image_arg;
    IMAGE_DECODE_STRUCT image;
    MEDIA_SOURCE SOURCE;
    struct stat_t jpg_info;
    
    image_dec_lock();
    #if API_DO_JPEG_EN == 1
    	image = image_decode_struct;
    	jpg_info.st_size = 0;
    	image_arg = arg;
    	SOURCE = src;
    	fs_state = lseek(SOURCE.type_ID.FileHandle, 0, SEEK_END);
		lseek(SOURCE.type_ID.FileHandle, 0, SEEK_SET);
    	temp_state = video_decode_jpeg_as_gp420_by_piece(SOURCE.type_ID.FileHandle,fs_state,(INT32U)image_arg.OutputBufWidth,(INT32U)image_arg.OutputBufHeight,(INT32U)image_arg.OutputBufPtr);
    #else
	    if(image_decode_ck == 0){
	       image_decode_ck = 1;
	       image = image_decode_struct;
	       image_arg = arg;
	       SOURCE = src;
	       temp_state = START_OK;
	       image.source_type = SOURCE.type;
	       
	       if(image.source_type == SOURCE_TYPE_SDRAM){
		     image.image_source = (INT32S)SOURCE.type_ID.memptr;
		     image.source_size = (image_arg.OutputWidth*image_arg.OutputHeight)*2;     //jpg file size
	       }else{
	         image.image_source = (INT32S)SOURCE.type_ID.FileHandle;
	         fs_state=fstat((INT16S)SOURCE.type_ID.FileHandle,&jpg_info);              //get image file infomation
		     if(fs_state == 0)
		        image.source_size = jpg_info.st_size;                                  //jpg file size
		     else
		        image.source_size = 0x3fffff;                                          //jpg file size  
		   }
		   
	       image.output_buffer_pointer = (INT32U)image_arg.OutputBufPtr;
	       image.output_buffer_width = (INT16U)image_arg.OutputBufWidth;
	       image.output_buffer_height = (INT16U)image_arg.OutputBufHeight;
	       image.output_image_width = (INT16U)image_arg.OutputWidth;
	       image.output_image_height = (INT16U)image_arg.OutputHeight;
		   
		   switch(image_arg.OutputFormat)
	       {          
	             case IMAGE_OUTPUT_FORMAT_RGB565:
	                  image.output_format = (INT8U)C_SCALER_CTRL_OUT_RGB565;
	                  break;           
	            
	             case IMAGE_OUTPUT_FORMAT_YUYV:
	                  image.output_format = (INT8U)C_SCALER_CTRL_OUT_YUYV;
	                  break;           
	             
	             case IMAGE_OUTPUT_FORMAT_UYVY:
	                  image.output_format = (INT8U)C_SCALER_CTRL_OUT_UYVY;
	                  break; 
	       		 
	       		 default:
	       			 temp_state = RESOURCE_NO_FOUND_ERROR;
	       			 break;                
	       }
		   
		   if(temp_state != RESOURCE_NO_FOUND_ERROR)
		   {
		   	   if(image_arg.ScalerOutputRatio != FIT_OUTPUT_SIZE) {
		   		  DBG_PRINT("GPL32700 Only Support FIT_OUTPUT_SIZE!!!\r\n");		
		   		  temp_state = CODEC_START_STATUS_ERROR_MAX; 
		   	   }		       
		       image.output_ratio = image_arg.ScalerOutputRatio;
		       image.out_of_boundary_color = image_arg.OutBoundaryColor;
		       image.cmd_id = 1;
		      #if _OPERATING_SYSTEM == _OS_UCOS2
		       //SEND MSG
		       msgQSend(image_msg_queue_id, MSG_IMAGE_DECODE_REQ, (void *)&image,  sizeof(IMAGE_DECODE_STRUCT), MSG_PRI_NORMAL);
		       temp_state = START_OK;        
		      #else
		       temp_state = RESOURCE_NO_FOUND_ERROR; 
		      #endif
	       }
	    }else{   
	       temp_state = RESOURCE_NO_FOUND_ERROR;  
	    }
    #endif
    image_dec_unlock();
    
    return temp_state;
}

void image_decode_stop(void)
{
     IMAGE_DECODE_STRUCT image;
     INT32U msg_id;
     
     image_dec_lock();
	 image = image_decode_struct;
	 image.cmd_id = 1;
	 image_decode_ck = 0;
	#if _OPERATING_SYSTEM == _OS_UCOS2
	 msgQSend(image_msg_queue_id, MSG_IMAGE_STOP_REQ, (void *)&image,  sizeof(IMAGE_DECODE_STRUCT), MSG_PRI_NORMAL);
	 msgQReceive(ApQ, &msg_id, (void *)image_decode_task_para, sizeof(IMAGE_DECODE_STRUCT));
    #endif
     image_dec_unlock();
}
																
IMAGE_CODEC_STATUS image_decode_status(void)
{   
   INT32U msg_id,photo_status;
   IMAGE_DECODE_STRUCT *image;

   //RECEIVE MSG
   image_dec_lock();
   #if _OPERATING_SYSTEM == _OS_UCOS2
	   if(image_decode_ck)
	   {
		   msgQReceive(ApQ, &msg_id, (void *) &image_decode_task_para, sizeof(IMAGE_DECODE_STRUCT));
		   if(msg_id == MSG_IMAGE_DECODE_REPLY){  
		      image_decode_ck = 0;
		      image = (IMAGE_DECODE_STRUCT *)image_decode_task_para;
		      if((image->decode_status) != 0){
		        photo_status = IMAGE_CODEC_DECODE_FAIL; 
		      }else{
		      	if(pfunc_decode_end){
		      		pfunc_decode_end((INT32U)image->output_buffer_pointer);
		        }
		        photo_status = IMAGE_CODEC_DECODE_END; 
		      }
		   }else{     
		        photo_status = IMAGE_CODEC_STATUS_MAX;
		   }     
	   }
	   else
	       photo_status = IMAGE_CODEC_STATUS_MAX;
   #else
   	   photo_status = IMAGE_CODEC_STATUS_MAX;
   #endif    
   image_dec_unlock();
   
   return photo_status;             
}

CODEC_START_STATUS image_scale_start(IMAGE_SCALE_ARGUMENT arg)
{     	
	INT32U scaler_state;
	INT32U malloc_buffer_error,scaler_format_error;
	IMAGE_SCALE_STRUCT image_scaler;
	IMAGE_SCALE_ARGUMENT image_scaler_arg;
#if API_SCALER_SOFTWARE_EN == 1	
	DMA_STRUCT dma_struct;
	INT8S done1;
	INT16U in_scaler_x,out_scaler_x,SCALER_NUM,i,j;
	INT32U scaler_buffer_1=0,scaler_buffer_2=0,scaler_buffer_3=0,scaler_buffer_4=0,scaler_buffer_5=0,scaler_buffer_6=0,scaler_buffer_7=0,scaler_buffer_8=0;
	INT32U S_Buf_1=0,S_Buf_2=0,S_Buf_3=0,S_Buf_4=0,S_Buf_5=0,S_Buf_6=0,S_Buf_7=0,S_Buf_8=0,T_Buf,line_offset,dma_move_size;
	INT32S result;
#else
	ScalerFormat_t src;
#endif
   
    image_dec_lock();
    if(image_scaler_ck == 0){
    	image_scaler_ck = 1;
        malloc_buffer_error = 0;
        scaler_format_error = 0;
        image_scaler = image_scale_struct;
        image_scaler_arg = arg;

        //Software scaler function Supported the scaler x size to exceed 1024 pixel. 
        if((image_scaler_arg.OutputBufWidth > 1024) || (image_scaler_arg.OutputWidth > 1024)){
           #if API_SCALER_SOFTWARE_EN == 0
				src.input_format = image_scaler_arg.InputFormat;
				src.input_width = image_scaler_arg.InputWidth;
				src.input_height = image_scaler_arg.InputHeight;
				src.input_visible_width = image_scaler_arg.InputVisibleWidth;
				src.input_visible_height = image_scaler_arg.InputVisibleHeight;	
				src.input_x_offset = image_scaler_arg.InputOffsetX;
				src.input_y_offset = image_scaler_arg.InputOffsetY;	
				src.input_addr_y = (INT32U)image_scaler_arg.InputBufPtr;
				src.input_addr_u = 0;
				src.input_addr_v = 0;
				src.output_format = image_scaler_arg.OutputFormat;
				src.output_width = image_scaler_arg.OutputWidth;
				src.output_height = image_scaler_arg.OutputHeight;
				src.output_buf_width = image_scaler_arg.OutputWidth;
				src.output_buf_height = image_scaler_arg.OutputHeight;	
				src.output_addr_y = (INT32U)image_scaler_arg.OutputBufPtr;
				src.output_x_offset = 0;
				src.output_addr_u = 0;
				src.output_addr_v = 0;	
				src.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;	
				src.scale_mode = C_SCALER_FULL_SCREEN_BY_RATIO;
				src.digizoom_m = 10;	
				src.digizoom_n = 10;
				scaler_state = drv_l2_scaler_trigger(C_USE_SCALER_NUMBER, 1, (ScalerFormat_t *)&src, image_scaler_arg.OutBoundaryColor);           
           #else
	           image_scaler_ck = SCALER_BLOCK_START;
	           if((image_scaler_arg.OutputWidth > 1024)&&(image_scaler_arg.OutputWidth <= 2048)){
	               SCALER_NUM = SCALER_SIZE_2;
	               if (image_scaler_arg.InputWidth & SCALER_2_MULTIPLE) {
	                   image_scaler_arg.InputWidth = (image_scaler_arg.InputWidth + SCALER_2_MULTIPLE) & ~SCALER_2_MULTIPLE;
				       DBG_PRINT(" scaler input Width Size error!\r\n");
	               }
	               
	               in_scaler_x = (image_scaler_arg.InputWidth / 2);
	               if (image_scaler_arg.OutputWidth & SCALER_8_MULTIPLE) {
	                   image_scaler_arg.OutputWidth = (image_scaler_arg.OutputWidth + SCALER_8_MULTIPLE) & ~SCALER_8_MULTIPLE;
				       DBG_PRINT(" scaler Output Width Size error!\r\n");
			       }
	               
	               out_scaler_x = (image_scaler_arg.OutputWidth / 2);// Make sure output width is at least multiple of 8 pixels
	               scaler_buffer_1 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_2 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               S_Buf_1 = scaler_buffer_1;
	               S_Buf_2 = scaler_buffer_2;
	               if(!scaler_buffer_1 || !scaler_buffer_2){
						 gp_free((void *) scaler_buffer_1);
						 gp_free((void *) scaler_buffer_2);
						 DBG_PRINT(" 1024 < scaler size <=2048 malloc buffer error!\r\n");
						 malloc_buffer_error = 1;
				   }
	           }else if((image_scaler_arg.OutputWidth > 2048)&&(image_scaler_arg.OutputWidth <= 4096)){
	               SCALER_NUM = SCALER_SIZE_4;
	               if (image_scaler_arg.InputWidth & SCALER_2_MULTIPLE) {
	                   image_scaler_arg.InputWidth = (image_scaler_arg.InputWidth + SCALER_2_MULTIPLE) & ~SCALER_2_MULTIPLE;
				       DBG_PRINT(" scaler input Width Size error!\r\n");
	               }               
	               
	               in_scaler_x = (image_scaler_arg.InputWidth / 4);
	               if (image_scaler_arg.OutputWidth & SCALER_8_MULTIPLE) {
				       image_scaler_arg.OutputWidth = (image_scaler_arg.OutputWidth + SCALER_8_MULTIPLE) & ~SCALER_8_MULTIPLE;
				       DBG_PRINT(" scaler Output Width Size error!\r\n");
			       }               
	               
	               out_scaler_x = (image_scaler_arg.OutputWidth / 4);// Make sure output width is at least multiple of 8 pixels
	               scaler_buffer_1 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_2 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_3 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_4 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               S_Buf_1 = scaler_buffer_1;
	               S_Buf_2 = scaler_buffer_2;
	               S_Buf_3 = scaler_buffer_3;
	               S_Buf_4 = scaler_buffer_4;
	               if(!scaler_buffer_1 || !scaler_buffer_2 || !scaler_buffer_3 || !scaler_buffer_4){
						 gp_free((void *) scaler_buffer_1);
						 gp_free((void *) scaler_buffer_2);
						 gp_free((void *) scaler_buffer_3);
						 gp_free((void *) scaler_buffer_4);
						 DBG_PRINT(" 2048 < scaler size <=4096 malloc buffer error!\r\n");
						 malloc_buffer_error = 1;
				   }
	           }else{
	               SCALER_NUM = SCALER_SIZE_8;
	               if (image_scaler_arg.InputWidth & SCALER_2_MULTIPLE) {
	                   image_scaler_arg.InputWidth = (image_scaler_arg.InputWidth + SCALER_2_MULTIPLE) & ~SCALER_2_MULTIPLE;
				       DBG_PRINT(" scaler input Width Size error!\r\n");
	               }
	               
	               in_scaler_x = (image_scaler_arg.InputWidth / 8);
	               if (image_scaler_arg.OutputWidth & SCALER_8_MULTIPLE) {
	                   image_scaler_arg.OutputWidth = (image_scaler_arg.OutputWidth + SCALER_8_MULTIPLE) & ~SCALER_8_MULTIPLE;
				       DBG_PRINT(" scaler Output Width Size error!\r\n");
			       }               
	               
	               out_scaler_x = (image_scaler_arg.OutputWidth / 8);// Make sure output width is at least multiple of 8 pixels               
	               scaler_buffer_1 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_2 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_3 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_4 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_5 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_6 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_7 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               scaler_buffer_8 = (INT32U) gp_malloc_align((out_scaler_x*image_scaler_arg.OutputHeight)*2, 16);
	               S_Buf_1 = scaler_buffer_1;
	               S_Buf_2 = scaler_buffer_2;
	               S_Buf_3 = scaler_buffer_3;
	               S_Buf_4 = scaler_buffer_4;
	               S_Buf_5 = scaler_buffer_5;
	               S_Buf_6 = scaler_buffer_6;
	               S_Buf_7 = scaler_buffer_7;
	               S_Buf_8 = scaler_buffer_8;  
	               if(!scaler_buffer_1 || !scaler_buffer_2 || !scaler_buffer_3 || !scaler_buffer_4 || !scaler_buffer_5 || !scaler_buffer_6 || !scaler_buffer_7 || !scaler_buffer_8){
						 gp_free((void *) scaler_buffer_1);
						 gp_free((void *) scaler_buffer_2);
						 gp_free((void *) scaler_buffer_3);
						 gp_free((void *) scaler_buffer_4);
						 gp_free((void *) scaler_buffer_5);
						 gp_free((void *) scaler_buffer_6);
						 gp_free((void *) scaler_buffer_7);
						 gp_free((void *) scaler_buffer_8);
						 DBG_PRINT(" 4096 < scaler size <=8192 malloc buffer error!\r\n");
						 malloc_buffer_error = 1;
				   }                            
	           }    
			   
			   if(malloc_buffer_error == 0){     
				 for(i=0;i<SCALER_NUM;i++)
				 {
				   image_scaler.input_width = image_scaler_arg.InputWidth;
				   image_scaler.input_height = image_scaler_arg.InputHeight;
				   image_scaler.input_visible_width = image_scaler_arg.InputVisibleWidth;
				   image_scaler.input_visible_height = image_scaler_arg.InputVisibleHeight;
				   image_scaler.input_offset_x = ((image_scaler_arg.InputOffsetX+(i*in_scaler_x))<<16);
				   image_scaler.input_offset_y = image_scaler_arg.InputOffsetY;
				   if(image_scaler_arg.OutputWidthFactor!=0 || image_scaler_arg.OutputHeightFactor!=0){
				      image_scaler.output_width_factor = image_scaler_arg.OutputWidthFactor;
				      image_scaler.output_height_factor = image_scaler_arg.OutputHeightFactor;
				   }else{
				      image_scaler.output_width_factor = (image_scaler_arg.InputWidth<<16)/image_scaler_arg.OutputWidth;
				      image_scaler.output_height_factor = (image_scaler_arg.InputHeight<<16)/image_scaler_arg.OutputHeight;
				   } 
				   image_scaler.output_buffer_width = out_scaler_x;
				   image_scaler.output_buffer_height = image_scaler_arg.OutputHeight;
				   image_scaler.input_buf1 = (void *)image_scaler_arg.InputBufPtr;
				   if(i==0)
				     image_scaler.output_buf1 = (void *)scaler_buffer_1; 
				   else if(i==1)
				     image_scaler.output_buf1 = (void *)scaler_buffer_2;
				   else if(i==2)
				     image_scaler.output_buf1 = (void *)scaler_buffer_3;
				   else if(i==3)
				     image_scaler.output_buf1 = (void *)scaler_buffer_4;
				   else if(i==4)
				     image_scaler.output_buf1 = (void *)scaler_buffer_5;
				   else if(i==5)
				     image_scaler.output_buf1 = (void *)scaler_buffer_6;
				   else if(i==6)
				     image_scaler.output_buf1 = (void *)scaler_buffer_7;
				   else
				     image_scaler.output_buf1 = (void *)scaler_buffer_8;

				   switch(image_scaler_arg.InputFormat)
				   { 
				          case SCALER_INPUT_FORMAT_RGB565:
				               image_scaler.scaler_input_format = (INT8U)C_SCALER_CTRL_IN_RGB565;
				               break;                     
				          
				          case SCALER_INPUT_FORMAT_YUYV:
				               image_scaler.scaler_input_format = (INT8U)C_SCALER_CTRL_IN_YUYV;
				               break;           
				          
				          case SCALER_INPUT_FORMAT_UYVY:
				               image_scaler.scaler_input_format = (INT8U)C_SCALER_CTRL_IN_UYVY;
				               break;                 
				          
				          default:                   		
				               DBG_PRINT("scaler input format error!\r\n");
				               scaler_format_error = 1;
				               break;         
				   }
				   
				   switch(image_scaler_arg.OutputFormat)
				   {                  
				          case IMAGE_OUTPUT_FORMAT_RGB565:
				               image_scaler.scaler_output_format = (INT8U)C_SCALER_CTRL_OUT_RGB565;
				               break;                      
				          
				          case IMAGE_OUTPUT_FORMAT_YUYV:
				               image_scaler.scaler_output_format = (INT8U)C_SCALER_CTRL_OUT_YUYV;
				               break;           
				          
				          case IMAGE_OUTPUT_FORMAT_UYVY:
				               image_scaler.scaler_output_format = (INT8U)C_SCALER_CTRL_OUT_UYVY;
				               break;          
             
				          default:
				               DBG_PRINT("scaler output format error!\r\n");
				               scaler_format_error = 1;
				               break;         
				   } 
				   image_scaler.out_of_boundary_color = image_scaler_arg.OutBoundaryColor;
				   image_scaler.cmd_id = 1;
				   //SEND MSG
				   if(scaler_format_error == 0){ 
				    #if _OPERATING_SYSTEM == _OS_UCOS2
				      if(image_msg_queue_id)
				      {
				          msgQSend(image_msg_queue_id, MSG_IMAGE_SCALE_REQ, (void *)&image_scaler,  sizeof(IMAGE_SCALE_STRUCT), MSG_PRI_NORMAL);
				          //must check
				          if(i < (SCALER_NUM-1))
				             image_scale_status();
				      }else{
				          DBG_PRINT("image_msg_queue_id format error !\r\n");
				          break;                 
				      }   
				    #else
				      DBG_PRINT("OS_NONE error !\r\n");
				      break;                 
				    #endif    
				   }else{
				      DBG_PRINT("scaler format error !\r\n");
				      break;
				   }      
				 }
	           
		         if(scaler_format_error == 0){ 
		             T_Buf = (INT32U)image_scaler_arg.OutputBufPtr;
		             line_offset = (out_scaler_x*2);
		             dma_move_size = (out_scaler_x/2);                
		             //Use the DMA to combine image
		             for(i=0;i<image_scaler_arg.OutputBufHeight;i++)
		             {                          
			               for(j=0;j<SCALER_NUM;j++){
			                  done1 = C_DMA_STATUS_WAITING;
				              if(j==0)
				                 dma_struct.s_addr = (INT32U) S_Buf_1;
				              else if(j==1)
				                 dma_struct.s_addr = (INT32U) S_Buf_2;
				              else if(j==2)
				                 dma_struct.s_addr = (INT32U) S_Buf_3;
				              else if(j==3)
				                 dma_struct.s_addr = (INT32U) S_Buf_4;
				              else if(j==4)
				                 dma_struct.s_addr = (INT32U) S_Buf_5;
				              else if(j==5)
				                 dma_struct.s_addr = (INT32U) S_Buf_6;
				              else if(j==6)
				                 dma_struct.s_addr = (INT32U) S_Buf_7;
				              else
				                 dma_struct.s_addr = (INT32U) S_Buf_8;   	                        
				              dma_struct.t_addr = (INT32U) T_Buf;
				              dma_struct.width = DMA_DATA_WIDTH_4BYTE;
				              dma_struct.count = dma_move_size;
				              dma_struct.notify = &done1;
				              dma_struct.timeout = 0;
				              result = drv_l1_dma_transfer_wait_ready(&dma_struct);
				              if(result < 0)
				              {
				            	  scaler_state = CODEC_START_STATUS_ERROR_MAX;
				            	  return scaler_state;
				              }
				              T_Buf += line_offset;
				           }
				           //T_Buf+=line_offset;
				           if(SCALER_NUM==SCALER_SIZE_2){
			                  S_Buf_1 += line_offset;
			                  S_Buf_2 += line_offset;
			               }else if(SCALER_NUM==SCALER_SIZE_4){
			                  S_Buf_1 += line_offset;
			                  S_Buf_2 += line_offset;
			                  S_Buf_3 += line_offset;
			                  S_Buf_4 += line_offset; 
			               }else{
			                  S_Buf_1 += line_offset;
			                  S_Buf_2 += line_offset;
			                  S_Buf_3 += line_offset;
			                  S_Buf_4 += line_offset; 
			                  S_Buf_5 += line_offset;
			                  S_Buf_6 += line_offset;
			                  S_Buf_7 += line_offset;
			                  S_Buf_8 += line_offset;                               
			               }	
			         }
		             
		             if(SCALER_NUM==SCALER_SIZE_2){
		           	    gp_free((void *) scaler_buffer_1);
			            gp_free((void *) scaler_buffer_2);
		             }else if(SCALER_NUM==SCALER_SIZE_4){
		                gp_free((void *) scaler_buffer_1);
			            gp_free((void *) scaler_buffer_2);
			            gp_free((void *) scaler_buffer_3);
			            gp_free((void *) scaler_buffer_4);
		             }else{
		                gp_free((void *) scaler_buffer_1);
			            gp_free((void *) scaler_buffer_2);
			            gp_free((void *) scaler_buffer_3);
			            gp_free((void *) scaler_buffer_4);	                     
		                gp_free((void *) scaler_buffer_5);
			            gp_free((void *) scaler_buffer_6);
			            gp_free((void *) scaler_buffer_7);
			            gp_free((void *) scaler_buffer_8);
		             }
		             image_scaler_ck = 0;          
		             scaler_state = START_OK;
		           }else{
		             image_scaler_ck = 0;
		             scaler_state = RESOURCE_READ_ERROR; 
		           }    
		       }
	           image_scaler_ck = 0;
	           scaler_state = RESOURCE_NO_FOUND_ERROR;
           #endif 
        } else {
           image_scaler.input_width = image_scaler_arg.InputWidth;
           image_scaler.input_height = image_scaler_arg.InputHeight;
           image_scaler.input_visible_width = image_scaler_arg.InputVisibleWidth;
           image_scaler.input_visible_height = image_scaler_arg.InputVisibleHeight;
           image_scaler.input_offset_x = image_scaler_arg.InputOffsetX;
           image_scaler.input_offset_y = image_scaler_arg.InputOffsetY;
           if(image_scaler_arg.OutputWidthFactor != 0 || image_scaler_arg.OutputHeightFactor != 0){
             image_scaler.output_width_factor = image_scaler_arg.OutputWidthFactor;
             image_scaler.output_height_factor = image_scaler_arg.OutputHeightFactor;
           }else{
             image_scaler.output_width_factor = (image_scaler_arg.InputWidth<<16)/image_scaler_arg.OutputWidth;
             image_scaler.output_height_factor = (image_scaler_arg.InputHeight<<16)/image_scaler_arg.OutputHeight;
           }           
           image_scaler.output_buffer_width = image_scaler_arg.OutputBufWidth;// Make sure output width is at least multiple of 8 pixels
           if (image_scaler.output_buffer_width & SCALER_8_MULTIPLE) {
                 image_scaler.output_buffer_width = (image_scaler.output_buffer_width + SCALER_8_MULTIPLE) & ~SCALER_8_MULTIPLE;			// Must be 32-pixels alignment
			     DBG_PRINT(" scaler Output Width Size error!\r\n");
		   }
           image_scaler.output_buffer_height = image_scaler_arg.OutputBufHeight;
           image_scaler.input_buf1 = (void *)image_scaler_arg.InputBufPtr;
           image_scaler.output_buf1 = (void *)image_scaler_arg.OutputBufPtr;

           switch(image_scaler_arg.InputFormat)
           { 
                 case SCALER_INPUT_FORMAT_RGB565:
                      image_scaler.scaler_input_format = (INT8U)C_SCALER_CTRL_IN_RGB565;
                      break;                     
                 
                 case SCALER_INPUT_FORMAT_YUYV:
                      image_scaler.scaler_input_format = (INT8U)C_SCALER_CTRL_IN_YUYV;
                      break;           
                 
                 case SCALER_INPUT_FORMAT_UYVY:
                      image_scaler.scaler_input_format = (INT8U)C_SCALER_CTRL_IN_UYVY;
                      break;            
                 
                 default:		
                      DBG_PRINT("scaler input format error!\r\n");
                      scaler_format_error = 1;
                      break;         
           }
           
           switch(image_scaler_arg.OutputFormat)
           { 
                 case IMAGE_OUTPUT_FORMAT_RGB565:
                      image_scaler.scaler_output_format = (INT8U)C_SCALER_CTRL_OUT_RGB565;
                      break;           
                 
                 case IMAGE_OUTPUT_FORMAT_YUYV:
                      image_scaler.scaler_output_format = (INT8U)C_SCALER_CTRL_OUT_YUYV;
                      break;           
                 
                 case IMAGE_OUTPUT_FORMAT_UYVY:
                      image_scaler.scaler_output_format = (INT8U)C_SCALER_CTRL_OUT_UYVY;
                      break;  
				 
                 default:
					    DBG_PRINT("scaler output format error!\r\n");
						scaler_format_error = 1;
						break;                 
           } 
           image_scaler.out_of_boundary_color=image_scaler_arg.OutBoundaryColor;
           image_scaler.cmd_id = 1;
           if(scaler_format_error == 0){ 
              //SEND MSG
            #if _OPERATING_SYSTEM == _OS_UCOS2
              if(image_msg_queue_id)
              	msgQSend(image_msg_queue_id, MSG_IMAGE_SCALE_REQ, (void *)&image_scaler,  sizeof(IMAGE_SCALE_STRUCT), MSG_PRI_NORMAL);
              scaler_state = START_OK;
            #else
              scaler_state = RESOURCE_READ_ERROR;
            #endif
           }else{
              image_scaler_ck = 0;
              DBG_PRINT("scaler format error !\r\n");
              scaler_state = RESOURCE_READ_ERROR;
           } 
        }
    }else{       
        scaler_state = REENTRY_ERROR;       
    }
    image_dec_unlock();
    
    return scaler_state;     
}

IMAGE_CODEC_STATUS image_scale_status(void)
{
    INT32U msg_id,scaler_status;
    IMAGE_SCALE_STRUCT *image_scaler;
    
    image_dec_lock();
    #if _OPERATING_SYSTEM == _OS_UCOS2
	    if(image_scaler_ck){     
	       if(ApQ)
	       {
		       msgQReceive(ApQ, &msg_id, (void *) &image_decode_task_para, sizeof(IMAGE_SCALE_STRUCT));
		       if(msg_id == MSG_IMAGE_SCALE_REPLY){ 
			       if(image_scaler_ck != SCALER_BLOCK_START)
			       		image_scaler_ck = 0;
			       image_scaler = (IMAGE_SCALE_STRUCT *)image_decode_task_para;
			       if((image_scaler->status) != 0){
			           scaler_status = IMAGE_CODEC_DECODE_FAIL;
			       }else{
			           scaler_status = IMAGE_CODEC_DECODE_END;   
			       }
			       drv_l1_scaler_stop(C_USE_SCALER_NUMBER);
		       }
		       else
		       		scaler_status = IMAGE_CODEC_STATUS_MAX; 
	       }
	       else
	       {
	           image_scaler_ck = 0;
	           scaler_status = IMAGE_CODEC_STATUS_MAX;
	       }		
	    }else{      
	       scaler_status = IMAGE_CODEC_STATUS_MAX; 
	    }
    #else
    	scaler_status = IMAGE_CODEC_STATUS_MAX
    #endif
    image_dec_unlock();
    
    return scaler_status;
}


#if API_DO_JPEG_EN == 1
static INT32S jpeg_scaler_fifo_mode_flow_by_piece(INT16S fd, INT32U file_size, INT32U buff_addr)
{
    INT32S jpeg_status=0, scaler_status=0;
    INT8U  scaler_done=0 ;
    INT8U  *jpeg_dec_start ;
    INT32U jpeg_dec_len, jpeg_has_dec_len ;
    INT8U *data_addr ;
    data_addr = (INT8U *) jpeg_decode_image_vlc_addr_get() ;

    // decode 1st data piece of jpeg raw data
    jpeg_dec_start = data_addr ;
    jpeg_dec_len = JPEG_DEC_PIECE_SIZE- ((INT32U)data_addr - buff_addr) ;
    jpeg_has_dec_len = jpeg_dec_len ;
    
    jpeg_decode_on_the_fly_start(jpeg_dec_start, jpeg_dec_len); // start part of decode
    
	while(1) {
		// Now query whether JPEG decoding is done
		jpeg_status = jpeg_decode_status_query(1);
		
		if (jpeg_status & C_JPG_STATUS_DECODE_DONE) {
		
			while(1) {
				scaler_status = drv_l1_scaler_wait_idle(C_USE_SCALER_NUMBER);

		  		if(scaler_status == C_SCALER_STATUS_STOP)
		  		{
					drv_l1_scaler_start(C_USE_SCALER_NUMBER,ENABLE);
				}
				else if(scaler_status & C_SCALER_STATUS_DONE) 
				{				
					break;
				}
				else if(scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) 
				{
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} 
				else if(scaler_status & C_SCALER_STATUS_INPUT_EMPTY) 
				{
		  			drv_l1_scaler_restart(C_USE_SCALER_NUMBER);
		  		}
		  		else 
		  		{
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}
			}		
	
			break;
		} // if (jpeg_status & C_JPG_STATUS_DECODE_DONE) {
		
		if (jpeg_status & C_JPG_STATUS_INPUT_EMPTY) // 
		{
            if(JPEG_DEC_PIECE_SIZE < (file_size-jpeg_has_dec_len)) {
                jpeg_dec_len = JPEG_DEC_PIECE_SIZE ;
                jpeg_has_dec_len += JPEG_DEC_PIECE_SIZE ;
            } else {
                jpeg_dec_len = file_size-jpeg_has_dec_len ;
                jpeg_has_dec_len = file_size ;
            }		    

        	if (read(fd, buff_addr, jpeg_dec_len) <= 0) {
        		DBG_PRINT("jpeg_scaler_fifo_mode_flow_by_piece read jpeg file fail.\r\n");
        		return STATUS_FAIL;
        	}	
            
            if (jpeg_status & C_JPG_STATUS_OUTPUT_FULL) {  // special case!!, C_JPG_STATUS_INPUT_EMPTY & C_JPG_STATUS_OUTPUT_FULL @ the same time
                //DBG_PRINT("JPEG decoded by piece: OUTPUT FULL & INPUT EMPTY=================================\r\n");
                
    		  	if(!scaler_done) 
    		  	{
    			  	scaler_status = drv_l1_scaler_wait_idle(C_USE_SCALER_NUMBER);
    			  	if(scaler_status == C_SCALER_STATUS_STOP)
    			  	{
    					drv_l1_scaler_start(C_USE_SCALER_NUMBER,ENABLE);
    			  	}
    			  	else if(scaler_status & C_SCALER_STATUS_DONE)
    			  	{
    			  		scaler_done = 1;
    			  	}
    			  	else if(scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) 
    			  	{
    					DBG_PRINT("Scaler failed to finish its job\r\n");
    					break;
    				} 
    				else if(scaler_status & C_SCALER_STATUS_INPUT_EMPTY) 
    				{
    			  		drv_l1_scaler_restart(C_USE_SCALER_NUMBER);
    			  	}
    			  	else 
    			  	{
    			  		DBG_PRINT("Un-handled Scaler status!\r\n");
    			  		break;
    			  	}
    			}                  
                jpeg_decode_on_the_fly_start2((INT8U *)buff_addr, jpeg_dec_len);
                jpeg_status &= ~C_JPG_STATUS_OUTPUT_FULL ;   
                jpeg_status &= ~C_JPG_STATUS_INPUT_EMPTY ;                 
            }
            else {// for only C_JPG_STATUS_INPUT_EMPTY
                jpeg_decode_on_the_fly_start((INT8U *)buff_addr, jpeg_dec_len);
                jpeg_status &= ~C_JPG_STATUS_INPUT_EMPTY ;
            }
		}
		
		if(jpeg_status & C_JPG_STATUS_OUTPUT_FULL) 
		{	// Start scaler to handle the full output FIFO now
		  	if(!scaler_done) 
		  	{
			  	scaler_status = drv_l1_scaler_wait_idle(C_USE_SCALER_NUMBER);
			  	if(scaler_status == C_SCALER_STATUS_STOP)
			  	{
					drv_l1_scaler_start(C_USE_SCALER_NUMBER,ENABLE);
			  	}
			  	else if(scaler_status & C_SCALER_STATUS_DONE)
			  	{
			  		scaler_done = 1;
			  	}
			  	else if(scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) 
			  	{
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} 
				else if(scaler_status & C_SCALER_STATUS_INPUT_EMPTY) 
				{
			  		drv_l1_scaler_restart(C_USE_SCALER_NUMBER);
			  	}
			  	else 
			  	{
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}
			}
		
	  		// Now restart JPEG to output to next FIFO
	  		if(jpeg_decode_output_restart()) 
	  		{
	  			DBG_PRINT("Failed to call jpeg_decode_output_restart()\r\n");
	  			break;
	  		}
		}  // if(jpeg_status & C_JPG_STATUS_OUTPUT_FULL) 		

		if(jpeg_status & C_JPG_STATUS_STOP) 
		{
			DBG_PRINT("JPEG is not started!\r\n");
			break;
		}
		if(jpeg_status & C_JPG_STATUS_TIMEOUT)
		{
			DBG_PRINT("JPEG execution timeout!\r\n");
			break;
		}
		if(jpeg_status & C_JPG_STATUS_INIT_ERR) 
		{
			DBG_PRINT("JPEG init error!\r\n");
			break;
		}
		if(jpeg_status & C_JPG_STATUS_RST_VLC_DONE) 
		{
			DBG_PRINT("JPEG Restart marker number is incorrect!\r\n");
			break;
		}
		if(jpeg_status & C_JPG_STATUS_RST_MARKER_ERR) 
		{
			DBG_PRINT("JPEG Restart marker sequence error!\r\n");
			break;
		}			
	} // while loop of JPEG decode 			
	
	return scaler_status ;		
}

// only decode JPEG, not include AVI
static INT32S video_decode_jpeg_as_gp420_by_piece(INT16S fd, INT32U file_size, INT32U output_width, INT32U output_height, INT32U output_addr)
{
	INT16U img_valid_width, img_valid_height, img_extend_width, img_extend_height;
	INT32U jpg_temp_out_buf,temp;
	INT32S scaler_status;
	INT8U  use_scaler, pad_line_num;
	
	INT32U y_addr;
	INT32U fifo_line;
	//SCALER_MAS scaler1_mas;
	INT8U  bRatio_4_3_to_16_9 = 0 ;
	INT32U factor_y ;
	INT32U size_to_read;
	INT8U  *data_addr;
	INT32U file_buff_addr ;

	DBG_PRINT("video_decode_jpeg_as_gp422_by_piece()=============\r\n");

	if (!output_width || !output_height) {
		return -1;
	}

	// allocate buffer to decode
	file_buff_addr = (INT32U)gp_malloc_align(JPEG_DEC_PIECE_SIZE, 16);
	if (file_buff_addr==0) {
	    DBG_PRINT("video_decode_jpeg_as_gp422_by_piece() failed to allocate memory\r\n");
	    return -1;
	}

	// read file header or entire file
	if (file_size<=JPEG_DEC_PIECE_SIZE)
	    size_to_read = file_size;
	else
	    size_to_read = JPEG_DEC_PIECE_SIZE ;
    
    file_size = size_to_read;
	if (read(fd, file_buff_addr, size_to_read) <= 0) {
		gp_free((void *) file_buff_addr);
		DBG_PRINT("video_decode_jpeg_as_gp422_by_piece() read jpeg file fail.\r\n");
		return STATUS_FAIL;
	}
	data_addr = (INT8U *)file_buff_addr ;

	// Parse JPEG data header
	jpeg_decode_init();
	
	if (jpeg_decode_parse_header((INT8U *) data_addr, size_to_read) != JPEG_PARSE_OK) {
		return -1;
	}
		
	img_valid_width = jpeg_decode_image_width_get();
	img_valid_height = jpeg_decode_image_height_get();
	img_extend_width = jpeg_decode_image_extended_width_get();
	img_extend_height = jpeg_decode_image_extended_height_get();
		
	// Setup JPEG and Scaler engine for decoding
	drv_l1_scaler_init(C_USE_SCALER_NUMBER);
   	//scaler_fifo_line_set(SCALER_1, C_SCALER_CTRL_IN_FIFO_DISABLE);		// Default disable 
	drv_l1_scaler_input_format_set(C_USE_SCALER_NUMBER, C_SCALER_CTRL_IN_YUYV);
	drv_l1_scaler_output_format_set(C_USE_SCALER_NUMBER, C_SCALER_CTRL_OUT_YUYV);
	drv_l1_scaler_out_of_boundary_mode_set(C_USE_SCALER_NUMBER, 1);	
	drv_l1_scaler_out_of_boundary_color_set(C_USE_SCALER_NUMBER, 0x008080);			// Black
	
	jpg_temp_out_buf = 0;
	use_scaler = 0;
	pad_line_num = 0;
	
	//if (!(img_valid_width==1920 && img_valid_height==1080))
		//bRatio_4_3_to_16_9 = 1 ;

	if (img_valid_width>=2000) {
		drv_l1_jpeg_decode_scale_down_set(ENUM_JPG_DIV4);
		img_valid_width >>= 2;
		img_valid_height >>= 2;
		img_extend_width >>= 2;
		img_extend_height >>= 2;
	} else if (img_valid_width>1280) {
		drv_l1_jpeg_decode_scale_down_set(ENUM_JPG_DIV2);
		img_valid_width >>= 1;
		img_valid_height >>= 1;
		img_extend_width >>= 1;
		img_extend_height >>= 1;
	}

	fifo_line = 0 ;
	if (img_valid_width<=1718) { // FIFO=32 lines
		fifo_line = 32 ;
	} else if (img_valid_width<=3436) { // FIFO=16 lines
		fifo_line = 16 ;
	} 

    //jpg_temp_out_buf = 0xF8008700;
    
    jpg_temp_out_buf = (INT32U)gp_malloc_align((img_extend_width*fifo_line*2)*2, 16);
	if(fifo_line == 32)
		temp = C_SCALER_CTRL_IN_FIFO_32LINE;
	else if(fifo_line == 16)
		temp = C_SCALER_CTRL_IN_FIFO_16LINE;
		
	drv_l1_scaler_input_fifo_line_set(C_USE_SCALER_NUMBER,temp) ;
	drv_l1_scaler_input_A_addr_set(C_USE_SCALER_NUMBER,jpg_temp_out_buf, 0, 0) ;
	drv_l1_scaler_input_B_addr_set(C_USE_SCALER_NUMBER,jpg_temp_out_buf+img_extend_width*fifo_line*2, 0, 0) ;
	drv_l1_scaler_input_pixels_set(C_USE_SCALER_NUMBER, img_extend_width, img_extend_height);
	drv_l1_scaler_input_visible_pixels_set(C_USE_SCALER_NUMBER, img_valid_width, img_valid_height);	
	drv_l1_scaler_output_fifo_line_set(C_USE_SCALER_NUMBER,0) ;
	
	if (bRatio_4_3_to_16_9) {
		factor_y = (img_valid_height<<16)/(output_height-(pad_line_num<<1)) ;
		drv_l1_scaler_output_pixels_set(C_USE_SCALER_NUMBER, (img_valid_width<<16)/960, 
										factor_y, output_width, output_height-pad_line_num);	
	    y_addr = output_addr+(output_width*pad_line_num<<1)+320 ;
	} else {
		drv_l1_scaler_output_pixels_set(C_USE_SCALER_NUMBER, (img_valid_width<<16)/output_width, (img_valid_height<<16)/(output_height-(pad_line_num<<1)), 
										output_width, output_height-pad_line_num);	
		y_addr = output_addr+(output_width*pad_line_num<<1); 
	}
	
	drv_l1_scaler_output_addr_set(C_USE_SCALER_NUMBER, y_addr, NULL, NULL);
	
	#if 0
		// set scatop_1 path
		gp_memset((INT8S *)&scaler1_mas,0,sizeof(SCALER_MAS));
		scaler1_mas.mas_0 = MAS_EN_READ|MAS_EN_WRITE;
		drv_l1_scaler_mas_set(SCALER_1,&scaler1_mas);

		wrap_addr_set(WRAP_CSI2SCA, y_addr);
		wrap_path_set(WRAP_CSI2SCA,0,1);
		wrap_filter_flush(WRAP_CSI2SCA);

	    (*((volatile INT32U *) 0xC0190018)) = 0x300 ; // Conv422 reset & bypass
	    //(*((volatile INT32U *) 0xC01B0000)) |= 0x010 ; 
	    (*((volatile INT32U *) 0xC0180000)) &= ~0x800 ; 
    #endif

	// Start JPEG
	if(fifo_line == 32)
		temp = C_JPG_FIFO_32LINE;
	else if(fifo_line == 16)
		temp = C_JPG_FIFO_16LINE;
		
	if (jpeg_decode_output_set(jpg_temp_out_buf, 0x0, 0x0, temp)) {
		gp_free((void *) jpg_temp_out_buf);
		return -1;
	}			
	
	scaler_status = jpeg_scaler_fifo_mode_flow_by_piece(fd, file_size, file_buff_addr);					

	if (file_buff_addr)
	    gp_free((void *)file_buff_addr);
	
	// STOP JPEG and Scaler
	jpeg_decode_stop();
	drv_l1_scaler_stop(C_USE_SCALER_NUMBER);
	
	// paint black for left side
	if (bRatio_4_3_to_16_9) {
		//paint_GP420_left_side_of_960x720(output_addr);
	}
	
	if (scaler_status & C_SCALER_STATUS_DONE) { 
		return 0;
	}
	return -1;
}
#endif
