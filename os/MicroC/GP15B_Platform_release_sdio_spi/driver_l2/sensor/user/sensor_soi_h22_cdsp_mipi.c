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
#include "drv_l1_power.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_i2c.h"
#include "drv_l1_csi.h"
#include "drv_l1_mipi.h"
#include "drv_l1_cdsp.h"
#include "drv_l1_front.h"
#include "drv_l2_cdsp.h"
#include "drv_l2_cdsp_config.h"
#include "drv_l2_sensor.h"
#include "drv_l2_sccb.h"

#if (defined _SENSOR_SOI_H22_CDSP_MIPI) && (_SENSOR_SOI_H22_CDSP_MIPI == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//#define MIPI_ISR_TEST
 
#define SOI_H22_ID						0x60

#define MAX_ANALOG_GAIN					2.5

#define H22_MAX_EXPOSURE_TIME			(256*3)//0x30E//782
#define H22_MIN_EXPOSURE_TIME			0x08
#define H22_MAX_ANALOG_GAIN				(MAX_ANALOG_GAIN*256) //(3*256)	//4x
#define H22_MIN_ANALOG_GAIN				(1*256)
#define H22_MAX_DIGITAL_GAIN			(1.5*256)//0x3=4x
#define H22_MIN_DIGITAL_GAIN 			(1*256)//0:1x

#define H22_50HZ_EXP_TIME_TOTAL			212
#define	H22_50HZ_INIT_EV_IDX			140
#define H22_50HZ_NIGHT_EV_IDX			153
#define H22_50HZ_DAY_EV_IDX				100
#define H22_50HZ_MAX_EXP_IDX			(H22_50HZ_EXP_TIME_TOTAL-24)  // <= H22_50HZ_EXP_TIME_TOTAL

#define H22_60HZ_EXP_TIME_TOTAL			214
#define	H22_60HZ_INIT_EV_IDX			140
#define H22_60HZ_NIGHT_EV_IDX			153
#define H22_60HZ_DAY_EV_IDX				51
#define H22_60HZ_MAX_EXP_IDX			(H22_60HZ_EXP_TIME_TOTAL-22)  // <= H22_60HZ_EXP_TIME_TOTAL

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct regval8_s 
{
	INT8U reg_num;
	INT8U value;
} regval8_t;

