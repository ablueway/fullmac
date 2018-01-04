#ifndef __drv_l2_CDSP_H__
#define __drv_l2_CDSP_H__

#include "driver_l2.h"
#include "drv_l2_sensor.h"
#include "gp_aeawb.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* cdsp input source */
#define C_CDSP_FRONT		0x0
#define C_CDSP_SDRAM		0x1
#define C_CDSP_MIPI			0x2
#define C_CDSP_SKIP_WR		0x3
#define C_CDSP_MAX			0xFF

/* cdsp interrupt isr index */
#define C_ISR_EOF			0	//eof switch buffer
#define C_ISR_SENSOR		1	//sensor set ae/awb
#define C_ISR_OVERFLOW		2
#define C_ISR_MAX			3

/* dma mode */
#define C_DMA_CH			1	//0: auto switch, 1:dma a, 2: dma b

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MODE_NORMAL = 0,
	MODE_NEGATIVE,
	MODE_SOLARISE,
	MODE_EMBOSSMENT,
	MODE_BINARIZE,
	MODE_SEPIA,
	MODE_BLACKWHITE
} CDSP_RAW_SPECIAL_MODE;

typedef enum
{	
	MSG_CDSP_POSTPROCESS = 0x30000000,
	MSG_CDSP_SCALE_CROP,
	
	MSG_CDSP_BADPIX_OB,
	MSG_CDSP_LENS_CMP,
	MSG_CDSP_WBGAIN,
	MSG_CDSP_WB_OFFSET_DAY,
	MSG_CDSP_WB_OFFSET_NIGHT,
	MSG_CDSP_LUT_GAMMA,
	MSG_CDSP_INTERPOLATION,
	MSG_CDSP_RAW_SPEF,
	MSG_CDSP_EDGE,
	MSG_CDSP_EDGE_AMPGA_DAY,
	MSG_CDSP_EDGE_AMPGA_NIGHT,
	MSG_CDSP_COLOR_MATRIX,
	MSG_CDSP_POSWB_RGB2YUV,

	MSG_CDSP_YUV_INSERT,
	MSG_CDSP_YUV_HAVG,
	MSG_CDSP_SPEC_MODE,
	MSG_CDSP_SAT_HUE,
	MSG_CDSP_SAT_HUE_DAY,
	MSG_CDSP_SAT_HUE_NIGHT,
	MSG_CDSP_SUPPRESSION,
	MSG_CDSP_NEWDENOISE,

	MSG_CDSP_RAW_WIN,
	MSG_CDSP_AE_WIN,
	MSG_CDSP_AF_WIN,
	MSG_CDSP_AWB_WIN,
	MSG_CDSP_WBGAIN2,
	MSG_CDSP_HISTGM,

	
	MSG_CDSP_TARGET_AE,
	MSG_CDSP_TARGET_AE_NIGHT,
	MSG_CDSP_WB_MODE,
	MSG_CDSP_EV,
	MSG_CDSP_ISO,
	MSG_CDSP_BACKLIGHT_DETECT,
	
	MSG_CDSP_3A_STATISTIC,
	MSG_CDSP_LINCORR,
	MSG_CDSP_MAX
} CDSP_MSG_CTRL_ID;

typedef struct gpCdspCtrl_s 
{
	INT32U id;
	INT32U value;
} gpCdspCtrl_t;

typedef struct gpCdspSensor_s 
{
	INT32S (*sensor_set_ctrl)(gpCdspCtrl_t *ctrl);
	INT32S (*sensor_get_ctrl)(gpCdspCtrl_t *ctrl);
} gpCdspSensor_t;

typedef struct gpCdspFmt_s 
{
	INT32U image_source;
	INT32U input_format;
	INT32U output_format;
	INT16U hpixel;
	INT16U vline;
	INT16U hoffset;
	INT16U voffset;
	INT32U sensor_timing_mode;
	INT32U sensor_hsync_mode;
	INT32U sensor_vsync_mode;
} gpCdspFmt_t;

