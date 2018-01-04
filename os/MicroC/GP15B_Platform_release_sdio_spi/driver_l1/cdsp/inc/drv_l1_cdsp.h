#ifndef __drv_l1_CDSP_H__
#define __drv_l1_CDSP_H__

#include "driver_l1.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// image source
#define FRONT_INPUT		0
#define SDRAM_INPUT		1
#define MIPI_INPUT		2

// cdsp int type 
#define CDSP_INT_BIT		(1 << 0)
#define FRONT_VD_INT_BIT	(1 << 1)
#define FRONT_INT_BIT		(1 << 2)

// cdsp interrupt bit
#define CDSP_AFWIN_UPDATE	(1 << 0)
#define CDSP_AWBWIN_UPDATE	(1 << 1)
#define CDSP_AEWIN_SEND		(1 << 2)
#define CDSP_OVERFOLW		(1 << 3)
#define CDSP_EOF			(1 << 4)
#define CDSP_FACWR			(1 << 5)
#define CDSP_FIFO			(1 << 6)
#define CDSP_INT_ALL		0x37 
#define CDSP_INT_ALL_WFIFO	0x7F

// front vd interrupt bit
#define FRONT_VD_RISE		0x1
#define FRONT_VD_FALL		0x2

// front interrupt bit
#define VDR_EQU_VDRINT_NO	(1 << 0)
#define VDF_EQU_VDFINT_NO	(1 << 1)
#define SERIAL_DONE			(1 << 2)
#define SNAP_DONE			(1 << 3)
#define CLR_DO_CDSP			(1 << 4)
#define EHDI_FS_EQU_INTH_NO	(1 << 5)
#define FRONT_VVALID_RISE	(1 << 6)
#define FRONT_VVALID_FALL	(1 << 7)
#define OVERFLOW_RISE		(1 << 8)
#define OVERFLOW_FALL		(1 << 9)

// yuv special mode
#define SP_YUV_NONE			0
#define SP_YUV_NEGATIVE		1
#define SP_YUV_BINARIZE		2
#define SP_YUV_YbYcSatHue	3
#define SP_YUV_EMBOSSMENT	4
#define SP_YUV_EMBOSSMENTx2	5
#define SP_YUV_EMBOSSMENTx4	6
#define SP_YUV_EMBOSSMENTx8	7

// raw spec mode
#define SP_RAW_NONE			0
#define SP_RAW_NEGATIVE		1
#define SP_RAW_SOL_ARISE	2
#define SP_RAW_EMBOSSMENT	3
#define SP_RAW_BINARIZE		4
#define SP_RAW_SEPIA		5
#define SP_RAW_BLACK_WHITE	6

// image input source 
#define C_SDRAM_FMT_RAW8	0x10
#define C_SDRAM_FMT_RAW10	0x11
#define C_FRONT_FMT_RAW8	0x12
#define C_FRONT_FMT_RAW10	0x13
#define C_MIPI_FMT_RAW8		0x14
#define C_MIPI_FMT_RAW10	0x15

#define C_SDRAM_FMT_8Y4U4V	0x1F
#define C_SDRAM_FMT_VY1UY0	0x20
#define C_FRONT_FMT_UY1VY0	0x21
#define C_FRONT_FMT_Y1VY0U	0x22
#define C_FRONT_FMT_VY1UY0	0x23
#define C_FRONT_FMT_Y1UY0V	0x24
#define C_MIPI_FMT_Y1VY0U	0x25

// dma set
#define RD_A_WR_A		0
#define RD_A_WR_B		1
#define RD_B_WR_B		2
#define RD_B_WR_A		3
#define AUTO_SWITCH		4	

// fifo line set
#define LINE_8			0
#define LINE_16			1
#define LINE_32			2
#define LINE_64			3
#define LINE_NONE		4

// YUV mode
#define FMT_8Y4U4V		0
#define FMT_YUYV		1

// rgb path
#define RGB_NONE		0
#define RGB_PATH1		1	//From Lenscmp
#define RGB_PATH2		3	//From WBGain
#define RGB_PATH3		5	//From Lutgamma

// AF window size
#define C_AF256			0
#define C_AF512			1
#define C_AF1024		2
#define C_AF64			3
#define C_AF2048		4

