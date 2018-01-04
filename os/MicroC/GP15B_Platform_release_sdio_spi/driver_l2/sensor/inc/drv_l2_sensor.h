/******************************************************
* drv_l2_sensor.h
*
* Purpose: Sensor layer 2 header file
*
* Author: Eugene Hsu
*
* Date: 2014/08/25
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
*******************************************************/
#ifndef DRV_L2_SENSOR_H
#define DRV_L2_SENSOR_H
#include "project.h"


/*********************************************************************
        Structure, enumeration and definition
**********************************************************************/
#define CAPTURE_RAW_MODE 1
#define N_STEP 3 // 3: 3 steps, 4: 5steps, 5: 5steps
#define DUMMY_BUFFER_ADDRS2	0xF8500000

// sensor interface 
#define CSI_INTERFACE	0
#define CDSP_INTERFACE	1

// sccb interface
#define SCCB_GPIO		0
#define SCCB_HW_I2C		1
#define SCCB_MODE		SCCB_GPIO//SCCB_HW_I2C 

#define MAX_SENSOR_NUM	1	/* Support 1 sensor in platform */
#define MAX_INFO_NUM	4

/* Define the sensor name we support now */
#define SENSOR_OV7670_CSI_NAME			"ov_7670_csi"
#define SENSOR_GC0308_CSI_NAME			"gc_0308_csi"
#define SENSOR_OV3640_CSI_NAME			"ov_3640_csi"
#define SENSOR_OV7670_CDSP_NAME			"ov_7670_cdsp"
#define SENSOR_OV9712_CDSP_NAME			"ov_9712_cdsp"
#define SENSOR_OV3640_CSI_MIPI_NAME		"ov_3640_csi_mipi"
#define SENSOR_OV3640_CDSP_MIPI_NAME	"ov_3640_cdsp_mipi"
#define SENSOR_SOI_H22_CDSP_MIPI_NAME	"soi_h22_cdsp_mipi"
#define TVIN_TVP5150_CSI_NAME			"tvp_5150_csi"
#define SENSOR_GC1004_CDSP_MIPI_NAME	"gc_1004_cdsp_mipi"
#define SENSOR_GC1004_CDSP_NAME			"gc_1004_cdsp"
#define SENSOR_OV8865_CDSP_MIPI_NAME			"ov_8865_cdsp_mipi"

// sensor device enable
#define _SENSOR_OV7670_CSI				0
#define _SENSOR_GC0308_CSI				0
#define _SENSOR_OV3640_CSI				0
#define _SENSOR_OV7670_CDSP				0
#define _SENSOR_OV9712_CDSP				0
#define _SENSOR_OV3640_CSI_MIPI			0
#define _SENSOR_OV3640_CDSP_MIPI		0
#define _SENSOR_SOI_H22_CDSP_MIPI		0
#define _TVIN_TVP5150_CSI				0
#define _SENSOR_GC1004_CDSP_MIPI		0
#define _SENSOR_GC1004_CDSP				0
#define _SENSOR_OV8865_CDSP_MIPI		0

#if _SENSOR_OV8865_CDSP_MIPI == 1
#define MIPI_ND_HOFFSET 8
#else
#define MIPI_ND_HOFFSET 0
#endif

typedef enum
{
	MCLK_NONE = 0,
	MCLK_13_5M,
	MCLK_24M,
	MCLK_27M,
	MCLK_48M,
	MCLK_SYSTEM_DIV2,
	MCLK_SYSTEM_DIV4
} CSI_CLK_DEF;

typedef enum
{
	WIDTH_320 = 320,
	WIDTH_640 = 640,
	WIDTH_800 = 800,
	WIDTH_1024 = 1024,
	WIDTH_1280 = 1280,
	WIDTH_1920 = 1920
} WIDTH_MODE;

typedef enum
{
	HEIGHT_240 = 240,
	HEIGHT_480 = 480,
	HEIGHT_600 = 600,
	HEIGHT_768 = 768,
	HEIGHT_720 = 720,
	HEIGHT_1080 = 1080
} HEIGHT_MODE;

