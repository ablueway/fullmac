#ifndef __AP_GIF_H__
#define __AP_GIF_H__


#include "turnkey_image_task.h"

// GIF decode
#define C_GIF_DEC_WORKING_MEMORY_SIZE  	68000
#define C_GIF_DEC_FS_BUFFER_SIZE 		2048

#define C_GIF_STATUS_OK					0x00000000
#define C_GIF_STATUS_LINE_DONE			0x00000001
#define C_GIF_STATUS_FRAME_HEADER		0x80000000
#define C_GIF_STATUS_END_OF_FRAME		0x80000001
#define C_GIF_STATUS_END_OF_IMAGE		0x80000002
#define C_GIF_STATUS_NOT_DONE			0x80000080
#define C_GIF_STATUS_IMAGE_TOO_LARGE	0x80000082
#define C_GIF_STATUS_FILE_HAS_NO_FRAME	0x80000083
#define C_GIF_STATUS_ALLOC_SRAM			0x80000084
#define C_GIF_STATUS_NOT_A_GIF			0x80000085

extern INT32S gif_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S gif_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct);


#endif		// __AP_GIF_H__