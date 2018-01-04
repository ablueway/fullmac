/*
* Description: This file parse/decode/scale bmp images
*
* Author: Tristan Yang
*
* Date: 2009/08/17
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*/
#include "turnkey_filesrv_task.h"
#include "ap_bmp.h"
#include "gplib_bmp_decode.h"
#include "drv_l1_scaler.h"
#include "drv_l1_cache.h"

#define C_USE_SCALER_NUMBER				SCALER_0 
#define C_BMP_READ_FILE_BUFFER_SIZE1	512			// For parse header
#define C_BMP_READ_FILE_BUFFER_SIZE2	16384		// For decode bmp file
#define C_BMP_SCALE_FIFO_LINE_NUM		32

static INT32U bmp_output_format;
static INT8U bmp_output_ratio;             // 0=Fit to output_buffer_width and output_buffer_height, 1=Maintain ratio and fit to output_buffer_width or output_buffer_height, 2=Same as 1 but without scale up
static INT16U bmp_output_buffer_width;
static INT16U bmp_output_buffer_height;
static INT16U bmp_output_image_width;
static INT16U bmp_output_image_height;
static INT32U bmp_out_of_boundary_color;
static INT32U bmp_output_buffer_pointer;

static INT16U bmp_valid_width, bmp_valid_height;
static INT16U bmp_extend_width;
static INT32U bmp_decode_output;

// Scaler variables
static INT32U bmp_scaler_out_buffer;
static INT32U bmp_scaler_fifo_register = C_SCALER_CTRL_FIFO_32LINE;

INT32S bmp_scaler_set_parameters(INT32U fifo);
INT32S bmp_invert_image(void);
INT32S bmp_sprite_block_change_and_invert(INT32U start_addr, INT32U end_addr, INT8U block_w, INT8U block_h);

INT32S bmp_file_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser)
{
	TK_FILE_SERVICE_STRUCT fs_cmd;
	INT32U cmd_id;
	INT32S parse_status;
	INT32U fs_buffer_ptr;
	INT32S len;
	INT8U error;
  #if (defined GP_FILE_FORMAT_SUPPORT) && (GP_FILE_FORMAT_SUPPORT==GP_FILE_FORMAT_SET_1)
  	INT8U bmp_xor_flag = 1;
  #endif

	cmd_id = parser->cmd_id;

	fs_buffer_ptr = (INT32U) gp_malloc_align(C_BMP_READ_FILE_BUFFER_SIZE1, 16);
	if (fs_buffer_ptr == NULL) {
		DBG_PRINT("bmp_file_parse_header() failed to allocate memory\r\n");
		parser->parse_status = -2;

		return -2;
	}

	fs_cmd.fd = (INT16S) parser->image_source;
	fs_cmd.result_queue = image_task_fs_queue_a;
	fs_cmd.buf_addr = (INT32U) fs_buffer_ptr;
	fs_cmd.buf_size = C_BMP_READ_FILE_BUFFER_SIZE1;

	lseek((INT16S) parser->image_source, 0, SEEK_SET);

	bmp_decode_init();
	do {
		msgQSend(fs_msg_q_id, MSG_FILESRV_FS_READ, (void *) &fs_cmd, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_NORMAL);
		len = (INT32S) OSQPend(image_task_fs_queue_a, 200, &error);
		if (error!=OS_NO_ERR || len<0) {
			parser->parse_status = STATUS_FAIL;
			gp_free((void *) fs_buffer_ptr);

			return STATUS_FAIL;
		}

		// Check whether we have to break decoding this image
		if (image_task_handle_remove_request(cmd_id) > 0) {
			parser->parse_status = STATUS_FAIL;
			gp_free((void *) fs_buffer_ptr);

			return STATUS_FAIL;
		}

	  #if (defined GP_FILE_FORMAT_SUPPORT) && (GP_FILE_FORMAT_SUPPORT==GP_FILE_FORMAT_SET_1)
		if (bmp_xor_flag) {
			INT8U *ptr8;

			ptr8 = (INT8U *) fs_buffer_ptr;
			if (*ptr8==0xBD && *(ptr8+1)==0xB2) {
				INT32U *ptr32;
				INT8U i;

				ptr32 = (INT32U *) fs_buffer_ptr;
				for (i=0; i<128; i++) {
					*ptr32++ ^= 0xFFFFFFFF;
				}
			}
		}
		bmp_xor_flag = 0;
	  #endif

		parse_status = bmp_decode_parse_header((INT8U *) fs_buffer_ptr, len);
	} while (parse_status == BMP_PARSE_NOT_DONE) ;
	if (parse_status != BMP_PARSE_OK) {
		DBG_PRINT("Parse header failed. Skip this file\r\n");
		parser->parse_status = STATUS_FAIL;
		gp_free((void *) fs_buffer_ptr);

		return STATUS_FAIL;
	}

	parser->width = bmp_decode_image_width_get();
	parser->height = bmp_decode_image_height_get();

	parser->parse_status = STATUS_OK;
	gp_free((void *) fs_buffer_ptr);

	return STATUS_OK;
}