typedef struct gpCdspPostProcess_s 
{
	INT32U SetFlag;
	INT32U inFmt;
	INT32U yuvRange;
	INT32U width;
	INT32U height;
	INT32U inAddr;
	INT32U outAddr;
} gpCdspPostProcess_t;

typedef struct gpCdspScalePara_s 
{
	/* raw hscale */
	INT8U 	hscale_en;		/* 0:disable, 1:enable */ 		
	INT8U 	hscale_mode;	/* 0:drop, 1:filter */ 
	INT16U	dst_hsize;		/* 0~0xFFF */

	/* crop function */ 
	INT32U	crop_en;		/* 0:disable, 1:enable */ 
	INT16U	crop_hoffset;	/* 1~0xFFF */
	INT16U	crop_voffset;	/* 1~0xFFF */
	INT16U	crop_hsize;		/* 1~0xFFF */	
	INT16U	crop_vsize;		/* 1~0xFFF */
	
	/* yuv h/v scale down */
	INT8U	yuvhscale_en;	/* 0:disable, 1:enable */ 
	INT8U	yuvvscale_en;	/* 0:disable, 1:enable */ 
	INT8U	yuvhscale_mode; /* 0:drop, 1:filter */ 
	INT8U	yuvvscale_mode; /* 0:drop, 1:filter */ 
	INT16U	yuv_dst_hsize;	/* 0~0xFFF */
	INT16U	yuv_dst_vsize;	/* 0~0xFFF */

	/* read back size */
	INT16U	img_rb_h_size;		/* 0~0xFFF */
	INT16U	img_rb_v_size;		/* 0~0xFFF */
} gpCdspScalePara_t;

typedef struct gpCdspBadPixOB_s
{
	/* bad pixel */
	INT8U   badpixen; 		/* 0:disable, 1:enable */
	INT8U   badpixmirr;        /* badpixmirlen[0]: Enable badpix mirror 2 pixels, badpixmirren[1]: Enable badpix right mirror 2 pixels*/
	INT16U	bprthr;			/* 0~1023 */
	INT16U	bpgthr;			/* 0~1023 */
	INT16U	bpbthr;			/* 0~1023 */
	
	/* optical black */
	INT8U	manuoben;		/* 0:disable, 1:enable */
	INT8U   reserved1;
	INT16S	manuob;			/* -1024~1023 */
	
	INT8U   reserved2;
	INT8U   reserved3;
	INT8U	autooben;		/* 0:disable, 1:enable */
	INT8U	obtype;			/* 0:2x256,1:4x256,2:8x256,3:16x256,4:256x2,5:256x4,6:256x8,7:256x16 */
	
	INT16U	obHOffset;		/* 0~0xFFF */
	INT16U	obVOffset;		/* 0~0xFFF */
	INT16U	Ravg;			/* read back auto ob r avg */ 
	INT16U	GRavg;			/* read back auto ob gr avg */
	INT16U	Bavg;			/* read back auto ob b avg */
	INT16U	GBavg;			/* read back auto ob gb avg */
	
	/* white balance */
	INT8U	wboffseten;		/* 0:disable, 1:enable */
	INT8S	roffset;		/* -128 ~ 127 */
	INT8S	groffset;		/* -128 ~ 127 */
	INT8S	boffset;		/* -128 ~ 127 */
	INT8S	gboffset;		/* -128 ~ 127 */
} gpCdspBadPixOB_t;

typedef struct gpCdspLensCmp_s
{
	/* lens compensation */
	INT32U	lcen;				/* 0:disable, 1:enable 			*/
	INT16U	*maxtan8_table;		/* maxtan8 table				*/
	INT16U	*slop4_table;		/* slop4 table				*/
	INT16U	*clpoint_table;		/* clpoint table				*/
	INT16U  *rdiusfile0_table;	/* radiusfile0_table, size 512 byte */
	INT16U	*rdiusfile1_table;	/* radiusfile1_table, size 512 byte */
	
	INT16U	stepfactor;		/* 0:2^6,1:2^7,2:2^8,3:2^9,4:2^10,5:2^11,6:2^12,7:2^13 */
//	INT8U	xminc;			/* 0~0xF */
//	INT8U  	ymoinc;			/* 0~0xF */
//	INT8U  	ymeinc;			/* 0~0xF */

//	INT16U	centx;			/* 0~0x1FFF */	
//	INT16U	centy;			/* 0~0x1FFF */
//	INT16U	xmoffset;		/* 0~0xFFF */
//	INT16U	ymoffset;		/* 0~0xFFF */
	INT16U	seg_R;			/* 0~0xFFF */
} gpCdspLensCmp_t;

