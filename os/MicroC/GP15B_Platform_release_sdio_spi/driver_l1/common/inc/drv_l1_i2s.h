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
#ifndef __drv_l1_I2S_H__
#define __drv_l1_I2S_H__

#include "driver_l1.h"

typedef enum
{
	I2S_FIRST_FRAME_L = 0,
	I2S_FIRST_FRAME_R
} I2S_FIRST_FRAME_ENUM;

typedef enum
{
	I2S_FRAME_POLARITY_R = 0,
	I2S_FRAME_POLARITY_L
} I2S_FRAME_POLARITY_ENUM;

typedef enum
{
	I2S_FALLING_EDGE = 0,
	I2S_RISING_EDGE
} I2S_EDGE_MODE_ENUM;

typedef enum
{
	I2S_MSB_MODE = 0,
	I2S_LSB_MODE
} I2S_SEND_MODE_ENUM;

typedef enum
{
	I2S_ALIGN_RIGHT=0,
	I2S_ALIGN_LEFT
} I2S_ALIGN_ENUM;

typedef enum
{
	I2S_VALID_16 = 16,
	I2S_VALID_18 = 18,
	I2S_VALID_20 = 20,
	I2S_VALID_22 = 22,
	I2S_VALID_24 = 24,
	I2S_VALID_32 = 32,
	I2S_VALID_8 = 8
} I2S_VALID_DATA_ENUM;

typedef enum
{
	I2S_MODE_I2S = 0,
	I2S_MODE_NORMAL,
	I2S_MODE_DSP
} I2S_MODE_ENUM;

typedef enum
{
	I2S_MASTER = 0,
	I2S_SLAVE
} I2S_MASTER_MODE_ENUM;

typedef enum
{
	I2S_IRT_POLARITY_L = 0,
	I2S_IRT_POLARITY_H
} I2S_IRT_POLARITY_ENUM;

typedef enum
{
	I2S_OVERWRITE_ENABLE = 0,
	I2S_OVERWRITE_DISABLE
} I2S_OVWR_ENUM;

typedef enum
{
	I2S_INT_DISABLE = 0,
	I2S_INT_ENABLE
} I2S_INT_ENUM;

typedef enum
{
	I2S_MERGE_DISABLE = 0,
	I2S_MERGE_ENABLE
} I2S_MERGE_ENUM;

typedef enum
{
	I2S_MODE_STEREO = 0,
	I2S_MODE_MONO
} I2S_MONO_MODE_ENUM;

typedef enum
{
	I2S_MCLK_DIV_2 = 2,
	I2S_MCLK_DIV_3 = 3,
	I2S_MCLK_DIV_4 = 4,
	I2S_MCLK_DIV_6 = 6,
	I2S_MCLK_DIV_8 = 7
} I2S_MCLK_DIV_ENUM;

typedef enum
{
	I2S_FRAMESIZE_16 = 16,
	I2S_FRAMESIZE_24 = 24,
	I2S_FRAMESIZE_32 = 32,
	I2S_FRAMESIZE_48 = 48,
	I2S_FRAMESIZE_64 = 64,
	I2S_FRAMESIZE_96 = 96,
	I2S_FRAMESIZE_128 = 128,
	I2S_FRAMESIZE_176 = 176,
	I2S_FRAMESIZE_192 = 192
} I2S_FRAMESIZE_ENUM;

typedef enum
{
	MIC_INPUT = 0,
	LINE_IN_LR,
	I2S_RX_INPUT_MAX	
} I2S_RX_INPUT_PATH;

// i2s tx api
extern void drv_l1_i2s_tx_init(void);
extern void drv_l1_i2s_tx_exit(void);
extern void drv_l1_i2s_tx_set_firstframe(I2S_FIRST_FRAME_ENUM mode);
extern void drv_l1_i2s_tx_set_framepolarity(I2S_FRAME_POLARITY_ENUM mode);
extern void drv_l1_i2s_tx_set_edgemode(I2S_EDGE_MODE_ENUM mode);
extern void drv_l1_i2s_tx_set_sendmode(I2S_SEND_MODE_ENUM mode);
extern void drv_l1_i2s_tx_set_align(I2S_ALIGN_ENUM mode);
extern void drv_l1_i2s_tx_set_validdata(I2S_VALID_DATA_ENUM data);
extern void drv_l1_i2s_tx_spu_fs(INT32U sample_rate);
extern void drv_l1_i2s_tx_set_i2smode(I2S_MODE_ENUM mode);
extern void drv_l1_i2s_tx_set_master(I2S_MASTER_MODE_ENUM mode);
extern void drv_l1_i2s_tx_set_intpolarity(I2S_IRT_POLARITY_ENUM mode);
extern void drv_l1_i2s_tx_set_over_write(I2S_OVWR_ENUM mode);
extern void drv_l1_i2s_tx_set_int(I2S_INT_ENUM mode);
extern void drv_l1_i2s_tx_fifo_clear(void);
extern void drv_l1_i2s_tx_set_merge(I2S_MERGE_ENUM mode);
extern void drv_l1_i2s_tx_set_mono(I2S_MONO_MODE_ENUM mode);
extern void drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_ENUM mode);
extern void drv_l1_i2s_tx_set_mclk_div(I2S_MCLK_DIV_ENUM mode);

extern INT32S drv_l1_i2s_tx_sample_rate_set(INT32U hz);
extern INT32S drv_l1_i2s_tx_start(void);
extern INT32S drv_l1_i2s_tx_stop(void);

extern INT32S drv_l1_i2s_tx_dbf_put(INT16S *data, INT32U cwlen, OS_EVENT *os_q);
extern INT32S drv_l1_i2s_tx_dbf_set(INT16S *data, INT32U cwlen);
extern INT32S drv_l1_i2s_tx_dbf_status_get(void);
extern INT32S drv_l1_i2s_tx_dma_status_get(void);
extern void drv_l1_i2s_tx_dbf_free(void);

// i2s rx api
extern void drv_l1_i2s_rx_set_input_path(I2S_RX_INPUT_PATH path);
extern void drv_l1_i2s_rx_init(void);
extern void drv_l1_i2s_rx_exit(void);
extern void drv_l1_i2s_rx_fifo_clear(void);
extern INT32S drv_l1_i2s_rx_sample_rate_set(INT32U hz);
extern INT32S drv_l1_i2s_rx_mono_ch_set(void);
extern INT32S drv_l1_i2s_rx_start(void);
extern INT32S drv_l1_i2s_rx_stop(void);

extern INT32S drv_l1_i2s_rx_dbf_put(INT16S *data, INT32U cwlen, OS_EVENT *os_q);
extern INT32S drv_l1_i2s_rx_dbf_set(INT16S *data, INT32U cwlen);
extern INT32S drv_l1_i2s_rx_dbf_status_get(void);
extern INT32S drv_l1_i2s_rx_dma_status_get(void);
extern void drv_l1_i2s_rx_dbf_free(void);
#endif		// __drv_l1_I2S_H__