#include <stdio.h>
#include "application.h"
#include "image_decoder.h"
#include "image_encoder.h"
#include "drv_l2_ad_key.h"
#include "drv_l1_power.h"
#include "drv_l1_uart.h"
#include "drv_l2_display.h"

// decode
#define DECODE_JPG             			0
#define DECODE_GIF              		1
#define DECODE_BMP              		2
#define DECODE_TYPE_SELECT_END  		0xFF

// dencode
#define BLOCK_READ_LINE					16
#define ENCODE_H_SIZE					320
#define ENCODE_V_SIZE					240
#define ENCODE_ONCE   					0
#define ENCODE_BLOCK_READ_EN			1
#define ENCODE_BLOCK_RW_EN			    2

#define	DISKUSED		 				FS_SD1//FS_SD //FS_NAND1
#define CODEC_SIMPLE_EN					0
static INT16U width,height;

static INT32U image_encdoe_end(INT32U encode_buf,INT32U encode_size)
{

	DBG_PRINT("encode size = %d \r\n",encode_size);
	
	return encode_buf;
}
static INT32U image_decode_end(INT32U decode_buf)
{
	drv_l2_display_update(DISDEV_TFT,decode_buf);
	
	return decode_buf;
}

INT32S image_encode_scaler_start(INT32U decode_buf,INT32U scaler_buf,INT32U input_x,INT32U input_y)
{
	INT32S scaler_state;
	IMAGE_SCALE_ARGUMENT scaler_arg;
       
	scaler_arg.InputVisibleWidth = input_x;
	scaler_arg.InputVisibleHeight = input_y;
	scaler_arg.InputWidth = input_x;
	scaler_arg.InputHeight = input_y;
	scaler_arg.OutputWidthFactor = 0;
	scaler_arg.OutputHeightFactor = 0;
	scaler_arg.InputOffsetX = 0;
	scaler_arg.InputOffsetY = 0;
	scaler_arg.OutputBufWidth = ENCODE_H_SIZE;
	scaler_arg.OutputBufHeight = ENCODE_V_SIZE;
	scaler_arg.OutputWidth = ENCODE_H_SIZE;
	scaler_arg.OutputHeight = ENCODE_V_SIZE;
	scaler_arg.InputBufPtr = (INT8U *)decode_buf;
	scaler_arg.OutputBufPtr = (INT8U *)scaler_buf;
	scaler_arg.InputFormat = IMAGE_OUTPUT_FORMAT_YUYV;
	scaler_arg.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;
	scaler_arg.OutBoundaryColor = (INT32U)0x008080; 				 	//set the black for out of boundary color 
	image_scale_start(scaler_arg);

	while (1){
		scaler_state = image_scale_status();
		if (scaler_state == IMAGE_CODEC_DECODE_END) {
			break;
		}else if(scaler_state == IMAGE_CODEC_DECODE_FAIL) {
			DBG_PRINT("image scaler failed\r\n");
			break;
		}	
	}	
	  
	return scaler_state;
}