typedef struct gpCdspWhtBal_s
{
	/* white balance */
	INT8U	wboffseten;		/* 0:disable, 1:enable */
	INT8S	roffset;		/* -128 ~ 127 */
	INT8S	groffset;		/* -128 ~ 127 */
	INT8S	boffset;		/* -128 ~ 127 */
	INT8S	gboffset;		/* -128 ~ 127 */
	
	INT8U	wbgainen;		/* 0:disable, 1:enable */
	INT8U	global_gain;	/* 0~0xFF */
	INT8U 	reserved;
	
	INT16U	rgain;			/* 3.6 bit, 0/64~511/64 */
	INT16U	grgain;			/* 3.6 bit, 0/64~511/64 */
	INT16U	bgain;			/* 3.6 bit, 0/64~511/64 */
	INT16U	gbgain;			/* 3.6 bit, 0/64~511/64 */
} gpCdspWhtBal_t;

typedef struct gpCdspIntpl_s
{
	/* interpolation */
	INT8U	int_low_thr;	/* 0~0xFF */	
	INT8U	int_hi_thr;		/* 0~0xFF */
	/* raw special mode */
	INT8U	rawspecmode;	/* 0~6 */
	INT8U	reserved;
} gpCdspIntpl_t;

typedef struct gpCdspEdge_s
{
	/* edge lut table */
	INT8U	eluten;			/* 0:disable, 1:enable */
	INT8U 	*edge_table;	/* edge table, 256byte */

	/* edge enhance */
	INT8U	edgeen;			/* 0:disable, 1:enable */
	INT8U	lhdiv;			/* 1/lhdiv,  1, 2, 4, 8, 16, 32, 64, 128 */
	INT8U	lhtdiv;			/* 1/lhtdiv,  1, 2, 4, 8, 16, 32, 64, 128 */
	
	INT8U	lhcoring;		/* 0~0xFF */
	INT8U	ampga;			/* 1, 2, 3, 4 */
	INT8U	edgedomain;		/* 0:add y value, 1:add RGB value */
	INT8U	lhmode;			/* 0:USE HPF LF00 matrix, 1:default matrix */ 

	INT8U	Qthr;			/* edge thr set */
	/* edge filter */
	INT8U	lf00;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf01;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf02;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */

	INT8U	lf10;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf11;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf12;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf20;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */

	INT8U	lf21;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	lf22;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	reserved0;
	INT8U	reserved1;

	INT32U 	Qcnt;			/* read back edge thr count */
} gpCdspEdge_t;

typedef struct gpCdspCorMatrix_s
{
	/* color matrix */
	INT8U	colcorren;		/* 0:disable, 1:enable */
	INT8U	reserved0;
	
	INT16S	a11;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a12;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a13;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a21;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a22;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a23;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a31;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a32;			/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16S	a33;			/* 1 sign + 3.6 bit, -512/64~511/64 */
} gpCdspCorMatrix_t;

typedef struct gpCdspRgb2Yuv_s
{
	/* pre rb clamp */
	INT8U  	pre_rbclamp;
	/* rb clamp */
	INT8U  	rbclampen;
	INT16U 	rbclamp;
	
	/* Uv divider */
	INT8U	uvdiven;		/* 0:disable, 1:enable */
	INT8U 	reserved;
	INT8U	Yvalue_T1;		/* 0~0xFF, when Y=T1, UV divide 1/8 */
	INT8U	Yvalue_T2;		/* 0~0xFF, when Y=T2, UV divide 2/8 */
	INT8U	Yvalue_T3;		/* 0~0xFF, when Y=T3, UV divide 3/8 */
	INT8U	Yvalue_T4;		/* 0~0xFF, when Y=T4, UV divide 4/8 */
	INT8U	Yvalue_T5;		/* 0~0xFF, when Y=T5, UV divide 5/8 */
	INT8U	Yvalue_T6;		/* 0~0xFF, when Y=T6, UV divide 6/8 */
} gpCdspRgb2Yuv_t;