INT32S bmp_buffer_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser)
{
	INT32S parse_status;
  #if (defined GP_FILE_FORMAT_SUPPORT) && (GP_FILE_FORMAT_SUPPORT==GP_FILE_FORMAT_SET_1)
	INT8U *ptr8;

	ptr8 = (INT8U *) parser->image_source;
	if (*ptr8==0xBD && *(ptr8+1)==0xB2) {
		INT32U *ptr32;
		INT8U i;

		ptr32 = (INT32U *) parser->image_source;
		for (i=0; i<128; i++) {
			*ptr32++ ^= 0xFFFFFFFF;
		}
	}
  #endif

	parse_status = bmp_decode_parse_header((INT8U *) parser->image_source, parser->source_size);
	if (parse_status != BMP_PARSE_OK) {
		DBG_PRINT("bmp_buffer_parse_header() failed. Skip this file\r\n");
		parser->parse_status = STATUS_FAIL;

		return STATUS_FAIL;
	}

	parser->width = bmp_decode_image_width_get();
	parser->height = bmp_decode_image_height_get();

	parser->parse_status = STATUS_OK;

	return STATUS_OK;
}

INT32S bmp_scaler_set_parameters(INT32U fifo)
{
	INT32U factor;

	drv_l1_scaler_input_pixels_set(C_USE_SCALER_NUMBER,bmp_extend_width, bmp_valid_height);
	drv_l1_scaler_input_visible_pixels_set(C_USE_SCALER_NUMBER,bmp_valid_width, bmp_valid_height);
	if (bmp_output_image_width && bmp_output_image_height) {
	    drv_l1_scaler_output_pixels_set(C_USE_SCALER_NUMBER,(bmp_valid_width<<16)/bmp_output_image_width, (bmp_valid_height<<16)/bmp_output_image_height, bmp_output_buffer_width, bmp_output_buffer_height);
	} else {
	    if (!bmp_output_image_width) {
	        bmp_output_image_width = bmp_output_buffer_width;
	    }
	    if (!bmp_output_image_height) {
	        bmp_output_image_height = bmp_output_buffer_height;
	    }
    	if (bmp_output_ratio == 0x0) {      // Fit to output buffer width and height
      		drv_l1_scaler_output_pixels_set(C_USE_SCALER_NUMBER,(bmp_valid_width<<16)/bmp_output_image_width, (bmp_valid_height<<16)/bmp_output_image_height, bmp_output_buffer_width, bmp_output_buffer_height);
    	} else if (bmp_output_ratio==2 && bmp_valid_width<=bmp_output_image_width && bmp_valid_height<=bmp_output_image_height) {
    		drv_l1_scaler_output_pixels_set(C_USE_SCALER_NUMBER,1<<16, 1<<16, bmp_output_buffer_width, bmp_output_buffer_height);
    		bmp_output_image_width = bmp_valid_width;
    		bmp_output_image_height = bmp_output_buffer_height;
    	} else {						// Fit to output buffer width or height
      		if (bmp_output_image_height*bmp_valid_width > bmp_output_image_width*bmp_valid_height) {
      			factor = (bmp_valid_width<<16)/bmp_output_image_width;
      			bmp_output_image_height = (bmp_valid_height<<16)/factor;
      		} else {
      			factor = (bmp_valid_height<<16)/bmp_output_image_height;
      			bmp_output_image_width = (bmp_valid_width<<16)/factor;
      		}
      		drv_l1_scaler_output_pixels_set(C_USE_SCALER_NUMBER,factor, factor, bmp_output_buffer_width, bmp_output_buffer_height);
      	}
    }

	//scaler_input_addr_set(bmp_decode_output, 0, 0);
	drv_l1_scaler_input_A_addr_set(C_USE_SCALER_NUMBER,bmp_decode_output, 0, 0);
	drv_l1_scaler_input_B_addr_set(C_USE_SCALER_NUMBER,(bmp_decode_output+(bmp_extend_width*C_BMP_SCALE_FIFO_LINE_NUM*2)), 0, 0);

	if (bmp_output_buffer_pointer) {
	    bmp_scaler_out_buffer = bmp_output_buffer_pointer;
	} else {
	    bmp_scaler_out_buffer = (INT32U) gp_malloc((bmp_output_buffer_width*bmp_output_buffer_height)<<1);
	    if (!bmp_scaler_out_buffer) {
		    return -2;
		}
	}

 	drv_l1_scaler_output_addr_set(C_USE_SCALER_NUMBER,bmp_scaler_out_buffer, NULL, NULL);
 	drv_l1_scaler_fifo_line_set(C_USE_SCALER_NUMBER,fifo);
	drv_l1_scaler_YUV_type_set(C_USE_SCALER_NUMBER,C_SCALER_CTRL_TYPE_YCBCR);
	drv_l1_scaler_input_format_set(C_USE_SCALER_NUMBER,C_SCALER_CTRL_IN_RGB565);

	drv_l1_scaler_output_format_set(C_USE_SCALER_NUMBER,bmp_output_format);
	drv_l1_scaler_out_of_boundary_mode_set(C_USE_SCALER_NUMBER,1);			// Use Use color defined in scaler_out_of_boundary_color_set()
	drv_l1_scaler_out_of_boundary_color_set(C_USE_SCALER_NUMBER,bmp_out_of_boundary_color);

	return STATUS_OK;
}

