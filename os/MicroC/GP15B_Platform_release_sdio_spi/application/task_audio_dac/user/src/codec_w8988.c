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
#include "audio_codec_device.h"
#include "drv_l2_sccb.h"
#include "drv_l1_I2C.h"
#include "drv_l1_timer.h"

#if (defined CODEC_W8988) && (CODEC_W8988 == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define WM8988_ID			0x34	// WM8988 Chip ID - 0x34

#define WM8988_LINPUT_VOL	0
#define WM8988_RINPUT_VOL	1
#define WM8988_LOUT1_VOL	2
#define WM8988_ROUT1_VOL	3
#define WM8988_ADC_DAC_CTRL	5
#define WM8988_AUDIO_IF		7
#define WM8988_SAMPLE_RATE	8
#define WM8988_LDAC_VOL		10
#define WM8988_RDAC_VOL		11
#define WM8988_BASS_CTRL	12
#define WM8988_TREBLE_CTRL	13
#define WM8988_RESET_REG	15
#define WM8988_3D_CTRL		16
#define WM8988_ALC1_CTRL	17
#define WM8988_ALC2_CTRL	18
#define WM8988_ALC3_CTRL	19
#define WM8988_NOISE_GATE	20
#define WM8988_LADC_VOL		21
#define WM8988_RADC_VOL		22
#define WM8988_ADD1_CTRL	23
#define WM8988_ADD2_CTRL	24
#define WM8988_PWR_MGMT_1	25
#define WM8988_PWR_MGMT_2	26
#define WM8988_ADD3_CTRL	27
#define WM8988_ADC_INPUT_M	31
#define WM8988_ADC_LIN_PATH	32
#define WM8988_ADC_RIN_PATH	33
#define WM8988_LOUT_MIX_1	34
#define WM8988_LOUT_MIX_2	35
#define WM8988_ROUT_MIX_1	36
#define WM8988_ROUT_MIX_2	37
#define WM8988_LOUT2_VOL	40
#define WM8988_ROUT2_VOL	41
#define WM8988_LW_PWR_PLAY	67

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static void *w8988_handle;


static void w8988_write(INT8U reg, INT16U value)
{
	INT8U addr=0, data=0;
	INT32S ret;

__retry:		
	// addr is combined by reg[6:0] & value[8]
	// addr[7:0] = (reg[6:0] << 1) | (value[8] >> 8)
	addr = ((reg & 0xFF) << 1) | ((value & 0xF00) >> 8);
	
	// data is 8 bits
	// data[7:0] = value[7:0]
	data = value & 0xFF;
	ret = drv_l2_sccb_write(w8988_handle, addr, data);
	if(ret < 0) {
		DBG_PRINT("%s fail...\r\n", __func__);
		goto __retry;
	}
}

static INT32S w8988_init(void)
{
	w8988_handle = drv_l2_sccb_open(WM8988_ID, 8, 8);
	if(w8988_handle == 0) {
		return STATUS_FAIL;
	} 
	
	return STATUS_OK;
}

static void w8988_uninit(void)
{
	drv_l2_sccb_close(w8988_handle);
	w8988_handle = 0;
}

static INT32S w8988_start(INT32U mode)
{
	DBG_PRINT("%s.\r\n", __func__);
	
	//software reset
	//w8988_write(15, 0x00);
	//w8988_write(15, 0x01);
	//drv_msec_wait(100);
	
	// Setup WM8988 registers by I2C protocol
	w8988_write(67, 0x00);
	w8988_write(24, 0x00);
	
	w8988_write(25, 0xEC);		// power mgmt-1
	w8988_write(26, 0x1F8);		// power mgmt-2
	
	// Line-out 1 volume
	w8988_write(2, 0x15F);
	w8988_write(3, 0x15F);
	
	// Line-out 2 volume
	w8988_write(40, 0x15F);
	w8988_write(41, 0x15F);
	
	// ADC & DAC Control
	w8988_write(5, 0x0);
	
	// Data Lenght & Audio Data Format
	w8988_write(7, 0x2);	// 16bits & I2S format, slave mode
	//w8988_write(7, 0xA);	// 24bits & I2S format
	//w8988_write(7, 0xE);	// 32bits & I2S format
	
	w8988_write(34, 0x150);
	w8988_write(35, 0x50);
	w8988_write(36, 0x50);
	w8988_write(37, 0x150);
	
	return STATUS_OK;
}

static void w8988_stop(void)
{
	//software reset
	w8988_write(15, 0x00);
	w8988_write(15, 0x01);
}

static void w8988_adjust_volume(INT32U volume)
{
	//volume, 0 ~ 63
	volume <<= 1;
	volume &= 0x7F;
	
	// Line-out 2 volume
	w8988_write(40, 0x100 | volume);
	w8988_write(41, 0x100 | volume);
}

const AudCodecDev_t AudCodec_device = {
	w8988_init,
	w8988_uninit,
	w8988_start,
	w8988_stop,
	w8988_adjust_volume
};
#endif //(defined CODEC_W8988) && (CODEC_W8988 == 1)