typedef struct gpCdspGamma_s
{
	INT32U	lut_gamma_en;	/* 0:disable, 1:enable */
	INT32U 	*gamma_table;	/* gamma table, 512 byte */ 
} gpCdspGamma_t;

typedef struct gpCdspSpecMod_s
{
	/* yuv special mode */
	INT8U	yuvspecmode;	/* 0~7 */
	INT8U	binarthr;		/* 0~0xFF */
	INT8U	reserved0;
	INT8U	reserved1;
} gpCdspSpecMod_t;

typedef struct gpCdspYuvInsert_s
{
	/* yuv 422 to 444 */
	INT8U	yuv444_insert;	/* 0:disable, 1:enable */
	/* yuv coring threshold */
	INT8U	y_coring;		/* 0~0xF */
	INT8U	u_coring;		/* 0~0xF */
	INT8U	v_coring;		/* 0~0xF */
	/* yuv coring subtraction */
	INT8U	y_corval;		/* 0~0xF */
	INT8U	u_corval;		/* 0~0xF */
	INT8U	v_corval;		/* 0~0xF */
	INT8U	reserved;
} gpCdspYuvInsert_t;

typedef struct gpCdspYuvHAvg_s
{
	/* h average */
	INT8U	ytype;			/* 0:disable, 1:3tap, 2:5tap */
	INT8U	utype;			/* 0:disable, 1:3tap, 2:5tap */
	INT8U	vtype;			/* 0:disable, 1:3tap, 2:5tap */
	INT8U   reserved;
} gpCdspYuvHAvg_t;

typedef struct gpCdspSatHue_s
{
	INT8U	YbYcEn;			/* 0:disable, 1:enable */
	/* brightness & constract */
	INT8S	y_offset;		/* -128 ~ 127 */
	INT8S	u_offset;		/* -128 ~ 127 */
	INT8S	v_offset;		/* -128 ~ 127 */
	
	INT8U	y_scale;		/* 3.5 bit, 0/32~255/32 */	
	INT8U	u_scale;		/* 3.5 bit, 0/32~255/32 */
	INT8U	v_scale;		/* 3.5 bit, 0/32~255/32 */
	INT8U	reserved;
	
	/* saturation, hue */
	INT8S	u_huesindata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	INT8S	u_huecosdata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	INT8S	v_huesindata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
	INT8S	v_huecosdata;	/* 1 sign + 1.6 bit , -128/64~127/64 */
} gpCdspSatHue_t;

typedef struct gpCdspSuppression_s
{
	/* suppression */
	INT8U	suppressen;		/* 0:disable, 1:enable */
	INT8U   suppr_mode;		/* 0:edge, 1:denoise, 2:YLPF */
	
	/* y denoise */
	INT8U 	denoisen;		/* 0:disable, 1:enable */
	INT8U	denoisethrl;	/* 0~0xFF */
	INT8U	denoisethrwth;	/* 1, 2, 4, 8, 16, 32, 64, 128 */
	INT8U	yhtdiv;			/* 1, 2, 4, 8, 16, 32, 64, 128 */

	/* y lpf */
	INT8U	lowyen;			/* 0:disable, 1:enable */
	INT8U   reserved;
} gpCdspSuppression_t;

typedef struct gpCdspRawWin_s
{
	INT8U	aeawb_src;		/* 0:from poswb, 1:form awb line ctrl */
	INT8U	subsample; 		/* 0:disable, 1:1/2, 2:1/4 subsample */
	INT8U 	AeWinTest;		/* 0:disable, 1:enable */
	INT8U 	AfWinTest;		/* 0:disable, 1:enable */
	/* rgb window set */
	INT16U	hwdoffset;		/* 0~0x3FFF */
	INT16U	vwdoffset;		/* 0~0x3FFF */
	INT16U	hwdsize;		/* 0~0x1FF, h cell size */
	INT16U	vwdsize;		/* 0~0x1FF, v cell size */
} gpCdspRawWin_t;