INT32S bmp_file_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct)
{
	INT32U cmd_id;
	INT32S parse_status;
	INT8U *bmp_input_data_addr;
	INT32U header_len;
  	INT32U fly_len;
  	INT32S bmp_status;
  	INT32S scaler_status=0;
	INT8U scaler_done;
	TK_FILE_SERVICE_STRUCT fs_cmd_a, fs_cmd_b;
	INT32S read_cnt;
	INT8U error, fifo_a;
  #if (defined GP_FILE_FORMAT_SUPPORT) && (GP_FILE_FORMAT_SUPPORT==GP_FILE_FORMAT_SET_1)
  	INT8U bmp_xor_flag = 1;
  #endif

	if (!img_decode_struct) {
		return STATUS_FAIL;
	}
	if (!image_task_fs_queue_a || !image_task_fs_queue_b) {
		img_decode_struct->decode_status = STATUS_FAIL;

		return STATUS_FAIL;
	}

	cmd_id = img_decode_struct->cmd_id;
	bmp_output_format = img_decode_struct->output_format;
    bmp_output_ratio = img_decode_struct->output_ratio;
    bmp_output_buffer_width = img_decode_struct->output_buffer_width;
    bmp_output_buffer_height = img_decode_struct->output_buffer_height;
    bmp_output_image_width = img_decode_struct->output_image_width;
    bmp_output_image_height = img_decode_struct->output_image_height;
    bmp_out_of_boundary_color = img_decode_struct->out_of_boundary_color;
    bmp_output_buffer_pointer = img_decode_struct->output_buffer_pointer;

	// Setup a command that will be used to read data into fifo A
	fs_cmd_a.fd = (INT16S) img_decode_struct->image_source;
	fs_cmd_a.buf_addr = (INT32U) gp_malloc_align(C_BMP_READ_FILE_BUFFER_SIZE2, 16);
	if (!fs_cmd_a.buf_addr) {
    	DBG_PRINT("bmp_file_decode_and_scale() failed to allocate fs_cmd_a.buf_addr");
		img_decode_struct->decode_status = -2;

		return -2;
  	}
	fs_cmd_a.buf_size = C_BMP_READ_FILE_BUFFER_SIZE2;
	fs_cmd_a.result_queue = image_task_fs_queue_a;

	// Setup a command that will be used to read data into fifo B
	fs_cmd_b.fd = (INT16S) img_decode_struct->image_source;
	fs_cmd_b.buf_addr = (INT32U) gp_malloc_align(C_BMP_READ_FILE_BUFFER_SIZE2, 16);
	if (!fs_cmd_b.buf_addr) {
		DBG_PRINT("bmp_file_decode_and_scale() failed to allocate fs_cmd_b.buf_addr");
		img_decode_struct->decode_status = -2;
		gp_free((void *) fs_cmd_a.buf_addr);

		return -2;
	}
	fs_cmd_b.buf_size = C_BMP_READ_FILE_BUFFER_SIZE2;
	fs_cmd_b.result_queue = image_task_fs_queue_b;

	fifo_a = 1;				// We will read data to FIFO A first and then FIFO B
	header_len = 0;

	// Seek file pointer to begin
	lseek((INT16S) img_decode_struct->image_source, 0, SEEK_SET);

	// Initiate software header parser and hardware engine
	bmp_decode_init();
   	do {
		// Send this command to file server task
		msgQSend(fs_msg_q_id, MSG_FILESRV_FS_READ, (void *) &fs_cmd_a, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_NORMAL);
		read_cnt = (INT32S) OSQPend(image_task_fs_queue_a, 200, &error);
		if ((error!=OS_NO_ERR) || (read_cnt<=0)) {
			img_decode_struct->decode_status = STATUS_FAIL;
			gp_free((void *) fs_cmd_b.buf_addr);
			gp_free((void *) fs_cmd_a.buf_addr);

			return STATUS_FAIL;
	  	}

		// Check whether we have to break decoding this image
		if (image_task_handle_remove_request(cmd_id) > 0) {
			img_decode_struct->decode_status = STATUS_FAIL;
			gp_free((void *) fs_cmd_b.buf_addr);
			gp_free((void *) fs_cmd_a.buf_addr);

			return STATUS_FAIL;
		}

	  #if (defined GP_FILE_FORMAT_SUPPORT) && (GP_FILE_FORMAT_SUPPORT==GP_FILE_FORMAT_SET_1)
		if (bmp_xor_flag) {
			INT8U *ptr8;

			ptr8 = (INT8U *) fs_cmd_a.buf_addr;
			if (*ptr8==0x00 && *(ptr8+1)==0x27) {
				INT32U *ptr32;
				INT8U i;

				ptr32 = (INT32U *) fs_cmd_a.buf_addr;
				for (i=0; i<128; i++) {
					*ptr32++ ^= 0xFFFFFFFF;
				}
			}
		}
		bmp_xor_flag = 0;
	  #endif

	  	parse_status = bmp_decode_parse_header((INT8U *) fs_cmd_a.buf_addr, read_cnt);

   		if (parse_status == BMP_PARSE_NOT_DONE) {
   			header_len += read_cnt;
   		}
	} while (parse_status == BMP_PARSE_NOT_DONE) ;
	if (parse_status != BMP_PARSE_OK) {
		DBG_PRINT("Parse header failed. Skip this file\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
		gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);

	  	return STATUS_FAIL;
	}

	bmp_valid_width = bmp_decode_image_width_get();
	bmp_extend_width = bmp_valid_width + (bmp_valid_width & 0x1);
	bmp_valid_height = bmp_decode_image_height_get();
	if (!bmp_valid_width || !bmp_valid_height) {
		DBG_PRINT("BMP width or height is 0. Skip this file\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
		gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);

	  	return STATUS_FAIL;
	}

	bmp_decode_output = (INT32U) gp_malloc((bmp_extend_width*C_BMP_SCALE_FIFO_LINE_NUM*2)*2);	// RGB565 or RGB1555, A+B buffers
	if (!bmp_decode_output) {
		DBG_PRINT("Failed to allocate bmp_decode_output\r\n");
		img_decode_struct->decode_status = -2;
		gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);

		return -2;
	}

	if (bmp_decode_output_set((INT32U *) bmp_decode_output, C_BMP_SCALE_FIFO_LINE_NUM)) {
		DBG_PRINT("Failed to call bmp_decode_output_set()\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
	  	gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);
	  	gp_free((void *) bmp_decode_output);

		return STATUS_FAIL;
	}

	// Send a command to read fifo_b
	msgQSend(fs_msg_q_id, MSG_FILESRV_FS_READ, (void *) &fs_cmd_b, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_NORMAL);
	fifo_a ^= 0x1;

	bmp_input_data_addr = bmp_decode_data_addr_get();

	if ((INT32U) bmp_input_data_addr == (fs_cmd_a.buf_addr + read_cnt)) {
		msgQSend(fs_msg_q_id, MSG_FILESRV_FS_READ, (void *) &fs_cmd_a, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_NORMAL);
		fifo_a ^= 0x1;
		read_cnt = (INT32S) OSQPend(image_task_fs_queue_b, 200, &error);
		if ((error!=OS_NO_ERR) || (read_cnt<=0)) {
			DBG_PRINT("Failed to read BMP file\r\n");
			img_decode_struct->decode_status = STATUS_FAIL;
			OSQPend(image_task_fs_queue_a, 200, &error);
		  	gp_free((void *) fs_cmd_a.buf_addr);
    		gp_free((void *) fs_cmd_b.buf_addr);
    	  	gp_free((void *) bmp_decode_output);

	  		return STATUS_FAIL;
	  	}
		bmp_input_data_addr = (INT8U *) fs_cmd_b.buf_addr;
		fly_len = (fs_cmd_b.buf_addr+read_cnt) - ((INT32U) bmp_input_data_addr);
	} else {
		fly_len = (fs_cmd_a.buf_addr+read_cnt) - ((INT32U) bmp_input_data_addr);
	}

   	// Now start BMP decoding on the fly
	if (bmp_decode_on_the_fly_start(bmp_input_data_addr, fly_len)) {
		DBG_PRINT("Failed to call bmp_decode_on_the_fly_start()\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
		if (fifo_a) {
			OSQPend(image_task_fs_queue_a, 200, &error);
		} else {
			OSQPend(image_task_fs_queue_b, 200, &error);
		}
	  	gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);
	  	gp_free((void *) bmp_decode_output);

		return STATUS_FAIL;
	}

  	// Initiate Scaler
  	drv_l1_scaler_init(C_USE_SCALER_NUMBER);
  	scaler_done = 0;

	// Setup Scaler
	bmp_scaler_set_parameters(bmp_scaler_fifo_register);
  	if (!bmp_scaler_out_buffer) {
		bmp_decode_stop();
		DBG_PRINT("Failed to allocate bmp_scaler_out_buffer\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
		if (fifo_a) {
			OSQPend(image_task_fs_queue_a, 200, &error);
		} else {
			OSQPend(image_task_fs_queue_b, 200, &error);
		}
	  	gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);
	  	gp_free((void *) bmp_decode_output);

		return STATUS_FAIL;
  	}

  	while (1) {
  		bmp_status = bmp_decode_status_query();

		if (bmp_status & C_BMP_STATUS_DECODE_DONE) {
		  	// Wait until scaler finish its job
		  	while (!scaler_done) {
		  		scaler_status = drv_l1_scaler_wait_idle(C_USE_SCALER_NUMBER);
		  		if (scaler_status == C_SCALER_STATUS_STOP) {
					if (drv_l1_scaler_start(C_USE_SCALER_NUMBER,ENABLE)) {
						DBG_PRINT("Failed to call scaler_start\r\n");
						break;
					}
				} else if (scaler_status & C_SCALER_STATUS_DONE) {
					break;
				} else if (scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) {
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
		  			if (drv_l1_scaler_restart(C_USE_SCALER_NUMBER)) {
						DBG_PRINT("Failed to call scaler_restart\r\n");
						break;
					}
		  		} else {
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}
		  	}
			break;
		}

  		if (bmp_status & C_BMP_STATUS_INPUT_EMPTY) {
			if (fifo_a) {
				msgQSend(fs_msg_q_id, MSG_FILESRV_FS_READ, (void *) &fs_cmd_b, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_NORMAL);
				fifo_a ^= 0x1;
	   			read_cnt = (INT32S) OSQPend(image_task_fs_queue_a, 200, &error);
   				if ((error!=OS_NO_ERR) || (read_cnt<0)) {
	   				DBG_PRINT("Failed to read more data from BMP file\r\n");
		  			break;
			  	}

				// Now restart BMP decoding on the fly
		  		if (bmp_decode_on_the_fly_start((INT8U *) fs_cmd_a.buf_addr, fs_cmd_a.buf_size)) {
		  			DBG_PRINT("Failed to call bmp_decode_on_the_fly_start()\r\n");
		  			break;
		  		}
			} else {
				msgQSend(fs_msg_q_id, MSG_FILESRV_FS_READ, (void *) &fs_cmd_a, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_NORMAL);
				fifo_a ^= 0x1;
	   			read_cnt = (INT32S) OSQPend(image_task_fs_queue_b, 200, &error);
   				if ((error!=OS_NO_ERR) || (read_cnt<0)) {
	   				DBG_PRINT("Failed to read more data from BMP file\r\n");
		  			break;
			  	}

				// Now restart BMP decoding on the fly
		  		if (bmp_decode_on_the_fly_start((INT8U *) fs_cmd_b.buf_addr, fs_cmd_b.buf_size)) {
		  			DBG_PRINT("Failed to call bmp_decode_on_the_fly_start()\r\n");
		  			break;
		  		}
			}
		}

		if (bmp_status & C_BMP_STATUS_OUTPUT_FULL) {
		  	// Start scaler to handle the full output FIFO now
		  	if (!scaler_done) {
			  	scaler_status = drv_l1_scaler_wait_idle(C_USE_SCALER_NUMBER);
			  	if (scaler_status == C_SCALER_STATUS_STOP) {
					if (drv_l1_scaler_start(C_USE_SCALER_NUMBER,ENABLE)) {
						DBG_PRINT("Failed to call scaler_start\r\n");
						break;
					}
			  	} else if (scaler_status & C_SCALER_STATUS_DONE) {
			  		// Scaler might finish its job before BMP does when image is zoomed in.
			  		scaler_done = 1;
			  	} else if (scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) {
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
			  		if (drv_l1_scaler_restart(C_USE_SCALER_NUMBER)) {
						DBG_PRINT("Failed to call scaler_restart\r\n");
						break;
					}
			  	} else {
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}
			}

	  		// Now restart BMP to output to next FIFO
	  		if (bmp_decode_output_restart()) {
	  			DBG_PRINT("Failed to call bmp_decode_output_restart()\r\n");
	  			break;
	  		}
		}

		if (bmp_status & C_BMP_STATUS_STOP) {
			DBG_PRINT("BMP is not started!\r\n");
			break;
		}
		if (bmp_status & C_BMP_STATUS_INIT_ERR) {
			DBG_PRINT("BMP init error!\r\n");
			break;
		}

		// Check whether we have to break decoding this image
		if (image_task_handle_remove_request(cmd_id) > 0) {
			break;
		}
  	}

	bmp_decode_stop();

	drv_l1_scaler_stop(C_USE_SCALER_NUMBER);

  	gp_free((void *) bmp_decode_output);

	if ((bmp_status & C_BMP_STATUS_DECODE_DONE) && (scaler_status & C_SCALER_STATUS_DONE)) {
	    if (!bmp_output_buffer_pointer) {
	        bmp_output_buffer_pointer = bmp_scaler_out_buffer;
	    }
	    cache_invalid_range(bmp_scaler_out_buffer, (bmp_output_buffer_width*bmp_output_buffer_height)<<1);
	} else {
		img_decode_struct->decode_status = STATUS_FAIL;
    	if (!bmp_output_buffer_pointer) {
    	    gp_free((void *) bmp_scaler_out_buffer);
    	}
	  	if (fifo_a) {
			OSQPend(image_task_fs_queue_a, 200, &error);
		} else {
			OSQPend(image_task_fs_queue_b, 200, &error);
		}
	  	gp_free((void *) fs_cmd_a.buf_addr);
		gp_free((void *) fs_cmd_b.buf_addr);

        return STATUS_FAIL;
	}

	if (bmp_decode_line_direction_get() == 0) {		// BMP line direction is bottom to top, invert scaler output image
		bmp_invert_image();
	}
  	
  	if (fifo_a) {
		OSQPend(image_task_fs_queue_a, 200, &error);
	} else {
		OSQPend(image_task_fs_queue_b, 200, &error);
	}
  	gp_free((void *) fs_cmd_a.buf_addr);
	gp_free((void *) fs_cmd_b.buf_addr);

    img_decode_struct->decode_status = STATUS_OK;
    if (!img_decode_struct->output_buffer_pointer && bmp_output_buffer_pointer) {
        img_decode_struct->output_buffer_pointer = bmp_output_buffer_pointer;
    }
    img_decode_struct->output_image_width = bmp_output_image_width;
    img_decode_struct->output_image_height = bmp_output_image_height;

	return STATUS_OK;
}