INT32S image_sacler_enable(INT32U decode_output_ptr,INT32U do_count)
{	
	INT32U i,scale_output_ptr,count,flag,scaler_flag;
	INT32U scale_x_output,scale_y_output,scaler_state;
	IMAGE_SCALE_ARGUMENT scaler_arg;
	
	scale_output_ptr = (INT32U) gp_malloc_align((width*height*2)*2, 32);//malloc decode frame buffer
	if(scale_output_ptr == 0)
	{
		DBG_PRINT("scale_output_ptr malloc fail\r\n");
		return -1;
	}
	else
		gp_memset((INT8S *)scale_output_ptr, 0x00, (width*height*2)*2);	
	
	scale_x_output = (width>>1);
	scale_y_output = (height>>1);
	flag = 0;
	count = 0;
	scaler_flag = 0;
	i = 1;
	
	do{
	   if(count % 2 == 0){
          if(flag == 0){
            if (scale_x_output > (width + (width >> 1)) || scale_y_output > (height + (height >> 1))){
		        scale_x_output = (width + (width >> 1));
		        scale_y_output = (height + (height >> 1));
		        flag = 1;
            }
            else
            {
             	scale_x_output += 2;
            	scale_y_output += 2;             
            }            
          }else{
            if(scale_x_output < (width>>1) || scale_y_output < (height>>1) ){
		        scale_x_output = (width >> 1);
		        scale_y_output = (height >> 1);
		        flag = 0;
            }
            else
            {
                scale_x_output -= 2;
            	scale_y_output -= 2; 
            }           
          }      
       }
       scaler_arg.InputVisibleWidth = width;
       scaler_arg.InputVisibleHeight = height;
       scaler_arg.InputWidth = width;
       scaler_arg.InputHeight = height;
       scaler_arg.OutputWidthFactor = 0;
       scaler_arg.OutputHeightFactor = 0;
       scaler_arg.InputOffsetX = 0;
       scaler_arg.InputOffsetY = 0;
       scaler_arg.OutputBufWidth = width;
       scaler_arg.OutputBufHeight = height;
       scaler_arg.OutputWidth = scale_x_output;
       scaler_arg.OutputHeight = scale_y_output;
       scaler_arg.InputBufPtr = (INT8U *)decode_output_ptr;
       if(scaler_flag)
       		scaler_arg.OutputBufPtr = (INT8U *)(scale_output_ptr + (width*height*2));
       else
       		scaler_arg.OutputBufPtr = (INT8U *)scale_output_ptr;
       scaler_arg.InputFormat = IMAGE_OUTPUT_FORMAT_YUYV;
       scaler_arg.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;
       scaler_arg.OutBoundaryColor = (INT32U)0x008080; 				 	//set the black for out of boundary color 
       image_scale_start(scaler_arg);
       
       while (1){
			scaler_state = image_scale_status();
			if (scaler_state == IMAGE_CODEC_DECODE_END) {
				drv_l2_display_update(DISDEV_TFT,(scale_output_ptr + (scaler_flag*width*height*2)));
				if(scaler_flag)
					scaler_flag = 0;
				else
					scaler_flag = 1;  
				break;
			}else if(scaler_state == IMAGE_CODEC_DECODE_FAIL) {
				DBG_PRINT("image scaler failed\r\n");
				break;
			}	
      }
      count++;    
	  OSTimeDly(2);
      
      if(do_count)
      {
	      if(count > do_count)
	      	i = 0;	
      }		      	  
	}while( i == 1);
		
	if(scale_output_ptr)
		gp_free((void *)scale_output_ptr);
	
	return 0;	
}

void mount_disk(void)
{
	
	INT32S disk_free;
	while(1)
	{
		if(_devicemount(DISKUSED))					// Mount device
		{
			DBG_PRINT("Mount Disk Fail[%d]\r\n", DISKUSED);						
		}
		else
		{
			DBG_PRINT("Mount Disk success[%d]\r\n", DISKUSED);
			DBG_PRINT("StartTime = %d\r\n", OSTimeGet());
			disk_free = vfsFreeSpace(DISKUSED);
			DBG_PRINT("Disk Free Size = %dMbyte\r\n", disk_free/1024/1024);
			DBG_PRINT("EndTime = %d\r\n", OSTimeGet());			
			break;
		}
	}
}

void image_codec_demo_init(void)
{
	// sd init
	mount_disk();
	
	// display init
	drv_l2_display_init();
	drv_l2_display_start(DISDEV_TFT,DISP_FMT_YUYV);
	drv_l2_display_get_size(DISDEV_TFT,&width,&height);
	
	// adc dac power on
	drv_l1_power_ldo_codec_ctrl(ENABLE, LDO_CODEC_3P3V);	
	
	DBG_PRINT("\r\n*****************************************************************\r\n");
	DBG_PRINT("                 This is image codec demo                      **\r\n");
	DBG_PRINT("*****************************************************************\r\n");	

}

void image_codec_demo_uninit(void)
{
	drv_l2_ad_key_uninit();	
	drv_l2_display_uninit();
}