typedef struct gpCdspAF_s
{
	/* auto focuse */
	INT8U	af_win_en;		/* 0:disable, 1:enable */
	INT8U	af_win_hold;	/* 0:disable, 1:enable */
	INT8U	reserved00;
	INT8U	reserved01;
	/* af window 1 */
	INT16U	af1_hoffset;	/* 0~0xFFF */
	INT16U 	af1_voffset;	/* 0~0xFFF */
	INT16U 	af1_hsize;		/* 0~0xFFF */
	INT16U	af1_vsize;		/* 0~0xFFF */
	/* af window 2 */
	INT16U	af2_hoffset;	/* 0~0xFFF */
	INT16U 	af2_voffset;	/* 0~0xFFF */
	INT16U 	af2_hsize;		/* 64, 256, 512, 1024, 2048 */
	INT16U	af2_vsize;		/* 64, 256, 512, 1024, 2048 */
	/* af window 3 */
	INT16U	af3_hoffset;	/* 0~0xFFF */
	INT16U 	af3_voffset;	/* 0~0xFFF */
	INT16U 	af3_hsize;		/* 64, 256, 512, 1024, 2048 */
	INT16U	af3_vsize;		/* 64, 256, 512, 1024, 2048 */
} gpCdspAF_t;

typedef struct gpCdspAE_s
{
	/* auto expore */
	INT8U	ae_win_en;		/* 0:disable, 1:enable */
	INT8U	ae_win_hold;	/* 0:disable, 1:enable */
	INT8U	phaccfactor;		/* 4, 8, 16, 32, 64, 128 */
	INT8U	pvaccfactor;		/* 4, 8, 16, 32, 64, 128 */
	INT32S  ae_meter;
} gpCdspAE_t;

typedef struct gpCdspAWB_s
{
	/* auto white balance */
	INT8U	awb_win_en;		/* 0:disable, 1:enable */
	INT8U	awb_win_hold;	/* 0:disable, 1:enable */
	INT8U	awbclamp_en;	/* 0:disable, 1:enable */
	INT8U	awbwinthr;		/* 0~0xFF */
	INT8U	sindata;		/* 1 sign + 1.6bits, -128/64~127/64 */
	INT8U	cosdata;		/* 1 sign + 1.6bits, -128/64~127/64 */
	INT8U	reserved0;
	INT8U	reserved1;
	
	INT8U	Ythr0;			/* 0~0xFF */
	INT8U	Ythr1;			/* 0~0xFF */
	INT8U	Ythr2;			/* 0~0xFF */
	INT8U	Ythr3;			/* 0~0xFF */
	
	INT16S	UL1N1;			/* -256 ~ 255 */
	INT16S	UL1P1;			/* -256 ~ 255 */
	INT16S	VL1N1;			/* -256 ~ 255 */
	INT16S	VL1P1;			/* -256 ~ 255 */
	
	INT16S	UL1N2;			/* -256 ~ 255 */
	INT16S	UL1P2;			/* -256 ~ 255 */
	INT16S	VL1N2;			/* -256 ~ 255 */
	INT16S	VL1P2;			/* -256 ~ 255 */

	INT16S	UL1N3;			/* -256 ~ 255 */
	INT16S	UL1P3;			/* -256 ~ 255 */
	INT16S	VL1N3;			/* -256 ~ 255 */
	INT16S	VL1P3;			/* -256 ~ 255 */

} gpCdspAWB_t;

typedef struct gpCdspWbGain2_s
{
	/* wb gain2 */
	INT8U	wbgain2en;		/* 0:disable, 1:enable */
	INT8U	reserved;
	INT16U	rgain2;			/* 0~0x1FF */
	INT16U	ggain2;			/* 0~0x1FF */
	INT16U	bgain2;			/* 0~0x1FF */
} gpCdspWbGain2_t;