typedef enum
{
	MODE_CCIR_601 = 0,
	MODE_CCIR_656,
	MODE_CCIR_HREF
} TIMING_MODE;

typedef enum
{
	MODE_NONE_INTERLACE = 0,
	MODE_INTERLACE
} INTERLACE_MODE;

typedef enum
{
	MODE_POSITIVE_EDGE = 0,
	MODE_NEGATIVE_EDGE
} PCLK_MODE;

typedef enum
{
	MODE_ACTIVE_LOW = 0,
	MODE_ACTIVE_HIGH
} HVSYNC_MODE;

// sensor format is LSB to MSB
typedef enum
{
	V4L2_PIX_FMT_RGB565 = 0,
	V4L2_PIX_FMT_YUYV,
	V4L2_PIX_FMT_YVYU,
	V4L2_PIX_FMT_UYVY,
	V4L2_PIX_FMT_VYUY,
	V4L2_PIX_FMT_SBGGR8,
	V4L2_PIX_FMT_SGBRG8,
	V4L2_PIX_FMT_SGRBG8,
	V4L2_PIX_FMT_SRGGB8,
	V4L2_PIX_FMT_SGRBG10,
	V4L2_PIX_FMT_SRGGB10,	
	V4L2_PIX_FMT_SBGGR10,
	V4L2_PIX_FMT_SGBRG10,
	V4L2_PIX_FMT_MAX
} SENSOR_FORMAT;

typedef enum
{
	V4L2_CID_BRIGHTNESS = 0,
	V4L2_CID_CONTRAST,
	V4L2_CID_SATURATION,
	V4L2_CID_HUE,
	V4L2_CID_BLACK_LEVEL,
	V4L2_CID_AUTO_WHITE_BALANCE,
	V4L2_CID_DO_WHITE_BALANCE,
	V4L2_CID_RED_BALANCE,
	V4L2_CID_BLUE_BALANCE,
	V4L2_CID_GAMMA,
	V4L2_CID_WHITENESS,
	V4L2_CID_EXPOSURE,
	V4L2_CID_AUTOGAIN,
	V4L2_CID_GAIN,
	V4L2_CID_HFLIP,
	V4L2_CID_VFLIP,
	V4L2_CID_HCENTER,
	V4L2_CID_VCENTER,
	V4L2_CID_POWER_LINE_FREQUENCY,
	V4L2_CID_HUE_AUTO,
	V4L2_CID_WHITE_BALANCE_TEMPERATURE,
	V4L2_CID_SHARPNESS,
	V4L2_CID_BACKLIGHT_COMPENSATION,
	V4L2_CID_CHROMA_AGC,
	V4L2_CID_COLOR_KILLER,
	V4L2_CID_COLORFX,
	V4L2_CID_AUTOBRIGHTNESS,
	V4L2_CID_BAND_STOP_FILTER,
	V4L2_CID_LASTP1
} SENSOR_CTRL;

typedef struct sensor_calibration_s
{
	const unsigned short *ob;
	const unsigned char *lincorr;
	const unsigned short *lenscmp1;
	const unsigned short *lenscmp2;
	const unsigned short *lenscmp3;
	const unsigned short *lenscmp4;
	const unsigned short *lenscmp5;
	const unsigned short *lenscmp6;
	const unsigned short (*wb_gain)[2];
	const unsigned int *gamma1;
	const short *color_matrix1;
	const unsigned int *gamma2;
	const short *color_matrix2;

	const short *awb_thr;
} sensor_calibration_t;

typedef struct drv_l2_sensor_info_s
{
	INT32U mclk;				/* master clock */
	INT32U input_format;		/* input format */
	INT32U output_format;		/* output format */
	INT32U sensor_fps;			/* FPS in sensor */
	INT16U target_w; 			/* sensor width */
	INT16U target_h;			/* sensor height */
	INT16U sensor_w; 			/* sensor width */
	INT16U sensor_h;			/* sensor height */
	INT16U hoffset; 			/* sensor h offset */
	INT16U voffset;				/* sensor v offset */
	INT8U interface_mode;		/* input interface, HREF CCIR601 and CCIR656 */
	INT8U interlace_mode;		/* interlace mode */
	INT8U hsync_active;			/* hsync pin active level */
	INT8U vsync_active;			/* vsync pin active level */
} drv_l2_sensor_info_t;

