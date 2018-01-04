/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "gplib_jpeg_encode.h"
#include "drv_l1_jpeg.h"
#include "gplib_jpeg.h"

#if GPLIB_JPEG_ENCODE_EN == 1

static INT16U saved_quant_luminance[64];
static INT16U saved_quant_chrominance[64];
static INT8U saved_lum_quality;
static INT8U saved_chrom_quality;

extern INT16U quant50_luminance_table[];
extern INT16U quant50_chrominance_table[]; 

static INT8U jpeg_encode_started = 0;

void jpeg_encode_init(void)
{
	drv_l1_jpeg_init();

	jpeg_encode_started = 0;
}

// This API is called to fill quantization table of JPEG header
void jpeg_header_quantization_table_calculate(JPEG_QTABLE_ENUM table, INT32U quality, INT8U *qtable)
{
	INT32U i;
	INT16U *ptr;

	if (quality >= 100) {
		if (table == ENUM_JPEG_LUMINANCE_QTABLE) {
			for (i=0; i<64; i++) {
				if (qtable) {
					*qtable++ = 0x1;
				}
				saved_quant_luminance[i] = 0x1;
			}
			drv_l1_jpeg_quant_luminance_set(0, 64, &saved_quant_luminance[0]);
		} else {
			for (i=0; i<64; i++) {
				if (qtable) {
					*qtable++ = 0x1;
				}
				saved_quant_chrominance[i] = 0x1;
			}
			drv_l1_jpeg_quant_chrominance_set(0, 64, &saved_quant_chrominance[0]);
		}
		return;
	} else if (quality == 50) {
		if (table == ENUM_JPEG_LUMINANCE_QTABLE) {
			ptr = &quant50_luminance_table[0];
			drv_l1_jpeg_quant_luminance_set(0, 64, &quant50_luminance_table[0]);
		} else {
			ptr = &quant50_chrominance_table[0];
			drv_l1_jpeg_quant_chrominance_set(0, 64, &quant50_chrominance_table[0]);
		}
		if (qtable) {
			for (i=0; i<64; i++) {
				*qtable++ = (INT8U) ptr[zigzag_scan[i]];
			}
		}
		return;
	} else if (quality == 0) {
		quality = 30;
	}
	if (table == ENUM_JPEG_LUMINANCE_QTABLE) {
		if (saved_lum_quality != quality) {
			INT32U scale_factor, temp;

		    if (quality < 50) {
		    	scale_factor = 5000 / quality;
		    } else {
		    	scale_factor = 200 - (quality << 1);
		    }

		    for (i=0; i<64; i++) {
		    	// Calculate DC quantization table
				temp = quant50_luminance_table[i] * scale_factor + 50;
				if (temp >= 25500) {
					saved_quant_luminance[i] = 0x00FF;
				} else if (temp <= 199) {
					saved_quant_luminance[i] = 0x0001;
				} else {
					saved_quant_luminance[i] = (INT16U) (temp / 100);
				}
		    }
		    saved_lum_quality = quality;
		}
		ptr = &saved_quant_luminance[0];
		drv_l1_jpeg_quant_luminance_set(0, 64, &saved_quant_luminance[0]);
	} else {
		if (saved_chrom_quality != quality) {
			INT32U scale_factor, temp;

		    if (quality < 50) {
		    	scale_factor = 5000 / quality;
		    } else {
		    	scale_factor = 200 - (quality << 1);
		    }

		    for (i=0; i<64; i++) {
				// Calculate AC quantization table
				temp = quant50_chrominance_table[i] * scale_factor + 50;
				if (temp >= 25500) {
					saved_quant_chrominance[i] = 0x00FF;
				} else if (temp <= 199) {
					saved_quant_chrominance[i] = 0x0001;
				} else {
					saved_quant_chrominance[i] = (INT16U) (temp / 100);
				}
		    }
		    saved_chrom_quality = quality;
		}
		ptr = &saved_quant_chrominance[0];
		drv_l1_jpeg_quant_chrominance_set(0, 64, &saved_quant_chrominance[0]);
	}

	if (qtable) {
		for (i=0; i<64; i++) {
			*qtable++ = (INT8U) ptr[zigzag_scan[i]];
		}		
	}
}

INT32S jpeg_encode_input_size_set(INT16U width, INT16U height)
{
	if (!width || !height) {
		return -1;
	}

	return drv_l1_jpeg_image_size_set(width, height);
}

