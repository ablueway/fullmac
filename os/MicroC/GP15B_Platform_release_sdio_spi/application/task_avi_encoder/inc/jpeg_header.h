#ifndef __JPEG_HEADER_H__
#define __JPEG_HEADER_H__

#define JPEG_IMG_FORMAT_422		0x21
#define JPEG_IMG_FORMAT_420		0x22

extern INT32S jpeg_header_generate(INT32U yuv_fmt, INT32U quality, INT16U width, INT16U height);
extern INT8U *jpeg_header_get_addr(void);
extern INT32U jpeg_header_get_size(void);
#endif //__JPEG_HEADER_H__