typedef struct drv_l2_sensor_ops_s 
{
	char*	name;
	void   (*init)(void);
	void   (*uninit)(void);
	void   (*stream_start)(INT32U index, INT32U bufA, INT32U bufB);
	void   (*stream_stop)(void);
	drv_l2_sensor_info_t*   (*get_info)(INT32U index);
	drv_l2_sensor_info_t	info[MAX_INFO_NUM];
} drv_l2_sensor_ops_t;

/*********************************************************************
        External ops declaration for sensors we spport now
**********************************************************************/
#if (defined _SENSOR_OV7670_CSI) && (_SENSOR_OV7670_CSI == 1)
extern const drv_l2_sensor_ops_t ov7670_sensor_csi_ops;
#endif
#if (defined _SENSOR_GC0308_CSI) && (_SENSOR_GC0308_CSI == 1)
extern const drv_l2_sensor_ops_t gc0308_sensor_csi_ops;
#endif
#if (defined _SENSOR_OV3640_CSI) && (_SENSOR_OV3640_CSI == 1)
extern const drv_l2_sensor_ops_t ov3640_sensor_csi_ops;
#endif
#if (defined _SENSOR_OV7670_CDSP) && (_SENSOR_OV7670_CDSP == 1)
extern const drv_l2_sensor_ops_t ov7670_cdsp_ops;
#endif
#if (defined _SENSOR_OV9712_CDSP) && (_SENSOR_OV9712_CDSP == 1)
extern const drv_l2_sensor_ops_t ov9712_cdsp_ops;
#endif
#if (defined _SENSOR_OV3640_CSI_MIPI) && (_SENSOR_OV3640_CSI_MIPI == 1)
extern const drv_l2_sensor_ops_t ov3640_csi_mipi_ops;
#endif
#if (defined _SENSOR_OV3640_CDSP_MIPI) && (_SENSOR_OV3640_CDSP_MIPI == 1)
extern const drv_l2_sensor_ops_t ov3640_cdsp_mipi_ops;
#endif
#if (defined _SENSOR_SOI_H22_CDSP_MIPI) && (_SENSOR_SOI_H22_CDSP_MIPI == 1)
extern const drv_l2_sensor_ops_t soi_h22_cdsp_mipi_ops;
#endif
#if (defined _TVIN_TVP5150_CSI) && (_TVIN_TVP5150_CSI == 1)
extern const drv_l2_sensor_ops_t tvp5150_csi_ops;
#endif
#if (defined _SENSOR_GC1004_CDSP_MIPI) && (_SENSOR_GC1004_CDSP_MIPI == 1)
extern const drv_l2_sensor_ops_t gc1004_cdsp_mipi_ops;
#endif
#if (defined _SENSOR_GC1004_CDSP) && (_SENSOR_GC1004_CDSP == 1)
extern const drv_l2_sensor_ops_t gc1004_cdsp_ops;
#endif	
#if (defined _SENSOR_OV8865_CDSP_MIPI) && (_SENSOR_OV8865_CDSP_MIPI == 1)
extern const drv_l2_sensor_ops_t ov8865_cdsp_mipi_ops;
#endif	

/*********************************************************************
        External function declaration 
**********************************************************************/
extern void drv_l2_sensor_register_ops(drv_l2_sensor_ops_t* sensor);
extern void drv_l2_sensor_init(void);
extern drv_l2_sensor_ops_t* drv_l2_sensor_get_ops(INT32U index);

extern void drv_l2_sensor_set_path(INT32U sen_path);
extern void drv_l2_sensor_set_csi_source(INT32U from_mipi_en);
extern INT32S drv_l2_sensor_set_mclkout(INT32U mclk_sel);
#endif  //DRV_L2_SENSOR_H
