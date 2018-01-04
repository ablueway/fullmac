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

#ifndef __APPLICATION_CFG_H__
#define __APPLICATION_CFG_H__

// audio decode configure
#define APP_S880_DECODE_EN			0
#define APP_A1600_DECODE_EN			0
#define APP_A1800_DECODE_EN			0
#define APP_A6400_DECODE_EN			0
#define APP_WAV_CODEC_EN			1
#define APP_MP3_DECODE_EN			 0
#define APP_WMA_DECODE_EN			0
#define APP_AAC_DECODE_EN			0
#define APP_OGG_DECODE_EN			0

// audio decode post process configure
#define APP_CONST_PITCH_EN			0
#define APP_ECHO_EN					0
#define	APP_VOICE_CHANGER_EN		0
#define APP_UP_SAMPLE_EN			0

// audio record post process configure
#define APP_A1800_ENCODE_EN			0
#define APP_MP3_ENCODE_EN			0
#define	APP_DOWN_SAMPLE_EN			0
#define A1800_DOWN_SAMPLE_EN		0
#define APP_LPF_ENABLE				0


#endif		// __APPLICATION_CFG_H__