INT32S jpeg_simple_decode(void)
{
	INT32S ret;
	INT32U decode_state,decode_output_ptr;
	IMAGE_ARGUMENT image_decode;
	MEDIA_SOURCE image_source;
	struct f_info file_info;

	image_codec_demo_init();
		
	//malloc frame buffer
	decode_output_ptr = (INT32U) gp_malloc_align((width * height) * 2, 64);//malloc decode frame buffer
	if(decode_output_ptr == 0)
	{
		DBG_PRINT("decode_output_ptr malloc fail\r\n");
		goto _jpg_exit;
	}
	else
		gp_memset((INT8S *)decode_output_ptr, 0x00, (width * height) * 2);
	
	//image source infomation
	image_source.type = SOURCE_TYPE_FS;                                    //image file infomation form file system
	
	if (DISKUSED == FS_SD1){
		DBG_PRINT("Media : SDCard \r\n");
		chdir("F:\\");		   
	}
	else if (DISKUSED == FS_SD){
		DBG_PRINT("Media : SDCard \r\n");
		chdir("C:\\");			   
	}
	else if (DISKUSED == FS_NAND1){
		DBG_PRINT("Media : NAND \r\n");
		chdir("A:\\");
	}
	
	ret = _findfirst("*.jpg", &file_info, D_ALL);
	if(ret < 0) {
		DBG_PRINT("_findfirst fail...\r\n");
		return STATUS_FAIL;
	}
	else			
		DBG_PRINT("File Name = %s\r\n", file_info.f_name);	
	
	image_source.type_ID.FileHandle=(INT32U)open((char *)file_info.f_name, O_RDONLY);    //open jpg filef				   
	
	if(image_source.type_ID.FileHandle < 0)
	{
		DBG_PRINT("Open file fail !\r\n");
		return STATUS_FAIL;
	}
	
	image_decode.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;				//scaler out format
	//image output infomation
	image_decode.OutputBufPtr = (INT8U *)decode_output_ptr;           	//decode output buffer
	image_decode.OutputBufWidth = width;                  				//width of output buffer 
	image_decode.OutputBufHeight = height;                 				//Heigh of output buffer
	image_decode.OutputWidth = width;                     				//scaler width of output image
	image_decode.OutputHeight = height;                    				//scaler Heigh of output image
	image_decode.ScalerOutputRatio = FIT_OUTPUT_SIZE; 				 	//Fit to output_buffer_width and output_buffer_height for image output size
	image_decode.OutBoundaryColor = (INT32U)0x008080; 				 	//set the black for out of boundary color 	

	//image decode function
	image_decode_entrance();     										//global variable initial for image decode
	image_decode_end_register_callback(image_decode_end); 
	image_decode_start(image_decode,image_source);    					//image decode start
	
	while (1) {
		 decode_state = image_decode_status();
         if (decode_state == IMAGE_CODEC_DECODE_END) {
			 close(image_source.type_ID.FileHandle);
			 break;
		  }else if(decode_state == IMAGE_CODEC_DECODE_FAIL) {
		  	 DBG_PRINT("image decode failed\r\n");
			 break;
		  }	
	}	

	image_decode_stop();
_jpg_exit:
	
	return decode_output_ptr;
}