typedef struct gpCdspHistgm_s
{
	/* histgm */
	INT8U	his_en;			/* 0:disable, 1:enable */
	INT8U	his_hold_en;	/* 0:disable, 1:enable */
	INT8U	hislowthr;		/* 0~0xFF */
	INT8U	hishighthr;		/* 0~0xFF */

	INT32U 	hislowcnt;		/* read back result low count */
	INT32U 	hishicnt;		/* read back result hight count */
} gpCdspHistgm_t;

typedef struct gpCdsp3aResult_s
{
	/* ae */
	INT8U ae_win[64];

	/* af */
	INT64U 	af1_h_value;
	INT64U 	af1_v_value;
	INT64U 	af2_h_value;
	INT64U 	af2_v_value;
	INT64U 	af3_h_value;
	INT64U 	af3_v_value;

	/* awb */
	AWB_RESULT_t awb_result;

	/* histgm */
	INT32U hislowcnt;	/* histgm low count */
	INT32U hishicnt; /* histgm hight count */	
} gpCdsp3aResult_t;

typedef struct gpCdspNewDenoise_s
{
	/* new denoise enhance */
	INT8U	newdenoiseen;	/* 0:disable, 1:enable */
	INT8U	ndmiren;		/* 0~0xF */
	INT8U	ndmirvsel;		/* 0:cnt3eq1, 1:cnt3eq2 */
	/* edge enhance */
	INT8U	ndedgeen;		/* 0:disable, 1:enable */
	/* edge lut table */
	INT8U	ndeluten;		/* 0:disable, 1:enable */
	INT8U   *ndedge_table;	/* edge table, 256byte */	
	INT8U	ndlhdiv;			/* 1/lhdiv,  1, 2, 4, 8, 16, 32, 64, 128 */
	INT8U	ndlhtdiv;		/* 1/lhtdiv,  1, 2, 4, 8, 16, 32, 64, 128 */	
	INT8U	ndlhcoring;		/* 0~0xFF */
	INT8U	ndampga;		/* 1, 2, 3, 4 */
	INT8U	ndlhmode;		/* 0:USE HPF LF00 matrix, 1:default matrix */ 
	
	/* edge filter */
	INT8U	ndlf00;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf01;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf02;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf10;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf11;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf12;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf20;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf21;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	ndlf22;			/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U	reserved0;
} gpCdspNewDenoise_t;

typedef struct gpIspHybridRaw_s                                                                      
{                                                                                                    
    INT32U IspHeight;
    INT32U ISPWidth;
    
    /* new denoise enhance */                                                                                                                 
	INT8U DefectPixelSel;
	INT8U DefectPixelEnable;
	INT8U DenoiseEnable; 
   	INT8U CrosstalkEnable; 
    	
   	INT8U DPCth1; 
   	INT8U DPCth2; 
   	INT8U DPCth3; 
    	
   	INT8U DPCDefaultMode; 
   	INT8U DPCn; 
    	
   	INT8U DenoiseT1; 
   	INT8U DenoiseT2; 
   	INT8U DenoiseT3; 
   	INT8U DenoiseT4; 
   	INT8U DenoiseW1; 
   	INT8U DenoiseW2; 
   	INT8U DenoiseW3; 
    	
   	INT8U CrosstalkT1; 
   	INT8U CrosstalkT2; 
   	INT8U CrosstalkT3; 
   	INT8U CrosstalkT4; 
   	INT8U CrosstalkW1;
   	INT8U CrosstalkW2; 
   	INT8U CrosstalkW3; 
	INT8U CrosstalkGbGr;                            
} gpIspHybridRaw_t; 

typedef struct gpCdspLinCorr_s
{
	/* color matrix */
	INT8U	lincorren;		/* 0:disable, 1:enable */
	INT8U	reserved0;
	
	INT8U*	lincorr_table;	/* 1 sign + 3.6 bit, -512/64~511/64 */
} gpCdspLinCorr_t;