INT32S bmp_invert_image(void)
{
	INT32U buffer_size;
	INT32U temp_buffer;
	INT32U start_addr, end_addr;
	INT16U invert_height;

	if (bmp_output_image_height <= bmp_output_buffer_height) {
		invert_height = bmp_output_image_height;
	} else {
		invert_height = bmp_output_buffer_height;
	}
	if (invert_height <= 1) {
		return 0;
	}
	if (bmp_output_format == C_SCALER_CTRL_OUT_RGB565 ||
		bmp_output_format == C_SCALER_CTRL_OUT_YUYV ||
		bmp_output_format == C_SCALER_CTRL_OUT_UYVY)  {	
		
		buffer_size = bmp_output_buffer_width << 1;
		temp_buffer = (INT32U) gp_malloc(buffer_size);
		if (!temp_buffer) {
			return -2;
		}

		start_addr = (INT32U) bmp_output_buffer_pointer;
		end_addr = start_addr + buffer_size*(invert_height - 1);
		while (start_addr < end_addr) {
			INT32U *ptr1, *ptr2;
			INT16U size;

			ptr1 = (INT32U *) temp_buffer;
			ptr2 = (INT32U *) start_addr;
			size = buffer_size;
			while (size) {
				*ptr1++ = *ptr2++;
				size -= 4;
			}

			ptr1 = (INT32U *) start_addr;
			ptr2 = (INT32U *) end_addr;
			size = buffer_size;
			while (size) {
				*ptr1++ = *ptr2++;
				size -= 4;
			}

			ptr1 = (INT32U *) end_addr;
			ptr2 = (INT32U *) temp_buffer;
			size = buffer_size;
			while (size) {
				*ptr1++ = *ptr2++;
				size -= 4;
			}

			start_addr += buffer_size;
			end_addr -= buffer_size;
		}
	} else {
		// Invert block by block
		INT32U buf_sprite_num_x, image_sprite_num_x;
		INT16U block_w, block_h;

		switch (bmp_output_format) 
		{
			default:
				return -1;
		}
		
		buf_sprite_num_x = bmp_output_buffer_width / block_w;
		image_sprite_num_x = (bmp_output_image_width + block_w - 1) / block_w;

		if (invert_height & (block_h-1)) {
			INT32U line_block_size;
			INT16U start_x, start_y, end_y;
			INT16U i;

			temp_buffer = (INT32U) gp_malloc(block_w<<1);
			if (!temp_buffer) {
				return -2;
			}

			line_block_size = buffer_size*buf_sprite_num_x;
			start_x = 0;
			for (i=0; (i<image_sprite_num_x) && (i<buf_sprite_num_x); i++) {
				start_y = 0;
				end_y = invert_height - 1;
				while (start_y < end_y) {
					INT32U *ptr1, *ptr2;
					INT16U temp_x, temp_y;
					INT16U size;

					start_addr = (INT32U) bmp_output_buffer_pointer;
					temp_y = start_y;
					while (temp_y >= block_h) {
						start_addr += line_block_size;
						temp_y -= block_h;
					}
					temp_x = start_x;
					while (temp_x >= block_w) {
						start_addr += buffer_size;
						temp_x -= block_w;
					}
					while (temp_y) {
						start_addr += block_w << 1;
						temp_y--;
					}

					end_addr = (INT32U) bmp_output_buffer_pointer;
					temp_y = end_y;
					while (temp_y >= block_h) {
						end_addr += line_block_size;
						temp_y -= block_h;
					}
					temp_x = start_x;
					while (temp_x >= block_w) {
						end_addr += buffer_size;
						temp_x -= block_w;
					}
					while (temp_y) {
						end_addr += block_w << 1;
						temp_y--;
					}

					ptr1 = (INT32U *) temp_buffer;
					ptr2 = (INT32U *) start_addr;
					size = block_w<<1;
					while (size) {
						*ptr1++ = *ptr2++;
						size -= 4;
					}
					ptr1 = (INT32U *) start_addr;
					ptr2 = (INT32U *) end_addr;
					size = block_w<<1;
					while (size) {
						*ptr1++ = *ptr2++;
						size -= 4;
					}
					ptr1 = (INT32U *) end_addr;
					ptr2 = (INT32U *) temp_buffer;
					size = block_w<<1;
					while (size) {
						*ptr1++ = *ptr2++;
						size -= 4;
					}

					start_y++;
					end_y--;
				}
				start_x += block_w;
			}

		} else {
			temp_buffer = (INT32U) gp_malloc(buffer_size);
			if (!temp_buffer) {
				return -2;
			}

			start_addr = (INT32U) bmp_output_buffer_pointer;
			end_addr = start_addr + buffer_size * buf_sprite_num_x * ((invert_height-1)/block_h);
			while (start_addr < end_addr) {
				INT16U i;

				for (i=0; i<buf_sprite_num_x; i++) {
					if (i < image_sprite_num_x) {
						INT32U *ptr1, *ptr2;
						INT16U size;

						ptr1 = (INT32U *) temp_buffer;
						ptr2 = (INT32U *) start_addr;
						size = buffer_size;
						while (size) {
							*ptr1++ = *ptr2++;
							size -= 4;
						}

						bmp_sprite_block_change_and_invert(end_addr, start_addr, block_w, block_h);
						bmp_sprite_block_change_and_invert(temp_buffer, end_addr, block_w, block_h);
					}
					start_addr += buffer_size;
					end_addr += buffer_size;
				}
				end_addr -= (buffer_size * buf_sprite_num_x) << 1;
			}
			if (start_addr == end_addr) {
				INT16U i;

				for (i=0; (i<image_sprite_num_x) && (i<buf_sprite_num_x); i++) {
					INT32U *ptr1, *ptr2;
					INT16U size;

					ptr1 = (INT32U *) temp_buffer;
					ptr2 = (INT32U *) start_addr;
					size = buffer_size;
					while (size) {
						*ptr1++ = *ptr2++;
						size -= 4;
					}

					bmp_sprite_block_change_and_invert(temp_buffer, end_addr, block_w, block_h);
					start_addr += buffer_size;
					end_addr += buffer_size;
				}
			}
		}
	}
	gp_free((void *) temp_buffer);

	return 0;
}

INT32S bmp_sprite_block_change_and_invert(INT32U src_addr, INT32U target_addr, INT8U block_w, INT8U block_h)
{
	INT32U buffer_size;
	INT8U i;

	buffer_size = block_w << 1;
	target_addr += (block_h - 1) * buffer_size;
	for (i=0; i<block_h; i++) {
		INT32U *ptr1, *ptr2;
		INT16U size;

		ptr1 = (INT32U *) target_addr;
		ptr2 = (INT32U *) src_addr;
		size = buffer_size;
		while (size) {
			*ptr1++ = *ptr2++;
			size -= 4;
		}

		src_addr += buffer_size;
		target_addr -= buffer_size;
	}

	return 0;
}

INT32S bmp_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct)
{
	return 0;
}