INT32S jpeg_simple_once_encode(INT32U input_buf)
{
	char path[64];
	INT16S fd;
	IMAGE_ENCODE_ARGUMENT encode_info;
	INT32U encode_output_ptr,encode_state;

	encode_output_ptr = (INT32U) gp_malloc_align((ENCODE_H_SIZE*ENCODE_V_SIZE), 64);//malloc decode frame buffer
	if(encode_output_ptr == 0)
		DBG_PRINT("encode_output_ptr malloc fail\r\n");
	else
		gp_memset((INT8S *)encode_output_ptr, 0x00, (ENCODE_H_SIZE*ENCODE_V_SIZE));	
			
    //image encode infomation
	if (DISKUSED == FS_SD1){
		sprintf((char *)path, (const char *)"F:\\image_encode.jpeg");   
	}
	else if (DISKUSED == FS_SD){
		sprintf((char *)path, (const char *)"C:\\image_encode.jpeg");   
	}
	else if (DISKUSED == FS_NAND1){
		sprintf((char *)path, (const char *)"A:\\image_encode.jpeg");	
	}
	
	fd = open((CHAR *)path, O_CREAT|O_RDWR);
	if (fd >= 0)
	{	    	    
	    //encode mode setting    
	    encode_info.OutputBufPtr = (INT8U *)encode_output_ptr;
	    encode_info.FileHandle = fd;
	    encode_info.UseDisk = DISKUSED;
	  	encode_info.EncodeMode = IMAGE_ENCODE_ONCE_READ;
	    encode_info.InputWidth = width;                       					//width of input image
	    encode_info.InputHeight = height;                        				//Heigh of input image
	    encode_info.InputBufPtr.yaddr = (INT8U *)input_buf;       		    	//encode input buffer	
	    encode_info.QuantizationQuality = 50;                               	//encode quality
	    encode_info.InputFormat = IMAGE_ENCODE_INPUT_FORMAT_YUYV;   			//encode input format
	    encode_info.OutputFormat = IMAGE_ENCODE_OUTPUT_FORMAT_YUV422;       	//encode input format 
		
		image_encode_entrance();
		image_encode_end_func_register(image_encdoe_end);
		image_encode_start(encode_info);
		while (1) {
			  encode_state = image_encode_status();
	          if (encode_state == IMAGE_CODEC_DECODE_END) {		
				DBG_PRINT("image encode ok\r\n");			
				break;
			  }else if(encode_state == IMAGE_CODEC_DECODE_FAIL) {
				DBG_PRINT("image encode failed\r\n");
				break;
			  }	
		}
		image_encode_stop();                                                     //image encode stop
	}
	else
		DBG_PRINT("image_encode_error\r\n");
		
	return 0;
}

INT32U image_decode_enable(void)
{
	INT32S ret,temp;
	INT32U decode_state,decode_output_ptr,decode_type,type_flag,key;
	IMAGE_ARGUMENT image_decode;
	MEDIA_SOURCE image_source;
	struct f_info file_info;
	
	image_codec_demo_init();
	
	DBG_PRINT("\r\n***************************************************\r\n");
	DBG_PRINT("**                 AD_KEY Menu:                  **\r\n");
	DBG_PRINT("**KEY_1 image decode type selection              **\r\n");
	DBG_PRINT("**KEY_2 image decode start             			**\r\n");
	DBG_PRINT("**KEY_3 scaler start with decode buffer          **\r\n");
	DBG_PRINT("**KEY_4 image decode demo exit            	    **\r\n");
	DBG_PRINT("***************************************************\r\n");	
	
	if (DISKUSED == FS_SD1) {
		DBG_PRINT("Media : SDCard \r\n");
		chdir("F:\\");			   
	}
	else if (DISKUSED == FS_SD) {
		DBG_PRINT("Media : SDCard \r\n");
		chdir("C:\\");			   
	}
	else if (DISKUSED == FS_NAND1) {
		DBG_PRINT("Media : NAND \r\n");
		chdir("A:\\");	
	}
	
	//malloc frame buffer
	decode_output_ptr = (INT32U) gp_malloc_align((width * height *2), 64);//malloc decode frame buffer
	if(decode_output_ptr == 0)
	{
		DBG_PRINT("decode_output_ptr malloc fail\r\n");
		goto _decode_exit;
	}
	else
		gp_memset((INT8S *)decode_output_ptr, 0x00, (width * height * 2));		


	//image source infomation
	image_source.type = SOURCE_TYPE_FS;                                    //image file infomation form file system
	
	image_decode.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;				//scaler out format
	//image output infomation
	image_decode.OutputBufPtr = (INT8U *)decode_output_ptr;           	//decode output buffer
	image_decode.OutputBufWidth = width;                  				//width of output buffer 
	image_decode.OutputBufHeight = height;                 				//Heigh of output buffer
	image_decode.OutputWidth = width;                     				//scaler width of output image
	image_decode.OutputHeight = height;                    				//scaler Heigh of output image
	image_decode.ScalerOutputRatio = FIT_OUTPUT_SIZE; 				 	//Fit to output_buffer_width and output_buffer_height for image output size
	image_decode.OutBoundaryColor = (INT32U)0x008080; 				 	//set the black for out of boundary color 		

	drv_l2_ad_key_init();
	
	decode_type = DECODE_JPG;
	type_flag = 0;	
	
	while(1) 
	{
		key = drv_l2_ad_key_scan();
	
		if(key == ADKEY_IO1) {
			if(type_flag == DECODE_TYPE_SELECT_END)
				type_flag = temp;
	
			if(type_flag == 1)
			{
				decode_type = DECODE_GIF;
				type_flag = 2;
				DBG_PRINT("DECODE_GIF\r\n");
			
			}else if(type_flag == 2)
			{
				decode_type = DECODE_BMP;
				type_flag = 0;
				DBG_PRINT("DECODE_BMP\r\n");			
			}else
			{
				decode_type = DECODE_JPG;
				type_flag = 1;
				DBG_PRINT("DECODE_JPG\r\n");
			}	
		}else if(key == ADKEY_IO2) {
			
			if(type_flag == DECODE_TYPE_SELECT_END){
				ret = _findnext(&file_info);			
			}else{
				if(decode_type == DECODE_JPG)
					ret = _findfirst("*.jpg", &file_info, D_ALL);
				else if(decode_type == DECODE_GIF)	
					ret = _findfirst("*.gif", &file_info, D_ALL);
				else
					ret = _findfirst("*.bmp", &file_info, D_ALL);
				
				temp = type_flag;
				type_flag = DECODE_TYPE_SELECT_END;
			}
			
			if(ret < 0) {
				DBG_PRINT("_findfirst fail...\r\n");
				if(type_flag == DECODE_TYPE_SELECT_END)
					type_flag = 0;
			}
			else
			{			
				DBG_PRINT("File Name = %s\r\n", file_info.f_name);
				image_source.type_ID.FileHandle=(INT32U)open((char *)file_info.f_name, O_RDONLY);    //open file				   
				if(image_source.type_ID.FileHandle < 0){
					DBG_PRINT("Open file fail !\r\n");
				}else{
					//image decode function
					image_decode_entrance();     										//global variable initial for image decode
					image_decode_end_register_callback(image_decode_end); 
					image_decode_start(image_decode,image_source);    					//image decode start
					
					while (1) {
						 decode_state = image_decode_status();
				         if (decode_state == IMAGE_CODEC_DECODE_END) {
							 close(image_source.type_ID.FileHandle);
							 break;
						  }else if(decode_state == IMAGE_CODEC_DECODE_FAIL) {
						  	 DBG_PRINT("image decode failed\r\n");
							 break;
						  }	
					}	
					image_decode_stop();
				}
			}	
		}else if(key == ADKEY_IO3) {
			DBG_PRINT("image_scaler_start\r\n");
			image_sacler_enable(decode_output_ptr, (width << 1));
			DBG_PRINT("image_scaler_stop\r\n");
		}else if(key == ADKEY_IO4) {
			DBG_PRINT("image_decode_exit\r\n");   
			break;
		}
	}
_decode_exit:				
	if(decode_output_ptr)
		gp_free((void *)decode_output_ptr);	
	
	image_decode_exit();	
	image_codec_demo_uninit();
		
	return STATUS_OK;
}