INT32S jpeg_encode_input_format_set(INT32U format)
{
// always be YUYV or GP420 format, not need to change
/*
	if (format == C_JPEG_FORMAT_YUV_SEPARATE) {
		return jpeg_yuyv_encode_mode_disable();
	} else if (format == C_JPEG_FORMAT_YUYV) {
		return jpeg_yuyv_encode_mode_enable();
	}
*/
	return -1;
}

INT32S jpeg_encode_yuv_sampling_mode_set(INT32U encode_mode)
{
	if (encode_mode!=C_JPG_CTRL_YUV422 && encode_mode!=C_JPG_CTRL_YUV420) {
		return -1;
	}

	return drv_l1_jpeg_yuv_sampling_mode_set(encode_mode);
}

INT32S jpeg_encode_output_addr_set(INT32U addr)
{
	if (addr & 0xF) {
		DBG_PRINT("jpeg_encode_output_addr_set(): VLC address is not 16-byte alignment\r\n");
		return -1;
	}
	return drv_l1_jpeg_vlc_addr_set(addr);
}

extern void drv_l1_jpeg_huffman_table_set(void);
extern void drv_l1_jpeg_oldwu_function_enable(void);
extern void drv_l1_jpeg_huffman_table_release(void);
void jpeg_enable_scale_x2_engine(void)
{
	drv_l1_jpeg_huffman_table_set();
	drv_l1_jpeg_oldwu_function_enable();
	drv_l1_jpeg_huffman_table_release();
}

INT32S jpeg_encode_once_start(INT32U y_addr, INT32U u_addr, INT32U v_addr)
{
	if ((y_addr & 0x7) || (u_addr & 0x7) || (v_addr & 0x7)) {
		DBG_PRINT("Address is not 8-byte alignment when calling jpeg_encode_once_start()\r\n");
		return -1;
	}

	if (jpeg_encode_started) {
		DBG_PRINT("jpeg_encode_once_start(): JPEG encoding has already started\r\n");
		return -1;
	}
	if (drv_l1_jpeg_yuv_addr_set(y_addr, u_addr, v_addr)) {
		DBG_PRINT("Calling to drv_l1_jpeg_yuv_addr_set() failed\r\n");
		return -1;
	}
	if (drv_l1_jpeg_compression_start(NULL)) {
		DBG_PRINT("Calling to drv_l1_jpeg_compression_start() failed\r\n");
		return -1;
	}

	jpeg_encode_started = 1;

	return 0;
}

INT32S jpeg_encode_on_the_fly_start(INT32U y_addr, INT32U u_addr, INT32U v_addr, INT32U len)
{
	if (jpeg_encode_started) {
		// Make sure parameters are at least 8-byte alignment
		if ((y_addr & 0x7) || (u_addr & 0x7) || (v_addr & 0x7) || !len) {
			DBG_PRINT("Parameter is incorrect when calling jpeg_encode_on_the_fly_start()\r\n");
			return -1;
		}
		if (drv_l1_jpeg_multi_yuv_input_restart(y_addr, u_addr, v_addr, len)) {
			DBG_PRINT("Calling to drv_l1_jpeg_multi_yuv_input_restart() failed\r\n");
			return -1;
		}

		return 0;
	} else {
		if (!y_addr || (y_addr & 0x7) || !len) {
			DBG_PRINT("Parameter is incorrect when calling jpeg_encode_on_the_fly_start()\r\n");
			return -1;
		}
		if (drv_l1_jpeg_yuv_addr_set(y_addr, u_addr, v_addr)) {
			DBG_PRINT("Calling to drv_l1_jpeg_yuv_addr_set() failed\r\n");
			return -1;
		}
		if (drv_l1_jpeg_multi_yuv_input_init(len)) {
			DBG_PRINT("Calling to drv_l1_jpeg_multi_yuv_input_init() failed\r\n");
			return -1;
		}
		if (drv_l1_jpeg_compression_start(NULL)) {
			DBG_PRINT("Calling to drv_l1_jpeg_compression_start() failed\r\n");
			return -1;
		}
		jpeg_encode_started = 1;
	}

	return 0;
}

void jpeg_encode_stop(void)
{
	drv_l1_jpeg_stop();

	jpeg_encode_started = 0;
}

// Return the current status of JPEG engine
INT32S jpeg_encode_status_query(INT32U wait)
{
#if 1
	return drv_l1_jpeg_status_wait_idle(wait);
#else
	return drv_l1_jpeg_status_polling(wait);
#endif
}

INT32U jpeg_encode_vlc_cnt_get(void)
{
	return drv_l1_jpeg_compress_vlc_cnt_get();
}

#endif		// GPLIB_JPEG_ENCODE_EN