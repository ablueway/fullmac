#ifndef __TURNKEY_IMAGE_TASK_H__
#define __TURNKEY_IMAGE_TASK_H__

#include "application.h"

extern OS_EVENT *image_task_fs_queue_a;
extern OS_EVENT *image_task_fs_queue_b;

extern void image_task_entry(void *p_arg);
extern INT32S image_task_handle_remove_request(INT32U current_image_id);
extern INT32S image_task_scale_image(IMAGE_SCALE_STRUCT *para);
extern INT32S image_task_parse_image(IMAGE_HEADER_PARSE_STRUCT *para);
extern INT32S image_task_remove_request(INT32U state_id);

extern INT32S image_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S image_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct);
extern INT32S image_decode_ext_and_scale(IMAGE_DECODE_EXT_STRUCT *img_decode_ext_struct);
extern INT32S image_encode(IMAGE_ENCODE_STRUCT *img_encode_struct);
extern INT32S image_scale(IMAGE_SCALE_STRUCT *img_scale_struct);

// JPEG relative APIs
extern INT32S jpeg_file_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S jpeg_buffer_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S jpeg_file_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U clip_xy, INT32U clip_size);
extern INT32S jpeg_buffer_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct, INT32U clip_xy, INT32U clip_size);
extern INT32S jpeg_buffer_encode(IMAGE_ENCODE_STRUCT *img_encode_struct);

// Progressive JPEG relative APIs
extern INT32S jpeg_decode_progressive(IMAGE_DECODE_STRUCT *img_decode_struct);

// GIF relative APIs
extern INT32S gif_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S gif_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct);

///BMP relative APIs
extern INT32S bmp_file_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S bmp_buffer_parse_header(IMAGE_HEADER_PARSE_STRUCT *parser);
extern INT32S bmp_file_decode_and_scale(IMAGE_DECODE_STRUCT *img_decode_struct);
#endif		// __TURNKEY_IMAGE_TASK_H__