// AE windows size
#define C_AE4			0
#define C_AE8			1
#define C_AE16			2
#define C_AE32			3
#define C_AE64			4
#define C_AE128			5

// Output Format
#define YUYV			0
#define UYVY 			1
#define YVYU 			2
#define	VYUY			3

// Algorithm Select
#define mastumoto		0
#define missing_pixel	1

// DPC RCV Mode
#define default_mode	1
#define second_mode		0

// ISP_Crosstalk removal
#define Gb				1
#define Gr				2
#define GbGr			3

// Input Scal path
#define CDSP_PATH		0
#define	CSI_PATH		1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct edge_filter_s
{
	INT8U LPF00;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF01;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */	
	INT8U LPF02;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U RSD0;
	
	INT8U LPF10;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF11;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF12;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U RSD1;

	INT8U LPF20;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF21;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U LPF22;		/* [3]:0:positive, 1:negtive + [2:0]:0:0,1:1,2:2,3:3,4:4,5:8,6:16 */
	INT8U RSD2;
} edge_filter_t;

typedef struct color_matrix_s
{
	INT16U CcCof00;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U CcCof01;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U CcCof02;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	
	INT16U CcCof10;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U CcCof11;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U CcCof12;		/* 1 sign + 3.6 bit, -512/64~511/64 */

	INT16U CcCof20;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U CcCof21;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U CcCof22;		/* 1 sign + 3.6 bit, -512/64~511/64 */
	INT16U RSD0;
} color_matrix_t;

typedef struct uv_divide_s
{
	INT8U YT1;			/* 0~0xFF, when Y=T1, UV divide 1/8 */	
	INT8U YT2;			/* 0~0xFF, when Y=T2, UV divide 2/8 */
	INT8U YT3;			/* 0~0xFF, when Y=T3, UV divide 3/8 */
	INT8U YT4;			/* 0~0xFF, when Y=T4, UV divide 4/8 */
	INT8U YT5;			/* 0~0xFF, when Y=T5, UV divide 5/8 */
	INT8U YT6;			/* 0~0xFF, when Y=T6, UV divide 6/8 */
	INT8U RSD0;
	INT8U RSD1;
} uv_divide_t;

typedef struct awb_uv_thr_s
{
	INT16S UL1N1; 
	INT16S UL1P1;
	INT16S VL1N1;
	INT16S VL1P1;

	INT16S UL1N2; 
	INT16S UL1P2;
	INT16S VL1N2;
	INT16S VL1P2;

	INT16S UL1N3; 
	INT16S UL1P3;
	INT16S VL1N3;
	INT16S VL1P3;
} awb_uv_thr_t;

typedef struct af_windows_value_s
{
	INT32U h_value_l;
	INT32U h_value_h;
	INT32U v_value_l;
	INT32U v_value_h;
} af_windows_value_t;


/***************************
    CLOCK path
****************************/
extern INT32S drv_l1_CdspSetClockTree(INT8U enable, INT32U path, INT8U raw_fmt_en);

/***************************
    CDSP ISR & INT Handle
****************************/
extern INT32U drv_l1_CdspGetIntStatus(void);
extern INT32U drv_l1_CdspGetFrontVdIntStatus(void);
extern INT32U drv_l1_CdspGetFrontIntStatus(void);
extern INT32U drv_l1_CdspGetGlbIntStatus(void);

/***************************
       cdsp system
****************************/
extern void drv_l1_CdspReset(void);
extern void drv_l1_CdspSetGatingClk(INT8U enable);
extern void drv_l1_CdspSetRedoTriger(INT8U docdsp);
extern void drv_l1_CdspSetGatingValid(INT8U gating_gamma_vld, INT8U gating_badpixob_vld);
extern void drv_l1_CdspSetDarkSub(INT8U raw_sub_en, INT8U sony_jpg_en);
extern void drv_l1_CdspSetRawDataFormat(INT8U format);
extern void drv_l1_CdspSetYuvRange(INT8U signed_yuv_en);
extern void drv_l1_CdspSetSRAM(INT8U overflowen, INT16U sramthd);

/***************************
        interrupt
****************************/
extern void drv_l1_CdspSetIntEn(INT8U inten, INT8U bit);
extern void drv_l1_CdspClrIntStatus(INT8U bit);