typedef struct gpCdspFav_s
{
	/* color matrix */
	INT16U	ob_val;				/* optical blcak threshold */
	INT16U	ae_target;			/* target luminace */
	INT16U	ae_target_night;		/* target luminace in night */
	
	INT16U	awb_mode;
	INT16U  color_mode;
	INT16U  iso;
	INT8U	ev;
		
	INT8S	day_hue_u_offset;	/* -128 ~ +127,   +: more blue,  -: more yellow/green	*/
	INT8S	day_hue_v_offset;	/* -128 ~ +127,   +: more red,  -: more blue/green */
	INT8S	day_hue_y_offset;	/* -128 ~ +127	*/
	INT8S	day_sat_y_scale;	
	INT8S	day_sat_u_scale;	
	INT8S	day_sat_v_scale;
	INT8U	day_edge;			/*	0:soft, 1:smooth, 2:normal, 3:strong */
	INT8S	day_wb_offset;		/*	+: warm,   -: cold , +10~-10 */

	INT8S	night_hue_u_offset;	/* -128 ~ +127,   +: more blue,  -: more yellow/green	*/
	INT8S	night_hue_v_offset;	/* -128 ~ +127,   +: more red,  -: more blue/green */
	INT8S	night_hue_y_offset;	/* -128 ~ +127	*/
	INT8S	night_sat_y_scale;	
	INT8S	night_sat_u_scale;	
	INT8S	night_sat_v_scale;
	INT8U	night_edge;			/*	0:soft, 1:smooth, 2:normal, 3:strong */
	INT8S	night_wb_offset;		/*	+: warm,   -: cold , +10~-10 */	
	
	
} gpCdspFav_t;


typedef enum
{
	MODE_PURERAW = 0,
	MODE_LINCORR,
	MODE_LENSCMP,
	MODE_WBGAIN,
	MODE_LUTGAMMA,
	MODE_MAX 
} CDSP_RAW_CALIBRATION_MODE;

typedef struct gpCdspWBGain_s
{
	unsigned int max_rgain;
	unsigned int max_bgain;
	unsigned int min_rgain;
	unsigned int min_bgain;
}gpCdspWBGain_t;     

extern void drv_l2_CdspIsrRegister(INT32U isr_index, void (*handle)(void));
extern void drv_l2_CdspSensorCtrlRegister(gpCdspSensor_t *pCtrl);
extern void drv_l2_CdspTableRegister(sensor_calibration_t *sensor);
extern INT32S drv_l2_CdspPostProcess(gpCdspPostProcess_t *PostPress);
extern INT32S drv_l2_cdsp_get_ctrl(gpCdspCtrl_t *ctrl);
extern INT32S drv_l2_cdsp_set_ctrl(gpCdspCtrl_t *ctrl);
extern INT32S drv_l2_cdsp_set_fmt(gpCdspFmt_t *pFmt);
extern INT32S drv_l2_cdsp_stream_on(INT32U ae_awb_task_en, INT32U bufA, INT32U bufB); 
extern INT32S drv_l2_cdsp_stream_off(void);
extern INT32S drv_l2_cdsp_open(void);
extern INT32S drv_l2_cdsp_close(void);
extern void drv_l2_set_raw_yuv_path(INT32U path);
extern INT32U drv_l2_get_raw_yuv_path(void);                                                                                 
extern void save_raw10(INT32U capture_mode, INT32U g_frame_addr);
extern void cdsp_yuyv_restart(INT32U g_frame_addr);
#if (_SENSOR_GC1004_CDSP)||(_SENSOR_GC1004_CDSP_MIPI)
extern gpCdspWBGain_t *gc1004_awb_r_b_gain_boundary(void);
#endif
#if _SENSOR_SOI_H22_CDSP_MIPI
extern gpCdspWBGain_t *soi_h22_awb_r_b_gain_boundary(void);   
#endif	
extern void drv_l2_set_raw_capure_setting(INT32U lincor_lencmp);
extern void drv_l2_set_target_sensor_size(INT16U hsize, INT16U vsize);
#endif	/*__drv_l2_CDSP_H__*/