gpCdspWBGain_t *soi_h22_awb_r_b_gain_boundary(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if SCCB_MODE == SCCB_GPIO
	static void *h22_handle;
#elif SCCB_MODE == SCCB_HW_I2C
	static drv_l1_i2c_bus_handle_t h22_handle; 
#endif

static sensor_exposure_t jxh22_seInfo;
static int *p_expTime_table;
static INT8U h22_read_en;
static gpCdspWBGain_t soi_h22_wbgain;
static gpCdspFav_t myFav;

static const short g_H22_FavTable[] =
{
		0,		//ob_val
	50,//0x30,	//targetY
	20,		//targetY low = (TARGET_Y/4)
	AWB_AUTO_CVR,	//awb mode
	MODE_NORMAL,	//color mode
	ISO_AUTO,		//ISO
	6,				//EV // 0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0 
	0,	//	#define	DAYLIGHT_HUE_U_OFFSET  	0		//-128 ~ +127,   +: more blue,  -: more yellow/green	
	1,	//	#define	DAYLIGHT_HUE_V_OFFSET  	1 		//-128 ~ +127,   +: more red,  -: more blue/green
	-4,//-1,	//	#define	DAYLIGHT_HUE_Y_OFFSET  	-1		//-128 ~ +127	
	0x22,//	#define	DAYLIGHT_SAT_Y_SCALE  	0x22	//Default:0x20
	0x26,//	#define	DAYLIGHT_SAT_U_SCALE  	0x25	//Default:0x20 	// blud
	0x26,//	#define	DAYLIGHT_SAT_V_SCALE  	0x25	//Default:0x20	// red
	2,	//#define DAYLIGHT_EDGE			2
	0,	//#define DAYLIGHT_WB_OFFSET		0
	0,	//	#define	NIGHT_HUE_U_OFFSET  	0		//-128 ~ +127,   +: more blue,  -: more yellow/green	
	-4,//-4,	//	#define	NIGHT_HUE_V_OFFSET  	-4		//-128 ~ +127,   +: more red,  -: more blue/green
	-8,	//	#define	NIGHT_HUE_Y_OFFSET  	-1		//-128 ~ +127	
	0x22,//	#define	NIGHT_SAT_Y_SCALE  		0x22	//Default:0x20
	0x1B,//	#define	NIGHT_SAT_U_SCALE  		0x1B	//Default:0x20	// blud
	0x1B,//	#define	NIGHT_SAT_V_SCALE  		0x1B	//Default:0x20	// red
	2,	//#define NIGHT_EDGE				2	
	0	//	#define NIGHT_WB_OFFSET			0
/*
	0,		//ob_val
	0x30,	//targetY
	10,		//targetY low
	AWB_AUTO_CVR,	//awb mode
	MODE_NORMAL,	//color mode
	ISO_AUTO,		//ISO
	6,				//EV
	0,
	0,
	0,
	0x20,
	0x20,
	0x20,
	3,
	0,
	0,
	0,
	0,
	0x20,
	0x20,
	0x20,
	3,
	0	
	*/
};
 
static mipi_config_t h22_mipi_cfg =
{
	DISABLE, 			/* low power, 0:disable, 1:enable */
	D_PHY_SAMPLE_NEG, 	/* byte clock edge, 0:posedge, 1:negedge */
	NULL,				/* RSD 0 */
	NULL,				/* RSD 1 */
	
	MIPI_USER_DEFINE,	/* data mode, 0:auto detect, 1:user define */
	MIPI_RAW10,			/* data type, valid when data mode is 1*/
	MIPI_DATA_TO_CDSP,	/* data type, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */  
	NULL,				/* RSD 2 */
	
	1280,				/* width, 0~0xFFFF */
	720,				/* height, 0~0xFFFF */
	4, 					/* back porch, 0~0xF */
	4,					/* front porch, 0~0xF */
	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	NULL,				/* RSD 3 */
	
	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	100,				/* data mask, unit is ns */
	MIPI_CHECK_HS_SEQ	/* check hs sequence or LP00 for clock lane */
};

const regval8_t SOI_H22_MIPI_VGA_f60[] =
{
	//Sleep Mode
	{0x12,0x40},
	//Gain
	{0x0E,0x1D}, //for MCLK=24Mhz, please change to 1C for MCLK=12Mhz
	{0x0F,0x09},
	{0x10,0x20},
	{0x11,0x80},
	//AEC/AGC
	{0x13,0x01},//C6:enable AE
	{0x14,0x80},
	{0x16,0xA0},
	{0x17,0x40},
	{0x18,0xD5},
	{0x19,0x00},
	//DVP
	{0x1D,0x00},
	{0x1E,0x1C},
	{0x1F,0x10},
	//Frame
	{0x20,0x5C},
	{0x21,0x03},
	{0x22,0xEA},
	{0x23,0x02},
	//Window
	{0x24,0x84},	//enhance +4
	{0x25,0xE2},	//enhance +2
	{0x26,0x12},
	{0x27,0xC1},
	{0x28,0x0D},
	{0x29,0x00},
	//Physical
	{0x2C,0x28},
	{0x2D,0x28},
	{0x2E,0xA4},
	{0x2F,0x20},
	{0x37,0x62},
	{0x38,0x10},
	//MIPI
	{0x67,0x30},//Mipi sleep mode disable
	{0x6c,0x10},//Mipi active
	{0x76,0x20},//word counter low
	{0x77,0x03},//word counter high
	{0x12,0x00},
	{0xFF,0xFF}
};

const regval8_t SOI_H22_MIPI_720P[] =
{
	//Sleep Mode
	{0x12,0x40},
	//Gain
	{0x0E,0x1D}, //for MCLK=24Mhz, please change to 1C for MCLK=12Mhz
	{0x0F,0x09},
	{0x10,0x20},
	{0x11,0x80},
	//AEC/AGC
	{0x13,0x01},//C6:enable AE
	{0x14,0x80},
	{0x16,0xA0},
	{0x17,0x40},
	{0x18,0xD5},
	{0x19,0x00},
	//DVP
	{0x1D,0x00},
	{0x1E,0x1C},
	//Frame
	{0x20,0xDC},
	{0x21,0x05},
	{0x22,0x55},
	{0x23,0x03},
	//Window
	{0x24,0x04},	//enhance +4
	{0x25,0xD2},	//enhance +2
	{0x26,0x25},
	{0x27,0xC1},
	{0x28,0x0D},
	{0x29,0x00},
	//Physical
	{0x2C,0x00},
	{0x2D,0x0A},
	{0x2E,0xC2},
	{0x2F,0x20},
	{0x37,0x40},
	{0x38,0x98},
	//MIPI
	{0x67,0x30},//Mipi sleep mode disable
	{0x6c,0x10},//Mipi active
	{0x76,0x40},//word counter low
	{0x77,0x06},//word counter high
	{0x12,0x00},
	{0xFF,0xFF}
};

//1280x800x30_Mipi10
const regval8_t H22_MIPI_1280_800_30[] =
{
	//---Sleep Mode
	{0x12,0x40},
	//---Gain
	{0x0E,0x1D}, //for MCLK=24Mhz, please change to 1C for MCLK=12Mhz
	{0x0F,0x09},
	{0x10,0x20},
	{0x11,0x80},
	//---AEC/AGC
	{0x14,0x80},
	{0x16,0xA0},
	{0x17,0x40},
	{0x18,0xD5},
	{0x19,0x00},
	//---DVP
	{0x1D,0x00},
	{0x1E,0x1C},
	//---Frame
	{0x20,0xDC},
	{0x21,0x05},
	{0x22,0x55},
	{0x23,0x03},
	//---Window
	{0x24,0x04},	//enhance +4 for cdsp interpolation
	{0x25,0x22},	//enhance +2 for cdsp interpolation
	{0x26,0x35},
	{0x27,0xC1},
	{0x28,0x0D},
	{0x29,0x00},
	//---Physical
	{0x2C,0x00},
	{0x2D,0x00},
	{0x2E,0xCC},
	{0x2F,0x20},
	//;;;;;
	{0x37,0x4A},
	{0x38,0x70},
	// MIPI
	{0x67,0x30},//Mipi sleep mode disable
	{0x6c,0x10},//Mipi active
	{0x76,0x40},//word counter low
	{0x77,0x06},//word counter high
	{0x12,0x00},
	{0xFF,0xFF}
};

static const unsigned short g_H22_r_b_gain[71][2] = 
{
	{24, 141}, // 2
	{28, 140},
	{32, 139},
	{36, 138},
	{39, 137},
	{42, 136},
	{46, 135},
	{49, 134},
	{51, 133},
	{54, 132}, // 3
	{56, 131},
	{59, 130},
	{61, 129},
	{63, 127},
	{65, 126},
	{67, 125}, // 3.6
	{68, 124},
	{70, 123},
	{71, 122},
	{72, 120}, // 4
	{74, 119},
	{75, 118},
	{76, 117},
	{77, 115},
	{77, 114},
	{78, 113},
	{79, 111},
	{79, 110},
	{80, 109},
	{80, 107}, // 5
	{81, 106},
	{81, 105},
	{81, 103},
	{82, 102},
	{82, 100},// 5.5
	{82, 99}, 
	{82, 98}, 
	{83, 96}, 
	{83, 95}, 
	{83, 93}, // 6
	{83, 92}, 
	{83, 90}, 
	{84, 88}, 
	{84, 87}, 
	{84, 85},  // 6.5
	{84, 84}, 
	{85, 82}, 
	{85, 81}, 
	{85, 79}, 
	{86, 77}, 
	{86, 76}, 
	{87, 74}, 
	{87, 72}, 
	{88, 71}, 
	{89, 69}, 
	{90, 67}, 
	{91, 65}, 
	{92, 64}, 
	{93, 62}, 
	{94, 60}, 
	{95, 58}, 
	{97, 56}, 
	{98, 55}, 
	{100, 53},
	{102, 51},
	{104, 49},
	{106, 47},
	{108, 45},
	{111, 43},
	{113, 41},
	{116, 39} 
};

static const unsigned int g_H22_gamma_045_table[] =
{
	0x04510d, 0x051112, 0x111417, 0x14451b, 0x111120, 0x044425, 0x111129, 0x05112d, 
	0x044432, 0x044436, 0x04443a, 0x04443e, 0x044442, 0x110446, 0x111149, 0x04444d, 
	0x111051, 0x044154, 0x111058, 0x10445b, 0x04415e, 0x011062, 0x011065, 0x010468, 
	0x01046b, 0x01046e, 0x041071, 0x041074, 0x104176, 0x010479, 0x04107c, 0x00417e, 
	0x041081, 0x010183, 0x101086, 0x040488, 0x01018a, 0x10408d, 0x10108f, 0x041091, 
	0x040493, 0x040495, 0x040497, 0x040499, 0x10109b, 0x00109d, 0x00409f, 0x0101a0, 
	0x1004a2, 0x0010a4, 0x0100a6, 0x1004a7, 0x0040a9, 0x1001aa, 0x0040ac, 0x1001ad, 
	0x0100af, 0x0004b0, 0x0400b2, 0x0040b3, 0x0004b4, 0x0400b6, 0x0100b7, 0x0010b8, 
	0x0004b9, 0x1000bb, 0x0400bc, 0x0100bd, 0x0040be, 0x0010bf, 0x0004c0, 0x0004c1, 
	0x0001c2, 0x0001c3, 0x0000c5, 0x0000c6, 0x0000c7, 0x0000c8, 0x0000c9, 0x0000ca, 
	0x0000cb, 0x0000cc, 0x0000cd, 0x0000ce, 0x0000cf, 0x0000d0, 0x0001d0, 0x0001d1, 
	0x0001d2, 0x0001d3, 0x0001d4, 0x0001d5, 0x0001d6, 0x0001d7, 0x0001d8, 0x0001d9, 
	0x0000db, 0x0000dc, 0x1000dd, 0x0400de, 0x0400df, 0x0100e0, 0x0040e1, 0x0010e2, 
	0x0004e3, 0x1000e5, 0x0100e6, 0x0040e7, 0x0004e8, 0x1000ea, 0x0100eb, 0x0004ec, 
	0x0400ee, 0x0040ef, 0x1001f0, 0x0040f2, 0x1004f3, 0x0040f5, 0x0401f6, 0x0010f8, 
	0x0100fa, 0x0404fb, 0x1010fd, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff
};

static const short g_H22_color_matrix4gamma045[9] = 
{	
	(short) (1.289461127029244000*64),
	(short) (-0.15633132671308050*64),
	(short) (-0.13312980031616348*64),
	(short) (-0.28924304019577968*64),
	(short) (1.448004578314210100*64),
	(short) (-0.15876153811843055*64),
	(short) (-0.02364080679490283*64),
	(short) (-1.03955844263753080*64),
	(short) (2.063199249432433800*64)
};

static const short g_H22_awb_thr[19] = 
{
	200, // awbwinthr
	
	0*64, // sindata
	1*64, // cosdata 
	
	 30, // Ythr0
	 90, // Ythr1
	150, // Ythr2 
	200, // Ythr3 
	
	-6, // UL1N1 
	 5, // UL1P1 
	-6, // VL1N1 
	 5, // VL1P1 
	
	-6, // UL1N2
	 6, // UL1P2
	-6, // VL1N2
	 6, // VL1P2
	
	-7, // UL1N3
	 7, // UL1P3
	-7, // VL1N3
	 7, // VL1P3
};

static const unsigned short g_h22_badpix_ob_table[12] = 
{
	0, 	// obautoen
	
	1, 	// obmanuen
	16, // maunob

	1, 	// wboffseten
	0, 	// wbo_r
	0, 	// wbo_gr
	0, 	// wbo_gb
	0, 	// wbo_b
	
	1,		//badPixel En
	160,	//bprthr
	160,	//bpgthr
	160		//bpbthr
};

static const sensor_calibration_t h22_cdsp_calibration =
{
	g_h22_badpix_ob_table,							//ob
	NULL, 							//lenscmp
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	g_H22_r_b_gain,					//wb_gain
	g_H22_gamma_045_table,			//gamma1
	g_H22_color_matrix4gamma045,	//color_matrix1
	g_H22_gamma_045_table,			//gamma2
	g_H22_color_matrix4gamma045,	//color_matrix2
	g_H22_awb_thr					//awb_thr	
};

static const int g_h22_exp_time_gain_60Hz[H22_60HZ_EXP_TIME_TOTAL][3] = 
{ // {time, analog gain, digital gain}
{8,	    (int)(1.00*256), (int)(1.00*256)},
{9,	    (int)(1.00*256), (int)(1.00*256)},
{9,	    (int)(1.00*256), (int)(1.00*256)},
{9,	    (int)(1.00*256), (int)(1.00*256)},
{10,	(int)(1.00*256), (int)(1.00*256)},
{10,	(int)(1.00*256), (int)(1.00*256)},
{10,	(int)(1.00*256), (int)(1.00*256)},
{11,	(int)(1.00*256), (int)(1.00*256)},
{11,	(int)(1.00*256), (int)(1.00*256)},
{12,	(int)(1.00*256), (int)(1.00*256)},
{12,	(int)(1.00*256), (int)(1.00*256)},
{12,	(int)(1.00*256), (int)(1.00*256)},
{13,	(int)(1.00*256), (int)(1.00*256)},
{13,	(int)(1.00*256), (int)(1.00*256)},
{14,	(int)(1.00*256), (int)(1.00*256)},
{14,	(int)(1.00*256), (int)(1.00*256)},
{15,	(int)(1.00*256), (int)(1.00*256)},
{15,	(int)(1.00*256), (int)(1.00*256)},
{16,	(int)(1.00*256), (int)(1.00*256)},
{16,	(int)(1.00*256), (int)(1.00*256)},
{17,	(int)(1.00*256), (int)(1.00*256)},
{18,	(int)(1.00*256), (int)(1.00*256)},
{18,	(int)(1.00*256), (int)(1.00*256)},
{19,	(int)(1.00*256), (int)(1.00*256)},
{19,	(int)(1.00*256), (int)(1.00*256)},
{20,	(int)(1.00*256), (int)(1.00*256)},
{21,	(int)(1.00*256), (int)(1.00*256)},
{22,	(int)(1.00*256), (int)(1.00*256)},
{22,	(int)(1.00*256), (int)(1.00*256)},
{23,	(int)(1.00*256), (int)(1.00*256)},
{24,	(int)(1.00*256), (int)(1.00*256)},
{25,	(int)(1.00*256), (int)(1.00*256)},
{26,	(int)(1.00*256), (int)(1.00*256)},
{27,	(int)(1.00*256), (int)(1.00*256)},
{28,	(int)(1.00*256), (int)(1.00*256)},
{29,	(int)(1.00*256), (int)(1.00*256)},
{30,	(int)(1.00*256), (int)(1.00*256)},
{31,	(int)(1.00*256), (int)(1.00*256)},
{32,	(int)(1.00*256), (int)(1.00*256)},
{33,	(int)(1.00*256), (int)(1.00*256)},
{34,	(int)(1.00*256), (int)(1.00*256)},
{35,	(int)(1.00*256), (int)(1.00*256)},
{36,	(int)(1.00*256), (int)(1.00*256)},
{38,	(int)(1.00*256), (int)(1.00*256)},
{39,	(int)(1.00*256), (int)(1.00*256)},
{40,	(int)(1.00*256), (int)(1.00*256)},
{42,	(int)(1.00*256), (int)(1.00*256)},
{43,	(int)(1.00*256), (int)(1.00*256)},
{45,	(int)(1.00*256), (int)(1.00*256)},
{46,	(int)(1.00*256), (int)(1.00*256)},
{48,	(int)(1.00*256), (int)(1.00*256)},
{50,	(int)(1.00*256), (int)(1.00*256)},
{51,	(int)(1.00*256), (int)(1.00*256)},
{53,	(int)(1.00*256), (int)(1.00*256)},
{55,	(int)(1.00*256), (int)(1.00*256)},
{57,	(int)(1.00*256), (int)(1.00*256)},
{59,	(int)(1.00*256), (int)(1.00*256)},
{61,	(int)(1.00*256), (int)(1.00*256)},
{63,	(int)(1.00*256), (int)(1.00*256)},
{66,	(int)(1.00*256), (int)(1.00*256)},
{68,	(int)(1.00*256), (int)(1.00*256)},
{70,	(int)(1.00*256), (int)(1.00*256)},
{73,	(int)(1.00*256), (int)(1.00*256)},
{75,	(int)(1.00*256), (int)(1.00*256)},
{78,	(int)(1.00*256), (int)(1.00*256)},
{81,	(int)(1.00*256), (int)(1.00*256)},
{84,	(int)(1.00*256), (int)(1.00*256)},
{87,	(int)(1.00*256), (int)(1.00*256)},
{90,	(int)(1.00*256), (int)(1.00*256)},
{93,	(int)(1.00*256), (int)(1.00*256)},
{96,	(int)(1.00*256), (int)(1.00*256)},
{99,	(int)(1.00*256), (int)(1.00*256)},
{103,	(int)(1.00*256), (int)(1.00*256)},
{107,	(int)(1.00*256), (int)(1.00*256)},
{110,	(int)(1.00*256), (int)(1.00*256)},
{114,	(int)(1.00*256), (int)(1.00*256)},
{118,	(int)(1.00*256), (int)(1.00*256)},
{122,	(int)(1.00*256), (int)(1.00*256)},
{127,	(int)(1.00*256), (int)(1.00*256)},
{131,	(int)(1.00*256), (int)(1.00*256)},
{136,	(int)(1.00*256), (int)(1.00*256)},
{141,	(int)(1.00*256), (int)(1.00*256)},
{145,	(int)(1.00*256), (int)(1.00*256)},
{151,	(int)(1.00*256), (int)(1.00*256)},
{156,	(int)(1.00*256), (int)(1.00*256)},
{161,	(int)(1.00*256), (int)(1.00*256)},
{167,	(int)(1.00*256), (int)(1.00*256)},
{173,	(int)(1.00*256), (int)(1.00*256)},
{179,	(int)(1.00*256), (int)(1.00*256)},
{185,	(int)(1.00*256), (int)(1.00*256)},
{192,	(int)(1.00*256), (int)(1.00*256)},
{199,	(int)(1.00*256), (int)(1.00*256)},
{206,	(int)(1.00*256), (int)(1.00*256)},
{213,	(int)(1.00*256), (int)(1.00*256)},
{213,	(int)(1.00*256), (int)(1.04*256)},
{213,	(int)(1.06*256), (int)(1.00*256)},
{213,	(int)(1.13*256), (int)(1.00*256)},
{213,	(int)(1.13*256), (int)(1.00*256)},
{213,	(int)(1.19*256), (int)(1.00*256)},
{213,	(int)(1.25*256), (int)(1.00*256)},
{213,	(int)(1.25*256), (int)(1.00*256)},
{213,	(int)(1.31*256), (int)(1.00*256)},
{213,	(int)(1.38*256), (int)(1.00*256)},
{213,	(int)(1.44*256), (int)(1.00*256)},
{213,	(int)(1.44*256), (int)(1.00*256)},
{213,	(int)(1.50*256), (int)(1.00*256)},
{213,	(int)(1.56*256), (int)(1.00*256)},
{213,	(int)(1.63*256), (int)(1.00*256)},
{213,	(int)(1.69*256), (int)(1.00*256)},
{213,	(int)(1.75*256), (int)(1.00*256)},
{213,	(int)(1.81*256), (int)(1.00*256)},
{213,	(int)(1.88*256), (int)(1.00*256)},
{213,	(int)(1.94*256), (int)(1.00*256)},
{426,	(int)(1.00*256), (int)(1.00*256)},
{426,	(int)(1.00*256), (int)(1.04*256)},
{426,	(int)(1.06*256), (int)(1.00*256)},
{426,	(int)(1.13*256), (int)(1.00*256)},
{426,	(int)(1.13*256), (int)(1.00*256)},
{426,	(int)(1.19*256), (int)(1.00*256)},
{426,	(int)(1.25*256), (int)(1.00*256)},
{426,	(int)(1.25*256), (int)(1.00*256)},
{426,	(int)(1.31*256), (int)(1.00*256)},
{426,	(int)(1.38*256), (int)(1.00*256)},
{426,	(int)(1.44*256), (int)(1.00*256)},
{426,	(int)(1.44*256), (int)(1.00*256)},
{426,	(int)(1.50*256), (int)(1.00*256)},
{426,	(int)(1.56*256), (int)(1.00*256)},
{426,	(int)(1.63*256), (int)(1.00*256)},
{426,	(int)(1.69*256), (int)(1.00*256)},
{426,	(int)(1.75*256), (int)(1.00*256)},
{426,	(int)(1.81*256), (int)(1.00*256)},
{426,	(int)(1.88*256), (int)(1.00*256)},
{426,	(int)(1.94*256), (int)(1.00*256)},
{639,	(int)(1.31*256), (int)(1.01*256)},
{639,	(int)(1.38*256), (int)(1.00*256)},
{639,	(int)(1.44*256), (int)(1.00*256)},
{639,	(int)(1.44*256), (int)(1.00*256)},
{639,	(int)(1.50*256), (int)(1.00*256)},
{639,	(int)(1.56*256), (int)(1.00*256)},
{639,	(int)(1.63*256), (int)(1.00*256)},
{639,	(int)(1.69*256), (int)(1.00*256)},
{639,	(int)(1.75*256), (int)(1.00*256)},
{639,	(int)(1.81*256), (int)(1.00*256)},
{639,	(int)(1.88*256), (int)(1.00*256)},
{639,	(int)(1.94*256), (int)(1.00*256)},
{852,	(int)(1.50*256), (int)(1.00*256)},
{852,	(int)(1.56*256), (int)(1.00*256)},
{852,	(int)(1.63*256), (int)(1.00*256)},
{852,	(int)(1.69*256), (int)(1.00*256)},
{852,	(int)(1.75*256), (int)(1.00*256)},
{852,	(int)(1.81*256), (int)(1.00*256)},
{852,	(int)(1.88*256), (int)(1.00*256)},
{852,	(int)(1.94*256), (int)(1.00*256)},
{852,	(int)(2.00*256), (int)(1.00*256)},
{852,	(int)(2.00*256), (int)(1.04*256)},
{852,	(int)(2.13*256), (int)(1.00*256)},
{852,	(int)(2.25*256), (int)(1.00*256)},
{852,	(int)(2.25*256), (int)(1.00*256)},
{852,	(int)(2.38*256), (int)(1.00*256)},
{852,	(int)(2.50*256), (int)(1.00*256)},
{852,	(int)(2.50*256), (int)(1.00*256)},
{852,	(int)(2.63*256), (int)(1.00*256)},
{852,	(int)(2.75*256), (int)(1.00*256)},
{852,	(int)(2.88*256), (int)(1.00*256)},
{852,	(int)(2.88*256), (int)(1.00*256)},
{852,	(int)(3.00*256), (int)(1.00*256)},
{852,	(int)(3.13*256), (int)(1.00*256)},
{852,	(int)(3.25*256), (int)(1.00*256)},
{852,	(int)(3.38*256), (int)(1.00*256)},
{852,	(int)(3.50*256), (int)(1.00*256)},
{852,	(int)(3.63*256), (int)(1.00*256)},
{852,	(int)(3.75*256), (int)(1.00*256)},
{852,	(int)(3.88*256), (int)(1.00*256)},
{852,	(int)(4.00*256), (int)(1.00*256)},
{852,	(int)(4.00*256), (int)(1.04*256)},
{852,	(int)(4.25*256), (int)(1.00*256)},
{852,	(int)(4.25*256), (int)(1.04*256)},
{852,	(int)(4.50*256), (int)(1.00*256)},
{852,	(int)(4.75*256), (int)(1.00*256)},
{852,	(int)(5.00*256), (int)(1.00*256)},
{852,	(int)(5.00*256), (int)(1.04*256)},
{852,	(int)(5.25*256), (int)(1.00*256)},
{852,	(int)(5.50*256), (int)(1.00*256)},
{852,	(int)(5.50*256), (int)(1.04*256)},
{852,	(int)(5.75*256), (int)(1.02*256)},
{852,	(int)(6.00*256), (int)(1.00*256)},
{852,	(int)(6.25*256), (int)(1.00*256)},
{852,	(int)(6.50*256), (int)(1.00*256)},
{852,	(int)(6.75*256), (int)(1.00*256)},
{852,	(int)(7.00*256), (int)(1.00*256)},
{852,	(int)(7.00*256), (int)(1.04*256)},
{852,	(int)(7.00*256), (int)(1.07*256)},
{852,	(int)(7.00*256), (int)(1.11*256)},
{852,	(int)(7.00*256), (int)(1.15*256)},
{852,	(int)(7.00*256), (int)(1.19*256)},
{852,	(int)(7.00*256), (int)(1.23*256)},
{852,	(int)(7.00*256), (int)(1.27*256)},
{852,	(int)(7.00*256), (int)(1.32*256)},
{852,	(int)(7.00*256), (int)(1.37*256)},
{852,	(int)(7.00*256), (int)(1.41*256)},
{852,	(int)(7.00*256), (int)(1.46*256)},
{852,	(int)(7.00*256), (int)(1.52*256)},
{852,	(int)(7.00*256), (int)(1.57*256)},
{852,	(int)(7.00*256), (int)(1.62*256)},
{852,	(int)(7.00*256), (int)(1.68*256)},
{852,	(int)(7.00*256), (int)(1.74*256)},
{852,	(int)(7.00*256), (int)(1.80*256)},
{852,	(int)(7.00*256), (int)(1.87*256)},
{852,	(int)(7.00*256), (int)(1.93*256)},
{852,	(int)(7.00*256), (int)(2.00*256)},
{852,	(int)(7.25*256), (int)(2.00*256)},
{852,	(int)(7.50*256), (int)(2.00*256)},
{852,	(int)(7.75*256), (int)(2.00*256)},
{852,	(int)(8.00*256), (int)(2.00*256)}
};

static const  int g_h22_exp_time_gain_50Hz[H22_50HZ_EXP_TIME_TOTAL][3] = 
{ // {time, analog gain, digital gain}
{8, (int)(1.00*256), (int)(1.00*256)},
{9, (int)(1.00*256), (int)(1.00*256)},
{9, (int)(1.00*256), (int)(1.00*256)},
{9, (int)(1.00*256), (int)(1.00*256)},
{10, (int)(1.00*256), (int)(1.00*256)},
{10, (int)(1.00*256), (int)(1.00*256)},
{10, (int)(1.00*256), (int)(1.00*256)},
{11, (int)(1.00*256), (int)(1.00*256)},
{11, (int)(1.00*256), (int)(1.00*256)},
{11, (int)(1.00*256), (int)(1.00*256)},
{12, (int)(1.00*256), (int)(1.00*256)},
{12, (int)(1.00*256), (int)(1.00*256)},
{13, (int)(1.00*256), (int)(1.00*256)},
{13, (int)(1.00*256), (int)(1.00*256)},
{13, (int)(1.00*256), (int)(1.00*256)},
{14, (int)(1.00*256), (int)(1.00*256)},
{14, (int)(1.00*256), (int)(1.00*256)},
{15, (int)(1.00*256), (int)(1.00*256)},
{15, (int)(1.00*256), (int)(1.00*256)},
{16, (int)(1.00*256), (int)(1.00*256)},
{17, (int)(1.00*256), (int)(1.00*256)},
{17, (int)(1.00*256), (int)(1.00*256)},
{18, (int)(1.00*256), (int)(1.00*256)},
{18, (int)(1.00*256), (int)(1.00*256)},
{19, (int)(1.00*256), (int)(1.00*256)},
{20, (int)(1.00*256), (int)(1.00*256)},
{20, (int)(1.00*256), (int)(1.00*256)},
{21, (int)(1.00*256), (int)(1.00*256)},
{22, (int)(1.00*256), (int)(1.00*256)},
{23, (int)(1.00*256), (int)(1.00*256)},
{23, (int)(1.00*256), (int)(1.00*256)},
{24, (int)(1.00*256), (int)(1.00*256)},
{25, (int)(1.00*256), (int)(1.00*256)},
{26, (int)(1.00*256), (int)(1.00*256)},
{27, (int)(1.00*256), (int)(1.00*256)},
{28, (int)(1.00*256), (int)(1.00*256)},
{29, (int)(1.00*256), (int)(1.00*256)},
{30, (int)(1.00*256), (int)(1.00*256)},
{31, (int)(1.00*256), (int)(1.00*256)},
{32, (int)(1.00*256), (int)(1.00*256)},
{33, (int)(1.00*256), (int)(1.00*256)},
{34, (int)(1.00*256), (int)(1.00*256)},
{36, (int)(1.00*256), (int)(1.00*256)},
{37, (int)(1.00*256), (int)(1.00*256)},
{38, (int)(1.00*256), (int)(1.00*256)},
{39, (int)(1.00*256), (int)(1.00*256)},
{41, (int)(1.00*256), (int)(1.00*256)},
{42, (int)(1.00*256), (int)(1.00*256)},
{44, (int)(1.00*256), (int)(1.00*256)},
{45, (int)(1.00*256), (int)(1.00*256)},
{47, (int)(1.00*256), (int)(1.00*256)},
{49, (int)(1.00*256), (int)(1.00*256)},
{50, (int)(1.00*256), (int)(1.00*256)},
{52, (int)(1.00*256), (int)(1.00*256)},
{54, (int)(1.00*256), (int)(1.00*256)},
{56, (int)(1.00*256), (int)(1.00*256)},
{58, (int)(1.00*256), (int)(1.00*256)},
{60, (int)(1.00*256), (int)(1.00*256)},
{62, (int)(1.00*256), (int)(1.00*256)},
{64, (int)(1.00*256), (int)(1.00*256)},
{66, (int)(1.00*256), (int)(1.00*256)},
{69, (int)(1.00*256), (int)(1.00*256)},
{71, (int)(1.00*256), (int)(1.00*256)},
{74, (int)(1.00*256), (int)(1.00*256)},
{76, (int)(1.00*256), (int)(1.00*256)},
{79, (int)(1.00*256), (int)(1.00*256)},
{82, (int)(1.00*256), (int)(1.00*256)},
{84, (int)(1.00*256), (int)(1.00*256)},
{87, (int)(1.00*256), (int)(1.00*256)},
{91, (int)(1.00*256), (int)(1.00*256)},
{94, (int)(1.00*256), (int)(1.00*256)},
{97, (int)(1.00*256), (int)(1.00*256)},
{100, (int)(1.00*256), (int)(1.00*256)},
{104, (int)(1.00*256), (int)(1.00*256)},
{108, (int)(1.00*256), (int)(1.00*256)},
{111, (int)(1.00*256), (int)(1.00*256)},
{115, (int)(1.00*256), (int)(1.00*256)},
{119, (int)(1.00*256), (int)(1.00*256)},
{124, (int)(1.00*256), (int)(1.00*256)},
{128, (int)(1.00*256), (int)(1.00*256)},
{133, (int)(1.00*256), (int)(1.00*256)},
{137, (int)(1.00*256), (int)(1.00*256)},
{142, (int)(1.00*256), (int)(1.00*256)},
{147, (int)(1.00*256), (int)(1.00*256)},
{152, (int)(1.00*256), (int)(1.00*256)},
{158, (int)(1.00*256), (int)(1.00*256)},
{163, (int)(1.00*256), (int)(1.00*256)},
{169, (int)(1.00*256), (int)(1.00*256)},
{175, (int)(1.00*256), (int)(1.00*256)},
{181, (int)(1.00*256), (int)(1.00*256)},
{187, (int)(1.00*256), (int)(1.00*256)},
{194, (int)(1.00*256), (int)(1.00*256)},
{201, (int)(1.00*256), (int)(1.00*256)},
{208, (int)(1.00*256), (int)(1.00*256)},
{215, (int)(1.00*256), (int)(1.00*256)},
{223, (int)(1.00*256), (int)(1.00*256)},
{231, (int)(1.00*256), (int)(1.00*256)},
{239, (int)(1.00*256), (int)(1.00*256)},
{247, (int)(1.00*256), (int)(1.00*256)},
{256, (int)(1.00*256), (int)(1.00*256)},
{256, (int)(1.00*256), (int)(1.04*256)},
{256, (int)(1.06*256), (int)(1.00*256)},
{256, (int)(1.13*256), (int)(1.00*256)},
{256, (int)(1.13*256), (int)(1.00*256)},
{256, (int)(1.19*256), (int)(1.00*256)},
{256, (int)(1.25*256), (int)(1.00*256)},
{256, (int)(1.25*256), (int)(1.00*256)},
{256, (int)(1.31*256), (int)(1.00*256)},
{256, (int)(1.38*256), (int)(1.00*256)},
{256, (int)(1.44*256), (int)(1.00*256)},
{256, (int)(1.44*256), (int)(1.00*256)},
{256, (int)(1.50*256), (int)(1.00*256)},
{256, (int)(1.56*256), (int)(1.00*256)},
{256, (int)(1.63*256), (int)(1.00*256)},
{256, (int)(1.69*256), (int)(1.00*256)},
{256, (int)(1.75*256), (int)(1.00*256)},
{256, (int)(1.81*256), (int)(1.00*256)},
{256, (int)(1.88*256), (int)(1.00*256)},
{256, (int)(1.94*256), (int)(1.00*256)},
{512, (int)(1.00*256), (int)(1.00*256)},
{512, (int)(1.00*256), (int)(1.04*256)},
{512, (int)(1.06*256), (int)(1.00*256)},
{512, (int)(1.13*256), (int)(1.00*256)},
{512, (int)(1.13*256), (int)(1.00*256)},
{512, (int)(1.19*256), (int)(1.00*256)},
{512, (int)(1.25*256), (int)(1.00*256)},
{512, (int)(1.25*256), (int)(1.00*256)},
{512, (int)(1.31*256), (int)(1.00*256)},
{512, (int)(1.38*256), (int)(1.00*256)},
{512, (int)(1.44*256), (int)(1.00*256)},
{512, (int)(1.44*256), (int)(1.00*256)},
{512, (int)(1.50*256), (int)(1.00*256)},
{512, (int)(1.56*256), (int)(1.00*256)},
{512, (int)(1.63*256), (int)(1.00*256)},
{512, (int)(1.69*256), (int)(1.00*256)},
{512, (int)(1.75*256), (int)(1.00*256)},
{512, (int)(1.81*256), (int)(1.00*256)},
{512, (int)(1.88*256), (int)(1.00*256)},
{512, (int)(1.94*256), (int)(1.00*256)},
{768, (int)(1.31*256), (int)(1.00*256)},
{768, (int)(1.38*256), (int)(1.00*256)},
{768, (int)(1.44*256), (int)(1.00*256)},
{768, (int)(1.44*256), (int)(1.00*256)},
{768, (int)(1.50*256), (int)(1.00*256)},
{768, (int)(1.56*256), (int)(1.00*256)},
{768, (int)(1.63*256), (int)(1.00*256)},
{768, (int)(1.69*256), (int)(1.00*256)},
{768, (int)(1.75*256), (int)(1.00*256)},
{768, (int)(1.81*256), (int)(1.00*256)},
{768, (int)(1.88*256), (int)(1.00*256)},
{768, (int)(1.94*256), (int)(1.00*256)},
{768, (int)(2.0*256), (int)(1.00*256)},
{768, (int)(2.00*256), (int)(1.04*256)},
{768, (int)(2.13*256), (int)(1.00*256)},
{768, (int)(2.25*256), (int)(1.00*256)},
{768, (int)(2.25*256), (int)(1.00*256)},
{768, (int)(2.38*256), (int)(1.00*256)},
{768, (int)(2.50*256), (int)(1.00*256)},
{768, (int)(2.50*256), (int)(1.00*256)},
{768, (int)(2.63*256), (int)(1.00*256)},
{768, (int)(2.75*256), (int)(1.00*256)},
{768, (int)(2.88*256), (int)(1.00*256)},
{768, (int)(2.88*256), (int)(1.00*256)},
{768, (int)(3.00*256), (int)(1.00*256)},
{768, (int)(3.13*256), (int)(1.00*256)},
{768, (int)(3.25*256), (int)(1.00*256)},
{768, (int)(3.38*256), (int)(1.00*256)},
{768, (int)(3.50*256), (int)(1.00*256)},
{768, (int)(3.63*256), (int)(1.00*256)},
{768, (int)(3.75*256), (int)(1.00*256)},
{768, (int)(3.88*256), (int)(1.00*256)},
{768, (int)(4.00*256), (int)(1.00*256)},
{768, (int)(4.00*256), (int)(1.04*256)},
{768, (int)(4.25*256), (int)(1.00*256)},
{768, (int)(4.25*256), (int)(1.04*256)},
{768, (int)(4.50*256), (int)(1.00*256)},
{768, (int)(4.75*256), (int)(1.00*256)},
{768, (int)(5.00*256), (int)(1.00*256)},
{768, (int)(5.00*256), (int)(1.04*256)},
{768, (int)(5.25*256), (int)(1.00*256)},
{768, (int)(5.50*256), (int)(1.00*256)},
{768, (int)(5.50*256), (int)(1.04*256)},
{768, (int)(5.75*256), (int)(1.02*256)},
{768, (int)(6.00*256), (int)(1.00*256)},
{768, (int)(6.25*256), (int)(1.00*256)},
{768, (int)(6.50*256), (int)(1.00*256)},
{768, (int)(6.75*256), (int)(1.00*256)},
{768, (int)(7.00*256), (int)(1.00*256)},
{768, (int)(7.00*256), (int)(1.04*256)},
{768, (int)(7.00*256), (int)(1.07*256)},
{768, (int)(7.00*256), (int)(1.11*256)},
{768, (int)(7.00*256), (int)(1.15*256)},
{768, (int)(7.00*256), (int)(1.19*256)},
{768, (int)(7.00*256), (int)(1.23*256)},
{768, (int)(7.00*256), (int)(1.27*256)},
{768, (int)(7.00*256), (int)(1.32*256)},
{768, (int)(7.00*256), (int)(1.37*256)},
{768, (int)(7.00*256), (int)(1.41*256)},
{768, (int)(7.00*256), (int)(1.46*256)},
{768, (int)(7.00*256), (int)(1.52*256)},
{768, (int)(7.00*256), (int)(1.57*256)},
{768, (int)(7.00*256), (int)(1.62*256)},
{768, (int)(7.00*256), (int)(1.68*256)},
{768, (int)(7.00*256), (int)(1.74*256)},
{768, (int)(7.00*256), (int)(1.80*256)},
{768, (int)(7.00*256), (int)(1.87*256)},
{768, (int)(7.00*256), (int)(1.93*256)},
{768, (int)(7.00*256), (int)(2.00*256)},
{768, (int)(7.25*256), (int)(2.00*256)},
{768, (int)(7.50*256), (int)(2.00*256)},
{768, (int)(7.75*256), (int)(2.00*256)},
{768, (int)(8.00*256), (int)(2.00*256)}
};

static const int H22_analog_gain_table[65] = 
{
	// coarse gain = 0
	(int)(1.00*256+0.5), (int)(1.06*256+0.5), (int)(1.13*256+0.5), (int)(1.19*256+0.5), 
	(int)(1.25*256+0.5), (int)(1.31*256+0.5), (int)(1.38*256+0.5), (int)(1.44*256+0.5), 
	(int)(1.50*256+0.5), (int)(1.56*256+0.5), (int)(1.63*256+0.5), (int)(1.69*256+0.5), 
	(int)(1.75*256+0.5), (int)(1.81*256+0.5), (int)(1.88*256+0.5), (int)(1.94*256+0.5),
	
	// coarse gain = 1
	(int)(2.00*256+0.5), (int)(2.13*256+0.5), (int)(2.25*256+0.5), (int)(2.38*256+0.5), 
	(int)(2.50*256+0.5), (int)(2.63*256+0.5), (int)(2.75*256+0.5), (int)(2.88*256+0.5),
	(int)(3.00*256+0.5), (int)(3.13*256+0.5), (int)(3.25*256+0.5), (int)(3.38*256+0.5),
	(int)(3.50*256+0.5), (int)(3.63*256+0.5), (int)(3.75*256+0.5), (int)(3.88*256+0.5),

	// coarse gain = 3
	(int)(4.00*256+0.5), (int)(4.25*256+0.5), (int)(4.50*256+0.5), (int)(4.75*256+0.5), 
	(int)(5.00*256+0.5), (int)(5.25*256+0.5), (int)(5.50*256+0.5), (int)(5.75*256+0.5), 
	(int)(6.00*256+0.5), (int)(6.25*256+0.5), (int)(6.50*256+0.5), (int)(6.75*256+0.5), 
	(int)(7.00*256+0.5), (int)(7.25*256+0.5), (int)(7.50*256+0.5), (int)(7.75*256+0.5), 
	
	// coarse gain = 7
	(int)(8.00*256+0.5), (int)(8.50*256+0.5), (int)(9.00*256+0.5), (int)(9.50*256+0.5), 
	(int)(10.00*256+0.5), (int)(10.50*256+0.5), (int)(11.00*256+0.5), (int)(11.50*256+0.5), 
	(int)(12.00*256+0.5), (int)(12.50*256+0.5), (int)(13.00*256+0.5), (int)(13.50*256+0.5), 
	(int)(14.00*256+0.5), (int)(14.50*256+0.5), (int)(15.00*256+0.5), (int)(15.50*256+0.5), 
	
	// coarse gain = 15
	(int)(16.00*256+0.5)
};

static INT32S h22_sccb_open(void)
{
#if SCCB_MODE == SCCB_GPIO
	h22_handle = drv_l2_sccb_open(SOI_H22_ID, 8, 8);
	if(h22_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_init();
	h22_handle.slaveAddr = SOI_H22_ID;
	h22_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void h22_sccb_close(void)
{
#if SCCB_MODE == SCCB_GPIO
	if(h22_handle) {
		drv_l2_sccb_close(h22_handle);
		h22_handle = NULL;
	}	
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_uninit();
	h22_handle.slaveAddr = 0;
	h22_handle.clkRate = 0;
#endif
}

static INT32S h22_sccb_write(INT8U reg, INT8U value)
{
#if SCCB_MODE == SCCB_GPIO
	return drv_l2_sccb_write(h22_handle, reg, value);

#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[2];
	
	data[0] = reg;
	data[1] = value;
	return drv_l1_i2c_bus_write(&h22_handle, data, 2);
#endif
}

#if 0
static INT32S h22_sccb_read(INT8U reg, INT8U *value)
{
#if SCCB_MODE == SCCB_GPIO
	INT16U data;
	
	if(drv_l2_sccb_read(h22_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[1];
	
	data[0] = reg;
	if(drv_l1_i2c_bus_write(&h22_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	if(drv_l1_i2c_bus_read(&h22_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	*value = data[0];
#endif
	return STATUS_OK;
}
#endif

static INT32S h22_sccb_write_table(regval8_t *pTable)
{
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}
		
		if(h22_sccb_write(pTable->reg_num, pTable->value) < 0) {
			DBG_PRINT("sccb write fail.\r\n");
			continue;
		}
		pTable++;
	}
	return STATUS_OK;
}

gpCdspWBGain_t *soi_h22_awb_r_b_gain_boundary(void)
{
	int i;
	int max_r_gain, max_b_gain, min_r_gain, min_b_gain;
	
	max_r_gain = max_b_gain = 0;
	min_r_gain = min_b_gain = 255;
	
	for(i = 10 ; i < 55 ; i++)
	{
		if(max_r_gain < g_H22_r_b_gain[i][0]) max_r_gain = g_H22_r_b_gain[i][0];
		if(max_b_gain < g_H22_r_b_gain[i][1]) max_b_gain = g_H22_r_b_gain[i][1];
		if(min_r_gain > g_H22_r_b_gain[i][0]) min_r_gain = g_H22_r_b_gain[i][0];
		if(min_b_gain > g_H22_r_b_gain[i][1]) min_b_gain = g_H22_r_b_gain[i][1];
	}
	
	soi_h22_wbgain.max_rgain = max_r_gain;
	soi_h22_wbgain.max_bgain = max_b_gain;
	soi_h22_wbgain.min_rgain = min_r_gain;
	soi_h22_wbgain.min_bgain = min_b_gain;

	return &soi_h22_wbgain;
}

static int H22_cvt_analog_gain(int analog_gain)
{
	int i;
	int coarse_gain, fine_gain;
	int *p_analog_gain_table = (int *)H22_analog_gain_table;

	for(i=0; i<65; i++)
	{
		if(analog_gain >= p_analog_gain_table[i] && analog_gain < p_analog_gain_table[i+1])
			break;
	} 

	if(i < 16) {
		coarse_gain = 0;
		fine_gain = i;
	}
	else if(i < (16 + 16)) {
		coarse_gain = 1;
		fine_gain = (i - 16); 
	}
	else if(i < (16 + 16 + 16)) {
		coarse_gain = 3;
		fine_gain = (i - 16 - 16);
	}
	else if(i < (16 + 16 + 16 + 16)) {
		coarse_gain = 7;
		fine_gain = (i - 16 - 16 -16);
	}
	else {
		coarse_gain = 15;
		fine_gain = 0;
	}

	return ((coarse_gain << 4) | fine_gain);
}

#if 0
static int H22_get_real_analog_gain(int analog_gain)
{
	int real_analog_gain;
	int coarse_gain, fine_gain;
	int *p_analog_gain_table = (int *)H22_analog_gain_table;
	
	coarse_gain = (analog_gain >> 4) & 0x7;
	fine_gain = (analog_gain & 0xf);

	if(coarse_gain == 0) {
		real_analog_gain = p_analog_gain_table[fine_gain];
	}
	else if(coarse_gain == 1) {
		real_analog_gain = p_analog_gain_table[16 + fine_gain];
	}
	else if(coarse_gain == 3) {
		real_analog_gain = p_analog_gain_table[16 + 16 + fine_gain];
	}
	else if(coarse_gain == 7) {
		real_analog_gain = p_analog_gain_table[16 + 16 + 16 + fine_gain];
	}
	else if(coarse_gain == 15) {
		real_analog_gain = p_analog_gain_table[16 + 16 + 16 + 16];
	}
	else {
		DBG_PRINT("H22 analog gain Err!\r\n");
	}
	
	return real_analog_gain;
}
#endif

static INT32S H22_set_exposure_time(sensor_exposure_t *si)
{
	INT8U cvt_gain;
		
	// AGC Gain / analog gain
	cvt_gain = H22_cvt_analog_gain(si->analog_gain);
	h22_sccb_write(0x00, cvt_gain);
		
	// set exposure time
	h22_sccb_write(0x01, (si->time & 0xFF));
	h22_sccb_write(0x02, (si->time >> 8) & 0xFF);
	//DBG_PRINT("wtime = 0x%x, cvt_gain = 0x%x\r\n", si->time, cvt_gain);
	
	return 0;
}

static INT32S H22_set_xfps_exposure_time(sensor_exposure_t *si)
{
	int idx;
	
	si->sensor_ev_idx += si->ae_ev_idx;
	if(si->sensor_ev_idx >= si->max_ev_idx) {
		si->sensor_ev_idx = si->max_ev_idx - 1;
	}
	
	if(si->sensor_ev_idx < 0) {
		si->sensor_ev_idx = 0;
	}
	
	idx = si->sensor_ev_idx * 3;
	si ->time = p_expTime_table[idx];
	si ->analog_gain = p_expTime_table[idx+1];
	si ->digital_gain = p_expTime_table[idx+2];

	return 0;
}

static void H22_get_exposure_time(sensor_exposure_t *se)
{
#if 0
	INT8U t1, t2;
	INT8U cvt_gain;
	
	// AGC Gain / analog gain	
	h22_sccb_read(0x00, &cvt_gain); 
	se->analog_gain = H22_get_real_analog_gain(cvt_gain);	

	// exposure time
	h22_sccb_read(0x01, &t1); 
	h22_sccb_read(0x02, &t2);
	se->time = (t1 & 0xFF) | ((t2 & 0xFF) << 8);
	//DBG_PRINT("rtime = 0x%x, cvt_gain = 0x%x\r\n", se->time, cvt_gain); 	
#endif
}

static void H22_set_exp_freq(int freq)
{
	if(freq == 50)
	{
		jxh22_seInfo.sensor_ev_idx = H22_50HZ_INIT_EV_IDX;
		jxh22_seInfo.ae_ev_idx = 0;
		jxh22_seInfo.daylight_ev_idx= H22_50HZ_DAY_EV_IDX;
		jxh22_seInfo.night_ev_idx= H22_50HZ_NIGHT_EV_IDX;			
		jxh22_seInfo.max_ev_idx = H22_50HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_h22_exp_time_gain_50Hz;
	}
	else if(freq == 60)
	{
		jxh22_seInfo.sensor_ev_idx = H22_60HZ_INIT_EV_IDX;
		jxh22_seInfo.ae_ev_idx = 0;
		jxh22_seInfo.daylight_ev_idx= H22_60HZ_DAY_EV_IDX;
		jxh22_seInfo.night_ev_idx= H22_60HZ_NIGHT_EV_IDX;
		jxh22_seInfo.max_ev_idx = H22_60HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_h22_exp_time_gain_60Hz;
	}
}

static void H22_cdsp_eof_isr(void)
{
	if(h22_read_en) {
		h22_read_en = 0;
		H22_set_exposure_time(&jxh22_seInfo);
	}
}

static INT32S H22_set_ctrl(gpCdspCtrl_t *ctrl)
{
	INT32S ret;

	switch(ctrl->id)
	{
	case V4L2_CID_BACKLIGHT_COMPENSATION:
		DBG_PRINT("NightMode = %d\n", ctrl->value);
		if(ctrl->value) {	// NIGH MODE ON
			
		} else {	// NIGH MODE OFF
			
		}
		break;
		
	case V4L2_CID_EXPOSURE:
		{
			sensor_exposure_t *si;
			
			si = (sensor_exposure_t *) ctrl->value;
			jxh22_seInfo.userISO = si->userISO;
			if(p_expTime_table != 0) {
				jxh22_seInfo.ae_ev_idx = si->ae_ev_idx;
				ret = H22_set_xfps_exposure_time(&jxh22_seInfo);
				h22_read_en = 1;
			} else {
				jxh22_seInfo.time = si->time;
				jxh22_seInfo.digital_gain = si->digital_gain >> 1;
				jxh22_seInfo.analog_gain = si->analog_gain;
				ret = H22_set_exposure_time(&jxh22_seInfo);
			}
		}
		break;
		
	case V4L2_CID_POWER_LINE_FREQUENCY:
		H22_set_exp_freq(ctrl->value);
		break;
		
	default:
		return STATUS_FAIL;			
	}

	return STATUS_OK;
}

static INT32S H22_get_ctrl(gpCdspCtrl_t *ctrl)
{
	switch(ctrl->id)
	{
	case V4L2_CID_EXPOSURE:
		H22_get_exposure_time(&jxh22_seInfo);
		ctrl->value = (int)&jxh22_seInfo;
		break;
		
	case V4L2_CID_BRIGHTNESS:
		//ctrl->value = 64 - (sensor_total_ev - ov9712_seInfo.max_ev_idx);
		break;
		
	default:
		return STATUS_FAIL;		
	}

	return STATUS_OK;
}

static const gpCdspSensor_t h22_ctrl =
{
	H22_set_ctrl,
	H22_get_ctrl
};

#ifdef MIPI_ISR_TEST
static void mipi_h22_handle(INT32U event)
{
	if(event & MIPI_LANE0_SOT_SYNC_ERR) {
		DBG_PRINT("LANE0_SOT_SYNC_ERR\r\n");
	}

	if(event & MIPI_HD_1BIT_ERR) {
		DBG_PRINT("HD_1BIT_ERR\r\n");
	}
	
	if(event & MIPI_HD_NBIT_ERR) {
		DBG_PRINT("HD_NBIT_ERR\r\n");
	}
	
	if(event & MIPI_DATA_CRC_ERR) {
		DBG_PRINT("DATA_CRC_ERR\r\n");
	}
	
	if(event & MIPI_LANE1_SOT_SYNC_ERR) {
		DBG_PRINT("LANE1_SOT_SYNC_ERR\r\n");
	}
	
	if(event & MIPI_CCIR_SOF) {
		DBG_PRINT("CCIR_SOF\r\n");
	}
}
#endif

static void H22_set_favorite(void)
{
	myFav.ob_val = (INT16U)g_H22_FavTable[0];
	myFav.ae_target = (INT16U)g_H22_FavTable[1];
	myFav.ae_target_night = (INT16U)g_H22_FavTable[2];
	myFav.awb_mode = (INT16U)g_H22_FavTable[3];
	myFav.color_mode = (INT16U)g_H22_FavTable[4];
	myFav.iso = (INT16U)g_H22_FavTable[5];
	myFav.ev = (INT16U)g_H22_FavTable[6];
	myFav.day_hue_u_offset = (INT8S)g_H22_FavTable[7];
	myFav.day_hue_v_offset = (INT8S)g_H22_FavTable[8];
	myFav.day_hue_y_offset = (INT8S)g_H22_FavTable[9];
	myFav.day_sat_y_scale = (INT8S)g_H22_FavTable[10];
	myFav.day_sat_u_scale = (INT8S)g_H22_FavTable[11];
	myFav.day_sat_v_scale = (INT8S)g_H22_FavTable[12];
	myFav.day_edge= (INT8U)g_H22_FavTable[13];

	myFav.night_wb_offset = (INT8S)g_H22_FavTable[14];
	myFav.night_hue_u_offset = (INT8S)g_H22_FavTable[15];
	myFav.night_hue_v_offset = (INT8S)g_H22_FavTable[16];
	myFav.night_hue_y_offset = (INT8S)g_H22_FavTable[17];
	myFav.night_sat_y_scale = (INT8S)g_H22_FavTable[18];
	myFav.night_sat_u_scale = (INT8S)g_H22_FavTable[19];
	myFav.night_sat_v_scale = (INT8S)g_H22_FavTable[20];
	myFav.night_edge= (INT8U)g_H22_FavTable[21];
	myFav.night_wb_offset = (INT8S)g_H22_FavTable[22];
}

/**
 * @brief   initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void h22_cdsp_mipi_init(void)
{
	// ae/awb init
	jxh22_seInfo.max_time = H22_MAX_EXPOSURE_TIME;
	jxh22_seInfo.min_time = H22_MIN_EXPOSURE_TIME;

	jxh22_seInfo.max_digital_gain = H22_MAX_DIGITAL_GAIN ;
	jxh22_seInfo.min_digital_gain = H22_MIN_DIGITAL_GAIN ;

	jxh22_seInfo.max_analog_gain = H22_MAX_ANALOG_GAIN;
	jxh22_seInfo.min_analog_gain = H22_MIN_ANALOG_GAIN;

	jxh22_seInfo.analog_gain = jxh22_seInfo.min_analog_gain;
	jxh22_seInfo.digital_gain = jxh22_seInfo.min_digital_gain;
	jxh22_seInfo.time = jxh22_seInfo.max_time;
	jxh22_seInfo.userISO = ISO_AUTO;
	
	H22_set_exp_freq(60);
	
	H22_set_favorite();

	// Turn on LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(1, LDO_LDO28_2P8V);
	
	// set Mclk(CSI_CLKO) to IOD12, for MIPI Mclk
	R_FUNPOS1 |= 0x42;		
	
	// mclk output
	drv_l2_sensor_set_mclkout(soi_h22_cdsp_mipi_ops.info[0].mclk);
		
	// request sccb
	h22_sccb_open();

	// cdsp init
	drv_l2_cdsp_open();
	drv_l2_CdspTableRegister((sensor_calibration_t *)&h22_cdsp_calibration);
	drv_l2_CdspSensorCtrlRegister((gpCdspSensor_t *)&h22_ctrl);
		
	// mipi enable
	drv_l1_mipi_init(ENABLE);
	DBG_PRINT("Sensor soi h22 cdsp mipi init completed\r\n");
}	

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void h22_cdsp_mipi_uninit(void)
{
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE);

	// cdsp disable
	drv_l2_cdsp_close();

	// mipi disable
	drv_l1_mipi_init(DISABLE);

	// release sccb
	h22_sccb_close();
	
	/* Turn off LDO 2.8V for CSI sensor */
	drv_l1_power_ldo_28_ctrl(0, LDO_LDO28_2P8V);
}	

/**
 * @brief   stream start function
 * @param   info index
 *			
 * @return 	none
 */
static void h22_cdsp_mipi_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U target_w, target_h;
	gpCdspFmt_t format;

	// set sensor size
	DBG_PRINT("h22 index = %d\r\n", index);
	switch(index)
	{
	case 0:
		h22_sccb_write_table((regval8_t *)SOI_H22_MIPI_VGA_f60);
		break;
		
	case 1:
		h22_sccb_write_table((regval8_t *)SOI_H22_MIPI_720P);
		break;
		
	case 2:
		h22_sccb_write_table((regval8_t *)H22_MIPI_1280_800_30);
		break;	
		
	default:
		return;	
	}
	
	// mclk output
	drv_l2_sensor_set_mclkout(soi_h22_cdsp_mipi_ops.info[index].mclk);
			
	// set cdsp format
	format.image_source = C_CDSP_MIPI;
	format.input_format = soi_h22_cdsp_mipi_ops.info[index].input_format;
	format.output_format = soi_h22_cdsp_mipi_ops.info[index].output_format;
	target_w = soi_h22_cdsp_mipi_ops.info[index].target_w;
	target_h = soi_h22_cdsp_mipi_ops.info[index].target_h;
	format.hpixel = soi_h22_cdsp_mipi_ops.info[index].sensor_w;
	format.vline = soi_h22_cdsp_mipi_ops.info[index].sensor_h;
	format.hoffset = soi_h22_cdsp_mipi_ops.info[index].hoffset;
	format.voffset = soi_h22_cdsp_mipi_ops.info[index].voffset;
	format.sensor_timing_mode = soi_h22_cdsp_mipi_ops.info[index].interface_mode;
	format.sensor_hsync_mode = soi_h22_cdsp_mipi_ops.info[index].hsync_active;
	format.sensor_vsync_mode = soi_h22_cdsp_mipi_ops.info[index].vsync_active;
	if(drv_l2_cdsp_set_fmt(&format) < 0) {
		DBG_PRINT("cdsp set fmt err!!!\r\n");
	}
	
	// set scale down
	if((format.hpixel > target_w) || (format.vline > target_h)) {
		drv_l2_cdsp_set_yuv_scale(target_w, target_h);
	}
	
	// cdsp start
#if 0
	drv_l2_cdsp_stream_on(DISABLE, bufA, bufB);	
#else	
	drv_l2_cdsp_stream_on(ENABLE, bufA, bufB);
	
	// set ae/awb
	
	drv_l2_cdsp_enable(target_w, target_h, myFav);	
	
	drv_l2_CdspIsrRegister(C_ISR_SENSOR, H22_cdsp_eof_isr);
#endif

	R_CDSP_MIPI_CTRL &= ~0x7;

	// mipi start
//	h22_mipi_cfg.h_size = format.hpixel;
//	h22_mipi_cfg.v_size = format.vline;
	drv_l1_mipi_set_parameter(&h22_mipi_cfg);
	
#ifdef MIPI_ISR_TEST	
	drv_l1_mipi_isr_register(mipi_h22_handle);
	drv_l1_mipi_set_irq_enable(ENABLE, MIPI_INT_ALL);
#endif
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */ 
static void h22_cdsp_mipi_stream_off(void)
{
	drv_l2_cdsp_stream_off();
}	

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */ 
static drv_l2_sensor_info_t* h22_cdsp_mipi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1)) {
		return NULL;
	} else {
		return (drv_l2_sensor_info_t*)&soi_h22_cdsp_mipi_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t soi_h22_cdsp_mipi_ops =
{
	SENSOR_SOI_H22_CDSP_MIPI_NAME,		/* sensor name */
	h22_cdsp_mipi_init,
	h22_cdsp_mipi_uninit,
	h22_cdsp_mipi_stream_on,
	h22_cdsp_mipi_stream_off,
	h22_cdsp_mipi_get_info,
	{
		/* 1st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SGRBG10,		/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			30,							/* FPS in sensor */
			640,						/* target width */
			480, 						/* target height */	
			640,						/* sensor width */
			480, 						/* sensor height */	
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 2st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YVYU,			/* output format */
			30,							/* FPS in sensor */
			1280,						/* target width */
			720, 						/* target height */	
			1280,						/* sensor width */
			720, 						/* sensor height */
			0,							/* sensor h offset */ 
			0,                          /* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 3st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SGRBG10,		/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			30,							/* FPS in sensor */
			1280,						/* target width */
			800, 						/* target height */	
			1280,						/* sensor width */
			800,		 				/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
	}
};
#endif //(defined _SENSOR_SOI_H22_CDSP_MIPI) && (_SENSOR_SOI_H22_CDSP_MIPI == 1)