/***************************
          other
****************************/
extern void drv_l1_CdspSetLineInterval(INT16U line_interval);
extern void drv_l1_CdspSetExtLine(INT16U linesize,INT16U lineblank);

/***************************
         path set
****************************/
extern void drv_l1_CdspSetDataSource(INT8U image_source);
extern void drv_l1_CdspSetRawPath(INT8U raw_mode, INT8U cap_mode, INT8U yuv_mode, INT8U yuv_fmt);
extern void drv_l1_CdspSetRawPath2(INT8U raw_mode, INT8U cap_mode, INT8U yuv_mode);
extern void drv_l1_CdspSetYuvMuxPath(INT8U redoedge);
extern void drv_l1_CdspSetLensCmpPath(INT8U yuvlens);
extern void drv_l1_CdspSetExtLinePath(INT8U extinen, INT8U path);
extern void drv_l1_CdspSetLineCtrl(INT8U ybufen);

/***************************
    cdsp dma buffer set
****************************/
extern void drv_l1_CdspSetYuvBuffA(INT16U width, INT16U height, INT32U buffer_addr);
extern void drv_l1_CdspSetYuvBuffB(INT16U width, INT16U height, INT32U buffer_addr);
extern void drv_l1_CdspSetRawBuffSize(INT16U width, INT16U height, INT16U hoffset, INT8U rawbit);
extern void drv_l1_CdspSetRawBuff(INT32U buffer_addr);
extern void drv_l1_CdspSetReadBackSize(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_CdspSetDmaConfig(INT8U buffer_mode);
extern void drv_l1_CdspSetFIFOMode(INT8U frcen, INT8U fifo_en);
extern void drv_l1_CdspSetFifoLine(INT8U line_mode);

/***************************
       clamp set
****************************/
extern void drv_l1_CdspSetClampEn(INT8U clamphsizeen, INT16U clamphsize);
extern void drv_l1_CdspSetPreRBClamp(INT8U pre_rb_clamp);
extern INT32U drv_l1_CdspGetPreRBClamp(void);
extern void drv_l1_CdspSetRBClamp(INT8U rbclampen, INT8U rbclamp);
extern void drv_l1_CdspGetRBClamp(INT8U *rbclampen, INT8U *rbclamp);
extern void drv_l1_CdspSetUvDivideEn(INT8U uvDiven);
extern INT32U gpHalCdspGetUvDivideEn(void);
extern void drv_l1_CdspSetUvDivide(uv_divide_t *pDiv);
extern void drv_l1_CdspGetUvDivide(uv_divide_t *UVDivide);
extern void drv_l1_CdspSetQcntThr(INT16U Qthr, INT16U PreRBclamp);

/***************************
       bad pixel set
****************************/
extern void drv_l1_CdspSetBadPixelEn(INT8U badpixen, INT8U badpixmirrlen);
extern void drv_l1_CdspGetBadPixelEn(INT8U *badpixen, INT8U *badpixmirrlen);
extern void drv_l1_CdspSetBadPixel(INT8U bprthr, INT8U bpgthr, INT8U bpbthr);
extern void drv_l1_CdspGetBadPixel(INT8U *bprthr, INT8U *bpgthr, INT8U *bpbthr);

/***************************
     optical black set
****************************/
extern void drv_l1_CdspSetManuOB(INT8U manuoben, INT16U manuob);
extern void drv_l1_CdspGetManuOB(INT8U *manuoben, INT16U *manuob);
extern void drv_l1_CdspSetAutoOB(INT8U autooben, INT8U obtype, INT8U curfob);
extern void drv_l1_CdspGetAutoOB(INT8U *autooben, INT8U *obtype, INT16U *obHOffset, INT16U *obVOffset);
extern void drv_l1_CdspSetOBHVoffset(INT16U obHOffset, INT16U obVOffset);
extern void drv_l1_CdspGetAutoOBAvg(INT16U *ob_avg_r, INT16U *ob_avg_gr, INT16U *ob_avg_b, INT16U *ob_avg_gb);

/***************************
     lens compensation
****************************/
extern void drv_l1_CdspSetLensCmpTable(INT16U *plensdata);
extern void drv_l1_CdspSetLensCmpEn(INT8U lcen);
extern INT32U drv_l1_CdspGetLensCmpEn(void);
extern void drv_l1_CdspSetLensCmpPos(INT16U centx, INT16U centy, INT16U xmoffset, INT16U ymoffset);
extern void drv_l1_CdspGetLensCmpPos(INT16U *centx, INT16U *centy, INT16U *xmoffset, INT16U *ymoffset);
extern void drv_l1_CdspSetLensCmp(INT8U stepfactor, INT8U xminc, INT8U ymoinc, INT8U ymeinc);
extern void drv_l1_CdspGetLensCmp(INT8U *stepfactor, INT8U *xminc, INT8U *ymoinc, INT8U *ymeinc);

/***************************
    New ISP Macro Table
****************************/
extern void drv_l1_IspSetLiCor(INT8U *plicordata);
extern void drv_l1_IspSetHr0(INT32U value);
extern void drv_l1_IspSetHr1(INT32U value);
extern void drv_l1_IspSetHr2(INT32U value);
extern void drv_l1_IspSetHr3(INT32U value);
extern void drv_l1_IspSetLucMaxTan8SlopCLP(INT16U *pmaxtan8data, INT16U *pslopdata, INT16U *pclpdata);
extern void drv_l1_IspSetRadusF0(INT16U *pradusf0data);
extern void drv_l1_IspSetRadusF1(INT16U *pradusf1data);
extern void drv_l1_IspSetLenCmp(INT32U lcen,INT32U stepfactor);

/***************************
      raw h scaler
****************************/
extern void drv_l1_CdspSetRawHScaleEn(INT8U hscale_en, INT8U hscale_mode);
extern void drv_l1_CdspSetRawHScale(INT32U src_hsize, INT32U dst_hsize);

/***************************
     white balance set
****************************/
extern void drv_l1_CdspSetWbGainEn(INT8U wbgainen);
extern void drv_l1_CdspSetWbGain(INT16U r_gain, INT16U gr_gain, INT16U b_gain, INT16U gb_gain);
extern void drv_l1_CdspSetWbOffsetEn(INT8U wboffseten);
extern void drv_l1_CdspSetWbOffset(INT8U roffset, INT8U groffset, INT8U boffset, INT8U gboffset);
extern INT32U drv_l1_CdspGetWbGain(INT16U *prgain, INT16U *pgrgain, INT16U *pbgain, INT16U *pgbgain);
extern void drv_l1_CdspGetWbOffset(INT8U *pwboffseten, INT8S *proffset, INT8S *pgroffset, INT8S *pboffset,	INT8S *pgboffset);

/***************************
     global gain
****************************/
extern void drv_l1_CdspSetGlobalGain(INT8U global_gain);
extern INT32U drv_l1_CdspGetGlobalGain(void);

/***************************
          gain2
****************************/
extern void drv_l1_CdspSetWbGain2En(INT8U wbgain2en);
extern void drv_l1_CdspSetWbGain2(INT16U rgain2, INT16U ggain2, INT16U bgain2);
extern void drv_l1_CdspGetWbGain2(INT8U *pwbgain2en, INT16U *prgain, INT16U *pggain, INT16U *pbgain);

/***************************
         gamma set
****************************/
extern void drv_l1_CdspSetGammaTable(INT32U *pGammaTable);
extern void drv_l1_CdspSetLutGammaEn(INT8U lut_gamma_en);
extern INT32U drv_l1_CdspGetLutGammaEn(void);

/***************************
      interpolation set
****************************/
extern void drv_l1_CdspSetIntplMirEn(INT8U intplmirhlen, INT8U intplmirvsel, INT8U intplcnt2sel);
extern void drv_l1_CdspSetIntplThr(INT8U int_low_thr, INT8U int_hi_thr);
extern void drv_l1_CdspGetIntplThr(INT8U *int_low_thr, INT8U *int_hi_thr);

/***************************
  edge in intpolation set
****************************/
extern void drv_l1_CdspSetEdgeEn(INT8U edgeen);
extern INT32U drv_l1_CdspGetEdgeEn(void);
extern void drv_l1_CdspSetEdgeFilter(edge_filter_t *pEdgeFilter);
extern void drv_l1_CdspGetEdgeFilter(edge_filter_t *pEdgeFilter);
extern void drv_l1_CdspSetEdgeLCoring(INT8U lhdiv, INT8U lhtdiv, INT8U lhcoring,INT8U lhmode);
extern void drv_l1_CdspGetEdgeLCoring(INT8U *lhdiv, INT8U *lhtdiv, INT8U *lhcoring, INT8U *lhmode);
extern void drv_l1_CdspSetEdgeAmpga(INT8U ampga);
extern INT32U drv_l1_CdspGetEdgeAmpga(void);
extern void drv_l1_CdspSetEdgeDomain(INT8U edgedomain);
extern INT32U drv_l1_CdspGetEdgeDomain(void);
extern void drv_l1_CdspSetEdgeQthr(INT8U Qthr);
extern INT32U drv_l1_CdspGetEdgeQCnt(void);

/***************************
    edge lut table set
****************************/
extern void drv_l1_CdspSetEdgeLutTable(INT8U *pLutEdgeTable);
extern void drv_l1_CdspSetEdgeLutTableEn(INT8U eluten);
extern INT32U drv_l1_CdspGetEdgeLutTableEn(void);
extern void drv_l1_CdspSetNewDEdgeLut(INT8U *pLutNewDEdgeTable);

/***************************
    color matrix set
****************************/
extern void drv_l1_CdspSetColorMatrixEn(INT8U colcorren);
extern INT32U drv_l1_CdspGetColorMatrixEn(void);
extern void drv_l1_CdspSetColorMatrix(color_matrix_t *pCM);
extern void drv_l1_CdspGetColorMatrix(color_matrix_t *pCM);

/***************************
    raw special mode set
****************************/
extern void drv_l1_CdspSetRawSpecMode(INT8U rawspecmode);
extern INT32U drv_l1_CdspGetRawSpecMode(void);

/***************************
  YUV insert & coring set
****************************/
extern void drv_l1_CdspSetYuv444InsertEn(INT8U yuvinserten);
extern INT32U drv_l1_CdspGetYuv444InsertEn(void);
extern void drv_l1_CdspSetYuvCoring(INT8U y_corval_coring, INT8U u_corval_coring, INT8U v_corval_coring);
extern void drv_l1_CdspGetYuvCoring(INT8U *y_corval_coring, INT8U *u_corval_coring, INT8U *v_corval_coring);

/***************************
       crop set
****************************/
extern void drv_l1_CdspSetCropEn(INT8U hvEnable);
extern void drv_l1_CdspSetCrop(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_CdspGetCrop(INT8U *hvEnable, INT32U *hoffset, INT32U *voffset, INT32U *hsize, INT32U *vsize);

/***************************
       YUV havg set
****************************/
extern void drv_l1_CdspSetYuvHAvg(INT8U yuvhavgmiren, INT8U ytype, INT8U utype, INT8U vtype);
extern void drv_l1_CdspGetYuvHAvg(INT8U *yuvhavgmiren, INT8U *ytype, INT8U *utype, INT8U *vtype);

/***************************
    YUV special mode set
****************************/
extern void drv_l1_CdspSetYuvSpecMode(INT8U yuvspecmode);
extern INT32U drv_l1_CdspGetYuvSpecMode(void);
extern void drv_l1_CdspSetBinarizeModeThr(INT8U binarthr);
extern INT32U drv_l1_CdspGetBinarizeModeThr(void);

/***************************
 Y brightness & contrast set
****************************/
extern void drv_l1_CdspSetBriCont(INT8U YbYcEn, INT32U yb,INT32U yc);
extern void drv_l1_CdspSetBriContEn(INT8U YbYcEn);
extern INT32U drv_l1_CdspGetBriContEn(void);
extern void drv_l1_CdspSetYuvSPEffOffset(INT8S y_offset, INT8S u_offset, INT8S v_offset);
extern void drv_l1_CdspGetYuvSPEffOffset(INT8S *y_offset, INT8S *u_offset, INT8S *v_offset);
extern void drv_l1_CdspSetYuvSPEffScale(INT8U y_scale, INT8U u_scale, INT8U v_scale);
extern void drv_l1_CdspGetYuvSPEffScale(INT8U *y_scale, INT8U *u_scale, INT8U *v_scale);

/***************************
   UV saturation & hue set
****************************/
extern void drv_l1_CdspSetSatHue(INT32U uv_sat_scale, INT32U uoffset, INT32U voffset, 
						  		 INT32U u_huesindata, INT32U u_huecosdata, INT32U v_huesindata, INT32U v_huecosdata);
extern void drv_l1_CdspSetYuvSPHue(INT16U u_huesindata, INT16U u_huecosdata, INT16U v_huesindata, INT16U v_huecosdata);
extern void drv_l1_CdspGetYuvSPHue(INT8S *u_huesindata, INT8S *u_huecosdata,	INT8S *v_huesindata, INT8S *v_huecosdata);

/***************************
      yuv h & v scale
****************************/
extern void drv_l1_CdspSetYuvHScaleEn(INT8U yuvhscale_en, INT8U yuvhscale_mode);
extern void drv_l1_CdspSetYuvVScaleEn(INT8U vscale_en, INT8U vscale_mode);
extern void drv_l1_CdspSetYuvHScale(INT16U hscaleaccinit, INT16U yuvhscalefactor);
extern void drv_l1_CdspSetYuvVScale(INT16U vscaleaccinit, INT16U yuvvscalefactor);

/***************************
      Suppression set
****************************/
extern void drv_l1_CdspSetUvSupprEn(INT8U suppressen);
extern void drv_l1_CdspSetUvSuppr(INT8U yuvsupmirvsel, INT8U fstextsolen, INT8U yuvsupmiren);

/***************************
Y edge in uv suppression set
****************************/
extern void drv_l1_CdspSetEdgeSrc(INT8U posyedgeen);
extern INT32U drv_l1_CdspGetEdgeSrc(void);

/***************************
Y denoise in suppression set
****************************/
extern void drv_l1_CdspSetYDenoiseEn(INT8U denoisen);
extern INT32U drv_l1_CdspGetYDenoiseEn(void);
extern void drv_l1_CdspSetYDenoise(INT8U denoisethrl,	INT8U denoisethrwth, INT8U yhtdiv);
extern void drv_l1_CdspGetYDenoise(INT8U *denoisethrl,	INT8U *denoisethrwth, INT8U *yhtdiv);

/***************************
  Y LPF in suppression set
****************************/
extern void drv_l1_CdspSetYLPFEn(INT8U lowyen);
extern INT32U drv_l1_CdspGetYLPFEn(void);

/***************************
        New ISP
****************************/
extern void drv_l1_IspSetImageSize(INT16U hsize, INT16U vsize);
extern void drv_l1_CdspSetLinCorrTable(INT8U *rgb_tbl);
extern void drv_l1_IspSetLinCorrEn(INT8U lincorr_en);
extern void drv_l1_IspSetDbpcSel(INT8U algorithm_sel);
extern void drv_l1_IspSetDpcEn(INT8U dpc_en,INT8U algorithm_sel);
extern void drv_l1_IspSetDpcRcvModeSelThr(INT8U sel_mode, INT16U DPCth1, INT16U DPCth2, INT16U DPCth3);
extern void drv_l1_IspSetSmoothFactor(INT16U DPCn);
extern void drv_l1_IspSetCrostlkGbGrEn(INT8U ctk_en, INT8U ctk_gbgr);
extern void drv_l1_IspSetCrostlkThold(INT16U ctk_thr1, INT16U ctk_thr2, INT16U ctk_thr3, INT16U ctk_thr4);
extern void drv_l1_IspSetCrostlkWeight(INT16U ctkw1, INT16U ctkw2, INT16U ctkw3);
extern void drv_l1_IspSetDenoiseEn(INT8U denoise_en);
extern void drv_l1_IspSetDenoiseThold(INT16U rdn_thr1,INT16U rdn_thr2,INT16U rdn_thr3,INT16U rdn_thr4);
extern void drv_l1_IspSetDenoiseWeight(INT16U rdnw1, INT16U rdnw2, INT16U rdnw3);
extern void drv_l1_CdspSetNewDenoiseEn(INT8U newdenoiseen);
extern INT32U drv_l1_CdspGetNewDenoiseEn(void);
extern void drv_l1_CdspSetNewDenoise(INT8U ndmirvsel, INT8U ndmiren);
extern void drv_l1_CdspGetNewDenoise(INT8U *ndmirvsel, INT8U *ndmiren);
extern void drv_l1_CdspSetNdEdgeEn(INT8U ndedgeen,INT8U ndeluten);
extern void drv_l1_CdspGetNdEdgeEn(INT8U *ndedgeen, INT8U *ndeluten);
extern void drv_l1_CdspSetNdEdgeFilter(edge_filter_t *pNdEdgeFilter);
extern void drv_l1_CdspGetNdEdgeFilter(edge_filter_t *pNdEdgeFilter);
extern void drv_l1_CdspSetNdEdgeLCoring(INT8U ndlhdiv, INT8U ndlhtdiv, INT8U ndlhcoring);
extern void drv_l1_CdspGetNdEdgeLCoring(INT8U *ndlhdiv, INT8U *ndlhtdiv, INT8U *ndlhcoring);
extern void drv_l1_CdspSetNdEdgeAmpga(INT8U ndampga);
extern INT32U drv_l1_CdspGetNdEdgeAmpga(void);

/***************************
   auto focuse / af set
****************************/
extern void drv_l1_CdspSetAFEn(INT8U af_en, INT8U af_win_hold);
extern void drv_l1_CdspGetAFEn(INT8U *af_en, INT8U *af_win_hold);
extern void drv_l1_CdspSetAfWin1(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_CdspSetAfWin2(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern void drv_l1_CdspSetAfWin3(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize);
extern INT32S drv_l1_CdspGetAFWinVlaue(INT8U windows_no, af_windows_value_t *af_value);

/***************************
auto white balance / awb set
****************************/
extern void drv_l1_CdspSetAWBEn(INT8U awb_win_en, INT8U awb_win_hold);
extern void drv_l1_CdspSetAWB(INT8U awbclamp_en, INT8U sindata, INT8U cosdata, INT8U awbwinthr);
extern void drv_l1_CdspSetAwbYThr(INT8U Ythr0, INT8U Ythr1, INT8U Ythr2, INT8U Ythr3);
extern void drv_l1_CdspSetAwbUVThr(awb_uv_thr_t *UVthr);

/***************************
   auto expore / ae set
****************************/
extern void drv_l1_CdspSetAeAwbSrc(INT8U raw_en);
extern INT32U drv_l1_CdspGetAeAwbSrc(void);
extern void drv_l1_CdspSetAeAwbSubSample(INT8U subample);
extern INT32U drv_l1_CdspGetAeAwbSubSample(void);
extern void drv_l1_CdspSetAEEn(INT8U ae_win_en, INT8U ae_win_hold);
extern void drv_l1_CdspGetAEEn(INT8U *ae_win_en, INT8U *ae_win_hold);
extern INT32U drv_l1_CdspGetAEActBuff(void);
extern void drv_l1_CdspSetAEWin(INT8U phaccfactor, INT8U pvaccfactor);
extern void drv_l1_CdspSetAEBuffAddr(INT32U winaddra,INT32U winaddrb);
extern void drv_l1_CdspSetRGBWin(INT16U hwdoffset, INT16U vwdoffset, INT16U hwdsize, INT16U vwdsize);
extern void drv_l1_CdspGetRGBWin(INT16U *hwdoffset, INT16U *vwdoffset, INT16U *hwdsize, INT16U *vwdsize);
extern void drv_l1_CdspSet3ATestWinEn(INT8U AeWinTest, INT8U AfWinTest);

/***************************
       histgm set
****************************/
extern void drv_l1_CdspSetHistgmEn(INT8U his_en, INT8U his_hold_en);
extern void drv_l1_CdspSetHistgm(INT8U hislowthr, INT8U hishighthr);
extern void drv_l1_CdspGetHistgm(INT32U *hislowcnt, INT32U *hishighcnt);

/***************************
       awb sum set
****************************/
extern INT32S drv_l1_CdspGetAwbSumCnt(INT8U section, INT32U *sumcnt);
extern INT32S drv_l1_CdspGetAwbSumG(INT8U section, INT32U *sumgl, INT32U *sumgh);
extern INT32S drv_l1_CdspGetAwbSumRG(INT8U section, INT32U *sumrgl, INT32S *sumrgh);
extern INT32S drv_l1_CdspGetAwbSumBG(INT8U section, INT32U *sumbgl, INT32S *sumbgh);

/***************************
   Motion Detection set
****************************/
extern void drv_l1_CdspSetMDEn(INT8U enable);
extern void drv_l1_CdspSetMD(INT8U enable, INT8U threshold, INT16U width, INT32U working_buf);
extern INT32U drv_l1_CdspGetResult(void);
#endif //__drv_l1_CDSP_H__
