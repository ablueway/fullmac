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
#include "driver_l1.h"
#include "drv_l1_sfr.h"
#include "drv_l1_system.h"
#include "drv_l1_csi.h"
#include "drv_l1_timer.h"
#include "drv_l1_i2c.h"
#include "drv_l1_power.h"
#include "drv_l1_interrupt.h"
#include "drv_l2_sensor.h"
#include "drv_l2_sccb.h"

#if (defined _SENSOR_OV3640_CSI) && (_SENSOR_OV3640_CSI == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	OV3640_ID		0x78

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if SCCB_MODE == SCCB_GPIO
	static void *ov3640_handle;
#elif SCCB_MODE == SCCB_HW_I2C
	static drv_l1_i2c_bus_handle_t ov3640_handle; 
#endif

static INT32S ov3640_sccb_open(void)
{
#if SCCB_MODE == SCCB_GPIO
	ov3640_handle = drv_l2_sccb_open(OV3640_ID, 16, 8);
	if(ov3640_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_init();
	ov3640_handle.slaveAddr = OV3640_ID;
	ov3640_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void ov3640_sccb_close(void)
{
#if SCCB_MODE == SCCB_GPIO
	if(ov3640_handle) {
		drv_l2_sccb_close(ov3640_handle);
		ov3640_handle = NULL;
	}	
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_uninit();
	ov3640_handle.slaveAddr = 0;
	ov3640_handle.clkRate = 0;
#endif
}

static INT32S ov3640_sccb_write(INT16U reg, INT8U value)
{
#if SCCB_MODE == SCCB_GPIO
	return drv_l2_sccb_write(ov3640_handle, reg, value);

#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[3];
	
	data[0] = (reg >> 8) & 0xFF;
	data[1] = reg & 0xFF;
	data[2] = value;
	return drv_l1_i2c_bus_write(&ov3640_handle, data, 3);
#endif
}

#if 0
static INT32S ov3640_sccb_read(INT16U reg, INT8U *value)
{
#if SCCB_MODE == SCCB_GPIO
	INT16U data;
	
	if(drv_l2_sccb_read(ov3640_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[2];
	
	data[0] = (reg >> 8) & 0xFF;
	data[1] = reg & 0xFF;
	if(drv_l1_i2c_bus_write(&ov3640_handle, data, 2) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	if(drv_l1_i2c_bus_read(&ov3640_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	*value = data[0];
#endif
	return STATUS_OK;
}
#endif

static INT32S ov3640_init(void)
{
	DBG_PRINT("%s\r\n", __func__);
	ov3640_sccb_write(0x3012, 0x90);	// [7]:Reset; [6:4]=001->XGA mode
	ov3640_sccb_write(0x30a9, 0xdb);	// for 1.5V
	ov3640_sccb_write(0x304d, 0x45);
	ov3640_sccb_write(0x3087, 0x16);
	ov3640_sccb_write(0x309c, 0x1a);
	ov3640_sccb_write(0x30a2, 0xe4);
	ov3640_sccb_write(0x30aa, 0x42);
	ov3640_sccb_write(0x30b0, 0xff);
	ov3640_sccb_write(0x30b1, 0xff);
	ov3640_sccb_write(0x30b2, 0x10);
	ov3640_sccb_write(0x300e, 0x32);
	ov3640_sccb_write(0x300f, 0x21);
	ov3640_sccb_write(0x3010, 0x20);
	ov3640_sccb_write(0x3011, 0x01);
	ov3640_sccb_write(0x304c, 0x82);
	ov3640_sccb_write(0x30d7, 0x10);
	ov3640_sccb_write(0x30d9, 0x0d);
	ov3640_sccb_write(0x30db, 0x08);
	ov3640_sccb_write(0x3016, 0x82);
	ov3640_sccb_write(0x3018, 0x48);	// Luminance High Range=72 after Gamma=0x86=134; 0x40->134
	ov3640_sccb_write(0x3019, 0x40);	// Luminance Low Range=64 after Gamma=0x8f=143; 0x38->125
	ov3640_sccb_write(0x301a, 0x82);
	ov3640_sccb_write(0x307d, 0x00);
	ov3640_sccb_write(0x307c, 0x01);
	ov3640_sccb_write(0x3087, 0x02);
	ov3640_sccb_write(0x3082, 0x20);
	ov3640_sccb_write(0x3070, 0x00);	// 50Hz Banding MSB
	ov3640_sccb_write(0x3071, 0xbb);	// 50Hz Banding LSB
	ov3640_sccb_write(0x3072, 0x00);	// 60Hz Banding MSB
	ov3640_sccb_write(0x3073, 0xa6);	// 60Hz Banding LSB
	ov3640_sccb_write(0x301c, 0x07);	// max_band_step_50hz
	ov3640_sccb_write(0x301d, 0x08);	// max_band_step_60hz
	ov3640_sccb_write(0x3015, 0x12);	// [6:4]:1 dummy frame; [2:0]:AGC gain 8x
	ov3640_sccb_write(0x3014, 0x84);	// [7]:50hz; [6]:auto banding detection disable; [3]:night modedisable
	ov3640_sccb_write(0x3013, 0xf7);	// AE_en
	ov3640_sccb_write(0x3030, 0x11);	// Avg_win_Weight0
	ov3640_sccb_write(0x3031, 0x11);	// Avg_win_Weight1
	ov3640_sccb_write(0x3032, 0x11);	// Avg_win_Weight2
	ov3640_sccb_write(0x3033, 0x11);	// Avg_win_Weight3
	ov3640_sccb_write(0x3034, 0x11);	// Avg_win_Weight4
	ov3640_sccb_write(0x3035, 0x11);	// Avg_win_Weight5
	ov3640_sccb_write(0x3036, 0x11);	// Avg_win_Weight6
	ov3640_sccb_write(0x3037, 0x11);	// Avg_win_Weight7
	ov3640_sccb_write(0x3038, 0x01);	// Avg_Win_Hstart=285
	ov3640_sccb_write(0x3039, 0x1d);	// Avg_Win_Hstart=285
	ov3640_sccb_write(0x303a, 0x00);	// Avg_Win_Vstart=10
	ov3640_sccb_write(0x303b, 0x0a);	// Avg_Win_Vstart=10
	ov3640_sccb_write(0x303c, 0x02);	// Avg_Win_Width=512x4=2048
	ov3640_sccb_write(0x303d, 0x00);	// Avg_Win_Width=512x4=2048
	ov3640_sccb_write(0x303e, 0x01);	// Avg_Win_Height=384x4=1536
	ov3640_sccb_write(0x303f, 0x80);	// Avg_Win_Height=384x4=1536
	ov3640_sccb_write(0x3047, 0x00);	// [7]:avg_based AE
	ov3640_sccb_write(0x30b8, 0x20);
	ov3640_sccb_write(0x30b9, 0x17);
	ov3640_sccb_write(0x30ba, 0x04);
	ov3640_sccb_write(0x30bb, 0x08);
	ov3640_sccb_write(0x30a9, 0xdb);	// for 1.5V

	ov3640_sccb_write(0x3104, 0x02);
	ov3640_sccb_write(0x3105, 0xfd);
	ov3640_sccb_write(0x3106, 0x00);
	ov3640_sccb_write(0x3107, 0xff);
	ov3640_sccb_write(0x3100, 0x02);

	ov3640_sccb_write(0x3300, 0x13);	// [0]: LENC disable; [1]: AF enable
	ov3640_sccb_write(0x3301, 0xde);	// [1]: BC_en; [2]: WC_en; [4]: CMX_en
	ov3640_sccb_write(0x3302, 0xcf);	// [0]: AWB_en; [1]: AWB_gain_en; [2]: Gamma_en; [7]: Special_Effect_en
	ov3640_sccb_write(0x3304, 0xfc);	// [4]: Add bias to gamma result; [5]: Enable Gamma bias function
	ov3640_sccb_write(0x3306, 0x5c);	// Reserved ???
	ov3640_sccb_write(0x3307, 0x11);	// Reserved ???
	ov3640_sccb_write(0x3308, 0x00);	// [7]: AWB_mode=advanced
	ov3640_sccb_write(0x330b, 0x1c);	// Reserved ???
	ov3640_sccb_write(0x330c, 0x18);	// Reserved ???
	ov3640_sccb_write(0x330d, 0x18);	// Reserved ???
	ov3640_sccb_write(0x330e, 0x56);	// Reserved ???
	ov3640_sccb_write(0x330f, 0x5c);	// Reserved ???
	ov3640_sccb_write(0x3310, 0xd0);	// Reserved ???
	ov3640_sccb_write(0x3311, 0xbd);	// Reserved ???
	ov3640_sccb_write(0x3312, 0x26);	// Reserved ???
	ov3640_sccb_write(0x3313, 0x2b);	// Reserved ???
	ov3640_sccb_write(0x3314, 0x42);	// Reserved ???
	ov3640_sccb_write(0x3315, 0x42);	// Reserved ???
	ov3640_sccb_write(0x331b, 0x09);	// Gamma YST1
	ov3640_sccb_write(0x331c, 0x18);	// Gamma YST2
	ov3640_sccb_write(0x331d, 0x30);	// Gamma YST3
	ov3640_sccb_write(0x331e, 0x58);	// Gamma YST4
	ov3640_sccb_write(0x331f, 0x66);	// Gamma YST5
	ov3640_sccb_write(0x3320, 0x72);	// Gamma YST6
	ov3640_sccb_write(0x3321, 0x7d);	// Gamma YST7
	ov3640_sccb_write(0x3322, 0x86);	// Gamma YST8
	ov3640_sccb_write(0x3323, 0x8f);	// Gamma YST9
	ov3640_sccb_write(0x3324, 0x97);	// Gamma YST10
	ov3640_sccb_write(0x3325, 0xa5);	// Gamma YST11
	ov3640_sccb_write(0x3326, 0xb2);	// Gamma YST12
	ov3640_sccb_write(0x3327, 0xc7);	// Gamma YST13
	ov3640_sccb_write(0x3328, 0xd8);	// Gamma YST14
	ov3640_sccb_write(0x3329, 0xe8);	// Gamma YST15
	ov3640_sccb_write(0x332a, 0x20);	// Gamma YSLP15
	ov3640_sccb_write(0x332b, 0x00);	// [3]: WB_mode=auto
	ov3640_sccb_write(0x332d, 0x64);	// [6]:de-noise auto mode; [5]:edge auto mode; [4:0]:edge threshold
	ov3640_sccb_write(0x3355, 0x06);	// Special_Effect_CTRL: [1]:Sat_en; [2]: Cont_Y_en
	ov3640_sccb_write(0x3358, 0x40);	// Special_Effect_Sat_U
	ov3640_sccb_write(0x3359, 0x40);	// Special_Effect_Sat_V
	ov3640_sccb_write(0x336a, 0x52);	// LENC R_A1
	ov3640_sccb_write(0x3370, 0x46);	// LENC G_A1
	ov3640_sccb_write(0x3376, 0x38);	// LENC B_A1

	ov3640_sccb_write(0x3400, 0x00);	// [2:0];Format input source=DSP TUV444
	ov3640_sccb_write(0x3403, 0x42);	// DVP Win Addr
	ov3640_sccb_write(0x3404, 0x00);	// [5:0]: yuyv

	ov3640_sccb_write(0x3507, 0x06);	// ???
	ov3640_sccb_write(0x350a, 0x4f);	// ???

	ov3640_sccb_write(0x3600, 0xc0);	// VSYNC_CTRL

	ov3640_sccb_write(0x3302, 0xcf);	// [0]: AWB_enable
	ov3640_sccb_write(0x300d, 0x01);	// PCLK/2
	ov3640_sccb_write(0x3012, 0x10);	// [6:4]=001->XGA mode
	ov3640_sccb_write(0x3013, 0xf7);	// AE_enable
	ov3640_sccb_write(0x3020, 0x01);	// HS=285
	ov3640_sccb_write(0x3021, 0x1d);	// HS=285
	ov3640_sccb_write(0x3022, 0x00);	// VS = 6
	ov3640_sccb_write(0x3023, 0x06);	// VS = 6
	ov3640_sccb_write(0x3024, 0x08);	// HW=2072
	ov3640_sccb_write(0x3025, 0x18);	// HW=2072
	ov3640_sccb_write(0x3026, 0x03);	// VW=772
	ov3640_sccb_write(0x3027, 0x04);	// VW=772
	ov3640_sccb_write(0x3028, 0x09);	// HTotalSize=2375
	ov3640_sccb_write(0x3029, 0x47);	// HTotalSize=2375
	ov3640_sccb_write(0x302a, 0x03);	// VTotalSize=784
	ov3640_sccb_write(0x302b, 0x10);	// VTotalSize=784
	ov3640_sccb_write(0x304c, 0x82);
	ov3640_sccb_write(0x3075, 0x24);	// VSYNCOPT
	ov3640_sccb_write(0x3086, 0x00);	// Sleep/Wakeup
	ov3640_sccb_write(0x3088, 0x04);	// x_output_size=1024
	ov3640_sccb_write(0x3089, 0x00);	// x_output_size=1024
	ov3640_sccb_write(0x308a, 0x03);	// y_output_size=768
	ov3640_sccb_write(0x308b, 0x00);	// y_output_size=768
	ov3640_sccb_write(0x308d, 0x04);
	ov3640_sccb_write(0x30d7, 0x90);	// ???
	ov3640_sccb_write(0x3302, 0xef);	// [5]: Scale_en, [0]: AWB_enable
	ov3640_sccb_write(0x335f, 0x34);	// Scale_VH_in
	ov3640_sccb_write(0x3360, 0x0c);	// Scale_H_in = 0x40c = 1036
	ov3640_sccb_write(0x3361, 0x04);	// Scale_V_in = 0x304 = 772
	ov3640_sccb_write(0x3362, 0x34);	// Scale_VH_out
	ov3640_sccb_write(0x3363, 0x08);	// Scale_H_out = 0x408 = 1032
	ov3640_sccb_write(0x3364, 0x04);	// Scale_V_out = 0x304 = 772
	ov3640_sccb_write(0x300e, 0x32);
	ov3640_sccb_write(0x300f, 0x21);
	ov3640_sccb_write(0x3011, 0x00);	// for 30 FPS
	return ov3640_sccb_write(0x304c, 0x82);
}

static INT32S ov3640_preview(void)
{
	DBG_PRINT("%s\r\n", __func__);
	ov3640_sccb_write(0x3302, 0xcf);	// [0]: AWB_enable
	ov3640_sccb_write(0x300d, 0x01);	// PCLK/2
	ov3640_sccb_write(0x3012, 0x10);	// [6:4]=001->XGA mode
	ov3640_sccb_write(0x3013, 0xf7);	// AE_enable
	ov3640_sccb_write(0x3020, 0x01);	// HS=285
	ov3640_sccb_write(0x3021, 0x1d);	// HS=285
	ov3640_sccb_write(0x3022, 0x00);	// VS = 6
	ov3640_sccb_write(0x3023, 0x06);	// VS = 6
	ov3640_sccb_write(0x3024, 0x08);	// HW=2072
	ov3640_sccb_write(0x3025, 0x18);	// HW=2072
	ov3640_sccb_write(0x3026, 0x03);	// VW=772
	ov3640_sccb_write(0x3027, 0x04);	// VW=772
	ov3640_sccb_write(0x3028, 0x09);	// HTotalSize=2375
	ov3640_sccb_write(0x3029, 0x47);	// HTotalSize=2375
	ov3640_sccb_write(0x302a, 0x03);	// VTotalSize=784
	ov3640_sccb_write(0x302b, 0x10);	// VTotalSize=784
	ov3640_sccb_write(0x304c, 0x82);
	ov3640_sccb_write(0x3075, 0x24);	// VSYNCOPT
	ov3640_sccb_write(0x3086, 0x00);	// Sleep/Wakeup
	ov3640_sccb_write(0x3088, 0x04);	// x_output_size=1024
	ov3640_sccb_write(0x3089, 0x00);	// x_output_size=1024
	ov3640_sccb_write(0x308a, 0x03);	// y_output_size=768
	ov3640_sccb_write(0x308b, 0x00);	// y_output_size=768
	ov3640_sccb_write(0x308d, 0x04);
	ov3640_sccb_write(0x30d7, 0x90);	// ???
	ov3640_sccb_write(0x3302, 0xef);	// [5]: Scale_en, [0]: AWB_enable
	ov3640_sccb_write(0x335f, 0x34);	// Scale_VH_in
	ov3640_sccb_write(0x3360, 0x0c);	// Scale_H_in = 0x40c = 1036
	ov3640_sccb_write(0x3361, 0x04);	// Scale_V_in = 0x304 = 772
	ov3640_sccb_write(0x3362, 0x34);	// Scale_VH_out
	ov3640_sccb_write(0x3363, 0x08);	// Scale_H_out = 0x408 = 1032
	ov3640_sccb_write(0x3364, 0x04);	// Scale_V_out = 0x304 = 772
	ov3640_sccb_write(0x300e, 0x32);
	ov3640_sccb_write(0x300f, 0x21);
	ov3640_sccb_write(0x3011, 0x00);	// for 30 FPS
	return ov3640_sccb_write(0x304c, 0x82);
}

static INT32S ov3640_capture(void)
{
	DBG_PRINT("%s\r\n", __func__);
	ov3640_sccb_write(0x3302, 0xce);	//[0]: AWB_disable
	ov3640_sccb_write(0x300d, 0x00);	// PCLK/1
	ov3640_sccb_write(0x300e, 0x39);
	ov3640_sccb_write(0x300f, 0x21);
	ov3640_sccb_write(0x3010, 0x20);
	ov3640_sccb_write(0x3011, 0x00);
	ov3640_sccb_write(0x3012, 0x00);	// [6:4]=000->QXGA mode
	ov3640_sccb_write(0x3013, 0xf2);	// AE_disable
	ov3640_sccb_write(0x3020, 0x01);	// HS=285
	ov3640_sccb_write(0x3021, 0x1d);	// HS=285
	ov3640_sccb_write(0x3022, 0x00);	// VS = 10
	ov3640_sccb_write(0x3023, 0x0a);	// VS = 10
	ov3640_sccb_write(0x3024, 0x08);	// HW=2072
	ov3640_sccb_write(0x3025, 0x18);	// HW=2072
	ov3640_sccb_write(0x3026, 0x06);	// VW=1548
	ov3640_sccb_write(0x3027, 0x0c);	// VW=1548
	ov3640_sccb_write(0x3028, 0x09);	// HTotalSize=2375
	ov3640_sccb_write(0x3029, 0x47);	// HTotalSize=2375
	ov3640_sccb_write(0x302a, 0x06);	// VTotalSize=1568
	ov3640_sccb_write(0x302b, 0x20);	// VTotalSize=1568
	ov3640_sccb_write(0x304c, 0x81);	// ???
	ov3640_sccb_write(0x3075, 0x44);	// VSYNCOPT
	ov3640_sccb_write(0x3088, 0x08);	// x_output_size=2048
	ov3640_sccb_write(0x3089, 0x00);	// x_output_size=2048
	ov3640_sccb_write(0x308a, 0x06);	// y_output_size=1536
	ov3640_sccb_write(0x308b, 0x00);	// y_output_size=1536
	ov3640_sccb_write(0x30d7, 0x10);	// ???
	ov3640_sccb_write(0x3302, 0xee);	// [5]: Scale_en, [0]: AWB_disable
	ov3640_sccb_write(0x335f, 0x68);	// Scale_VH_in
	ov3640_sccb_write(0x3360, 0x18);	// Scale_H_in = 0x818 = 2072
	ov3640_sccb_write(0x3361, 0x0c);	// Scale_V_in = 0x60c = 1548
	ov3640_sccb_write(0x3362, 0x68);	// Scale_VH_out
	ov3640_sccb_write(0x3363, 0x08);	// Scale_H_out = 0x808 = 2056
	return ov3640_sccb_write(0x3364, 0x04);	// Scale_V_out = 0x604 = 1540
}

static INT32S ov3640_record(void)
{
	DBG_PRINT("%s\r\n", __func__);
	ov3640_sccb_write(0x3302, 0xcf);	// [0]: AWB_enable
	ov3640_sccb_write(0x300d, 0x01);	// PCLK/2
	ov3640_sccb_write(0x3012, 0x10);	// [6:4]=001->XGA mode
	ov3640_sccb_write(0x3013, 0xf7);	// AE_enable
	ov3640_sccb_write(0x3020, 0x01);	// HS=285
	ov3640_sccb_write(0x3021, 0x1d);	// HS=285
	ov3640_sccb_write(0x3022, 0x00);	// VS = 6
	ov3640_sccb_write(0x3023, 0x06);	// VS = 6
	ov3640_sccb_write(0x3024, 0x08);	// HW=2072
	ov3640_sccb_write(0x3025, 0x18);	// HW=2072
	ov3640_sccb_write(0x3026, 0x03);	// VW=772
	ov3640_sccb_write(0x3027, 0x04);	// VW=772
	ov3640_sccb_write(0x3028, 0x09);	// HTotalSize=2375
	ov3640_sccb_write(0x3029, 0x47);	// HTotalSize=2375
	ov3640_sccb_write(0x302a, 0x03);	// VTotalSize=784
	ov3640_sccb_write(0x302b, 0x10);	// VTotalSize=784
	ov3640_sccb_write(0x304c, 0x82);
	ov3640_sccb_write(0x3075, 0x24);	// VSYNCOPT
	ov3640_sccb_write(0x3086, 0x00);	// Sleep/Wakeup
	ov3640_sccb_write(0x3088, 0x04);	// x_output_size=1024
	ov3640_sccb_write(0x3089, 0x00);	// x_output_size=1024
	ov3640_sccb_write(0x308a, 0x03);	// y_output_size=768
	ov3640_sccb_write(0x308b, 0x00);	// y_output_size=768
	ov3640_sccb_write(0x308d, 0x04);
	ov3640_sccb_write(0x30d7, 0x90);	// ???
	ov3640_sccb_write(0x3302, 0xef);	// [5]: Scale_en, [0]: AWB_enable
	ov3640_sccb_write(0x335f, 0x34);	// Scale_VH_in
	ov3640_sccb_write(0x3360, 0x0c);	// Scale_H_in = 0x40c = 1036
	ov3640_sccb_write(0x3361, 0x04);	// Scale_V_in = 0x304 = 772
	ov3640_sccb_write(0x3362, 0x34);	// Scale_VH_out
	ov3640_sccb_write(0x3363, 0x08);	// Scale_H_out = 0x408 = 1032
	ov3640_sccb_write(0x3364, 0x04);	// Scale_V_out = 0x304 = 772
	ov3640_sccb_write(0x300e, 0x32);
	ov3640_sccb_write(0x300f, 0x21);
	ov3640_sccb_write(0x3011, 0x00);	// for 30 FPS
	return ov3640_sccb_write(0x304c, 0x82);
}

/**
 * @brief   ov3640 initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void ov3640_csi_init(void)
{
	/* Turn on LDO 2.8V for CSI sensor */
	drv_l1_power_ldo_28_ctrl(1, LDO_LDO28_2P8V);
	
	/* set horizontal/vertical scale ratio to 0 */
	R_CSI_TG_HRATIO = 0;
	R_CSI_TG_VRATIO = 0;
	
	/* Sensor field 0 vertical latch start register */
	R_CSI_TG_VL0START = 0x0000;
	/* *P_Sensor_TG_V_L1Start = 0x0000 */
	R_CSI_TG_VL1START = 0x0000;
	/* Sensor horizontal start register */
	R_CSI_TG_HSTART = 0x0000;

	/* reset control 0/1 */
	R_CSI_TG_CTRL0 = 0;
	R_CSI_TG_CTRL1 = 0;

	/* register CSI interrupt */
	vic_irq_register(VIC_CSI, drv_l1_csi_isr);
	
	/* mclk output */
	drv_l2_sensor_set_mclkout(ov3640_sensor_csi_ops.info[0].mclk);
	
	/* request SCCB */
	ov3640_sccb_open();	
	DBG_PRINT("Sensor GC0308 csi interface init completed\r\n");
}	

/**
 * @brief   ov3640 un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void ov3640_csi_uninit(void)
{
	// disable mclk 
	drv_l2_sensor_set_mclkout(MCLK_NONE);
	
	// csi disable
	R_CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CSIEN;	//disable sensor controller
	R_CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOEN;	//disable sensor clock out

	// release sccb
	ov3640_sccb_close();
	
	// Turn off LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(0, LDO_LDO28_2P8V);
}	

/**
 * @brief   ov3640 stream start function
 * @param   info index
 *			
 * @return 	none
 */
static void ov3640_csi_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	/* enable CSI output clock for SCCB */
	drv_l2_sensor_set_mclkout(ov3640_sensor_csi_ops.info[index].mclk);
	
	/* set start frame buffer address */	
	if(bufA) {
		drv_l1_csi_set_buf(bufA);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", ov3640_sensor_csi_ops.name);
		ov3640_sensor_csi_ops.stream_stop();
	}	

	/* Set Horizontal/Vertical counter reset, Vertical counter incresase selection, sensor owner inerrupt enable, capture/preview mode */
	R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HRST | MASK_CSI_CTRL0_VADD | MASK_CSI_CTRL0_VRST | MASK_CSI_CTRL0_OWN_IRQ | MASK_CSI_CTRL0_CAP;
		
	/* Set delay clock 244[0:3] */
	R_CSI_TG_CTRL1 |= DELAY_1CLOCK;
	
	R_CSI_TG_CTRL1 |= MASK_CSI_CTRL1_RGB1555;

	/* According to the specific information, set CSI register */
	/* set input interface */
	if(ov3640_sensor_csi_ops.info[index].interface_mode == MODE_CCIR_HREF) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HREF;
	}
	
	/* set input & output format */
	if(ov3640_sensor_csi_ops.info[index].input_format == V4L2_PIX_FMT_VYUY) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVIN | MASK_CSI_CTRL0_YUVTYPE;
	}	

	if(ov3640_sensor_csi_ops.info[index].output_format == V4L2_PIX_FMT_VYUY) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVOUT;
	}	
	
	/* set width and height */
	if(ov3640_init() < 0) {
		DBG_PRINT("ov3640 init fail!!!\r\n");
	}
	
	DBG_PRINT("%s = %d\r\n", __func__, index);
	switch(index)
	{
	case 0:
		ov3640_preview();
		R_CSI_TG_HWIDTH = ov3640_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = ov3640_sensor_csi_ops.info[index].target_h;
		R_CSI_TG_HRATIO = 0;		// No Scale
		R_CSI_TG_VRATIO = 0;		// No Scale	
		break;
		
	case 1:
		ov3640_capture();
		R_CSI_TG_HWIDTH = ov3640_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = ov3640_sensor_csi_ops.info[index].target_h;
		R_CSI_TG_HRATIO = 0;		// No Scale
		R_CSI_TG_VRATIO = 0;		// No Scale	
		break;
		
	case 2:
		ov3640_record();
		R_CSI_TG_HWIDTH = ov3640_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = ov3640_sensor_csi_ops.info[index].target_h;
		R_CSI_TG_HRATIO = 0;		// No Scale
		R_CSI_TG_VRATIO = 0;		// No Scale	
		break;
		
	default:
		while(1);	
	}
	
	/* csi long burst width need 32 align */
	if(ov3640_sensor_csi_ops.info[index].target_w & 0x1F) {
		R_CSI_SEN_CTRL &= ~(1 << 7);
	} else {
		R_CSI_SEN_CTRL |= 1 << 7;
	}
	
	/* Enable interrupt */
	vic_irq_enable(VIC_CSI);
	
	/* Clear sensor interrupt status */
	R_TGR_IRQ_STATUS = 0x3F;
	
	/* enable frame end interrupt */
	R_TGR_IRQ_EN |= MASK_CSI_FRAME_END_FLAG | MASK_CSI_FIFO_OVERFLOW_FLAG;
	
	/* enable CSI module */
	R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CSIEN;
}

/**
 * @brief   ov3640 stream stop function
 * @param   none
 * @return 	none
 */ 
static void ov3640_csi_stream_stop(void)
{
	/* Disable interrupt */
	vic_irq_disable(VIC_CSI);
	
	/* Disable sensor interrupt */
	R_TGR_IRQ_EN &= ~MASK_CSI_FRAME_END_ENABLE;

	/* Disable CSI module */
	R_CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CSIEN;

	/* Disable sensor output clock*/
	R_CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOEN;	

	/* Clear sensor interrupt status */
	R_TGR_IRQ_STATUS = 0x3F;
}	

/**
 * @brief   ov3640 get info function
 * @param   none
 * @return 	pointer to sensor information data
 */ 
static drv_l2_sensor_info_t* ov3640_csi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
		return NULL;
	else
		return (drv_l2_sensor_info_t*)&ov3640_sensor_csi_ops.info[index];
}
	
/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t ov3640_sensor_csi_ops =
{
	SENSOR_OV3640_CSI_NAME,				/* sensor name */
	ov3640_csi_init,
	ov3640_csi_uninit,
	ov3640_csi_stream_start,
	ov3640_csi_stream_stop,
	ov3640_csi_get_info,
	{
		/* 1st info */
		{
			MCLK_24M,					/* CSI clock */
			V4L2_PIX_FMT_VYUY,			/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			CSI_SENSOR_FPS_30,			/* FPS in sensor */
			1024,						/* target width */
			768, 						/* target height */
			1024,						/* sensor width */
			768, 						/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 2nd info */	
		{
			MCLK_24M,					/* CSI clock */
			V4L2_PIX_FMT_VYUY,			/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			CSI_SENSOR_FPS_30,			/* FPS in sensor */
			2048,						/* target width */
			1536, 						/* target height */
			2048,						/* sensor width */
			1536, 						/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 3st info */
		{
			MCLK_24M,					/* CSI clock */
			V4L2_PIX_FMT_VYUY,			/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			CSI_SENSOR_FPS_30,			/* FPS in sensor */
			1024,						/* target width */
			768, 						/* target height */
			1024,						/* sensor width */
			768, 						/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		}
	}
};
#endif //(defined _SENSOR_GC03080_CSI) && (_SENSOR_GC03080_CSI == 1)