INT32S image_encode_enable(INT32U buffer_ptr)
{
	char path[64];
	INT16S fd;
	IMAGE_ENCODE_ARGUMENT encode_info;
	INT32U encode_output_ptr,encode_state,scaler_output_ptr;
	INT32U h_size,v_size,image_num,key,encode_type,type_flag,input_buf;
 	IMAGE_SCALE_ARGUMENT  scaler_arg; 
 	
	DBG_PRINT("\r\n***************************************************\r\n");
	DBG_PRINT("**                 AD_KEY Menu:                  **\r\n");
	DBG_PRINT("**KEY_1 image encode type selection              **\r\n");
	DBG_PRINT("**KEY_2 image encode start             			**\r\n");
	DBG_PRINT("**KEY_3 image encode demo exit            	    **\r\n");
	DBG_PRINT("***************************************************\r\n");
	
	if (DISKUSED == FS_SD1){
		DBG_PRINT("Media : SDCard \r\n");
		chdir("F:\\");			   
	}
	else if (DISKUSED == FS_SD){
		DBG_PRINT("Media : SDCard \r\n");
		chdir("C:\\");			   
	}
	else if (DISKUSED == FS_NAND1){
		DBG_PRINT("Media : NAND \r\n");
		chdir("A:\\");	
	}	

	if(width > 	ENCODE_H_SIZE)
		h_size = width;
	else
		h_size = ENCODE_H_SIZE;
		
	if(height > ENCODE_V_SIZE)
		v_size = height; 
	else
		v_size = ENCODE_V_SIZE;	

	encode_output_ptr = (INT32U) gp_malloc_align((h_size * v_size),32);//malloc decode frame buffer
	if(encode_output_ptr == 0)
	{
		DBG_PRINT("encode_output_ptr malloc fail\r\n");
		goto _encode_exit;
	}
	else
		gp_memset((INT8S *)encode_output_ptr, 0x00, (h_size * v_size));	
	
    //encode mode setting    
    encode_info.OutputBufPtr = (INT8U *)encode_output_ptr;
    encode_info.UseDisk = DISKUSED;	
    encode_info.QuantizationQuality = 50;                               	//encode quality
    encode_info.InputFormat = IMAGE_ENCODE_INPUT_FORMAT_YUYV;   			//encode input format
    encode_info.OutputFormat = IMAGE_ENCODE_OUTPUT_FORMAT_YUV422;       	//encode input format 	
	
	drv_l2_ad_key_init();
	
	encode_type = ENCODE_ONCE;
	type_flag = 0;	
	image_num = 0;
	scaler_output_ptr = 0;

	while(1)
	{
		key = drv_l2_ad_key_scan();
	
		if(key == ADKEY_IO1) {		
			if(type_flag == 1)
			{
				encode_type = ENCODE_BLOCK_READ_EN;
				type_flag = 2;
				DBG_PRINT("ENCODE_BLOCK_READ_EN\r\n");
			
			}else if(type_flag == 2)
			{
				encode_type = ENCODE_BLOCK_RW_EN;
				type_flag = 0;
				DBG_PRINT("ENCODE_BLOCK_RW_EN\r\n");			
			}else
			{
				encode_type = ENCODE_ONCE;
				type_flag = 1;
				DBG_PRINT("ENCODE_ONCE\r\n");
			}			
		}else if(key == ADKEY_IO2){
			//image encode infomation
			if (encode_type == ENCODE_BLOCK_READ_EN)
				sprintf((char *)path, (const char *)"image_encode_block_read%03d.jpeg",image_num++);
			else if (encode_type == ENCODE_BLOCK_RW_EN)
				sprintf((char *)path, (const char *)"image_encode_bloock_rw%03d.jpeg",image_num++);
			else
				sprintf((char *)path, (const char *)"image_encode_once%03d.jpeg",image_num++);		   
			
			fd = open((CHAR *)path, O_CREAT|O_RDWR);
			if (fd >= 0)
			{	    
			    if(encode_type != ENCODE_ONCE)
			    {
				    if(width != ENCODE_H_SIZE || height != ENCODE_V_SIZE)
				    {
						if(scaler_output_ptr == 0){
							scaler_output_ptr = (INT32U) gp_malloc_align((ENCODE_H_SIZE * ENCODE_V_SIZE * 2), 64);//malloc decode frame buffer
							if(encode_output_ptr == 0){
								DBG_PRINT("encode_output_ptr malloc fail\r\n");
								goto _encode_exit;
							}
							else
								gp_memset((INT8S *)encode_output_ptr, 0x00, (ENCODE_H_SIZE * ENCODE_V_SIZE * 2));	
							image_encode_scaler_start(buffer_ptr,scaler_output_ptr,width,height);
							input_buf = scaler_output_ptr;	
							goto encode_info;
							h_size = ENCODE_H_SIZE;
    						v_size = ENCODE_V_SIZE;						
						}else{
							input_buf = scaler_output_ptr;	
							goto encode_info;;
    					}	 	    
				    }
				    else
				    {
						input_buf = buffer_ptr;	
					encode_info:	
						h_size = ENCODE_H_SIZE;
						v_size = ENCODE_V_SIZE;						
						//image scaler output infomation
						scaler_arg.InputVisibleWidth = h_size;
						scaler_arg.InputVisibleHeight = BLOCK_READ_LINE;
						scaler_arg.InputWidth = h_size;
						scaler_arg.InputHeight = v_size;
						scaler_arg.InputOffsetX = 0;
						scaler_arg.InputOffsetY = 0;
						scaler_arg.OutputWidthFactor = (h_size << 16)/(h_size * 2);   				     			//scale x factor
						scaler_arg.OutputHeightFactor = (BLOCK_READ_LINE << 16)/(BLOCK_READ_LINE * 2);              //scale y factor, 240/16, 480/32
						scaler_arg.OutputWidth = (h_size * 2);
						scaler_arg.OutputHeight = (BLOCK_READ_LINE * 2);                                            //scale output y size length 
						scaler_arg.InputBufPtr = (INT8U *)input_buf;
						scaler_arg.OutBoundaryColor = (INT32U)0x008080;
						scaler_arg.InputFormat = SCALER_INPUT_FORMAT_YUYV;
						scaler_arg.OutputFormat = IMAGE_OUTPUT_FORMAT_YUYV;    
					  	encode_info.ScalerInfoPtr = (IMAGE_SCALE_ARGUMENT *)&scaler_arg;						
				    }		
			    }else{
			    	input_buf = buffer_ptr;	
			    	h_size = width;
    				v_size = height; 
			    }
			    //encode mode setting    
			    encode_info.FileHandle = fd;
			 
			    if (encode_type == ENCODE_BLOCK_READ_EN)
			    	encode_info.EncodeMode = IMAGE_ENCODE_BLOCK_READ;
			    else if(encode_type == ENCODE_BLOCK_RW_EN)
			    	encode_info.EncodeMode = IMAGE_ENCODE_BLOCK_READ_WRITE;
				else
					encode_info.EncodeMode = IMAGE_ENCODE_ONCE_READ;
			    			    
			    if(encode_info.EncodeMode == IMAGE_ENCODE_ONCE_READ)
			    {
				    encode_info.InputWidth = h_size;                       					//width of input image
				    encode_info.InputHeight = v_size;                        				//Heigh of input image
				    encode_info.InputBufPtr.yaddr = (INT8U *)input_buf;       		    	//encode input buffer			    
			    }else{
					encode_info.InputWidth = (h_size * 2);                       			    //width of input image
					encode_info.InputHeight = (v_size * 2);                        		    //Heigh of input image 
					encode_info.BlockLength = (BLOCK_READ_LINE * 2); 
				    encode_info.InputBufPtr.yaddr = 0;       		    					//encode input buffer	
	            }

				image_encode_entrance();
				image_encode_end_func_register(image_encdoe_end);
				image_encode_start(encode_info);
				while (1) {
					  encode_state = image_encode_status();
			          if (encode_state == IMAGE_CODEC_DECODE_END) {		
						DBG_PRINT("image encode ok\r\n");			
						break;
					  }else if(encode_state == IMAGE_CODEC_DECODE_FAIL) {
						DBG_PRINT("image encode failed\r\n");
						break;
					  }	
				}
				image_encode_stop();                                                     //image encode stop	
			}
			else
				DBG_PRINT("image_encode_error\r\n");			
		}else if(key == ADKEY_IO2){
			DBG_PRINT("image_encode_exit\r\n");
			break;
		}	
	}		
_encode_exit:	
	
	image_encode_exit();
	image_codec_demo_uninit();	
	
	return 0;
}

void image_decode_demo(void)
{
	image_decode_enable();
	while(1)
		OSTimeDly(1);	
}

void image_encode_demo(void)
{
	INT32S decode_ptr;
	
	decode_ptr = jpeg_simple_decode();
	if(decode_ptr)
		image_encode_enable(decode_ptr);
	else
		DBG_PRINT("image_encode_demo_error\r\n");
	while(1)
		OSTimeDly(1);	
}
	    
void image_decode_simple_demo(void)
{
#if	CODEC_SIMPLE_EN == 1
	jpeg_simple_decode();
#else
	image_decode_demo();
#endif
	while(1)
		OSTimeDly(1);		
}

void image_encode_simple_demo(void)
{
#if	CODEC_SIMPLE_EN == 1
	INT32U decode_ptr;

	decode_ptr = jpeg_simple_decode();
	if(decode_ptr)
		jpeg_simple_once_encode(decode_ptr);
	else
		DBG_PRINT("image_encode_simple_demo_error\r\n");
#else
	image_encode_demo();
#endif		
	while(1)
		OSTimeDly(1);		
}




