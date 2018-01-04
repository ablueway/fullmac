#ifndef __IMAGE_ENCODER_H__
#define __IMAGE_ENCODER_H__

#include "application.h"

typedef struct {
        INT32U       encode_buffer_ptr;
        INT32U       encode_size;        
} ENCODE_BLOCK_WRITE_ARGUMENT;

//Image Encode /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void image_encode_entrance(void);
extern void image_encode_exit(void);
extern INT32U image_encode_start(IMAGE_ENCODE_ARGUMENT arg);
extern void image_encode_stop(void);
extern IMAGE_CODEC_STATUS image_encode_status(void);
extern void image_encode_end_func_register(INT32U (*func)(INT32U encode_buf, INT32U encode_size));
extern void image_encode_block_read_func_register(INT32U (*func)(IMAGE_ENCODE_ARGUMENT *encode_info,IMAGE_ENCODE_PTR *encode_ptr));
#endif