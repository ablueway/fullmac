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
#include <string.h>
#include "application.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_cache.h"
#include "drv_l1_cdsp.h"
#include "drv_l1_front.h"
#include "drv_l2_cdsp.h"
#include "drv_l2_sensor.h"
#include "gp_stdlib.h"

#include "gplib_mm_gplus.h"
#if _OPERATING_SYSTEM == _OS_UCOS2	
#include "os.h"
#endif 

#if (defined _DRV_L2_CDSP) && (_DRV_L2_CDSP == 1)                  
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_WAKE_UP			1
#define C_STOP				2
#define C_SET_SENSOR		3

#define SKIP_CNT			3

#define _SET_YUV_PATH_		1
#define RAW_BIT				10
#define RAW_MODE			0x01

#define C_QUEUE_MAX			2
#define C_TASK_STACK_SIZE	1024

#define CDSP_AE_CTRL_EN		0x01
#define CDSP_AWB_CTRL_EN	0x02
#define CDSP_AF_CTRL_EN		0x04
#define CDSP_HIST_CTRL_EN	0x08
#define CDSP_LUM_STATUS		0x10
#define CDSP_LOW_LUM		0x20
#define CDSP_HIGH_LUM		0x40
#define CDSP_AWB_SET_GAIN	0x80
#define CDSP_SAT_SWITCH		0x100
#define CDSP_AE_UPDATE		0x200

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define __FUNCTION__		__func__
#if 0// debug message off
#include "gplib_print_string.h"
#define DEBUG				DBG_PRINT
#else
#define DEBUG(...)
#endif

#define MEMCPY				gp_memcpy
#define CLEAR(ptr, cblen)	gp_memset((INT8S *)ptr, 0x00, cblen)
#define RETURN(x)			ret = x; goto __exit;
#define ABS(x)				((x) >= 0 ? (x) : -(x))

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpCdspDev_s
{
	OS_EVENT *cdsp_wait_queue;
	void *	 cdsp_wait_queue_buf[C_QUEUE_MAX];

	/*Sub device*/
	gpCdspSensor_t *sensor_sd;	 
	void (*interrupt_handle[C_ISR_MAX])(void);
	
	
	/* AE buffer */
	INT32U aealign4_0;
	INT8U aeAddr0[64];
	INT32U aealign4_1;
	INT8U aeAddr1[64];

	INT8U OpenCnt;	/* open count */
	INT8U RunFlag;	/* Cdsp running flag */
	INT8U SkipCnt;	/* Cdsp skip count */
	INT8U MclkEn;	/* mclk enable flag */
	
	/* format set */	
	INT32U imgSrc; /* image input source, sdram, front, mipi */
	INT32U inFmt; /* input format */
	INT32U rawFmtFlag; /* raw Format Flag */
	INT32U rawFmt; /* raw Format Flag */
	INT16U imgWidth; /* image/csi h size */
	INT16U imgHeight; /* image/csi v size */
	INT16U imgRbWidth; /* output buffer h size */
	INT16U imgRbHeight; /* output buffer v size */

	/* cdsp module set */	
	gpCdspScalePara_t scale; /* scale down set */
	gpCdspSuppression_t suppression; /* suppression mode */	
	gpCdsp3aResult_t result;

	sensor_calibration_t sensor_cdsp;
	gpIspHybridRaw_t IspHybridRaw;
	
	/* cdsp 2A */
	OS_EVENT *ae_awb_wait_queue;
	OS_EVENT *ae_set_sensor_msg;
	void *	 ae_awb_wait_queue_buf[C_QUEUE_MAX];
	void *	 ae_set_sensor_msg_buf[1];
	OS_EVENT *aeawb_sem;
	INT32U aeawb_task_stack[C_TASK_STACK_SIZE];
	INT32U ae_set_sensor_task_stack[C_TASK_STACK_SIZE/4];		
	INT32U ae_awb_task_en;
	volatile INT32U ae_awb_flag;
	INT32S color_temp_init_val;
	INT32S sensor_ev_idx;
	INT32S ae_target;
	INT32S ae_target_night;
	
	INT32S getSensorInfo;
	INT32S sensor_gain_thr, sensor_time_thr;
	INT32S night_gain_thr;
	
	INT32S wb_offset_night, wb_offset_day;
	INT32S edge_day, edge_night;
	
	INT32S sat_yuv_level[4][6];
	INT32S sat_yuv_thr[4];
	INT32S edge_level[4];
	INT32S sat_contr_idx;
	uv_divide_t UVDivide;
	
	//ae, awb
	INT8U 	 *ae_workmem;
	INT8U	 *awb_workmem;
	AWB_MODE awbmode;
} gpCdspDev_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
 static void drv_l2_cdsp_edge_level_init(void);
static void drv_l2_cdsp_sat_contrast_init(void);
static void drv_l2_cdsp_sat_contrast_thr_init(int max_ev_idx, int night_ev_idx);
static void drv_l2_ae_awb_lock(void);
static void drv_l2_ae_awb_unlock(void);
static void drv_l2_ae_awb_process(void *arg);
static void drv_l2_ae_set_sensor_process(void *arg);
void drv_l2_Cdsp_hwFront_SetSize(INT32U hsize, INT32U vsize);
void drv_l2_Cdsp_hwIsp_SetImageSize(INT16U hsize,INT16U vsize);
void drv_l2_Set_Size_before_NewDenoise(INT8U idx, INT8U enable);
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern const INT8U LiTable_rgb[];
extern const INT16U MaxTan8[];
extern const INT16U Slope4[];
extern const INT16U CLPoint[];
extern const INT16U Radius_File_0[];
extern const INT16U Radius_File_1[];
extern const INT8U LEElut[];
extern const INT32U gammatable[];
extern const INT32U OV7725_gammatable[];
extern const INT16U g_lenscmp_table[];
extern const INT8U g_gamma_table[];
extern const INT8U g_edge_table[];
extern const INT8U g_nd_edge_table[];

static gpCdspDev_t *CdspDev = NULL;

static INT32U g_set_yuv_path = 0;
static INT16U target_sensor_hsize = 0;
static INT16U target_sensor_vsize = 0;
static INT8U target_sensor_index = 0;
static INT8U sensor_type=0; //0: para, 1: mipi
// path: 0 = raw
// path: 1 = yuv
void drv_l2_set_raw_yuv_path(INT32U path)
{
	g_set_yuv_path = path;
}

INT32U drv_l2_get_raw_yuv_path(void)
{
	return g_set_yuv_path;
}

extern INT32U flag_update;
extern INT32U update_value;
extern INT32U g_frame_addr1;
static INT32U capture_raw_set= 0;

void drv_l2_set_target_sensor_size(INT16U hsize, INT16U vsize)
{
	INT32U i;
	drv_l2_sensor_ops_t *pSensor;
	drv_l2_sensor_info_t *pInfo;
	
	target_sensor_hsize = hsize;
	target_sensor_vsize = vsize;


	for(i=0; i<3; i++) {
		pSensor = drv_l2_sensor_get_ops(0);
		pInfo = pSensor->get_info(i);
		if( (pInfo->target_w == target_sensor_hsize)&&
		     (pInfo->target_h == target_sensor_vsize) ) {
			target_sensor_index = i;
			break;
		}
	}
}

INT32U drv_l2_get_raw_capture_setting(void)
{
	return capture_raw_set;
}

void drv_l2_set_raw_capure_setting(INT32U lincor_lencmp)
{	
	capture_raw_set = lincor_lencmp;
}

/**
 * @brief	cdsp set fifo size
 * @param	width[in]: input width
 * @return	none
 */
INT32U drv_l2_CdspSetFifoSize(INT16U width)
{
#if 0 
	if(width >= 1280) {
		width = 0x100;
	} else {
		while(width >= 0x100) {
			width >>= 1;
		}
	}
#else
	width = 0x100;
#endif	

	DEBUG("fifo=0x%x.\r\n", width);
	drv_l1_CdspSetSRAM(ENABLE, width);
	drv_l1_CdspSetLineInterval(0x28);
	return width;
}

/**
 * @brief	cdsp set bad pixel
 * @param	argp[in]: bad pixel ob parameter
 * @return	none
 */
void drv_l2_CdspSetBadPixOb(gpCdspBadPixOB_t *argp)
{
	/* bad pixel */
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->badpixen);
	if(argp->badpixen) {
		drv_l1_CdspSetBadPixel(argp->bprthr, argp->bpgthr, argp->bpbthr);
		drv_l1_CdspSetBadPixelEn(argp->badpixen, 0x3); /* enable mirror */
	} else {
		drv_l1_CdspSetBadPixelEn(DISABLE, 0x3);
	}

	/* optical black */
	if(argp->manuoben || argp->autooben) {
		/* enable white balance offset when use ob */
		if(argp->wboffseten == 0) {
			argp->wboffseten = 1;
		}
		
		drv_l1_CdspSetWbOffset(argp->roffset, argp->groffset, argp->boffset, argp->gboffset);
		drv_l1_CdspSetWbOffsetEn(argp->wboffseten); 
		//hwCdsp_SetAverageOB(argp->wboffseten, argp->roffset, argp->groffset, argp->boffset, argp->gboffset);
		drv_l1_CdspSetManuOB(argp->manuoben, argp->manuob);
		drv_l1_CdspSetAutoOB(argp->autooben, argp->obtype, 0);
	} else {
		drv_l1_CdspSetManuOB(DISABLE, argp->manuob);
		drv_l1_CdspSetAutoOB(DISABLE, argp->obtype, 0);
	}
}

/**
 * @brief	cdsp get bad pixel
 * @param	argp[out]: bad pixel ob parameter
 * @return	none
 */
void drv_l2_CdspGetBadPixOb(gpCdspBadPixOB_t *argp)
{
	/* bad pixel */
	drv_l1_CdspGetBadPixelEn(&argp->badpixen, &argp->badpixmirr);
	drv_l1_CdspGetBadPixel((INT8U *)&argp->bprthr, (INT8U *)&argp->bpgthr, (INT8U *)&argp->bpbthr);

	/* optical black */
	drv_l1_CdspGetWbOffset(&argp->wboffseten, &argp->roffset, &argp->groffset, &argp->boffset, &argp->gboffset);
	drv_l1_CdspGetManuOB((INT8U *)&argp->manuoben, (INT16U *)&argp->manuob);
	drv_l1_CdspGetAutoOB(&argp->autooben, &argp->obtype, &argp->obHOffset, &argp->obVOffset);
	drv_l1_CdspGetAutoOBAvg(&argp->Ravg, &argp->GRavg, &argp->Bavg, &argp->GBavg);
}

/**
 * @brief	cdsp set lens compenasation
 * @param	raw_flag[in]: raw foramt flag
 * @param	argp[in]: lens compensation struct
 * @return	none
 */
void drv_l2_CdspSetLensCmp(unsigned char raw_flag, gpCdspLensCmp_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->lcen);
	if(argp->lcen) {
		if(!argp->maxtan8_table) {
			argp->maxtan8_table = (INT16U*)MaxTan8;
		}
		if(!argp->slop4_table){
			argp->slop4_table = (INT16U*)Slope4;
		}
		if(!argp->clpoint_table){
			argp->clpoint_table = (INT16U*)CLPoint;
		}
		
		drv_l1_IspSetLucMaxTan8SlopCLP(argp->maxtan8_table, argp->slop4_table, argp->clpoint_table);

		if(argp->rdiusfile0_table){
			drv_l1_IspSetRadusF0(argp->rdiusfile0_table);
		} else {
			drv_l1_IspSetRadusF0((INT16U*)Radius_File_0);
		}

		if(argp->rdiusfile1_table){
			drv_l1_IspSetRadusF1(argp->rdiusfile1_table);
		} else {
			drv_l1_IspSetRadusF1((INT16U*)Radius_File_1);
		}
		
//		drv_l1_CdspSetLensCmpPos(argp->centx, argp->centy, argp->xmoffset, argp->ymoffset);
//		drv_l1_CdspSetLensCmp(argp->stepfactor, argp->xminc, argp->ymoinc, argp->ymeinc);
		drv_l1_IspSetLenCmp(argp->lcen,argp->stepfactor);
	} else {
		//drv_l1_CdspSetLensCmpEn(DISABLE);
		drv_l1_IspSetLenCmp(DISABLE,0);
	}

	if(raw_flag) {
		drv_l1_CdspSetLensCmpPath(0);
	} else {
		drv_l1_CdspSetLensCmpPath(1);
	}
}

/**
 * @brief	cdsp get lens compenasation
 * @param	argp[out]: lens compensation struct
 * @return	none
 */
void drv_l2_CdspGetLensCmp(gpCdspLensCmp_t *argp)
{
//	drv_l1_CdspGetLensCmpPos(&argp->centx, &argp->centy, &argp->xmoffset, &argp->ymoffset);
//	drv_l1_CdspGetLensCmp(&argp->stepfactor, &argp->xminc, &argp->ymoinc, &argp->ymeinc);
	argp->lcen = drv_l1_CdspGetLensCmpEn();
	argp->maxtan8_table = NULL;
	argp->slop4_table = NULL;
	argp->clpoint_table = NULL;
	argp->rdiusfile0_table = NULL;
	argp->rdiusfile1_table = NULL;
}

/**
 * @brief	cdsp set white balance
 * @param	argp[in]: white balance struct
 * @return	none
 */
void drv_l2_CdspSetWhiteBalance(gpCdspWhtBal_t *argp)
{
	//DBG_PRINT("R gain = %d, B gain = %d\r\n", argp->rgain, argp->bgain);
	drv_l1_CdspSetWbGain(argp->rgain, argp->grgain, argp->bgain, argp->gbgain);
	drv_l1_CdspSetWbGainEn(argp->wbgainen); 
	if(argp->wbgainen) {
		drv_l1_CdspSetGlobalGain(argp->global_gain);
	}
}

/**
 * @brief	cdsp get white balance
 * @param	argp[out]: white balance struct
 * @return	none
 */
void drv_l2_CdspGetWhiteBalance(gpCdspWhtBal_t *argp)
{
	argp->wbgainen = drv_l1_CdspGetWbGain(&argp->rgain, &argp->grgain, &argp->bgain, &argp->gbgain);
	argp->global_gain =	drv_l1_CdspGetGlobalGain();
}

/**
 * @brief	cdsp set lut gamma table
 * @param	argp[in]: lut gamma struct
 * @return	none
 */
void drv_l2_CdspSetLutGamma(gpCdspGamma_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->lut_gamma_en);
	if(argp->lut_gamma_en)
	{
		if(argp->gamma_table) {
			drv_l1_CdspSetGammaTable(argp->gamma_table);
		} else {
			drv_l1_CdspSetGammaTable((unsigned int *)g_gamma_table);
		}
		drv_l1_CdspSetLutGammaEn(argp->lut_gamma_en);
	} else {
		drv_l1_CdspSetLutGammaEn(DISABLE);
	}
}

/**
 * @brief	cdsp get lut gamma table
 * @param	argp[out]: lut gamma struct
 * @return	none
 */
void drv_l2_CdspGetLutGamma(gpCdspGamma_t *argp)
{
	argp->lut_gamma_en = drv_l1_CdspGetLutGammaEn();
	argp->gamma_table = NULL;
}

/**
 * @brief	cdsp set lut gamma table
 * @param	width[in]: input width
 * @param	argp[in]: interpolution struct 
 * @return	none
 */
void drv_l2_CdspSetIntpl(INT16U width, gpCdspIntpl_t *argp)
{
#if 1
	/* down mirror enable */
	drv_l1_CdspSetIntplMirEn(0xF, 1, 0);
	/* down mirror, need enable extline */
	drv_l1_CdspSetLineCtrl(0);
	drv_l1_CdspSetExtLine(width, 0x280);
	drv_l1_CdspSetExtLinePath(1, 0);
#else
	/* down mirror disable */
	drv_l1_CdspSetIntplMirEn(0x7, 0, 0);
	drv_l1_CdspSetLineCtrl(0);
	drv_l1_CdspSetExtLine(width, 0x280);
	drv_l1_CdspSetExtLinePath(0, 0);
#endif
	drv_l1_CdspSetIntplThr(argp->int_low_thr, argp->int_hi_thr);
	drv_l1_CdspSetRawSpecMode(argp->rawspecmode);

	/* disbale suppression */
	drv_l1_CdspSetUvSupprEn(DISABLE);
}

/**
 * @brief	cdsp get lut gamma table
 * @param	argp[out]: interpolution struct 
 * @return	none
 */
void drv_l2_CdspGetIntpl(gpCdspIntpl_t *argp)
{
	drv_l1_CdspGetIntplThr(&argp->int_low_thr, &argp->int_hi_thr);
	argp->rawspecmode = drv_l1_CdspGetRawSpecMode();
}

/**
 * @brief	cdsp set edge
 * @param	raw_flag[in]: raw data flag
 * @param	argp[in]: edge struct 
 * @return	none
 */
void drv_l2_CdspSetEdge(INT8U raw_flag, gpCdspEdge_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->edgeen);
	if(argp->edgeen) {
		if(argp->edge_table) {
			drv_l1_CdspSetEdgeLutTable(argp->edge_table);
		} else {
			drv_l1_CdspSetEdgeLutTable((unsigned char *)g_edge_table);
		}
		
		drv_l1_CdspSetEdgeLutTableEn(argp->eluten);
		drv_l1_CdspSetEdgeEn(argp->edgeen);
		drv_l1_CdspSetEdgeLCoring(argp->lhdiv, argp->lhtdiv, argp->lhcoring, argp->lhmode);
		drv_l1_CdspSetEdgeAmpga(argp->ampga);
		drv_l1_CdspSetEdgeDomain(argp->edgedomain);
		drv_l1_CdspSetEdgeQthr(argp->Qthr);

		if(raw_flag) {
			drv_l1_CdspSetEdgeSrc(0);
		} else {
			drv_l1_CdspSetEdgeSrc(1);
		}
		
		/*3x3 programing matrix */
		if(argp->lhmode == 0) {
			edge_filter_t LPF;

			LPF.LPF00 = argp->lf00;
			LPF.LPF01 = argp->lf01;
			LPF.LPF02 = argp->lf02;
			
			LPF.LPF10 = argp->lf10;
			LPF.LPF11 = argp->lf11;
			LPF.LPF12 = argp->lf12;

			LPF.LPF20 = argp->lf20;
			LPF.LPF21 = argp->lf21;
			LPF.LPF22 = argp->lf22;
			drv_l1_CdspSetEdgeFilter(&LPF);
		}
	} else {
		drv_l1_CdspSetEdgeEn(DISABLE);
		drv_l1_CdspSetEdgeLutTableEn(DISABLE);
	}
}

/**
 * @brief	cdsp get edge
 * @param	argp[out]: interpolution struct 
 * @return	none
 */
void drv_l2_CdspGetEdge(gpCdspEdge_t *argp)
{
	edge_filter_t LPF;
	
	argp->eluten = drv_l1_CdspGetEdgeLutTableEn();
	argp->edge_table = NULL;
	argp->edgeen = drv_l1_CdspGetEdgeEn();
	drv_l1_CdspGetEdgeLCoring(&argp->lhdiv, &argp->lhtdiv, &argp->lhcoring, &argp->lhmode);
	argp->ampga = drv_l1_CdspGetEdgeAmpga();
	argp->edgedomain = drv_l1_CdspGetEdgeDomain();
	argp->Qthr = drv_l1_CdspGetEdgeQCnt();

	drv_l1_CdspGetEdgeFilter(&LPF);
	argp->lf00 = LPF.LPF00;
	argp->lf01 = LPF.LPF01;
	argp->lf02 = LPF.LPF02;

	argp->lf10 = LPF.LPF10;
	argp->lf11 = LPF.LPF11;
	argp->lf12 = LPF.LPF12;

	argp->lf20 = LPF.LPF20;
	argp->lf21 = LPF.LPF21;
	argp->lf22 = LPF.LPF22;
}

/**
 * @brief	cdsp set color matrix
 * @param	argp[in]: color matrix struct 
 * @return	none
 */
void drv_l2_CdspSetColorMatrix(gpCdspCorMatrix_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->colcorren);
	if(argp->colcorren) {
		color_matrix_t CM;

		CM.CcCof00 = argp->a11;
		CM.CcCof01 = argp->a12;
		CM.CcCof02 = argp->a13;

		CM.CcCof10 = argp->a21;
		CM.CcCof11 = argp->a22;
		CM.CcCof12 = argp->a23;

		CM.CcCof20 = argp->a31;
		CM.CcCof21 = argp->a32;
		CM.CcCof22 = argp->a33;
		drv_l1_CdspSetColorMatrix(&CM);
		drv_l1_CdspSetColorMatrixEn(argp->colcorren);
	} else {
		drv_l1_CdspSetColorMatrixEn(DISABLE);
	}
}

/**
 * @brief	cdsp get color matrix
 * @param	argp[out]: color matrix struct 
 * @return	none
 */
void drv_l2_CdspGetColorMatrix(gpCdspCorMatrix_t *argp)
{
	color_matrix_t CM;

	drv_l1_CdspGetColorMatrix(&CM);
	argp->a11 = CM.CcCof00;
	argp->a12 = CM.CcCof01;
	argp->a13 = CM.CcCof02;

	argp->a21 = CM.CcCof10;
	argp->a22 = CM.CcCof11;
	argp->a23 = CM.CcCof12;
	
	argp->a31 = CM.CcCof20;
	argp->a32 = CM.CcCof21;
	argp->a33 = CM.CcCof22;
	argp->colcorren = drv_l1_CdspGetColorMatrixEn();
}

/**
 * @brief	cdsp set RGB to YUV parameter
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetRgbToYuv(gpCdspRgb2Yuv_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->uvdiven);
	drv_l1_CdspSetPreRBClamp(argp->pre_rbclamp);
	drv_l1_CdspSetRBClamp(argp->rbclampen, argp->rbclamp);
	if(argp->uvdiven) {
		uv_divide_t UVDivide;

		UVDivide.YT1 = argp->Yvalue_T1;
		UVDivide.YT2 = argp->Yvalue_T2;
		UVDivide.YT3 = argp->Yvalue_T3;
		UVDivide.YT4 = argp->Yvalue_T4;
		UVDivide.YT5 = argp->Yvalue_T5;
		UVDivide.YT6 = argp->Yvalue_T6;
		drv_l1_CdspSetUvDivide(&UVDivide);
		drv_l1_CdspSetUvDivideEn(argp->uvdiven);
	} else {
		drv_l1_CdspSetUvDivideEn(DISABLE);
	}
}

/**
 * @brief	cdsp get RGB to YUV parameter
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetRgbToYuv(gpCdspRgb2Yuv_t *argp)
{
	uv_divide_t UVDivide;
	
	argp->pre_rbclamp = drv_l1_CdspGetPreRBClamp();
	drv_l1_CdspGetRBClamp(&argp->rbclampen, (unsigned char *)&argp->rbclamp);
	argp->uvdiven = gpHalCdspGetUvDivideEn();
	drv_l1_CdspGetUvDivide(&UVDivide);
	argp->Yvalue_T1 = UVDivide.YT1;
	argp->Yvalue_T2 = UVDivide.YT2;
	argp->Yvalue_T3 = UVDivide.YT3;
	argp->Yvalue_T4 = UVDivide.YT4;
	argp->Yvalue_T5 = UVDivide.YT5;
	argp->Yvalue_T6 = UVDivide.YT6;
}

/**
 * @brief	cdsp set YUV 444 insert
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetYuv444Insert(gpCdspYuvInsert_t *argp)
{
	INT8U y_value, u_value, v_value;

	DEBUG("%s = %d\r\n", __FUNCTION__, argp->yuv444_insert);
	y_value = ((argp->y_corval & 0x0F) << 4)|(argp->y_coring & 0xF);
	u_value = ((argp->u_corval & 0x0F) << 4)|(argp->u_coring & 0xF);
	v_value = ((argp->v_corval & 0x0F) << 4)|(argp->v_coring & 0xF);
	drv_l1_CdspSetYuv444InsertEn(argp->yuv444_insert);
	drv_l1_CdspSetYuvCoring(y_value, u_value, v_value);
}

/**
 * @brief	cdsp get YUV444 insert
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetYuv444Insert(gpCdspYuvInsert_t *argp)
{
	INT8U y_value, u_value, v_value;
	
	argp->yuv444_insert = drv_l1_CdspGetYuv444InsertEn();
	drv_l1_CdspGetYuvCoring(&y_value, &u_value, &v_value);
	argp->y_coring = y_value & 0x0F;
	argp->y_corval = (y_value >> 4) & 0x0F;
	argp->u_coring = u_value & 0x0F;
	argp->u_corval = (u_value >> 4) & 0x0F;
	argp->v_coring = v_value & 0x0F;
	argp->v_corval = (v_value >> 4) & 0x0F;
}

/**
 * @brief	cdsp set YUV Horization average
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetYuvHavg(gpCdspYuvHAvg_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, 0x03);
	drv_l1_CdspSetYuvHAvg(0x03, argp->ytype, argp->utype, argp->vtype);
}

/**
 * @brief	cdsp get YUV Horization average
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetYuvHavg(gpCdspYuvHAvg_t *argp)
{
	drv_l1_CdspGetYuvHAvg(&argp->reserved, &argp->ytype, &argp->utype, &argp->vtype);
}

/**
 * @brief	cdsp set YUV special mode
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetSpecialMode(gpCdspSpecMod_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->yuvspecmode);
	if(argp->yuvspecmode ==  2) {
		drv_l1_CdspSetBinarizeModeThr(argp->binarthr);
	}
	
	drv_l1_CdspSetYuvSpecMode(argp->yuvspecmode);
}

/**
 * @brief	cdsp get YUV special mode
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetSpecialMode(gpCdspSpecMod_t *argp)
{
	argp->binarthr = drv_l1_CdspGetBinarizeModeThr();
	argp->yuvspecmode = drv_l1_CdspGetYuvSpecMode();
}

/**
 * @brief	cdsp set saturation and hue
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetSatHue(gpCdspSatHue_t *argp)
{
	drv_l1_CdspSetYuvSPHue(argp->u_huesindata, argp->u_huecosdata, argp->v_huesindata, argp->v_huecosdata);
	drv_l1_CdspSetYuvSPEffOffset(argp->y_offset, argp->u_offset, argp->v_offset);
	drv_l1_CdspSetYuvSPEffScale(argp->y_scale, argp->u_scale, argp->v_scale);
	drv_l1_CdspSetBriContEn(argp->YbYcEn);
}

/**
 * @brief	cdsp get saturation and hue 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetSatHue(gpCdspSatHue_t *argp)
{
	drv_l1_CdspGetYuvSPHue(&argp->u_huesindata, &argp->u_huecosdata, &argp->v_huesindata, &argp->v_huecosdata);
	drv_l1_CdspGetYuvSPEffOffset(&argp->y_offset, &argp->u_offset, &argp->v_offset);
	drv_l1_CdspGetYuvSPEffScale(&argp->y_scale, &argp->u_scale, &argp->v_scale);
	argp->YbYcEn = drv_l1_CdspGetBriContEn();
}

/**
 * @brief	cdsp set suppression
 * @param	width[in]: input width
 * @param	argp[in]: parameter struct 
 * @param	edge[in]: edge parameter struct 
 * @return	none
 */
void drv_l2_CdspSetSuppression(INT16U width, gpCdspSuppression_t *argp, gpCdspEdge_t *edge)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->suppressen);
	if(argp->suppressen) {
	#if 1
		/* down mirror enable */
		drv_l1_CdspSetUvSuppr(1, 1, 0xF);
		/* use down mirror must enable extline */
		drv_l1_CdspSetLineCtrl(1);
		drv_l1_CdspSetExtLine(width, 0x280);
		drv_l1_CdspSetExtLinePath(1, 1);
	#else
		/* down mirror disable */
		drv_l1_CdspSetUvSuppr(1, 1, 0xD);
		drv_l1_CdspSetLineCtrl(1);
		drv_l1_CdspSetExtLine(width, 0x280);
		drv_l1_CdspSetExtLinePath(0, 0);
	#endif
		drv_l1_CdspSetUvSupprEn(ENABLE);
		if(argp->suppr_mode >  2) {
			argp->suppr_mode = 2;
		}

		switch(argp->suppr_mode)
		{
		case 0:
			drv_l1_CdspSetYDenoiseEn(DISABLE);
			drv_l1_CdspSetYLPFEn(DISABLE);
			drv_l2_CdspSetEdge(0, edge);
			break;
		case 1:
			DEBUG("%s: enable denoise\r\n", __FUNCTION__);
			drv_l1_CdspSetEdgeEn(DISABLE);
			drv_l1_CdspSetEdgeLutTableEn(DISABLE);
			drv_l1_CdspSetYLPFEn(DISABLE);
			drv_l1_CdspSetYDenoise(argp->denoisethrl, argp->denoisethrwth, argp->yhtdiv);
			drv_l1_CdspSetYDenoiseEn(argp->denoisen);
			break;
		case 2:
			drv_l1_CdspSetEdgeEn(DISABLE);
			drv_l1_CdspSetEdgeLutTableEn(DISABLE);
			drv_l1_CdspSetYDenoiseEn(DISABLE);
			drv_l1_CdspSetYLPFEn(argp->lowyen);
			break;
		}
	} else {
		/* dow mirror disable */
		drv_l1_CdspSetUvSuppr(0, 0, 0);
		drv_l1_CdspSetLineCtrl(0);
		drv_l1_CdspSetExtLine(width, 0x280);
		drv_l1_CdspSetExtLinePath(0, 0);

		drv_l1_CdspSetUvSupprEn(DISABLE);
		drv_l1_CdspSetEdgeEn(DISABLE);
		drv_l1_CdspSetEdgeLutTableEn(DISABLE);
		drv_l1_CdspSetYDenoiseEn(DISABLE);
		drv_l1_CdspSetYLPFEn(DISABLE);
	}
}

/**
 * @brief	cdsp get suppression 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetSuppression(gpCdspSuppression_t *argp)
{
	drv_l1_CdspGetYDenoise(&argp->denoisethrl, &argp->denoisethrwth, &argp->yhtdiv);
	argp->denoisen = drv_l1_CdspGetYDenoiseEn();
	argp->lowyen = drv_l1_CdspGetYLPFEn();
}

/**
 * @brief	cdsp set new denoise 
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetNewDenoise(gpCdspNewDenoise_t *argp)
{	
	if(argp->newdenoiseen) {
		drv_l2_Set_Size_before_NewDenoise(target_sensor_index, ENABLE);
		if(argp->ndeluten) {
			drv_l1_CdspSetNewDEdgeLut((unsigned char *)g_nd_edge_table);
		} else {
			drv_l1_CdspSetNewDEdgeLut(argp->ndedge_table);		
		}
		
		drv_l1_CdspSetNewDenoiseEn(argp->newdenoiseen);
		drv_l1_CdspSetNewDenoise(argp->ndmirvsel, argp->ndmiren);
		drv_l1_CdspSetNdEdgeEn(argp->ndedgeen, argp->ndeluten);
		drv_l1_CdspSetNdEdgeLCoring(argp->ndlhdiv, argp->ndlhtdiv, argp->ndlhcoring);
		drv_l1_CdspSetNdEdgeAmpga(argp->ndampga);
		

		/*3x3 programing matrix */
		if(argp->ndlhmode == 0) {
			edge_filter_t NDEdgeFilter;

			NDEdgeFilter.LPF00 = argp->ndlf00;
			NDEdgeFilter.LPF01 = argp->ndlf01;
			NDEdgeFilter.LPF02 = argp->ndlf02;
			
			NDEdgeFilter.LPF10 = argp->ndlf10;
			NDEdgeFilter.LPF11 = argp->ndlf11;
			NDEdgeFilter.LPF12 = argp->ndlf12;

			NDEdgeFilter.LPF20 = argp->ndlf20;
			NDEdgeFilter.LPF21 = argp->ndlf21;
			NDEdgeFilter.LPF22 = argp->ndlf22;
			drv_l1_CdspSetNdEdgeFilter(&NDEdgeFilter);
		}
	} else {
		drv_l2_Set_Size_before_NewDenoise(target_sensor_index, DISABLE);	
		drv_l1_CdspSetNewDenoiseEn(DISABLE);
		drv_l1_CdspSetNdEdgeEn(DISABLE, DISABLE);
	}
}

/**
 * @brief	cdsp get new denoise 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetNewDenoise(gpCdspNewDenoise_t *argp)
{
	edge_filter_t NDEdgeFilter;
	
	argp->newdenoiseen = drv_l1_CdspGetNewDenoiseEn();
	drv_l1_CdspGetNewDenoise(&argp->ndmirvsel, &argp->ndmiren);
	drv_l1_CdspGetNdEdgeEn(&argp->ndedgeen, &argp->ndeluten);
	DEBUG("%s: ndedgeen = %d, ndeluten = %d\r\n", __FUNCTION__, argp->ndedgeen, argp->ndeluten);
	
	drv_l1_CdspGetNdEdgeLCoring(&argp->ndlhdiv, &argp->ndlhtdiv, &argp->ndlhcoring);
	argp->ndampga =	drv_l1_CdspGetNdEdgeAmpga();
		
	drv_l1_CdspGetNdEdgeFilter(&NDEdgeFilter);
	argp->ndlf00 = NDEdgeFilter.LPF00;
	argp->ndlf01 = NDEdgeFilter.LPF01;
	argp->ndlf02 = NDEdgeFilter.LPF02;
	
	argp->ndlf10 = NDEdgeFilter.LPF10;
	argp->ndlf11 = NDEdgeFilter.LPF11;
	argp->ndlf12 = NDEdgeFilter.LPF12;

	argp->ndlf20 = NDEdgeFilter.LPF20;
	argp->ndlf21 = NDEdgeFilter.LPF21;
	argp->ndlf22 = NDEdgeFilter.LPF22;
}

/**
 * @brief	cdsp set raw window 
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetRawWin(INT16U width, INT16U height, gpCdspRawWin_t *argp)
{
	unsigned int x, y;

	if(argp->hwdoffset == 0) {
		argp->hwdoffset = 1;
	}
	
	if(argp->vwdoffset == 0) {
		argp->vwdoffset = 1;
	}
		
	if(argp->aeawb_src == 0) {
		x = argp->hwdoffset + argp->hwdsize * 8;
		y = argp->vwdoffset + argp->vwdsize * 8;
		if(x >= width) {
			x = width - argp->hwdoffset;
			argp->hwdsize = x / 8;
		}

		if(y >= height) {
			y = height - argp->vwdoffset;
			argp->vwdsize = y / 8;
		}
	} else {
		x = argp->hwdoffset*2 + argp->hwdsize*2 * 8;
		y = argp->vwdoffset*2 + argp->vwdsize*2 * 8;
		if(x >= width) {
			x = width - argp->hwdoffset*2;
			argp->hwdsize = x / 8;
			argp->hwdsize >>= 1;
		}

		if(y >= height) {
			y = height - argp->vwdoffset*2;
			argp->vwdsize = y / 8;
			argp->vwdsize >>= 1;
		}
	}

	
	DBG_PRINT("AeWinTest = %d\r\n", argp->AeWinTest);
	DBG_PRINT("RawWinOffset[%d,%d]\r\n", argp->hwdoffset, argp->vwdoffset);
	DBG_PRINT("RawWinCellSize[%d,%d]\r\n", argp->hwdsize, argp->vwdsize);
	
	drv_l1_CdspSetAeAwbSrc(argp->aeawb_src);
	drv_l1_CdspSetAeAwbSubSample(argp->subsample);
	drv_l1_CdspSetRGBWin(argp->hwdoffset, argp->vwdoffset, argp->hwdsize, argp->vwdsize);
	drv_l1_CdspSet3ATestWinEn(argp->AeWinTest, argp->AfWinTest);
}

/**
 * @brief	cdsp get raw window 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetRawWin(gpCdspRawWin_t *argp)
{
	argp->aeawb_src = drv_l1_CdspGetAeAwbSrc();
	argp->subsample = drv_l1_CdspGetAeAwbSubSample();
	//drv_l1_CdspGet3ATestWinEn(&argp->AeWinTest, &argp->AfWinTest);
	drv_l1_CdspGetRGBWin(&argp->hwdoffset, &argp->vwdoffset, &argp->hwdsize, &argp->vwdsize);
}

/**
 * @brief	cdsp set af windows
 * @param	width[in]: input width 
 * @param	height[in]: input height 
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetAF(INT16U width, INT16U height, gpCdspAF_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->af_win_en);
	if(argp->af_win_en) {
		unsigned int x, y;
		/* af1 */
		if(argp->af1_hsize >= width) {
			argp->af1_hsize = width;
		}

		if(argp->af1_vsize >= height) {
			argp->af1_hsize = height;
		}
		
		x = argp->af1_hoffset + argp->af1_hsize;
		y = argp->af1_voffset + argp->af1_vsize;
		if(x >= width) {
			argp->af1_hoffset = width - 1 - argp->af1_hsize;
		}
		
		if(y >= height) {
			argp->af1_voffset = height - 1 - argp->af1_vsize;
		}
		
		/* af2 */
		if(argp->af2_hsize >= width) {
			argp->af2_hsize = width;
		}

		if(argp->af2_vsize >= height) {
			argp->af2_hsize = height;
		}

		if(argp->af2_hsize < 64) {
			argp->af2_hsize = 64;
		}
		
		if(argp->af2_vsize < 64) {
			argp->af2_vsize = 64;
		}
		
		x = argp->af2_hoffset + argp->af2_hsize;
		y = argp->af2_voffset + argp->af2_vsize;
		if(x >= width) {
			argp->af2_hoffset = width - 1 - argp->af2_hsize;
		}

		if(y >= height) {
			argp->af2_voffset = height - 1 - argp->af2_vsize;
		}
		
		/* af3 */
		if(argp->af3_hsize >= width) {
			argp->af3_hsize = width;
		}
		
		if(argp->af3_vsize >= height) {
			argp->af3_hsize = height;
		}

		if(argp->af3_hsize < 64) {
			argp->af3_hsize = 64;
		}

		if(argp->af3_vsize < 64) {
			argp->af3_vsize = 64;
		}
		
		x = argp->af3_hoffset + argp->af3_hsize;
		y = argp->af3_voffset + argp->af3_vsize;
		if(x >= width) {
			argp->af3_hoffset = width - 1 - argp->af3_hsize;
		}

		if(y >= height) {
			argp->af3_voffset = height - 1 - argp->af3_vsize;
		}

		DEBUG("Af1Offset[%d,%d]\r\n", argp->af1_hoffset, argp->af1_voffset);
		DEBUG("Af1Size[%d,%d]\r\n", argp->af1_hsize, argp->af1_vsize);
		DEBUG("Af2Offset[%d,%d]\r\n", argp->af2_hoffset, argp->af2_voffset);
		DEBUG("Af2Size[%d,%d]\r\n", argp->af2_hsize, argp->af2_vsize);
		DEBUG("Af3Offset[%d,%d]\r\n", argp->af3_hoffset, argp->af3_voffset);
		DEBUG("Af3Size[%d,%d]\r\n", argp->af3_hsize, argp->af3_vsize);
		
		drv_l1_CdspSetAfWin1(argp->af1_hoffset, argp->af1_voffset, argp->af1_hsize, argp->af1_hsize);
		drv_l1_CdspSetAfWin2(argp->af2_hoffset, argp->af2_voffset, argp->af2_hsize, argp->af2_hsize);
		drv_l1_CdspSetAfWin3(argp->af3_hoffset, argp->af3_voffset, argp->af3_hsize, argp->af3_hsize);
		drv_l1_CdspSetAFEn(argp->af_win_en, argp->af_win_hold);
		drv_l1_CdspSetIntEn(ENABLE, CDSP_AFWIN_UPDATE);
	} else {
		drv_l1_CdspSetAFEn(DISABLE, DISABLE);
		drv_l1_CdspSetIntEn(DISABLE, CDSP_AFWIN_UPDATE);
	}
}

/**
 * @brief	cdsp get af window 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetAF(gpCdspAF_t *argp)
{
	//drv_l1_CdspGetAfWin1(&argp->af1_hoffset, &argp->af1_voffset, &argp->af1_hsize, &argp->af1_hsize);
	//drv_l1_CdspGetAfWin2(&argp->af2_hoffset, &argp->af2_voffset, &argp->af2_hsize, &argp->af2_hsize);
	//drv_l1_CdspGetAfWin3(&argp->af3_hoffset, &argp->af3_voffset, &argp->af3_hsize, &argp->af3_hsize);
	drv_l1_CdspGetAFEn(&argp->af_win_en, &argp->af_win_hold);
}

/**
 * @brief	cdsp set ae windows
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetAE(gpCdspAE_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->ae_win_en);
	if(argp->ae_win_en) {
		DEBUG("AeWinAddr0 = 0x%x.\r\n", CdspDev->aeAddr0);
		DEBUG("AeWinAddr1 = 0x%x.\r\n", CdspDev->aeAddr1);
		drv_l1_CdspSetAEBuffAddr((INT32U)CdspDev->aeAddr0, (INT32U)CdspDev->aeAddr1);
		drv_l1_CdspSetAEEn(argp->ae_win_en, argp->ae_win_hold);
		drv_l1_CdspSetAEWin(argp->phaccfactor, argp->pvaccfactor);
		drv_l1_CdspSetIntEn(ENABLE, CDSP_AEWIN_SEND);
	} else {
		drv_l1_CdspSetAEEn(DISABLE, DISABLE);
		drv_l1_CdspSetIntEn(DISABLE, CDSP_AEWIN_SEND);
	}
}

/**
 * @brief	cdsp get ae window 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetAE(gpCdspAE_t *argp)
{
	drv_l1_CdspGetAEEn(&argp->ae_win_en, &argp->ae_win_hold);
	//drv_l1_CdspGetAEWin(&argp->phaccfactor, &argp->pvaccfactor);
}

/**
 * @brief	cdsp set awb windows
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetAWB(gpCdspAWB_t *argp)
{
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->awb_win_en);
	if(argp->awb_win_en) {
		awb_uv_thr_t UVthr;

		UVthr.UL1N1 = argp->UL1N1;
		UVthr.UL1P1 = argp->UL1P1;
		UVthr.VL1N1	= argp->VL1N1;
		UVthr.VL1P1 = argp->VL1P1;

		UVthr.UL1N2 = argp->UL1N2;
		UVthr.UL1P2 = argp->UL1P2;
		UVthr.VL1N2	= argp->VL1N2;
		UVthr.VL1P2 = argp->VL1P2;

		UVthr.UL1N3 = argp->UL1N3;
		UVthr.UL1P3 = argp->UL1P3;
		UVthr.VL1N3	= argp->VL1N3;
		UVthr.VL1P3 = argp->VL1P3;
		drv_l1_CdspSetAWB(argp->awbclamp_en, argp->sindata, argp->cosdata, argp->awbwinthr);
		drv_l1_CdspSetAwbYThr(argp->Ythr0, argp->Ythr1, argp->Ythr2, argp->Ythr3);
		drv_l1_CdspSetAwbUVThr(&UVthr);
		drv_l1_CdspSetAWBEn(argp->awb_win_en, argp->awb_win_hold);
		drv_l1_CdspSetIntEn(ENABLE, CDSP_AWBWIN_UPDATE);
	} else {
		drv_l1_CdspSetAWBEn(DISABLE, DISABLE);
		drv_l1_CdspSetIntEn(DISABLE, CDSP_AWBWIN_UPDATE);
	}
}

/**
 * @brief	cdsp get awb window 
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetAWB(gpCdspAWB_t *argp)
{
	//awb_uv_thr_t UVthr;

	//drv_l1_HalCdspGetAWB(&argp->awbclamp_en, &argp->sindata, &argp->cosdata, &argp->awbwinthr);
	//drv_l1_HalCdspGetAwbYThr(&argp->Ythr0, &argp->Ythr1, &argp->Ythr2, &argp->Ythr3);
	//drv_l1_HalCdspGetAwbUVThr(&UVthr);
	//drv_l1_HalCdspGetAWBEn(&argp->awb_win_en, &argp->awb_win_hold);
	
	//argp->UL1N1 = UVthr.UL1N1;
	//argp->UL1P1 = UVthr.UL1P1;
	//argp->VL1N1 = UVthr.VL1N1;
	//argp->VL1P1 = UVthr.VL1P1;

	//argp->UL1N2 = UVthr.UL1N2;
	//argp->UL1P2 = UVthr.UL1P2;
	//argp->VL1N2 = UVthr.VL1N2;
	//argp->VL1P2 = UVthr.VL1P2;

	//argp->UL1N3 = UVthr.UL1N3;
	//argp->UL1P3 = UVthr.UL1P3;
	//argp->VL1N3 = UVthr.VL1N3;
	//argp->VL1P3 = UVthr.VL1P3;
}

/**
 * @brief	cdsp set gain 2
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetWBGain2(gpCdspWbGain2_t *argp)
{
	//DEBUG("%s = %d\r\n", __FUNCTION__, argp->wbgain2en);
	if(argp->wbgain2en) {
		drv_l1_CdspSetWbGain2(argp->rgain2, argp->ggain2, argp->bgain2);
		drv_l1_CdspSetWbGain2En(ENABLE);
	} else {
		drv_l1_CdspSetWbGain2En(DISABLE);
	}
}

/**
 * @brief	cdsp get gain 2
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetWBGain2(gpCdspWbGain2_t *argp)
{
	drv_l1_CdspGetWbGain2(&argp->wbgain2en, &argp->rgain2, &argp->ggain2, &argp->bgain2);
}

/**
 * @brief	cdsp set gain Histgm
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetHistgm(gpCdspHistgm_t *argp)
{
	if(argp->his_en) {
		drv_l1_CdspSetHistgm(argp->hislowthr, argp->hishighthr);
		drv_l1_CdspSetHistgmEn(ENABLE, argp->his_hold_en);
	} else {
		drv_l1_CdspSetHistgmEn(DISABLE, DISABLE);
	}
}

/**
 * @brief	cdsp get Histgm
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetHistgm(gpCdspHistgm_t *argp)
{
	drv_l1_CdspGetHistgm((INT32U *)&argp->hislowthr, (INT32U *)&argp->hishighthr);
	//drv_l1_CdspGetHistgmEn(&argp->his_en, &argp->his_hold_en);
}

/**
 * @brief	cdsp set scale and crop
 * @param	argp[in]: parameter struct 
 * @return	none
 */
INT32S drv_l2_CdspSetScaleCrop(gpCdspScalePara_t *argp)
{
	INT8U clamphsizeen;
	INT16U clamphsize, src_width, src_height;
	INT32U temp;

	src_width = CdspDev->imgWidth;
	src_height = CdspDev->imgHeight;
	clamphsizeen = 0;
	clamphsize = CdspDev->imgRbWidth;

	/* raw h scale down*/
	if(CdspDev->rawFmtFlag) {
		if(argp->hscale_en && (src_width > argp->dst_hsize)) {
			DEBUG("HScaleEn\r\n");
			argp->dst_hsize &= ~(0x1); 	/* 2 align */
			src_width = argp->dst_hsize;
			clamphsize = argp->dst_hsize;
			drv_l1_CdspSetRawHScale(src_width, argp->dst_hsize);
			drv_l1_CdspSetRawHScaleEn(argp->hscale_en, argp->hscale_mode);
		} else {
			argp->hscale_en = 0;
			drv_l1_CdspSetRawHScaleEn(DISABLE, argp->hscale_mode);
		}
	} else {
		argp->hscale_en = 0;
		drv_l1_CdspSetRawHScaleEn(DISABLE, argp->hscale_mode);
	}

	/* crop */
	if((argp->hscale_en == 0) &&
		argp->crop_en &&
		(src_width > argp->crop_hsize) &&
		(src_height > argp->crop_vsize)) {
		
		DEBUG("CropEn\r\n");
		if(argp->crop_hoffset == 0) {
			argp->crop_hoffset = 1;
		}

		if(argp->crop_voffset == 0) {
			argp->crop_voffset = 1;
		}
		
		temp = argp->crop_hoffset + argp->crop_hsize;
		if(temp > src_width) {
			argp->crop_hsize = src_width - argp->crop_hoffset;
		}
		
		temp = argp->crop_voffset + argp->crop_vsize;
		if(temp > src_height) {
			argp->crop_vsize = src_height - argp->crop_voffset;
		}
		
		src_width = argp->crop_hsize;
		src_height = argp->crop_vsize;
		clamphsize = argp->crop_hsize;
		drv_l1_CdspSetCrop(argp->crop_hoffset, argp->crop_voffset, argp->crop_hsize, argp->crop_vsize);
		drv_l1_CdspSetCropEn(argp->crop_en);
	} else {
		argp->crop_en = 0;
		drv_l1_CdspSetCropEn(DISABLE);
	}

	/* yuv h scale down*/
	if(argp->yuvhscale_en && (src_width > argp->yuv_dst_hsize)) {
		DEBUG("YuvHScaleEn, %d -> %d\r\n", src_width, argp->yuv_dst_hsize);
		if(argp->yuv_dst_hsize > argp->img_rb_h_size) {
			argp->yuv_dst_hsize = argp->img_rb_h_size;
		}
		
		argp->yuv_dst_hsize &= ~(0x1); 	/* 2 align */
		clamphsizeen = 1;
		clamphsize = argp->yuv_dst_hsize;
		temp = (argp->yuv_dst_hsize<<16)/src_width + 1;
		drv_l1_CdspSetYuvHScale(temp, temp);
		drv_l1_CdspSetYuvHScaleEn(ENABLE, argp->yuvhscale_mode);
	} else if(src_width > argp->img_rb_h_size) {
		DEBUG("YuvHScaleEn1, %d -> %d\r\n", src_width, argp->img_rb_h_size);
		argp->yuv_dst_hsize &= ~(0x1); 	/* 2 align */
		clamphsizeen = 1;
		clamphsize = argp->img_rb_h_size;
		temp = (argp->img_rb_h_size<<16)/src_width + 1;
		drv_l1_CdspSetYuvHScale(temp, temp);
		drv_l1_CdspSetYuvHScaleEn(ENABLE, argp->yuvhscale_mode);
	} else {
		argp->yuvhscale_en = 0;
		drv_l1_CdspSetYuvHScaleEn(DISABLE, argp->yuvhscale_mode);
	}

	/* yuv v scale down*/
	if(argp->yuvvscale_en && (src_height > argp->yuv_dst_vsize)) {
		DEBUG("YuvVScaleEn, %d -> %d\r\n", src_height, argp->yuv_dst_vsize);
		if(argp->yuv_dst_vsize >  argp->img_rb_v_size) {
			argp->yuv_dst_vsize = argp->img_rb_v_size;
		}
		
		argp->yuv_dst_vsize &= ~(0x1); 	/* 2 align */
		temp = (argp->yuv_dst_vsize<<16)/src_height + 1;
		drv_l1_CdspSetYuvVScale(temp, temp);
		drv_l1_CdspSetYuvVScaleEn(ENABLE, argp->yuvvscale_mode);
	} else if(src_height > argp->img_rb_v_size) {
		DEBUG("YuvVScaleEn1, %d -> %d\r\n", src_height, argp->img_rb_v_size);
		argp->yuv_dst_vsize &= ~(0x1); 	/* 2 align */
		temp = (argp->img_rb_v_size<<16)/src_height + 1;
		drv_l1_CdspSetYuvVScale(temp, temp);
		drv_l1_CdspSetYuvVScaleEn(ENABLE, argp->yuvvscale_mode);
	} else {
		argp->yuvvscale_en = 0;
		drv_l1_CdspSetYuvVScaleEn(DISABLE, argp->yuvhscale_mode);
	}

	/* set clamp enable and clamp h size */
	drv_l1_CdspSetClampEn(clamphsizeen, clamphsize);
	CdspDev->imgRbWidth = argp->img_rb_h_size;
	CdspDev->imgRbHeight = argp->img_rb_v_size;

	/* set fifo */	
	drv_l2_CdspSetFifoSize(CdspDev->imgRbWidth);
	
	DEBUG("Clampen = %d\r\n", clamphsizeen);
	DEBUG("ClampSize = %d\r\n", clamphsize);
	DEBUG("rbSize = %dx%d\r\n", CdspDev->imgRbWidth, CdspDev->imgRbHeight);
	DEBUG("SrcSize = %dx%d\r\n", CdspDev->imgWidth, CdspDev->imgHeight);
	return 0;
}

/**
 * @brief	cdsp get crop and scale
 * @param	argp[out]: parameter struct 
 * @return	none
 */
void drv_l2_CdspGetScaleCrop(gpCdspScalePara_t *argp)
{
	//drv_l1_CdspGetRawHScaleEn(&argp->hscale_en, &argp->hscale_mode);
	//drv_l1_CdspGetCrop(&argp->crop_hoffset, &argp->crop_voffset, &argp->crop_hsize, &argp->crop_vsize);
	//argp->crop_en = drv_l1_CdspGetCropEn();
	//drv_l1_CdspGetYuvHScaleEn(&argp->yuvhscale_en, &argp->yuvhscale_mode);
	//drv_l1_CdspGetYuvVScaleEn(&argp->yuvvscale_en, &argp->yuvvscale_mode);
}

/**
 * @brief	cdsp set ae and af after scale
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetScaleAeAf(gpCdspScalePara_t *argp)
{
	unsigned short w, h;
	//gpCdspRawWin_t raw_win;
	//gpCdspAE_t ae;
	gpCdspAF_t af;

	if(argp->crop_en) {
		w = argp->crop_hsize;
		h = argp->crop_vsize;
	} else {
		w = CdspDev->imgWidth;
		h = CdspDev->imgHeight;
	}

	//gpCdspGetRawWin(&raw_win);
	//gpCdspSetRawWin(w, h, &raw_win);

	drv_l2_CdspGetAF(&af);
	drv_l2_CdspSetAF(w, h, &af);

	//gpCdspGetAE(&ae);
	//gpCdspSetAE(&ae);
}


/**
 * @brief	cdsp set linear correction
 * @param	argp[in]: parameter struct 
 * @return	none
 */
void drv_l2_CdspSetLinCorr(gpCdspLinCorr_t *argp)
{
	
	DEBUG("%s = %d\r\n", __FUNCTION__, argp->lincorren);

	if(argp->lincorren) {
		drv_l1_IspSetLinCorrEn(argp->lincorren);
	} else {
		drv_l1_IspSetLinCorrEn(argp->lincorren);
		return;
	}
	
	drv_l1_CdspSetLinCorrTable(argp->lincorr_table);
}
void drv_l2_IspSetHybridRaw(gpIspHybridRaw_t *argp)
{
	/*ISP image size*/
	drv_l1_IspSetImageSize(argp->ISPWidth, argp->IspHeight);		//¤@©w­n¶}±ҧ_«hµLªk¨ϥηsª©ªºISP
	
#if 1 //Bad pixel isp function
	drv_l1_IspSetDpcRcvModeSelThr(argp->DPCDefaultMode, argp->DPCth1, argp->DPCth2, argp->DPCth3);	//DPC_rcv_mode,DPCth1,DPCth2,DPCth3
	drv_l1_IspSetDpcEn(argp->DefectPixelEnable, argp->DefectPixelSel);
	drv_l1_IspSetSmoothFactor(argp->DPCn);	//for bad pixel

	//Crosstalk Enable/Disable
	drv_l1_IspSetCrostlkGbGrEn(argp->CrosstalkEnable, argp->CrosstalkGbGr);	//Gb&Gr
	drv_l1_IspSetCrostlkThold(argp->CrosstalkT1, argp->CrosstalkT2, argp->CrosstalkT3, argp->CrosstalkT4);
	drv_l1_IspSetCrostlkWeight(argp->CrosstalkW1, argp->CrosstalkW2, argp->CrosstalkW3);
	
	drv_l1_IspSetDenoiseEn(argp->DenoiseEnable);
	drv_l1_IspSetDenoiseThold(argp->DenoiseT1, argp->DenoiseT2, argp->DenoiseT3, argp->DenoiseT4);
	drv_l1_IspSetDenoiseWeight(argp->DenoiseW1, argp->DenoiseW2, argp->DenoiseW3);
#endif
	
#if 1	//New ISP Linearity Correction
	drv_l1_IspSetLinCorrEn(DISABLE);
#endif
}

static INT32S drv_l2_cdsp_handle_of(void)
{
	DEBUG("CDSP: overflow\r\n");
	if(CdspDev->interrupt_handle[C_ISR_OVERFLOW]) {
		CdspDev->interrupt_handle[C_ISR_OVERFLOW]();
	}
	
	return 0;
}

static INT32S drv_l2_cdsp_handle_facwr(void)
{
	DEBUG("CDSP: facwr\r\n");
	return 0;
}

static void drv_l2_cdsp_handle_postprocess(void)
{
	OSQPost(CdspDev->cdsp_wait_queue, (void *)C_WAKE_UP);
}

static void drv_l2_cdsp_handle_eof(void)
{
	/* set scale crop & skip frame */
	if(CdspDev->SkipCnt > 0) {
		if(CdspDev->SkipCnt == SKIP_CNT) {
			drv_l2_CdspSetScaleCrop(&CdspDev->scale);
		}

		CdspDev->SkipCnt--;
		return;
	}

	if(CdspDev->interrupt_handle[C_ISR_EOF]) {
		CdspDev->interrupt_handle[C_ISR_EOF]();
	}	

	//if(CdspDev->interrupt_handle[C_ISR_SENSOR] && (CdspDev->ae_awb_flag &CDSP_AE_UPDATE) != 0) {
	//	CdspDev->interrupt_handle[C_ISR_SENSOR]();
	//	CdspDev->ae_awb_flag &= (~CDSP_AE_UPDATE);
	//}	
}

static void drv_l2_cdsp_handle_ae(INT32U eof)
{
	gpCdsp3aResult_t *argp = &CdspDev->result;
	INT32U *ptr;
	INT32U i;
		
	//if((CdspDev->ae_awb_flag & (CDSP_AE_CTRL_EN | CDSP_AE_UPDATE)) != 0) {
	//	return;
	//	}

	if(eof != 0)	DBG_PRINT("1 AE ISR, flag = 0x%x, eof = 0x%x\r\n", CdspDev->ae_awb_flag, eof);
//////
	drv_l1_CdspSetAEEn(0, 1); //ae win hold
	
	if(drv_l1_CdspGetAEActBuff()) {		
		ptr = (unsigned int *)CdspDev->aeAddr1;		
	} else {
		ptr = (unsigned int *)CdspDev->aeAddr0;
	}


	{
		unsigned int *p_aewin;
		unsigned int val0, val1, val2, val3;
		
		
		
		p_aewin = (unsigned int *)argp->ae_win;
		i = 64/16;
		do {			
			val0 = *ptr++;
			val1 = *ptr++;
			val2 = *ptr++;
			val3 = *ptr++;
			*p_aewin++ = val0;
			*p_aewin++ = val1;
			*p_aewin++ = val2;			
			*p_aewin++ = val3;
			i--;
		} while(i != 0);
	}
	
	
	CdspDev->ae_awb_flag |= CDSP_AE_CTRL_EN;
	if(CdspDev->ae_awb_task_en) {
		OSQPost(CdspDev->ae_awb_wait_queue, (void *)C_WAKE_UP);
	}
	drv_l1_CdspSetAEEn(1, 0); //ae win update
}

static void drv_l2_cdsp_handle_awb(void)
{
	INT32S tempsh;
	INT32U cnt, temph, templ, tempsl;		
	AWB_RESULT_t *awb_result = &CdspDev->result.awb_result;
	
	//DEBUG("AWB ISR, flag = 0x%x\r\n", CdspDev->ae_awb_flag);
	if((CdspDev->ae_awb_flag & CDSP_AWB_CTRL_EN) != 0) {
		return;
	}
	drv_l1_CdspSetAWBEn(0,1);
		
	drv_l1_CdspGetAwbSumCnt(1, &cnt);
	awb_result->sumcnt[0] = cnt;
	drv_l1_CdspGetAwbSumCnt(2, &cnt);
	awb_result->sumcnt[1] = cnt;
	drv_l1_CdspGetAwbSumCnt(3, &cnt);
	awb_result->sumcnt[2] = cnt;

	drv_l1_CdspGetAwbSumG(1, &templ, &temph);
	awb_result->sumg[0] = temph;
	awb_result->sumg[0] <<= 16;
	awb_result->sumg[0] |= templ;
	drv_l1_CdspGetAwbSumG(2, &templ, &temph);
	awb_result->sumg[1] = temph;
	awb_result->sumg[1] <<= 16;
	awb_result->sumg[1] |= templ;
	drv_l1_CdspGetAwbSumG(3, &templ, &temph);
	awb_result->sumg[2] = temph;
	awb_result->sumg[2] <<= 16;
	awb_result->sumg[2] |= templ;

	drv_l1_CdspGetAwbSumRG(1, &tempsl, &tempsh);
	//DEBUG("tempsh = 0x%x, tempsl = 0x%x\r\n", tempsh, tempsl);	
	awb_result->sumrg[0] = tempsh;
	awb_result->sumrg[0] <<= 16;
	awb_result->sumrg[0] |= tempsl;
	//DEBUG("awb_result->sumrg[0] = 0x%010lx\r\n", awb_result->sumrg[0]);
	
	drv_l1_CdspGetAwbSumRG(2, &tempsl, &tempsh);
	//DEBUG("tempsh = 0x%x, tempsl = 0x%x\r\n", tempsh, tempsl);	
	awb_result->sumrg[1] = tempsh;
	awb_result->sumrg[1] <<= 16;
	awb_result->sumrg[1] |= tempsl;
	//DEBUG("awb_result->sumrg[1] = 0x%010lx\r\n", awb_result->sumrg[1]);	
	
	drv_l1_CdspGetAwbSumRG(3, &tempsl, &tempsh);
	//DEBUG("tempsh = 0x%x, tempsl = 0x%x\r\n", tempsh, tempsl);	
	awb_result->sumrg[2] = tempsh;
	awb_result->sumrg[2] <<= 16;
	awb_result->sumrg[2] |= tempsl;
	//DEBUG("awb_result->sumrg[2] = 0x%010lx\r\n", awb_result->sumrg[2]);	
	 

	drv_l1_CdspGetAwbSumBG(1, &tempsl, &tempsh);
	//DEBUG("tempsh = 0x%x, tempsl = 0x%x\r\n", tempsh, tempsl);	
	awb_result->sumbg[0] = tempsh;
	awb_result->sumbg[0] <<= 16;
	awb_result->sumbg[0] |= tempsl;
	//DEBUG("awb_result->sumbg[0] = 0x%llx\r\n", awb_result->sumbg[0]);
	
	drv_l1_CdspGetAwbSumBG(2, &tempsl, &tempsh);
	//DEBUG("tempsh = 0x%x, tempsl = 0x%x\r\n", tempsh, tempsl);	
	awb_result->sumbg[1] = tempsh;
	awb_result->sumbg[1] <<= 16;	
	awb_result->sumbg[1] |= tempsl;
	//DEBUG("awb_result->sumbg[1] = 0x%010lx\r\n", awb_result->sumbg[1]);
	
	drv_l1_CdspGetAwbSumBG(3, &tempsl, &tempsh);
	//DEBUG("tempsh = 0x%x, tempsl = 0x%x\r\n", tempsh, tempsl);	
	awb_result->sumbg[2] = tempsh;
	awb_result->sumbg[2] <<= 16;
	awb_result->sumbg[2] |= tempsl;
	//DEBUG("awb_result->sumbg[2] = 0x%010lx\r\n", awb_result->sumbg[2]);
	
	CdspDev->ae_awb_flag |= CDSP_AWB_CTRL_EN;
	if(CdspDev->ae_awb_task_en) {
		OSQPost(CdspDev->ae_awb_wait_queue, (void *)C_WAKE_UP);
	}
	drv_l1_CdspSetAWBEn(1,0);
}

static void drv_l2_cdsp_handle_af(void)
{
	gpCdsp3aResult_t *argp = &CdspDev->result;
	af_windows_value_t af_value;
	
	drv_l1_CdspGetAFWinVlaue(1, &af_value);
	argp->af1_h_value = (af_value.h_value_h << 16) | af_value.h_value_l;
	argp->af1_v_value = (af_value.v_value_h << 16) | af_value.v_value_l;

	drv_l1_CdspGetAFWinVlaue(2, &af_value);
	argp->af2_h_value = (af_value.h_value_h << 16) | af_value.h_value_l;
	argp->af2_v_value = (af_value.v_value_h << 16) | af_value.v_value_l;

	drv_l1_CdspGetAFWinVlaue(3, &af_value);
	argp->af3_h_value = (af_value.h_value_h << 16) | af_value.h_value_l;
	argp->af3_v_value = (af_value.v_value_h << 16) | af_value.v_value_l;
}


void drv_l2_Set_Size_before_NewDenoise(INT8U idx, INT8U enable)
{
	drv_l2_sensor_ops_t *pSensor =  drv_l2_sensor_get_ops(0);
	drv_l2_sensor_info_t *pInfo;
	pInfo = pSensor->get_info(idx);
	DBG_PRINT("nd%d \r\n",enable);
	
	if(enable)
	{
		drv_l2_Cdsp_hwFront_SetSize(pInfo->sensor_w, pInfo->sensor_h+2);
		drv_l2_Cdsp_hwIsp_SetImageSize(pInfo->sensor_w, pInfo->sensor_h+2);
		if(sensor_type == 1)//mipi
			R_CDSP_MIPI_HVSIZE = ((pInfo->sensor_w+MIPI_ND_HOFFSET)&0xfff) |(( (pInfo->sensor_h+2)&0xfff)<<12);
	}else{
		drv_l2_Cdsp_hwFront_SetSize(pInfo->sensor_w, pInfo->sensor_h);
		drv_l2_Cdsp_hwIsp_SetImageSize(pInfo->sensor_w, pInfo->sensor_h);
		if(sensor_type == 1)//mipi
			R_CDSP_MIPI_HVSIZE = (pInfo->sensor_w&0xfff) |(( (pInfo->sensor_h)&0xfff)<<12);
	}
	
}

void drv_l2_Cdsp_hwFront_SetSize(INT32U hsize, INT32U vsize)
{
	R_CDSP_FRAME_H_SETTING &= ~0xFFF;
	R_CDSP_FRAME_V_SETTING &= ~0xFFF;
	R_CDSP_FRAME_H_SETTING = R_CDSP_FRAME_H_SETTING | (hsize & 0xFFF);
	R_CDSP_FRAME_V_SETTING = R_CDSP_FRAME_V_SETTING | (vsize & 0xFFF);
}

void drv_l2_Cdsp_hwIsp_SetImageSize(INT16U hsize,INT16U vsize)
{
	R_ISP_IMSIZE_CROSSTALK_WEIGHT = hsize & 0xFFF | (INT32U)vsize << 16;
}


static void drv_l2_cdsp_irq_handle(void)
{
	INT32U status;
	INT32U eof = 0;
	
	status = drv_l1_CdspGetGlbIntStatus();
	if(status & CDSP_INT_BIT) { 
		INT32U irq_status = drv_l1_CdspGetIntStatus();
		
		if (CdspDev == NULL) {
			return;
		}
		
		if(irq_status & CDSP_OVERFOLW) {
			drv_l2_cdsp_handle_of();
			return;
		}
		
		if(irq_status & CDSP_FACWR) {
			drv_l2_cdsp_handle_facwr();
			return;
		}
		
		if(irq_status & CDSP_EOF) {
			eof = 1;
			if(CdspDev->imgSrc == C_CDSP_SDRAM) {
				drv_l2_cdsp_handle_postprocess();
			} else {
				drv_l2_cdsp_handle_eof();
				#if CAPTURE_RAW_MODE
				if(flag_update==1)
				{
					save_raw10(update_value, g_frame_addr1);
					flag_update = 2; // turn back to YUV
				}
				else if(flag_update==2){

					cdsp_yuyv_restart(DUMMY_BUFFER_ADDRS2);
					flag_update = 0;
				}
				#endif
				// histgm
				if((CdspDev->ae_awb_flag & CDSP_HIST_CTRL_EN & CDSP_AE_UPDATE) == 0) {
					INT32U hislowcnt, hishicnt;
				
					drv_l1_CdspGetHistgm(&hislowcnt, &hishicnt);
					CdspDev->result.hislowcnt = hislowcnt;
					CdspDev->result.hishicnt = hishicnt;
					CdspDev->ae_awb_flag |= CDSP_HIST_CTRL_EN;
				}
			}
		
			// add by Comi for changing the CDSP setting
			// set AWB gain
			if((CdspDev->ae_awb_flag & CDSP_AWB_SET_GAIN) != 0)
			{
				gpCdspWhtBal_t wb_gain;
				
				wb_gain.wbgainen  = ENABLE;
				wb_gain.bgain = gp_cdsp_awb_get_b_gain(CdspDev->awb_workmem);
				wb_gain.rgain = gp_cdsp_awb_get_r_gain(CdspDev->awb_workmem);
				wb_gain.gbgain = 64;
				wb_gain.grgain = 64;
				wb_gain.global_gain = 32;
				drv_l2_CdspSetWhiteBalance(&wb_gain);
				CdspDev->ae_awb_flag &= (~CDSP_AWB_SET_GAIN);
			}

			#if 1
			// set YUV sat/hue
			if((CdspDev->ae_awb_flag & CDSP_SAT_SWITCH) != 0)
			{
				int idx = CdspDev->sat_contr_idx;
				int *p_sat = &CdspDev->sat_yuv_level[CdspDev->sat_contr_idx][0];
				
				drv_l1_CdspSetYuvSPEffOffset(p_sat[0], p_sat[1], p_sat[2]);
				drv_l1_CdspSetYuvSPEffScale(p_sat[3], p_sat[4], p_sat[5]);
				drv_l1_CdspSetUvDivide(&CdspDev->UVDivide);

				
				drv_l1_CdspSetEdgeAmpga(CdspDev->edge_level[idx]);
				
				CdspDev->ae_awb_flag &= (~CDSP_SAT_SWITCH);
			}
			
			if((CdspDev->ae_awb_flag & CDSP_LUM_STATUS) != 0) {	
				if((CdspDev->ae_awb_flag & CDSP_HIGH_LUM) != 0) {
					drv_l2_Set_Size_before_NewDenoise(target_sensor_index, DISABLE);
					drv_l1_CdspSetNewDenoiseEn(DISABLE);
				} else if((CdspDev->ae_awb_flag & CDSP_LOW_LUM) != 0) {
					drv_l2_Set_Size_before_NewDenoise(target_sensor_index, ENABLE);
					drv_l1_CdspSetNewDenoiseEn(ENABLE);
					//drv_l1_CdspSetNewDenoiseEn(DISABLE);
					//drv_l1_IspSetDenoiseEn(ENABLE); 
				}

				CdspDev->ae_awb_flag &= (~CDSP_LUM_STATUS);		
			}		
			#endif
 			//AE update, mipi no FRONT_VD_INT_BIT, update in cdsp eof
			if( ((CdspDev->ae_awb_flag & CDSP_AE_UPDATE) != 0)&&(sensor_type==1) )
			{
				OSQPost(CdspDev->ae_set_sensor_msg, (void *)C_SET_SENSOR);
			}
		}
		
		if(irq_status & CDSP_FIFO) {
			
		}
		
		if(irq_status& CDSP_AEWIN_SEND) {
			if((CdspDev->ae_awb_flag & CDSP_AE_UPDATE) == 0) 
				drv_l2_cdsp_handle_ae(eof);
		}
				
		if(irq_status & CDSP_AWBWIN_UPDATE) {
			drv_l2_cdsp_handle_awb();
		}	
		
		if(irq_status & CDSP_AFWIN_UPDATE) {
			drv_l2_cdsp_handle_af();
		}
	}

	if( (status & FRONT_VD_INT_BIT)&&(sensor_type==0) )
	{
		if(drv_l1_FrontVdIsEnable(1) == 1 && (CdspDev->ae_awb_flag & CDSP_AE_UPDATE) != 0)
		{
			OSQPost(CdspDev->ae_set_sensor_msg, (void *)C_SET_SENSOR);
		}
	}
	
	if(status & FRONT_INT_BIT) {
	
	}
}

/************************************
	   cdsp API
*************************************/
/**
 * @brief	cdsp register end of frame interrupt service function
 * @param	handle[in]: function
 * @return	none
 */
void drv_l2_CdspIsrRegister(INT32U isr_index, void (*handle)(void))
{
	if(isr_index >= C_ISR_MAX) {
		return;
	}

	CdspDev->interrupt_handle[isr_index] = handle;
}

/**
 * @brief	cdsp register sensor function table
 * @param	pTable[in]: sensor function
 * @return	none
 */
void drv_l2_CdspSensorCtrlRegister(gpCdspSensor_t *pCtrl)
{
	if(pCtrl == 0) {
		return;
	}
	
	CdspDev->sensor_sd = pCtrl;
}

/**
 * @brief	cdsp register sensor calibration table
 * @param	sensor[in]: sensor calibration table
 * @return	none
 */
void drv_l2_CdspTableRegister(sensor_calibration_t *sensor)
{
	if(sensor == 0) {
		return;
	}
	
	CdspDev->sensor_cdsp.ob = sensor->ob;
	CdspDev->sensor_cdsp.lincorr= sensor->lincorr;
	CdspDev->sensor_cdsp.lenscmp1= sensor->lenscmp1;
	CdspDev->sensor_cdsp.lenscmp2= sensor->lenscmp2;
	CdspDev->sensor_cdsp.lenscmp3= sensor->lenscmp3;
	CdspDev->sensor_cdsp.lenscmp4= sensor->lenscmp4;
	CdspDev->sensor_cdsp.lenscmp5= sensor->lenscmp5;
	CdspDev->sensor_cdsp.lenscmp6= sensor->lenscmp6;
	CdspDev->sensor_cdsp.wb_gain= sensor->wb_gain;
	CdspDev->sensor_cdsp.gamma1  = sensor->gamma1;
	CdspDev->sensor_cdsp.color_matrix1 = sensor->color_matrix1;
	CdspDev->sensor_cdsp.gamma2  = sensor->gamma2;
	CdspDev->sensor_cdsp.color_matrix2 = sensor->color_matrix2;
	CdspDev->sensor_cdsp.awb_thr = sensor->awb_thr;
}

/**
 * @brief	cdsp set post process
 * @param	PostPress[in]: process parameter
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_CdspPostProcess(gpCdspPostProcess_t *PostPress)
{
	INT8U err;
	INT32U cdsp_inFmt;
	INT32U msg, size;
	gpCdspYuvHAvg_t yuv_havg;	
	//gpCdspLensCmp_t lens_cmp;

	DEBUG("%s\r\n", __FUNCTION__);
	if(CdspDev->imgSrc != C_CDSP_SDRAM) {
		DEBUG("imag source is not SDRAM\r\n");
		return -1;
	}
	
	switch(PostPress->inFmt)	
	{	
	case V4L2_PIX_FMT_VYUY:
		cdsp_inFmt = C_SDRAM_FMT_VY1UY0;				
		CdspDev->rawFmtFlag = 0;				
		CdspDev->rawFmt = 0;		
		break;

	case V4L2_PIX_FMT_SBGGR8:
		cdsp_inFmt = C_SDRAM_FMT_RAW8;
		CdspDev->rawFmtFlag = 1;				
		CdspDev->rawFmt = 2;
		break;

	case V4L2_PIX_FMT_SGBRG8:
		cdsp_inFmt = C_SDRAM_FMT_RAW8;
		CdspDev->rawFmtFlag = 1;				
		CdspDev->rawFmt = 3;
		break;
		
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SGRBG10:			
		cdsp_inFmt = C_SDRAM_FMT_RAW8;
		CdspDev->rawFmtFlag = 1;				
		CdspDev->rawFmt = 0;
		break;
		
	case V4L2_PIX_FMT_SRGGB8:
		cdsp_inFmt = C_SDRAM_FMT_RAW8;
		CdspDev->rawFmtFlag = 1;				
		CdspDev->rawFmt = 1;
		break;
	
	default:
		DEBUG("input Format ERROR\r\n");
		return -1;	
	}

	DBG_PRINT("rawFmt = %d, rawFmtFlag = %d\r\n", CdspDev->rawFmt, CdspDev->rawFmtFlag);
	

	/* cdsp clock */
	drv_l1_CdspSetClockTree(ENABLE, CdspDev->imgSrc, CdspDev->rawFmtFlag);

	/* cdsp reset */
	drv_l1_CdspReset();
	drv_l1_FrontReset();
	drv_l1_CdspSetDataSource(C_CDSP_SDRAM);

	/* set module */	
	if(CdspDev->rawFmtFlag) {		
		gpCdspIntpl_t intpl;		
		gpCdspEdge_t edge;
		//gpCdspNewDenoise_t newdenoise;	
		
		drv_l2_CdspGetIntpl(&intpl);		
		drv_l2_CdspSetIntpl(CdspDev->imgWidth, &intpl);		
		
		drv_l2_CdspGetEdge(&edge);		
		drv_l2_CdspSetEdge(CdspDev->rawFmtFlag, &edge);
		
		//drv_l2_CdspGetNewDenoise(&newdenoise);
		//drv_l2_CdspSetNewDenoise(&newdenoise);
		
	} else {				
		gpCdspEdge_t edge;
		
		drv_l2_CdspGetEdge(&edge);		
		drv_l2_CdspSetSuppression(PostPress->width, &CdspDev->suppression, &edge);	
	}	

	//drv_l2_CdspGetLensCmp(&lens_cmp);	
	//drv_l2_CdspSetLensCmp(CdspDev->rawFmtFlag, &lens_cmp);	
	drv_l2_CdspGetYuvHavg(&yuv_havg);	
	drv_l2_CdspSetYuvHavg(&yuv_havg);	

	/* set scale & crop */	
	drv_l2_CdspSetScaleCrop(&CdspDev->scale);
	drv_l2_CdspSetScaleAeAf(&CdspDev->scale);

	/* set fifo */	
	drv_l2_CdspSetFifoSize(PostPress->width);
	
	/* set interface */
//#if _SET_YUV_PATH_
if(g_set_yuv_path == 1){
	switch(cdsp_inFmt)
	{
	case C_SDRAM_FMT_VY1UY0:
	case C_SDRAM_FMT_RAW8:
		drv_l1_CdspSetRawPath(0, 1, 1, 0); //yuyv out
		break;
	case C_SDRAM_FMT_RAW10:
		drv_l1_CdspSetRawPath(0, 0, 1, 0); //yuyv out
		break;
	}
}else{
//#else
	if(RAW_BIT == 8) {
		drv_l1_CdspSetRawPath(RAW_MODE, 1, 1, 0); //8bit raw out in 8-bit format
	} else if(RAW_BIT == 10) {
		drv_l1_CdspSetRawPath(RAW_MODE, 0, 1, 0); // packed 10bit raw out 
	} else {
		DEBUG("Err: Please set bit of raw\r\n");
	}
}	
//#endif	

	drv_l1_FrontYuvOrderSet(0);
		
	drv_l1_FrontInputPathSet(0);		//mipi disable	
	
	if(CdspDev->rawFmtFlag) {
		DEBUG("SDRAMRawFmt\r\n");
		drv_l1_CdspSetYuvRange(0x00);
		drv_l1_CdspSetRawDataFormat(CdspDev->rawFmt);
		drv_l1_FrontInputGate(0x1FF);
		drv_l1_CdspSetYuvMuxPath(0);	//raw path
		
		drv_l1_CdspSetRawBuff(PostPress->inAddr);
		if(cdsp_inFmt == C_SDRAM_FMT_RAW10) {
			drv_l1_CdspSetRawBuffSize(PostPress->width, PostPress->height, 0x00, 10);
		} else {
			drv_l1_CdspSetRawBuffSize(PostPress->width, PostPress->height, 0x00, 8);
		}
		
		drv_l1_CdspSetYuvBuffA(PostPress->width, PostPress->height, PostPress->outAddr);
		drv_l1_CdspSetDmaConfig(RD_A_WR_A);

		size = PostPress->width * PostPress->height;
		drv_l1_CdspSetReadBackSize(0x00, 0x00, PostPress->width, PostPress->height);
	} else {
		DEBUG("SDRAMYuvFmt\r\n");
		drv_l1_CdspSetYuvRange(PostPress->yuvRange);
		drv_l1_CdspSetRawDataFormat(0x00);
		drv_l1_FrontInputGate(0x1FF);
		drv_l1_CdspSetYuvMuxPath(1);			//yuv path
		
		drv_l1_CdspSetYuvBuffA(PostPress->width, PostPress->height, PostPress->inAddr);
		drv_l1_CdspSetYuvBuffB(PostPress->width, PostPress->height, PostPress->outAddr);
		drv_l1_CdspSetDmaConfig(RD_A_WR_B);

		size = PostPress->width*PostPress->height*2;
		drv_l1_CdspSetReadBackSize(0x00, 0x00, PostPress->width, PostPress->height);
	}
	
	/* clean dcache */
#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
	cache_drain_range(PostPress->inAddr, size);
#endif
	
	/* start & wait finish*/
	drv_l1_CdspClrIntStatus(CDSP_INT_ALL);
	drv_l1_CdspSetIntEn(ENABLE, CDSP_OVERFOLW|CDSP_EOF);
	drv_l1_CdspSetDataSource(C_CDSP_SDRAM);
	drv_l1_CdspSetRedoTriger(1);
	msg = (INT32U)OSQPend(CdspDev->cdsp_wait_queue, 0, &err);
	if(msg != C_WAKE_UP) {
		return -1;
	}
	
	/* stop */
	drv_l1_CdspSetIntEn(DISABLE, CDSP_OVERFOLW|CDSP_EOF);
	drv_l1_CdspSetDataSource(C_CDSP_SDRAM);
	drv_l1_CdspSetRedoTriger(0);
	
	/* invalid cache */
#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
	cache_invalid_range(PostPress->outAddr, PostPress->width*PostPress->height*2);
#endif
	return 0;
}

/**
 * @brief	cdsp get control
 * @param	ctrl[out]: control parameter
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_get_ctrl(gpCdspCtrl_t *ctrl)
{
	INT32S ret = 0;

	DEBUG("%s = 0x%x\r\n", __FUNCTION__, ctrl->id);
	switch(ctrl->id)
	{
		case MSG_CDSP_SCALE_CROP:
		{
			drv_l2_CdspGetScaleCrop(&CdspDev->scale);
			ret = MEMCPY((void*)ctrl->value, (void*)&CdspDev->scale, sizeof(gpCdspScalePara_t));
		}
		break;

		case MSG_CDSP_BADPIX_OB:
		{
			gpCdspBadPixOB_t bad_pixel;

			drv_l2_CdspGetBadPixOb(&bad_pixel);
			ret = MEMCPY((void*)ctrl->value, (void*)&bad_pixel, sizeof(gpCdspBadPixOB_t));
		}
		break;

		case MSG_CDSP_LENS_CMP:
		{
			gpCdspLensCmp_t lens_cmp;

			drv_l2_CdspGetLensCmp(&lens_cmp);
			ret = MEMCPY((void*)ctrl->value, (void*)&lens_cmp, sizeof(gpCdspLensCmp_t));
		}
		break;

		case MSG_CDSP_WBGAIN:
		{
			gpCdspWhtBal_t wht_bal;
			
			drv_l2_CdspGetWhiteBalance(&wht_bal);
			ret = MEMCPY((void*)ctrl->value, (void*)&wht_bal, sizeof(gpCdspWhtBal_t));
		}
		break;
		
		case MSG_CDSP_LUT_GAMMA:
		{
			gpCdspGamma_t lut_gamma;

			drv_l2_CdspGetLutGamma(&lut_gamma);
			ret = MEMCPY((void*)ctrl->value, (void*)&lut_gamma, sizeof(gpCdspGamma_t));
		}
		break;
		
		case MSG_CDSP_INTERPOLATION:
		{
			gpCdspIntpl_t intpl;

			drv_l2_CdspGetIntpl(&intpl);
			ret = MEMCPY((void*)ctrl->value, (void*)&intpl, sizeof(gpCdspIntpl_t));
		}
		break;
		
		case MSG_CDSP_EDGE:
		{
			gpCdspEdge_t edge;
			
			drv_l2_CdspGetEdge(&edge);
			ret = MEMCPY((void*)ctrl->value, (void*)&edge, sizeof(gpCdspEdge_t));
		}
		break;

		case MSG_CDSP_COLOR_MATRIX:
		{
			gpCdspCorMatrix_t matrix;

			drv_l2_CdspGetColorMatrix(&matrix);
			ret = MEMCPY((void*)ctrl->value, (void*)&matrix, sizeof(gpCdspCorMatrix_t));
		}
		break;

		case MSG_CDSP_POSWB_RGB2YUV:
		{
			gpCdspRgb2Yuv_t rgb2yuv;

			drv_l2_CdspGetRgbToYuv(&rgb2yuv);
			ret = MEMCPY((void*)ctrl->value, (void*)&rgb2yuv, sizeof(gpCdspRgb2Yuv_t));
		}
		break;

		case MSG_CDSP_YUV_INSERT:
		{
			gpCdspYuvInsert_t yuv_insert;

			drv_l2_CdspGetYuv444Insert(&yuv_insert);
			ret = MEMCPY((void*)ctrl->value, (void*)&yuv_insert, sizeof(gpCdspYuvInsert_t));
			break;
		}

		case MSG_CDSP_YUV_HAVG:
		{
			gpCdspYuvHAvg_t yuv_havg;

			drv_l2_CdspGetYuvHavg(&yuv_havg);
			ret = MEMCPY((void*)ctrl->value, (void*)&yuv_havg, sizeof(gpCdspYuvHAvg_t));
		}
		break;

		case MSG_CDSP_SPEC_MODE:
		{
			gpCdspSpecMod_t spec_mode;

			drv_l2_CdspGetSpecialMode(&spec_mode);
			ret = MEMCPY((void*)ctrl->value, (void*)&spec_mode, sizeof(gpCdspSpecMod_t));
		}
		break;
		
		case MSG_CDSP_TARGET_AE:
		{
			ret = MEMCPY((void*)ctrl->value, (void*)&CdspDev->ae_target, sizeof(INT32S));
		}
		break;
		
		case MSG_CDSP_WB_OFFSET_DAY:
		{
			ret = MEMCPY((void*)ctrl->value, (void*)&CdspDev->wb_offset_day, sizeof(INT32S));
		}
		break;

		case MSG_CDSP_WB_OFFSET_NIGHT:
		{
			ret = MEMCPY((void*)ctrl->value, (void*)&CdspDev->wb_offset_night, sizeof(INT32S));
		}
		break;

		case MSG_CDSP_SAT_HUE:
		{
			gpCdspSatHue_t sat_hue;

			drv_l2_CdspGetSatHue(&sat_hue);
			ret = MEMCPY((void*)ctrl->value, (void*)&sat_hue, sizeof(gpCdspSatHue_t));
			break;
		}
		
		case MSG_CDSP_SAT_HUE_DAY:
		{
			gpCdspSatHue_t sat_hue;

			drv_l2_CdspGetSatHue(&sat_hue);
			sat_hue.y_offset = CdspDev->sat_yuv_level[0][0];
			sat_hue.u_offset = CdspDev->sat_yuv_level[0][1];
			sat_hue.v_offset = CdspDev->sat_yuv_level[0][2];
			sat_hue.y_scale = CdspDev->sat_yuv_level[0][3];
			sat_hue.u_scale = CdspDev->sat_yuv_level[0][4];
			sat_hue.v_scale = CdspDev->sat_yuv_level[0][5];
			ret = MEMCPY((void*)ctrl->value, (void*)&sat_hue, sizeof(gpCdspSatHue_t));
			break;
		}

		case MSG_CDSP_SAT_HUE_NIGHT:
		{
			gpCdspSatHue_t sat_hue;
			
			drv_l2_CdspGetSatHue(&sat_hue);
			sat_hue.y_offset = CdspDev->sat_yuv_level[3][0];
			sat_hue.u_offset = CdspDev->sat_yuv_level[3][1];
			sat_hue.v_offset = CdspDev->sat_yuv_level[3][2];
			sat_hue.y_scale = CdspDev->sat_yuv_level[3][3];
			sat_hue.u_scale = CdspDev->sat_yuv_level[3][4];
			sat_hue.v_scale = CdspDev->sat_yuv_level[3][5];
			ret = MEMCPY((void*)ctrl->value, (void*)&sat_hue, sizeof(gpCdspSatHue_t));
			break;
		}

		case MSG_CDSP_SUPPRESSION:
		{
			drv_l2_CdspGetSuppression(&CdspDev->suppression);
			ret = MEMCPY((void*)ctrl->value, (void*)&CdspDev->suppression, sizeof(CdspDev->suppression));
			break;
		}

		case MSG_CDSP_NEWDENOISE:
		{
			gpCdspNewDenoise_t NewDenoise;

			drv_l2_CdspGetNewDenoise(&NewDenoise);			
			ret = MEMCPY((void*)ctrl->value, (void*)&NewDenoise, sizeof(NewDenoise));
			break;
		}	

		case MSG_CDSP_RAW_WIN:
		{
			gpCdspRawWin_t raw_win;

			drv_l2_CdspGetRawWin(&raw_win);
			ret = MEMCPY((void*)ctrl->value, (void*)&raw_win, sizeof(raw_win));
			break;
		}

		case MSG_CDSP_AE_WIN:
		{
			gpCdspAE_t ae;

			drv_l2_CdspGetAE(&ae);
			ret = MEMCPY((void*)ctrl->value, (void*)&ae, sizeof(ae));
			break;
		}

		case MSG_CDSP_AF_WIN:
		{
			gpCdspAF_t af;

			drv_l2_CdspGetAF(&af);
			ret = MEMCPY((void*)ctrl->value, (void*)&af, sizeof(af));
			break;
		}

		case MSG_CDSP_AWB_WIN:
		{
			gpCdspAWB_t awb;

			drv_l2_CdspGetAWB(&awb);
			ret = MEMCPY((void*)ctrl->value, (void*)&awb, sizeof(awb));
			break;
		}

		case MSG_CDSP_WBGAIN2:
		{
			gpCdspWbGain2_t wbgain2;

			drv_l2_CdspGetWBGain2(&wbgain2);
			ret = MEMCPY((void*)ctrl->value, (void*)&wbgain2, sizeof(wbgain2));
			break;
		}

		case MSG_CDSP_HISTGM:
		{
			gpCdspHistgm_t histgm;

			drv_l2_CdspGetHistgm(&histgm);
			ret = MEMCPY((void*)ctrl->value, (void*)&histgm, sizeof(histgm));
			break;
		}

		case MSG_CDSP_3A_STATISTIC:
		{
			ret = MEMCPY((void*)ctrl->value, (void*)&CdspDev->result, sizeof(CdspDev->result));
			break;
		}

		default:
			return STATUS_FAIL;
	}

	return ret;
}

/**
 * @brief	cdsp set control
 * @param	ctrl[in]: control parameter
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_set_ctrl(gpCdspCtrl_t *ctrl)
{
	INT16U w, h;
	INT32S ret = 0;

	DEBUG("%s = 0x%x\r\n", __FUNCTION__, ctrl->id);
	switch(ctrl->id)
	{
		case MSG_CDSP_POSTPROCESS:
		{
			gpCdspPostProcess_t post;
			
			MEMCPY((INT8S *)&post, (INT8S *)ctrl->value, sizeof(gpCdspPostProcess_t));
			CdspDev->imgSrc = C_CDSP_SDRAM;
			CdspDev->inFmt = post.inFmt;
			CdspDev->imgWidth = CdspDev->imgRbWidth = post.width;
			CdspDev->imgHeight = CdspDev->imgRbHeight = post.height;
			CdspDev->scale.img_rb_h_size = post.width;
			CdspDev->scale.img_rb_v_size = post.height;
			CdspDev->suppression.suppressen = 1;
			switch(CdspDev->inFmt)
			{
			case V4L2_PIX_FMT_VYUY:
			case V4L2_PIX_FMT_SBGGR8:
			case V4L2_PIX_FMT_SGBRG8:
			case V4L2_PIX_FMT_SGRBG8:
			case V4L2_PIX_FMT_SGRBG10:
				ret = drv_l2_CdspPostProcess(&post);
				break;

			default:
				RETURN(STATUS_FAIL);
			}
			break;
		}
		
		case MSG_CDSP_SCALE_CROP:
		{
			MEMCPY((void*)&CdspDev->scale, (void*)ctrl->value, sizeof(CdspDev->scale));
			if(CdspDev->RunFlag) {
				CdspDev->SkipCnt = SKIP_CNT;
			} else {
				CdspDev->SkipCnt = 0;
				ret = drv_l2_CdspSetScaleCrop(&CdspDev->scale);
				drv_l2_CdspSetScaleAeAf(&CdspDev->scale);
			}
			break;
		}

		case MSG_CDSP_BADPIX_OB:
		{
			gpCdspBadPixOB_t bad_pixel;
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
				
			MEMCPY((INT8S *)&bad_pixel, (INT8S *)ctrl->value, sizeof(gpCdspBadPixOB_t));
			bad_pixel.autooben = CdspDev->sensor_cdsp.ob[0];				
			bad_pixel.manuoben =  CdspDev->sensor_cdsp.ob[1];
			bad_pixel.manuob =  CdspDev->sensor_cdsp.ob[2];
			
			bad_pixel.wboffseten = CdspDev->sensor_cdsp.ob[3];
			bad_pixel.roffset = CdspDev->sensor_cdsp.ob[4];
			bad_pixel.groffset = CdspDev->sensor_cdsp.ob[5];
			bad_pixel.gboffset = CdspDev->sensor_cdsp.ob[6];
			bad_pixel.boffset = CdspDev->sensor_cdsp.ob[7];

			bad_pixel.badpixen = CdspDev->sensor_cdsp.ob[8];
			bad_pixel.bprthr = CdspDev->sensor_cdsp.ob[9];			
			bad_pixel.bpgthr = CdspDev->sensor_cdsp.ob[10];			
			bad_pixel.bpbthr = CdspDev->sensor_cdsp.ob[11];		
			drv_l2_CdspSetBadPixOb(&bad_pixel);
			break;
		}

		case MSG_CDSP_LENS_CMP:
		{
			gpCdspLensCmp_t lens_cmp;
			
			MEMCPY((INT8S *)&lens_cmp, (INT8S *)ctrl->value, sizeof(gpCdspLensCmp_t));
			lens_cmp.stepfactor = CdspDev->sensor_cdsp.lenscmp1[0];
			lens_cmp.seg_R = CdspDev->sensor_cdsp.lenscmp1[1];
			lens_cmp.maxtan8_table = (unsigned short  *)CdspDev->sensor_cdsp.lenscmp2;
			lens_cmp.slop4_table = (unsigned short  *)CdspDev->sensor_cdsp.lenscmp3;
			lens_cmp.clpoint_table = (unsigned short  *)CdspDev->sensor_cdsp.lenscmp4;
			lens_cmp.rdiusfile0_table = (unsigned short  *)CdspDev->sensor_cdsp.lenscmp5;
			lens_cmp.rdiusfile1_table = (unsigned short  *)CdspDev->sensor_cdsp.lenscmp6;	
			#if CAPTURE_RAW_MODE
			if( (N_STEP < 4)|(drv_l2_get_raw_capture_setting()<2) )
			{
				DBG_PRINT("Not set LenCmp %d \r\n",drv_l2_get_raw_capture_setting() );
				break;
			}
			else
				DBG_PRINT("LenCmp enable\r\n");
			#endif
			drv_l2_CdspSetLensCmp(CdspDev->rawFmtFlag, &lens_cmp);
			break;
		}

		case MSG_CDSP_WBGAIN:
		{
			gpCdspWhtBal_t wht_bal;
			
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
			
			MEMCPY((INT8S *)&wht_bal, (INT8S *)ctrl->value, sizeof(gpCdspWhtBal_t));
			drv_l2_CdspSetWhiteBalance(&wht_bal);
		}
		break;
		
		case MSG_CDSP_WB_OFFSET_DAY:
		{
			MEMCPY((INT8S *)&CdspDev->wb_offset_day, (INT8S *)ctrl->value, sizeof(CdspDev->wb_offset_day));
		}
		break;
		
		case MSG_CDSP_WB_OFFSET_NIGHT:
		{
			MEMCPY((INT8S *)&CdspDev->wb_offset_night, (INT8S *)ctrl->value, sizeof(CdspDev->wb_offset_night));
		}
		break;
			
		case MSG_CDSP_LUT_GAMMA:
		{
			gpCdspGamma_t lut_gamma;
			
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
			
			MEMCPY((INT8S *)&lut_gamma, (INT8S *)ctrl->value, sizeof(gpCdspGamma_t));
			lut_gamma.gamma_table = (unsigned int *)CdspDev->sensor_cdsp.gamma1;
			drv_l2_CdspSetLutGamma(&lut_gamma);
		}
		break;

		case MSG_CDSP_INTERPOLATION:
		{
			gpCdspIntpl_t intpl;
			
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
			
			MEMCPY((INT8S *)&intpl, (INT8S *)ctrl->value, sizeof(gpCdspIntpl_t));
			drv_l2_CdspSetIntpl(CdspDev->imgWidth, &intpl);
			break;
		}

		case MSG_CDSP_RAW_SPEF:
		{
			gpCdspIntpl_t intpl;

			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
			
			CLEAR(&intpl, sizeof(gpCdspIntpl_t));
			drv_l2_CdspGetIntpl(&intpl);
			
			MEMCPY((INT8S *)&intpl.rawspecmode, (INT8S *)ctrl->value, sizeof(intpl.rawspecmode));
			drv_l2_CdspSetIntpl(CdspDev->imgWidth, &intpl);
			DEBUG("RAW special mode = %d\r\n", intpl.rawspecmode);			
		}
		break;

		case MSG_CDSP_EDGE:
		{
			gpCdspEdge_t edge;
			
			MEMCPY((INT8S *)&edge, (INT8S *)ctrl->value, sizeof(gpCdspEdge_t));
			drv_l2_CdspSetEdge(CdspDev->rawFmtFlag, &edge);

		}
		break;

		case MSG_CDSP_EDGE_AMPGA_DAY:
		{
			int edge_ampga;

			MEMCPY((INT8S *)&edge_ampga, (INT8S *)ctrl->value, sizeof(edge_ampga));
			
			CdspDev->edge_day = edge_ampga;
			DEBUG("Day Edge AMPGA= %d\r\n", edge_ampga);

			drv_l2_cdsp_edge_level_init();
		}
		break;

		case MSG_CDSP_EDGE_AMPGA_NIGHT:
		{
			int edge_ampga;

			MEMCPY((INT8S *)&edge_ampga, (INT8S *)ctrl->value, sizeof(edge_ampga));
			
			CdspDev->edge_night = edge_ampga;
			DEBUG("Night Edge AMPGA= %d\r\n", edge_ampga);

			drv_l2_cdsp_edge_level_init();
		}
		break;

		case MSG_CDSP_COLOR_MATRIX:
		{	
			gpCdspCorMatrix_t matrix;
					
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
				
			MEMCPY((INT8S *)&matrix, (INT8S *)ctrl->value, sizeof(gpCdspCorMatrix_t));
			matrix.a11 = CdspDev->sensor_cdsp.color_matrix1[0];
			matrix.a12 = CdspDev->sensor_cdsp.color_matrix1[1];
			matrix.a13 = CdspDev->sensor_cdsp.color_matrix1[2];
			matrix.a21 = CdspDev->sensor_cdsp.color_matrix1[3];
			matrix.a22 = CdspDev->sensor_cdsp.color_matrix1[4];
			matrix.a23 = CdspDev->sensor_cdsp.color_matrix1[5];
			matrix.a31 = CdspDev->sensor_cdsp.color_matrix1[6];
			matrix.a32 = CdspDev->sensor_cdsp.color_matrix1[7];
			matrix.a33 = CdspDev->sensor_cdsp.color_matrix1[8];
			drv_l2_CdspSetColorMatrix(&matrix);
		}
		break;

		case MSG_CDSP_POSWB_RGB2YUV:
		{
			gpCdspRgb2Yuv_t rgb2yuv;
			
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
			
			MEMCPY((INT8S *)&rgb2yuv, (INT8S *)ctrl->value, sizeof(gpCdspRgb2Yuv_t));
			drv_l2_CdspSetRgbToYuv(&rgb2yuv);
		}
		break;

		case MSG_CDSP_YUV_INSERT:
		{
			gpCdspYuvInsert_t yuv_insert;
			
			MEMCPY((INT8S *)&yuv_insert, (INT8S *)ctrl->value, sizeof(gpCdspYuvInsert_t));
			drv_l2_CdspSetYuv444Insert(&yuv_insert);
		}
		break;
		
		case MSG_CDSP_YUV_HAVG:
		{
			gpCdspYuvHAvg_t yuv_havg;
			
			MEMCPY((INT8S *)&yuv_havg, (INT8S *)ctrl->value, sizeof(gpCdspYuvHAvg_t));
			drv_l2_CdspSetYuvHavg(&yuv_havg);
		}
		break;

		case MSG_CDSP_SPEC_MODE:
		{
			gpCdspSpecMod_t spec_mode;
			
			MEMCPY((INT8S *)&spec_mode, (INT8S *)ctrl->value, sizeof(gpCdspSpecMod_t));
			drv_l2_CdspSetSpecialMode(&spec_mode);
		}
		break;
		
		case MSG_CDSP_SAT_HUE:
		{
			gpCdspSpecMod_t spec_mode;
			gpCdspSatHue_t sat_hue;
			
			spec_mode.yuvspecmode = SP_YUV_YbYcSatHue;
			MEMCPY((INT8S *)&sat_hue, (INT8S *)ctrl->value, sizeof(gpCdspSatHue_t));
			drv_l2_CdspSetSpecialMode(&spec_mode);
			drv_l2_CdspSetSatHue(&sat_hue);
		}
		break;
		
		case MSG_CDSP_SAT_HUE_DAY:
		{
			gpCdspSpecMod_t spec_mode;
			gpCdspSatHue_t sat_hue;
	
			spec_mode.yuvspecmode = SP_YUV_YbYcSatHue;
			MEMCPY((INT8S *)&sat_hue, (INT8S *)ctrl->value, sizeof(gpCdspSatHue_t));
			drv_l2_CdspSetSpecialMode(&spec_mode);
			drv_l2_CdspSetSatHue(&sat_hue);
			
			drv_l2_ae_awb_lock();
			CdspDev->sat_yuv_level[0][0] = sat_hue.y_offset;
			CdspDev->sat_yuv_level[0][1] = sat_hue.u_offset;
			CdspDev->sat_yuv_level[0][2] = sat_hue.v_offset;
			CdspDev->sat_yuv_level[0][3] = sat_hue.y_scale;
			CdspDev->sat_yuv_level[0][4] = sat_hue.u_scale;
			CdspDev->sat_yuv_level[0][5] = sat_hue.v_scale;		
			drv_l2_cdsp_sat_contrast_init();
			drv_l2_ae_awb_unlock();
		}
		break;

		case MSG_CDSP_SAT_HUE_NIGHT:
		{
			gpCdspSatHue_t sat_hue;
			
			MEMCPY((INT8S *)&sat_hue, (INT8S *)ctrl->value, sizeof(gpCdspSatHue_t));
			drv_l2_ae_awb_lock();
			CdspDev->sat_yuv_level[3][0] = sat_hue.y_offset;
			CdspDev->sat_yuv_level[3][1] = sat_hue.u_offset;
			CdspDev->sat_yuv_level[3][2] = sat_hue.v_offset;
			CdspDev->sat_yuv_level[3][3] = sat_hue.y_scale;
			CdspDev->sat_yuv_level[3][4] = sat_hue.u_scale;
			CdspDev->sat_yuv_level[3][5] = sat_hue.v_scale;
			drv_l2_cdsp_sat_contrast_init();
			drv_l2_ae_awb_unlock();
		}
		break;
		
		case MSG_CDSP_SUPPRESSION:
		{
			gpCdspEdge_t edge;
			
			if(CdspDev->rawFmtFlag) {
				RETURN(STATUS_OK);
			}
			
			MEMCPY((void*)&CdspDev->suppression, (void*)ctrl->value, sizeof(CdspDev->suppression));
			CLEAR(&edge, sizeof(gpCdspEdge_t));
			drv_l2_CdspGetEdge(&edge);
			drv_l2_CdspSetSuppression(CdspDev->imgRbWidth, &CdspDev->suppression, &edge);
		}
		break;
		
		case MSG_CDSP_NEWDENOISE:
		{
			//gpCdspEdge_t edge;
			gpCdspNewDenoise_t NewDenoise;
			
			MEMCPY((INT8S *)&NewDenoise, (INT8S *)ctrl->value, sizeof(gpCdspNewDenoise_t));
			//drv_l2_CdspGetEdge(&edge);
			drv_l2_CdspSetNewDenoise(&NewDenoise);
		}	
		break;
		
		case MSG_CDSP_RAW_WIN:
		{
			gpCdspRawWin_t raw_win;

			if(CdspDev->scale.crop_en) {
				w = CdspDev->scale.crop_hsize;
				h = CdspDev->scale.crop_vsize;
			} else {
				w = CdspDev->imgWidth;
				h = CdspDev->imgHeight;
			}
			
			MEMCPY((INT8S *)&raw_win, (INT8S *)ctrl->value, sizeof(gpCdspRawWin_t));
			drv_l2_CdspSetRawWin(w, h, &raw_win);
			drv_l2_CdspGetRawWin(&raw_win);
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				DBG_PRINT("gp_cdsp_awb_set_cnt_thr & gp_cdsp_ae_set_lum_bound\r\n");
				gp_cdsp_awb_set_cnt_thr(CdspDev->awb_workmem, CdspDev->imgWidth, CdspDev->imgHeight, raw_win.subsample, 0);
				gp_cdsp_ae_set_lum_bound(CdspDev->ae_workmem, CdspDev->imgWidth, CdspDev->imgHeight, raw_win.hwdsize, raw_win.vwdsize);
				drv_l2_ae_awb_unlock();
			}
		}
		break;
	
		case MSG_CDSP_AE_WIN:
		{
			gpCdspAE_t ae;
			//gpCdspRawWin_t raw_win;
			
			MEMCPY((INT8S *)&ae, (INT8S *)ctrl->value, sizeof(gpCdspAE_t));
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				DBG_PRINT("gp_cdsp_ae_set_meter = 0x%x\r\n", ae.ae_meter);
				gp_cdsp_ae_set_meter(ae.ae_meter, CdspDev->ae_workmem);
				drv_l2_ae_awb_unlock();
			}
			
			//gpCdspGetRawWin(&raw_win);
			drv_l2_CdspSetAE(&ae);
		}
		break;
		
		case MSG_CDSP_AF_WIN:
		{
			gpCdspAF_t af;

			if(CdspDev->scale.crop_en) {
				w = CdspDev->scale.crop_hsize;
				h = CdspDev->scale.crop_vsize;
			} else {
				w = CdspDev->imgWidth;
				h = CdspDev->imgHeight;
			}
			
			MEMCPY((INT8S *)&af, (INT8S *)ctrl->value, sizeof(gpCdspAF_t));
			drv_l2_CdspSetAF(w, h, &af);
		}
		break;
		
		case MSG_CDSP_AWB_WIN:
		{
			gpCdspAWB_t awb;
			
			MEMCPY((INT8S *)&awb, (INT8S *)ctrl->value, sizeof(gpCdspAWB_t));
			awb.awbwinthr = CdspDev->sensor_cdsp.awb_thr[0];
			
			awb.sindata = CdspDev->sensor_cdsp.awb_thr[1];
			awb.cosdata = CdspDev->sensor_cdsp.awb_thr[2];

			awb.Ythr0 = CdspDev->sensor_cdsp.awb_thr[3];
			awb.Ythr1 = CdspDev->sensor_cdsp.awb_thr[4];
			awb.Ythr2 = CdspDev->sensor_cdsp.awb_thr[5];
			awb.Ythr3 = CdspDev->sensor_cdsp.awb_thr[6];

			awb.UL1N1 = CdspDev->sensor_cdsp.awb_thr[7]; 
			awb.UL1P1 = CdspDev->sensor_cdsp.awb_thr[8]; 
			awb.VL1N1 = CdspDev->sensor_cdsp.awb_thr[9]; 
			awb.VL1P1 = CdspDev->sensor_cdsp.awb_thr[10]; 

			awb.UL1N2 = CdspDev->sensor_cdsp.awb_thr[11]; 
			awb.UL1P2 = CdspDev->sensor_cdsp.awb_thr[12]; 
			awb.VL1N2 = CdspDev->sensor_cdsp.awb_thr[13]; 
			awb.VL1P2 = CdspDev->sensor_cdsp.awb_thr[14]; 

			awb.UL1N3 = CdspDev->sensor_cdsp.awb_thr[15]; 
			awb.UL1P3 = CdspDev->sensor_cdsp.awb_thr[16]; 
			awb.VL1N3 = CdspDev->sensor_cdsp.awb_thr[17]; 
			awb.VL1P3 = CdspDev->sensor_cdsp.awb_thr[18]; 
			
			drv_l2_CdspSetAWB(&awb);
		}
		break;
		
		case MSG_CDSP_WBGAIN2:
		{
			gpCdspWbGain2_t wbgain2;
			
			MEMCPY((INT8S *)&wbgain2, (INT8S *)ctrl->value, sizeof(gpCdspWbGain2_t));
			drv_l2_CdspSetWBGain2(&wbgain2);
		}
		break;
		
		case MSG_CDSP_HISTGM:
		{
			gpCdspHistgm_t histgm;
			
			MEMCPY((INT8S *)&histgm, (INT8S *)ctrl->value, sizeof(gpCdspHistgm_t));
			drv_l2_CdspSetHistgm(&histgm);
		}
		break;
		
		case MSG_CDSP_TARGET_AE:
		{
			INT32S targetLum;
			
			MEMCPY((INT8S *)&targetLum, (INT8S *)ctrl->value, sizeof(INT32S));
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				CdspDev->ae_target = targetLum;
				gp_cdsp_ae_set_target_lum(CdspDev->ae_workmem, targetLum);
				drv_l2_ae_awb_unlock();
			}
		}
		break;

		case MSG_CDSP_TARGET_AE_NIGHT:
		{
			INT32S targetLum;
			MEMCPY((INT8S *)&targetLum, (INT8S *)ctrl->value, sizeof(INT32S));
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				CdspDev->ae_target_night = targetLum;				
				gp_cdsp_ae_set_target_lum_night(CdspDev->ae_workmem, targetLum);
				drv_l2_ae_awb_unlock();
			}
			DEBUG("AE target night = %d\r\n", targetLum);
			break;
		}
		
		case MSG_CDSP_WB_MODE:
		{
			INT32S wbmode;
			
			MEMCPY((INT8S *)&wbmode, (INT8S *)ctrl->value, sizeof(INT32S));			
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				gp_cdsp_awb_set_mode(CdspDev->awb_workmem, wbmode);
				drv_l2_ae_awb_unlock();
			}
		}
		break;
		
		case MSG_CDSP_EV:
		{
			INT32S val;
			
			MEMCPY((INT8S *)&val, (INT8S *)ctrl->value, sizeof(INT32S));	
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				DEBUG("Set EV idx = %d\r\n", val);
				gp_cdsp_ae_set_ev(CdspDev->ae_workmem, val);
				drv_l2_ae_awb_unlock();
			}
		}
		break;
		
		case MSG_CDSP_BACKLIGHT_DETECT:
		{
			INT32S val;
			
			MEMCPY((INT8S *)&val, (INT8S *)ctrl->value, sizeof(INT32S));	
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				DEBUG("Set backlight = %d\r\n", val);
				gp_cdse_ae_set_backlight_detect(CdspDev->ae_workmem, val);
				drv_l2_ae_awb_unlock();
			}
		}
		break;
		
		case MSG_CDSP_ISO:
		{
			INT32S iso;
			gpCdspCtrl_t csi_ctrl;
			sensor_exposure_t *p_seinfo;
			
			if(CdspDev->sensor_sd == 0) {
				RETURN(STATUS_FAIL);
			}
			
			MEMCPY((INT8S *)&iso, (INT8S *)ctrl->value, sizeof(INT32S));
			if(iso != ISO_AUTO) {
				DEBUG("Set ISO = %d00\r\n", iso);
			} else {
				DEBUG("Set AUTO ISO\r\n");
			}
			
			csi_ctrl.id = V4L2_CID_EXPOSURE;
			ret = CdspDev->sensor_sd->sensor_get_ctrl(&csi_ctrl);
			if(ret < 0) {
				RETURN(STATUS_FAIL);
			}
			
			p_seinfo = (sensor_exposure_t *) csi_ctrl.value;
			p_seinfo->userISO = iso;
			DEBUG("Set ISO: 0x%x\r\n", iso);
			if(CdspDev->ae_awb_task_en) {
				drv_l2_ae_awb_lock();
				gp_cdsp_ae_set_iso(CdspDev->ae_workmem, iso, p_seinfo);
				drv_l2_ae_awb_unlock();
			}
		}
		break;
		case MSG_CDSP_LINCORR:
		{
			gpCdspLinCorr_t cdspLincorr;
			
			if(CdspDev->rawFmtFlag == 0) {
				RETURN(STATUS_OK);
			}
			
			MEMCPY((INT8S *)&cdspLincorr, (INT8S *)ctrl->value, sizeof(gpCdspLinCorr_t));
			cdspLincorr.lincorr_table = (unsigned char *)CdspDev->sensor_cdsp.lincorr;
			#if CAPTURE_RAW_MODE
			if( (N_STEP <5)|(drv_l2_get_raw_capture_setting()<1) )
			{
				DBG_PRINT("Not set LinCorr %d \r\n",drv_l2_get_raw_capture_setting() );
				break;
			}
			else
				DBG_PRINT("LinCorr enable\r\n");
			#endif
			drv_l2_CdspSetLinCorr(&cdspLincorr);
		}
		break;

		default:
			RETURN(STATUS_FAIL);
	}

__exit:
	return ret;
}

/**
 * @brief	cdsp set input parameter
 * @param	pFmt[in]: input parameter
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_set_fmt(gpCdspFmt_t *pFmt)
{
	INT32U cdsp_inFmt;
	gpCdspYuvHAvg_t yuv_havg;	
	//gpCdspLensCmp_t lens_cmp;
	gpHalFrontRoi_t roi;
	//gpHalFrontReshape_t reshapeCtl;	
	
	DEBUG("%s\r\n", __FUNCTION__);
	CdspDev->imgSrc = pFmt->image_source;
	CdspDev->inFmt = pFmt->input_format;
	CdspDev->imgWidth = CdspDev->imgRbWidth = pFmt->hpixel;
	CdspDev->imgHeight = CdspDev->imgRbHeight = pFmt->vline;
	CdspDev->scale.img_rb_h_size = pFmt->hpixel;
	CdspDev->scale.img_rb_v_size = pFmt->vline;
	
	switch(CdspDev->inFmt)
	{
	case V4L2_PIX_FMT_YUYV:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_VY1UY0;
		} else {
			cdsp_inFmt = C_MIPI_FMT_Y1VY0U;	
		}
		CdspDev->rawFmtFlag = 0;
		CdspDev->rawFmt = 0;
		break;
		
	case V4L2_PIX_FMT_YVYU:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_UY1VY0;
		} else {
			cdsp_inFmt = C_MIPI_FMT_Y1VY0U;
		}
		CdspDev->rawFmtFlag = 0;
		CdspDev->rawFmt = 0;
		break;
		
	case V4L2_PIX_FMT_UYVY:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_Y1UY0V;
		} else {
			cdsp_inFmt = C_MIPI_FMT_Y1VY0U;	//mipi format YVYU
		}
		CdspDev->rawFmtFlag = 0;
		CdspDev->rawFmt = 0;
		break;
		
	case V4L2_PIX_FMT_VYUY:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_Y1VY0U;
		} else {
			cdsp_inFmt = C_MIPI_FMT_Y1VY0U;
		}
		CdspDev->rawFmtFlag = 0;
		CdspDev->rawFmt = 0;
		break;

	case V4L2_PIX_FMT_SBGGR8:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW8;
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW8;
		}
		CdspDev->rawFmtFlag = 1;
		CdspDev->rawFmt = 2;
		break;

	case V4L2_PIX_FMT_SGBRG8:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW8;
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW8;
		}
		CdspDev->rawFmtFlag = 1;
		CdspDev->rawFmt = 3;
		break;
		
	case V4L2_PIX_FMT_SGRBG8:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW8;
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW8;
		}
		CdspDev->rawFmtFlag = 1;
		CdspDev->rawFmt = 0;
		break;
		
	case V4L2_PIX_FMT_SRGGB8:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW8;	
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW8;
		}		
		CdspDev->rawFmtFlag = 1;				
		CdspDev->rawFmt = 1;
		break;		
		
	case V4L2_PIX_FMT_SGRBG10:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW10;	
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW10;
		}
		CdspDev->rawFmtFlag = 1;
		CdspDev->rawFmt = 0;
		break;
	case V4L2_PIX_FMT_SRGGB10:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW10;	
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW10;
		}		
		CdspDev->rawFmtFlag = 1;				
		CdspDev->rawFmt = 1;
		break;
		
	case V4L2_PIX_FMT_SBGGR10:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW10;
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW10;
		}
		CdspDev->rawFmtFlag = 1;
		CdspDev->rawFmt = 2;
		break;
		
	case V4L2_PIX_FMT_SGBRG10:
		if(CdspDev->imgSrc == C_CDSP_FRONT) {
			cdsp_inFmt = C_FRONT_FMT_RAW10;
		} else {
			cdsp_inFmt = C_MIPI_FMT_RAW10;
		}
		CdspDev->rawFmtFlag = 1;
		CdspDev->rawFmt = 3;
		break;
		
	default:
		DEBUG("FmtNotSupport!\r\n");
		return STATUS_FAIL;
	}

	DEBUG("cdsp_inFmt = %d , CdspDev->inFmt = %d, CdspDev->rawFmt = %d\r\n", cdsp_inFmt, CdspDev->inFmt, CdspDev->rawFmt);

	/* cdsp clock */
	drv_l1_CdspSetClockTree(ENABLE, CdspDev->imgSrc, CdspDev->rawFmtFlag);
	
	/* cdsp reset */
	drv_l1_CdspReset();
	drv_l1_FrontReset();
	drv_l1_CdspSetDataSource(C_CDSP_SDRAM);

	/* set module */	
	if(CdspDev->rawFmtFlag) {		
		gpCdspIntpl_t intpl;		
		gpCdspEdge_t edge;
		gpCdspNewDenoise_t newdenoise;
		
		drv_l2_CdspGetIntpl(&intpl);		
		drv_l2_CdspSetIntpl(CdspDev->imgWidth, &intpl);		
		
		drv_l2_CdspGetEdge(&edge);		
		drv_l2_CdspSetEdge(CdspDev->rawFmtFlag, &edge);
		
		drv_l2_CdspGetNewDenoise(&newdenoise);
		drv_l2_CdspSetNewDenoise(&newdenoise);
	} else {				
		gpCdspEdge_t edge;
		
		drv_l2_CdspGetEdge(&edge);		
		drv_l2_CdspSetSuppression(CdspDev->imgWidth, &CdspDev->suppression, &edge);	
	}	

	//drv_l2_CdspGetLensCmp(&lens_cmp);	
	//drv_l2_CdspSetLensCmp(CdspDev->rawFmtFlag, &lens_cmp);	
	
	drv_l2_CdspGetYuvHavg(&yuv_havg);	
	drv_l2_CdspSetYuvHavg(&yuv_havg);	

	/* set scale & crop */
	drv_l2_CdspSetScaleCrop(&CdspDev->scale);	
	drv_l2_CdspSetScaleAeAf(&CdspDev->scale);	

	/* set fifo */	
	drv_l2_CdspSetFifoSize(CdspDev->imgRbWidth);

	switch(CdspDev->imgSrc)
	{
	case C_CDSP_FRONT:
		if(CdspDev->rawFmtFlag) {
			DEBUG("FrontRawFmt = %d\r\n", CdspDev->rawFmt);
			drv_l1_CdspSetYuvRange(0x00);
			drv_l1_CdspSetRawDataFormat(CdspDev->rawFmt);
			drv_l1_FrontYuvOrderSet(0);
		} else {
			DEBUG("FrontYuvFmt = %d\r\n", pFmt->input_format);
			drv_l1_CdspSetYuvRange(0x00);
			drv_l1_CdspSetRawDataFormat(0x00);
			switch(pFmt->input_format)
			{
			case V4L2_PIX_FMT_YUYV:
				drv_l1_FrontYuvOrderSet(3);
				break;
			case V4L2_PIX_FMT_UYVY:
				drv_l1_FrontYuvOrderSet(0);
				break;
			case V4L2_PIX_FMT_YVYU:
				drv_l1_FrontYuvOrderSet(1);
				break;
			case V4L2_PIX_FMT_VYUY:
				drv_l1_FrontYuvOrderSet(2);
				break;
			default:
				drv_l1_FrontYuvOrderSet(0);
			}
		}

		/* set interface */
//#if _SET_YUV_PATH_
if(g_set_yuv_path == 1){
		switch(pFmt->output_format)
		{
		case V4L2_PIX_FMT_YUYV:
			drv_l1_CdspSetRawPath(0, 0, 1, 0); //VYUY out
			break;
		case V4L2_PIX_FMT_UYVY:
			drv_l1_CdspSetRawPath(0, 0, 1, 3); //YVYU out
			break;
		case V4L2_PIX_FMT_YVYU:
			drv_l1_CdspSetRawPath(0, 0, 1, 2); //UYVY out
			break;
		case V4L2_PIX_FMT_VYUY:
			drv_l1_CdspSetRawPath(0, 0, 1, 1); //YUYV out
			break;
		default:
			drv_l1_CdspSetRawPath(0, 0, 1, 1); //YUYV out
		}	
}else{
//#else
		if(RAW_BIT == 8) {
			drv_l1_CdspSetRawPath(RAW_MODE, 1, 1, 0); // 8bit raw out in 8-bit format
		} else if(RAW_BIT == 10) {
			drv_l1_CdspSetRawPath(RAW_MODE, 0, 1, 0); // packed 10bit raw out 
		} else {
			DEBUG("Err: Please set bit of raw\r\n");
		}
}
//#endif
		
		drv_l1_FrontInputPathSet(0);		//mipi disable	
		if(CdspDev->rawFmtFlag) {
			drv_l1_FrontInputModeSet(0);	//raw format
			drv_l1_FrontYuvSubEn(0, 0, 0);
			drv_l1_FrontInputGate(0x1FC);
			drv_l1_CdspSetYuvMuxPath(0);	//raw path
			DEBUG("RAW Path\r\n");
		} else {
			drv_l1_FrontInputModeSet(1);	//yuv format
			drv_l1_FrontYuvSubEn(0, 1, 1);
			drv_l1_FrontInputGate(0x1E0);
			drv_l1_CdspSetYuvMuxPath(1);	//yuv path
			DEBUG("YUV Path\r\n");
		}

		if(pFmt->sensor_timing_mode == MODE_CCIR_601) {
			drv_l1_FrontDataTransMode(0);	//ccir601 interfcae
		} else {
			drv_l1_FrontDataTransMode(1);   //ccir656 interface
		}
		
	#if 0 //resharp
		if(CdspDev->rawFmtFlag) {
			reshapeCtl.mode = 0; //RAW
		} else {
			reshapeCtl.mode = 1; //YUV
		}
		
		reshapeCtl.hReshapeEn = 1;	
		reshapeCtl.vReshapeEn = 1;	
		reshapeCtl.vReshapeClkType = 0; // 0: H-sync ; 1: pclk	
		reshapeCtl.vBackOffEn = 0;	
		reshapeCtl.hRise = 2;	
		reshapeCtl.hFall = 1;	
		reshapeCtl.vRise = 2;	
		reshapeCtl.vFall = 1;	
		drv_l1_FrontSetReshape(reshapeCtl);
	#endif
	
		drv_l1_FrontSetSyncPolarity(pFmt->sensor_hsync_mode ? 0 : 1, 
									pFmt->sensor_vsync_mode ? 0 : 1,
									0);//CdspDev->SyncFlag);
	
		roi.hOffset = pFmt->hoffset;	
		roi.vOffset = pFmt->voffset;	
		if(roi.hOffset == 0) {
			roi.hOffset = 1;	
		} 

		if(roi.vOffset == 0) {
			roi.vOffset = 1;
		} 
		
		roi.hSize = pFmt->hpixel;		
		roi.vSize = pFmt->vline;
		if(pFmt->sensor_timing_mode == MODE_CCIR_HREF) {
			roi.vSize -= 1;
		} 
		
		drv_l1_FrontSetRoi(roi, 0);
		break;

	case C_CDSP_MIPI:
		if(CdspDev->rawFmtFlag) {		
			DEBUG("MipiRawFmt = %d\r\n", CdspDev->rawFmt);		
			drv_l1_CdspSetYuvRange(0x00);		
			drv_l1_CdspSetRawDataFormat(CdspDev->rawFmt);
			drv_l1_FrontYuvOrderSet(0);	
		} else {		
			DEBUG("MipiYuvFmt = 0\r\n");		
			drv_l1_CdspSetYuvRange(0x00);		
			drv_l1_CdspSetRawDataFormat(0x00);
			drv_l1_FrontYuvOrderSet(0);	
		}	

		/* set interface */	
//#if _SET_YUV_PATH_		
if(g_set_yuv_path == 1){
		switch(pFmt->output_format)
		{
		case V4L2_PIX_FMT_YUYV:
			drv_l1_CdspSetRawPath(0, 0, 1, 0); //VYUY out
			break;
		case V4L2_PIX_FMT_UYVY:
			drv_l1_CdspSetRawPath(0, 0, 1, 3); //YVYU out
			break;
		case V4L2_PIX_FMT_YVYU:
			drv_l1_CdspSetRawPath(0, 0, 1, 2); //UYVY out
			break;
		case V4L2_PIX_FMT_VYUY:
			drv_l1_CdspSetRawPath(0, 0, 1, 1); //YUYV out
			break;
		default:
			drv_l1_CdspSetRawPath(0, 0, 1, 1); //YUYV out
		}				
}else{
//#else
		if(RAW_BIT == 8) {
			drv_l1_CdspSetRawPath(1, 1, 1, 0); //8bit raw out in 8-bit format
		} else if(RAW_BIT == 10) {
			drv_l1_CdspSetRawPath(RAW_MODE, 0, 1, 0); // packed 10bit raw out 
		} else {
			DEBUG("Err: Please set bit of raw\r\n");
		}
}
//#endif		
		
		drv_l1_FrontInputPathSet(1);		//mipi enable	
		if(CdspDev->rawFmtFlag) {
			drv_l1_FrontInputModeSet(0);	//raw format
			drv_l1_FrontYuvSubEn(0, 0, 0);
			drv_l1_FrontInputGate(0x1FC);	//0x1FF
			drv_l1_CdspSetYuvMuxPath(0);	//raw path
		} else {
			drv_l1_FrontInputModeSet(1);  	//yuv format
			drv_l1_FrontYuvSubEn(0, 1, 1);
			drv_l1_FrontInputGate(0x1E0);	//0x1FF
			drv_l1_CdspSetYuvMuxPath(1);	//yuv path
		}
		
		drv_l1_FrontDataTransMode(0);		//ccir601 interfcae

	#if 0 //resharp		
		if(CdspDev->rawFmtFlag) {
			reshapeCtl.mode = 0; //RAW
		} else {
			reshapeCtl.mode = 1; //YUV
		}
		
		reshapeCtl.hReshapeEn = 1;	
		reshapeCtl.vReshapeEn = 1;	
		reshapeCtl.vReshapeClkType = 0; // 0: H-sync ; 1: pclk	
		reshapeCtl.vBackOffEn = 0;	
		reshapeCtl.hRise = 2;	
		reshapeCtl.hFall = 1;	
		reshapeCtl.vRise = 2;	
		reshapeCtl.vFall = 1;	
		drv_l1_FrontSetReshape(reshapeCtl);
	#endif
	
		drv_l1_FrontSetSyncPolarity(0, 0, 1);

		roi.hOffset = pFmt->hoffset;	
		roi.vOffset = pFmt->voffset;	
		roi.hSize = pFmt->hpixel;	
		roi.vSize = pFmt->vline;	
		drv_l1_FrontSetRoi(roi, 0);
		break;
		
	default:
		DEBUG("ImgSrcErr!\r\n");
		return STATUS_FAIL;
	}
	
	// ae awb setting
	if(CdspDev->sensor_sd) {
		INT32S ret;
		gpCdspCtrl_t csi_ctrl;
		sensor_exposure_t *seInfo;
	
		csi_ctrl.id = V4L2_CID_EXPOSURE;
		ret = CdspDev->sensor_sd->sensor_get_ctrl(&csi_ctrl);
		if(ret < 0) {
			DEBUG("sensor_get_ctrl fail\r\n");
		}
		
		seInfo = (sensor_exposure_t	*) csi_ctrl.value;
		gp_cdsp_ae_set_sensor_exp_time(CdspDev->ae_workmem, seInfo);
		CdspDev->sensor_gain_thr = seInfo->max_analog_gain >> 2;
		CdspDev->night_gain_thr = CdspDev->sensor_gain_thr << 2;
		if(CdspDev->sensor_gain_thr < (1*256)) CdspDev->sensor_gain_thr = 1*256;		
		if(CdspDev->night_gain_thr < (1*256)) CdspDev->night_gain_thr = 1*256;
		if(CdspDev->night_gain_thr > seInfo->max_analog_gain) CdspDev->night_gain_thr = seInfo->max_analog_gain;
		

		CdspDev->sensor_time_thr = seInfo->max_time >> 1;
		if(CdspDev->sensor_time_thr < (8)) CdspDev->sensor_time_thr = 8;

		DEBUG("Sensor: Gain thr = 0x%x, time thr = 0x%x, night_gain_thr = 0x%x\r\n", 
				CdspDev->sensor_gain_thr, CdspDev->sensor_time_thr, CdspDev->night_gain_thr);
				
		CdspDev->getSensorInfo = 1;		
	}
		
	return STATUS_OK;
}

/**
 * @brief	cdsp stream on
 * @param	bufA[in]: buffer a
 * @param	bufB[in]: buffer b
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_stream_on(INT32U ae_awb_task_en, INT32U bufA, INT32U bufB)
{
	INT8U err;
	
	CHAR *p;
	
	drv_l2_sensor_ops_t *SensorDevType;

	SensorDevType = drv_l2_sensor_get_ops(0);

	p = (CHAR *)strrchr((CHAR *)SensorDevType->name, 'm');
	if(p == 0) {
		DBG_PRINT("sensor type get name no m!\r\n");
	}
	
	if(strncmp((CHAR *)p, "mipi", 4) == 0) 
		sensor_type = 1;
	else
		sensor_type = 0;
	DBG_PRINT("sensor type 0x%x\r\n",sensor_type);
	
	
	// wake up awb task here
	if(ae_awb_task_en) {
		drv_l2_ae_awb_lock();
		gp_cdsp_aeawb_reset(CdspDev->ae_workmem, CdspDev->awb_workmem); 
		drv_l2_ae_awb_unlock();
		
		CdspDev->getSensorInfo |= 0x20;
		CdspDev->ae_awb_flag = 0;
		err = OSTaskCreate(drv_l2_ae_awb_process, (void *)CdspDev, &CdspDev->aeawb_task_stack[C_TASK_STACK_SIZE - 1], AE_AWB_PRIORITY);
		if(err != OS_NO_ERR) {
			CdspDev->ae_awb_task_en = 0;
			return STATUS_FAIL;
		}

		err = OSTaskCreate(drv_l2_ae_set_sensor_process, (void *)CdspDev, &CdspDev->ae_set_sensor_task_stack[C_TASK_STACK_SIZE/4 - 1], AE_SET_SENSOR_PRIORITY);
		if(err != OS_NO_ERR) {			
			return STATUS_FAIL;
		}
		
		CdspDev->ae_awb_task_en = 1;
	}

#if (C_DMA_CH == 1) || (C_DMA_CH == 2)
//#if _SET_YUV_PATH_
if(g_set_yuv_path == 1){
#if (C_DMA_CH == 1)
	drv_l1_CdspSetYuvBuffA(CdspDev->imgRbWidth, CdspDev->imgRbHeight, bufA);
	drv_l1_CdspSetDmaConfig(RD_A_WR_A);
#else
	drv_l1_CdspSetYuvBuffB(CdspDev->imgRbWidth, CdspDev->imgRbHeight, bufB);
	drv_l1_CdspSetDmaConfig(RD_B_WR_B);
#endif
//#else
}else{
	drv_l1_CdspSetRawBuff(bufA);
	drv_l1_CdspSetRawBuffSize(CdspDev->imgRbWidth, CdspDev->imgRbHeight, 0, RAW_BIT);
	drv_l1_CdspSetReadBackSize(0, 0, CdspDev->imgRbWidth, CdspDev->imgRbHeight);
}
//#endif
	drv_l1_CdspClrIntStatus(CDSP_INT_ALL);
	drv_l1_CdspSetIntEn(ENABLE, CDSP_OVERFOLW|CDSP_EOF);
	drv_l1_CdspSetDataSource(CdspDev->imgSrc);

#else	
	drv_l1_CdspSetYuvBuffA(CdspDev->imgRbWidth, CdspDev->imgRbHeight, bufA);
	drv_l1_CdspSetYuvBuffB(CdspDev->imgRbWidth, CdspDev->imgRbHeight, bufB);
	drv_l1_CdspSetDmaConfig(AUTO_SWITCH);

	drv_l1_CdspClrIntStatus(CDSP_INT_ALL);
	drv_l1_CdspSetIntEn(ENABLE, CDSP_OVERFOLW|CDSP_EOF);
	drv_l1_CdspSetDataSource(CdspDev->imgSrc);
#endif	

	drv_l1_FrontVdIntEn(ENABLE, 1); // enable VD INT

	CdspDev->RunFlag = 1;
	return 0;
}

/**
 * @brief	cdsp stream off
 * @param	none
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_stream_off(void)
{
	INT8U err;

	// stop awb task here
	if(CdspDev->ae_awb_task_en) {
		CdspDev->ae_awb_task_en = 0;
		err = OSQPost(CdspDev->ae_awb_wait_queue, (void *)C_STOP);
		if(err != OS_NO_ERR) {
			return -1;
		}

		err = OSQPost(CdspDev->ae_set_sensor_msg, (void *)C_STOP);
		if(err != OS_NO_ERR) {
			return -1;
		}
		
		// wait thread stop 
		OSTimeDly(10);
	}
	
	/* disable 3A and IRQ */
	drv_l1_CdspSetAWBEn(DISABLE, DISABLE);
	drv_l1_CdspSetAEEn(DISABLE, DISABLE);
	drv_l1_CdspSetAFEn(DISABLE, DISABLE);	
	drv_l1_CdspSetIntEn(DISABLE, CDSP_AFWIN_UPDATE|CDSP_AWBWIN_UPDATE|CDSP_AEWIN_SEND);	

	drv_l1_FrontVdIntEn(DISABLE, 1); // // enable VD INT
	
	CdspDev->RunFlag = 0;
	return 0;
}

/**
 * @brief	cdsp open
 * @param	none
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_open(void)
{
	INT8U err;
	INT32S ret;
	gpCdspWBGain_t *p_WB_Gain;
	
	if(CdspDev) {
		return -1;
	}
	DBG_PRINT("\r\n%s\r\n", gp_cdsp_aeawb_get_version());
	// cdsp clock enable
	drv_l1_CdspSetClockTree(ENABLE, SDRAM_INPUT, 1);
	
	// cdsp init	
	drv_l1_CdspReset();
	drv_l1_FrontReset();
	drv_l1_CdspSetDataSource(C_CDSP_SDRAM);
	
	drv_l1_CdspSetIntEn(DISABLE, CDSP_INT_ALL);
	drv_l1_CdspClrIntStatus(CDSP_INT_ALL);
	vic_irq_register(VIC_CDSP, drv_l2_cdsp_irq_handle);
	vic_irq_enable(VIC_CDSP);
	
	CdspDev = (gpCdspDev_t *)gp_malloc_align(sizeof(gpCdspDev_t), 4);
	if(CdspDev == 0) {
		return -1;
	}
	
	// cdsp parameter init
	CLEAR(CdspDev, sizeof(gpCdspDev_t));
	CdspDev->suppression.suppressen = ENABLE;
	CdspDev->suppression.suppr_mode = 1;
	CdspDev->suppression.denoisen = ENABLE;
	CdspDev->suppression.lowyen = DISABLE;
	CdspDev->suppression.denoisethrl = 0x4;
	CdspDev->suppression.denoisethrwth= 0x6;
	CdspDev->suppression.yhtdiv= 0x2; 
	
	CdspDev->cdsp_wait_queue = OSQCreate(&CdspDev->cdsp_wait_queue_buf[0], C_QUEUE_MAX);
	if(CdspDev->cdsp_wait_queue == 0) {
		DEBUG("cdsp_wait_queue Err!\r\n");
		RETURN(-1);
	}
	
	CdspDev->ae_awb_wait_queue = OSQCreate(&CdspDev->ae_awb_wait_queue_buf[0], C_QUEUE_MAX);
	if(CdspDev->ae_awb_wait_queue == 0) {
		DEBUG("ae_awb_wait_queue Err!\r\n");
		RETURN(-1);
	}


	CdspDev->ae_set_sensor_msg = OSQCreate(&CdspDev->ae_set_sensor_msg_buf[0], 1);
	if(CdspDev->ae_set_sensor_msg == 0) {
		DEBUG("ae_set_sensor_msg Err!\r\n");
		RETURN(-1);
	}
	
		
	CdspDev->aeawb_sem = OSSemCreate(1);
	if(CdspDev->aeawb_sem == 0) {
		DEBUG("aeawb_sem Err!\r\n");
		RETURN(-1);
	}
	
	CdspDev->ae_workmem = (INT8U *) gp_malloc_align(gp_cdsp_ae_get_workmem_size(), 16);
	if(CdspDev->ae_workmem == 0) {
		DEBUG("ae_workmem alloc Err!\r\n");
		RETURN(-1);
	}
		
	CdspDev->awb_workmem = (INT8U *) gp_malloc_align(gp_cdsp_awb_get_workmem_size(), 16);
	//CdspDev->awb_workmem = (INT8U *) gp_malloc_align(4096, 4);
	if(CdspDev->awb_workmem == 0) {
		DEBUG("awb_workmem alloc Err!\r\n");
		RETURN(-1);
	}
	
	// aeawb initial
	CLEAR(CdspDev->ae_workmem, gp_cdsp_ae_get_workmem_size());
	CLEAR(CdspDev->awb_workmem, gp_cdsp_awb_get_workmem_size());
	CdspDev->color_temp_init_val = 50;
	CdspDev->sensor_ev_idx = -1;
	CdspDev->awbmode = AWB_AUTO_CVR;
	gp_cdsp_aeawb_init(CdspDev->ae_workmem, CdspDev->awb_workmem, AWB_AUTO_CVR);		
	CdspDev->ae_awb_flag = 0;
	
	
	//nickfu 20150514
	#if _SENSOR_SOI_H22_CDSP_MIPI
    	p_WB_Gain = soi_h22_awb_r_b_gain_boundary();    
	#endif
	#if (_SENSOR_GC1004_CDSP)||(_SENSOR_GC1004_CDSP_MIPI)
	p_WB_Gain = gc1004_awb_r_b_gain_boundary();    
	#endif
  	//DBG_PRINT("max_rgain= %d, max_bgain= %d,min_rgain= %d, min_bgain= %d\r\n", p_WB_Gain->max_rgain, p_WB_Gain->max_bgain,  p_WB_Gain->min_rgain,  p_WB_Gain->min_bgain);    
    gp_cdsp_awb_set_r_b_gain_boundary(CdspDev->awb_workmem, p_WB_Gain->max_rgain, p_WB_Gain->max_bgain,  p_WB_Gain->min_rgain,  p_WB_Gain->min_bgain);
	
__exit:
	if(ret < 0) {
		if(CdspDev->cdsp_wait_queue) {
			OSQFlush(CdspDev->cdsp_wait_queue);
			OSQDel(CdspDev->cdsp_wait_queue, OS_DEL_ALWAYS, &err);
		}
		
		if(CdspDev->ae_awb_wait_queue) {
			OSQFlush(CdspDev->ae_awb_wait_queue);
			OSQDel(CdspDev->ae_awb_wait_queue, OS_DEL_ALWAYS, &err);
		}

		if(CdspDev->ae_set_sensor_msg) {
			OSQFlush(CdspDev->ae_set_sensor_msg);
			OSQDel(CdspDev->ae_set_sensor_msg, OS_DEL_ALWAYS, &err);
		}
		
		if(CdspDev->aeawb_sem) {
			OSSemDel(CdspDev->aeawb_sem, OS_DEL_ALWAYS, &err);
		}
		
		if(CdspDev->ae_workmem) {
			gp_free((void *)CdspDev->ae_workmem);
		}

		if(CdspDev->awb_workmem) {
			gp_free((void *)CdspDev->awb_workmem);
		}
		
		gp_free((void *)CdspDev);
		CdspDev = NULL;
	}
	return ret;
}

/**
 * @brief	cdsp close
 * @param	none
 * @return	0: Success, -1: Fail
 */
INT32S drv_l2_cdsp_close(void)
{
	INT8U err;
	
	if(CdspDev == 0) {
		return -1;
	}
	
	// cdsp uninit	
	drv_l1_CdspSetIntEn(DISABLE, CDSP_INT_ALL);
	drv_l1_CdspClrIntStatus(CDSP_INT_ALL);

	drv_l1_CdspReset();
	drv_l1_FrontReset();
	drv_l1_CdspSetDataSource(SDRAM_INPUT);
	
	// cdsp clock disable
	drv_l1_CdspSetClockTree(0, 0, 0);
	
	// free memory
	if(CdspDev->cdsp_wait_queue) {
		OSQFlush(CdspDev->cdsp_wait_queue);
		OSQDel(CdspDev->cdsp_wait_queue, OS_DEL_ALWAYS, &err);
	}
		
	if(CdspDev->ae_awb_wait_queue) {
		OSQFlush(CdspDev->ae_awb_wait_queue);
		OSQDel(CdspDev->ae_awb_wait_queue, OS_DEL_ALWAYS, &err);
	}

	if(CdspDev->ae_set_sensor_msg) {
		OSQFlush(CdspDev->ae_set_sensor_msg);
		OSQDel(CdspDev->ae_set_sensor_msg, OS_DEL_ALWAYS, &err);
	}
		
	if(CdspDev->aeawb_sem) {
		OSSemDel(CdspDev->aeawb_sem, OS_DEL_ALWAYS, &err);
	}

	if(CdspDev->ae_workmem) {
		gp_free((void *)CdspDev->ae_workmem);
	}

	if(CdspDev->awb_workmem) {
		gp_free((void *)CdspDev->awb_workmem);
	}

	gp_free((void *)CdspDev);
	CdspDev = NULL;
	return 0;
}

/************************************
	    ae awb process task
*************************************/
static void drv_l2_cdsp_edge_level_init(void)
{
	int step;

	CdspDev->edge_level[0] = CdspDev->edge_day;
	CdspDev->edge_level[3] = CdspDev->edge_night;

	step = (CdspDev->edge_day - CdspDev->edge_night + 1) / 3;

	CdspDev->edge_level[1] = CdspDev->edge_level[0] - step;
	CdspDev->edge_level[2] = CdspDev->edge_level[3] + step;	
}


static void drv_l2_cdsp_sat_contrast_init(void)
{
	int i;
	int step;
	
	for(i = 0 ; i < 6 ; i++)
	{
		step = (ABS(CdspDev->sat_yuv_level[0][i] - CdspDev->sat_yuv_level[3][i]) + 2) >> 2;
		if(CdspDev->sat_yuv_level[0][i] > CdspDev->sat_yuv_level[3][i]) step = -step;
		
		CdspDev->sat_yuv_level[1][i] = CdspDev->sat_yuv_level[0][i] + step;
		CdspDev->sat_yuv_level[2][i] = CdspDev->sat_yuv_level[3][i] - step;
	}	
}

static void drv_l2_cdsp_sat_contrast_thr_init(int max_ev_idx, int night_ev_idx)
{
	int step;
	int thr0, thr3;
		
	// thr
	thr0 = night_ev_idx - 10;
	thr3 = max_ev_idx - 4;
	step = (thr3 - thr0 + 2) >> 2;
	if(step < 0 ) step = 0;
	CdspDev->sat_yuv_thr[0] = thr0;
	CdspDev->sat_yuv_thr[1] = thr0 + step;
	CdspDev->sat_yuv_thr[2] = thr3 - step;
	CdspDev->sat_yuv_thr[3] = thr3 + step;
}

static void drv_l2_ae_awb_lock(void)
{
	INT8U err;
	
	OSSemPend(CdspDev->aeawb_sem, 0, &err);
}

static void drv_l2_ae_awb_unlock(void)
{
	OSSemPost(CdspDev->aeawb_sem);
}


static void drv_l2_ae_set_sensor_process(void *arg)
{
	
	while(1)
	{
		INT32U msg;
		INT8U err;
		
		msg = (INT32U)OSQPend(CdspDev->ae_set_sensor_msg, 0, &err);
		if(msg != C_SET_SENSOR || err != OS_NO_ERR) {
			DEBUG("drv_l2_ae_set_sensor_process should stop!\r\n");
			break;
		}

		//DBG_PRINT("AE set sensor\r\n");
		if((CdspDev->interrupt_handle[C_ISR_SENSOR] != 0) && ((CdspDev->ae_awb_flag & CDSP_AE_UPDATE) != 0)) {			
			CdspDev->interrupt_handle[C_ISR_SENSOR]();
			CdspDev->ae_awb_flag &= (~CDSP_AE_UPDATE);
		}
	}
}



static void drv_l2_ae_awb_process(void *arg) 
{
	INT8U err;
	INT32U ae_frame, awb_frame;
	INT32S ret, ae_stable = 1;
	gpCdsp3aResult_t *p_3a_result = &CdspDev->result;	
	INT8U *awb;
	INT8U *ae;
	gpCdspCtrl_t csi_ctrl;
	sensor_exposure_t *p_seInfo, seInfo;
	INT32S low_lum_switch_cnt, high_lum_switch_cnt;
	INT32S awb_low_lum_cnt, targetY_low;
	gpCdspWhtBal_t wb_gain;
	int pre_awb_ct;
	
	awb = CdspDev->awb_workmem;
	ae = CdspDev->ae_workmem;	
	DEBUG("\r\n\r\n=========== AE & AWB Process Init =========\r\n\r\n");
	DEBUG("\r\n%s\r\n\r\n", gp_cdsp_aeawb_get_version());
	
	// reset wb gain by initial color temperature	
	//DEBUG("color temperature init = %d\n", CdspDev->color_temp_init_val);
	gp_cdsp_awb_reset_wb_gain(awb, CdspDev->color_temp_init_val, CdspDev->sensor_cdsp.wb_gain);
	wb_gain.wbgainen  = ENABLE;
	wb_gain.bgain = gp_cdsp_awb_get_b_gain(CdspDev->awb_workmem);
	wb_gain.rgain = gp_cdsp_awb_get_r_gain(CdspDev->awb_workmem);	
	wb_gain.gbgain = 64;
	wb_gain.grgain = 64;
	wb_gain.global_gain = 32;
	drv_l2_CdspSetWhiteBalance(&wb_gain);
	
		
	awb_frame = 0;
#if 1
	while(1)
	{
		if(CdspDev->getSensorInfo == 0x21)
		{
			//DEBUG("Get Sensor Info: 0x%x\r\n", CdspDev->getSensorInfo);
			break;
		}

		awb_frame++;
		if(awb_frame >= 500)
		{
			DEBUG("Err: Not Get Sensor Info: 0x%x\r\n", CdspDev->getSensorInfo);
			break;
		}
		
		OSTimeDly(1);
	}
#else
	while(1) {		
		if(CdspDev->getSensorInfo == 0x21 || awb_frame == 0x10000000) 
			break;		
		awb_frame++;	
	}
	if(CdspDev->getSensorInfo != 0x21) DEBUG("Err: Not Get Sensor Info: 0x%x\r\n", CdspDev->getSensorInfo);
#endif
		
	awb_frame = 0;
	ae_frame = 0;
	CdspDev->ae_awb_flag = 0;
	CdspDev->ae_awb_flag |= (CDSP_LUM_STATUS | CDSP_HIGH_LUM);

	CdspDev->sensor_gain_thr = (1*256);
	CdspDev->sensor_time_thr = 0x100;
	if(CdspDev->getSensorInfo == 0x21)
	{
		int max_ev_idx;
		
		csi_ctrl.id = V4L2_CID_EXPOSURE;
		ret = CdspDev->sensor_sd->sensor_get_ctrl(&csi_ctrl);
		p_seInfo = (sensor_exposure_t *) csi_ctrl.value;
		if(ret < 0) {
			CdspDev->sensor_time_thr = 0x200;
			CdspDev->night_gain_thr = 0x200;
			CdspDev->sensor_gain_thr = 0x200;
		} else {		
			CdspDev->sensor_gain_thr = p_seInfo->max_analog_gain >> 2;
			CdspDev->night_gain_thr = CdspDev->sensor_gain_thr << 2;
			if(CdspDev->sensor_gain_thr < (1*256)) CdspDev->sensor_gain_thr = 1*256;		
			if(CdspDev->night_gain_thr < (1*256)) CdspDev->night_gain_thr = 1*256;
								
			if(CdspDev->night_gain_thr > p_seInfo->max_analog_gain)
				CdspDev->night_gain_thr = p_seInfo->max_analog_gain;

			CdspDev->sensor_time_thr = (int)(p_seInfo->max_time/3);
			if(CdspDev->sensor_time_thr < (8)) CdspDev->sensor_time_thr = 8;
		}
		
		gp_cdsp_ae_set_sensor_exp_time(CdspDev->ae_workmem, p_seInfo);

		CdspDev->sat_contr_idx = 0;
		max_ev_idx = p_seInfo->max_ev_idx;
		if(max_ev_idx < (p_seInfo->total_ev_idx - 25)) max_ev_idx = (p_seInfo->total_ev_idx - 25);
		//DBG_PRINT("max_ev_idx = %d, night_ev_idx = %d, total_ev_idx = %d\r\n", max_ev_idx, p_seInfo->night_ev_idx, p_seInfo->total_ev_idx);
		drv_l2_cdsp_sat_contrast_thr_init(max_ev_idx, p_seInfo->night_ev_idx); 
	}

/*	if(CdspDev->sensor_ev_idx != -1)
	{
		csi_ctrl.id = V4L2_CID_EXPOSURE;
		sd = CdspDev->sd;
		ret = sd->ops->core->g_ctrl(sd, &csi_ctrl);
		if(ret < 0) {
			CdspDev->sensor_status = CMS_I2C_TIMEOUT;
		}	
		p_seInfo = (sensor_exposure_t	*) csi_ctrl.value;
		p_seInfo->ae_ev_idx = CdspDev->sensor_ev_idx - p_seInfo->sensor_ev_idx;
		printk("ReInit: sensor ev idx = %d, ae idx = %d\n", p_seInfo->sensor_ev_idx, p_seInfo->ae_ev_idx);
		// reset sensor to saved value
		ret = sd->ops->core->s_ctrl(sd, &csi_ctrl);
		if(ret < 0) {
			CdspDev->sensor_status = CMS_I2C_TIMEOUT;
		}					
	}*/

	gp_cdsp_awb_autoset_r_b_gain_boundary(awb, CdspDev->sensor_cdsp.wb_gain);

	DEBUG("\r\n\r\n=========== AE & AWB Process Start =========\r\n\r\n");


	targetY_low = (int)(CdspDev->ae_target_night * 0.6);
	if(targetY_low > (CdspDev->ae_target >> 1)) CdspDev->ae_target = CdspDev->ae_target/3;
	
	low_lum_switch_cnt = high_lum_switch_cnt = 0;
	awb_low_lum_cnt = 0;
	pre_awb_ct = 50;
	
	while(1) {
		INT32U msg;
		
		msg = (INT32U)OSQPend(CdspDev->ae_awb_wait_queue, 0, &err);
		if(msg != C_WAKE_UP || err != OS_NO_ERR) {
			DEBUG("kthread aeawb process should stop!\r\n");
			break;
		}


		//DEBUG("0: AE/AWB process, flag = 0x%x\r\n", CdspDev->ae_awb_flag);
		
		if((CdspDev->ae_awb_flag & CDSP_AE_CTRL_EN) != 0)
		{			
			if((CdspDev->ae_awb_flag & CDSP_AE_UPDATE) == 0)	{
				ae_frame++;
			}
			else { 
				ae_frame = 0; 
			}

			
			if(ae_frame >= 3)
			{
				int ret;
				unsigned int hist_lo, hist_hi;

				drv_l1_CdspSetAEEn(0, 0);
				
				ae_frame = 0;	
					
				drv_l2_ae_awb_lock();
				
				//DEBUG("1: AE/AWB process, flag = 0x%x\r\n", CdspDev->ae_awb_flag);				
#if 1			
				csi_ctrl.id = V4L2_CID_EXPOSURE;
				ret = CdspDev->sensor_sd->sensor_get_ctrl(&csi_ctrl);
				if(ret < 0) {
					DEBUG("sensor_get_ctrl fail\n");
				}
				
				p_seInfo = (sensor_exposure_t*) csi_ctrl.value;

				//DEBUG("SensorExposure: time = 0x%x, analogGain = 0x%x, digitalGain = 0x%x\r\n", p_seInfo->time, p_seInfo->analog_gain, p_seInfo->digital_gain);

				MEMCPY((INT8S *)&seInfo, (INT8S *)p_seInfo, sizeof(sensor_exposure_t));

				//DEBUG("Digital Gain: %x, %x, %x\n", seInfo.digital_gain, seInfo.max_digital_gain, seInfo.min_digital_gain);
				//DEBUG("Analog Gain: %x, %x, %x\n", seInfo.analog_gain, seInfo.max_analog_gain, seInfo.min_analog_gain);
				//DEBUG("Time: %x, %x, %x\n", seInfo.time, seInfo.max_time, seInfo.min_time);
				hist_lo = 0;
				hist_hi = 0;
				if((CdspDev->ae_awb_flag & CDSP_HIST_CTRL_EN) != 0)
				{
					hist_lo = CdspDev->result.hislowcnt;
					hist_hi = CdspDev->result.hishicnt;
					CdspDev->ae_awb_flag &= (~CDSP_HIST_CTRL_EN);
					//DBG_PRINT("hist_hi = %d, hist_lo = %d\n", hist_hi, hist_lo);
				}
				

				ret = gp_cdsp_ae_calc_exp(ae, p_3a_result->ae_win, &seInfo, hist_hi, hist_lo);
				seInfo.ae_ev_idx = gp_cdsp_ae_get_result_ev(ae);
				//DBG_PRINT("ae_ev_idx = %d, lum = %d\r\n", seInfo.ae_ev_idx, gp_cdsp_ae_get_result_lum(ae));
				
				if(seInfo.ae_ev_idx != 0)
				{	
					//DBG_PRINT("ae_ev_idx = %d, lum = %d\r\n", seInfo.ae_ev_idx, gp_cdsp_ae_get_result_lum(ae));
					
					csi_ctrl.value = (int) &seInfo;
					csi_ctrl.id = V4L2_CID_EXPOSURE;
					ret = CdspDev->sensor_sd->sensor_set_ctrl(&csi_ctrl);
					if(ret < 0) {
						DEBUG("sensor_set_ctrl fail\r\n");
					}
					CdspDev->ae_awb_flag |= CDSP_AE_UPDATE;

					
					
					#if 1
					if(seInfo.sensor_ev_idx >= seInfo.night_ev_idx)
					{ 	// low light
						high_lum_switch_cnt = 0;
						
						if((CdspDev->ae_awb_flag & CDSP_LOW_LUM) == 0)
						{
							low_lum_switch_cnt++;							
							if(low_lum_switch_cnt >= 20) {
								CdspDev->ae_awb_flag &=( ~(CDSP_LUM_STATUS | CDSP_HIGH_LUM | CDSP_LOW_LUM));
								CdspDev->ae_awb_flag |= (CDSP_LUM_STATUS | CDSP_LOW_LUM);
						
								low_lum_switch_cnt = 0;
							}
						}
					}
					else if(seInfo.sensor_ev_idx <= (seInfo.night_ev_idx-35))//if(seInfo.analog_gain <= (int)(256*1.2))
					{ 	// Daylight
						low_lum_switch_cnt = 0;
						if((CdspDev->ae_awb_flag & CDSP_HIGH_LUM) == 0)
						{							
							high_lum_switch_cnt++;
							if(high_lum_switch_cnt >= 20)
							{
								CdspDev->ae_awb_flag &= (~(CDSP_LUM_STATUS | CDSP_HIGH_LUM | CDSP_LOW_LUM));
								CdspDev->ae_awb_flag |= (CDSP_LUM_STATUS | CDSP_HIGH_LUM);
								high_lum_switch_cnt = 0;
							}	
						}
					}
					else
					{
						low_lum_switch_cnt--;
						if(low_lum_switch_cnt < 0) low_lum_switch_cnt = 0;
						high_lum_switch_cnt--;
						if(high_lum_switch_cnt < 0) high_lum_switch_cnt = 0;
					}
					#endif
					
					if(seInfo.sensor_ev_idx >= CdspDev->sat_yuv_thr[3] && CdspDev->sat_contr_idx != 3)
					{						
						CdspDev->sat_contr_idx = 3;
						CdspDev->ae_awb_flag |= CDSP_SAT_SWITCH;
						//DEBUG("sat_contr_idx = %d\r\n", CdspDev->sat_contr_idx);
					}
					else if(seInfo.sensor_ev_idx < CdspDev->sat_yuv_thr[3] && seInfo.sensor_ev_idx >= CdspDev->sat_yuv_thr[2] && CdspDev->sat_contr_idx != 2)
					{						
						CdspDev->sat_contr_idx = 2;
						CdspDev->ae_awb_flag |= CDSP_SAT_SWITCH;
						//DEBUG("sat_contr_idx = %d\r\n", CdspDev->sat_contr_idx);
					}
					else if(seInfo.sensor_ev_idx < CdspDev->sat_yuv_thr[2] && seInfo.sensor_ev_idx >= CdspDev->sat_yuv_thr[1] && CdspDev->sat_contr_idx != 1)
					{						
						CdspDev->sat_contr_idx = 1;
						CdspDev->ae_awb_flag |= CDSP_SAT_SWITCH;
						//DEBUG("sat_contr_idx = %d\r\n", CdspDev->sat_contr_idx);
					}
					else if(seInfo.sensor_ev_idx < CdspDev->sat_yuv_thr[1] && CdspDev->sat_contr_idx != 0)
					{						
						CdspDev->sat_contr_idx = 0;
						CdspDev->ae_awb_flag |= CDSP_SAT_SWITCH;
						//DEBUG("sat_contr_idx = %d\r\n", CdspDev->sat_contr_idx);
					}
					

					ae_stable = 0;
				}
				else {
					ae_stable = 1;
				}	
#endif
				drv_l2_ae_awb_unlock();

				//DEBUG("ae_awb_flag = 0x%x\r\n", CdspDev->ae_awb_flag);

				drv_l1_CdspSetAEEn(1, 0);
			}

			//ae_stable = 1;

			CdspDev->ae_awb_flag &= (~CDSP_AE_CTRL_EN);
			//DEBUG("AE process, flag = 0x%x\r\n", CdspDev->ae_awb_flag);
		}


		if( (CdspDev->ae_awb_flag & CDSP_AWB_CTRL_EN) != 0) 
		{	
			int lum;
			lum = gp_cdsp_ae_get_result_lum(ae);
			if((CdspDev->ae_awb_flag & CDSP_AWB_SET_GAIN) == 0)
			{
				awb_frame++;
				if(awb_frame < 2) CdspDev->ae_awb_flag &= (~CDSP_AWB_SET_GAIN);
			}
			else 
			{
				awb_frame = 0;
				CdspDev->ae_awb_flag &= (~CDSP_AWB_SET_GAIN);
			}
			
			
			if(awb_frame >= 2)
			{
				gpCdspWbGain2_t wb_gain2;
				int ret, awb_ret;
				int awbmode = gp_cdsp_awb_get_mode(awb);
								
				awb_frame = 0;
				
				drv_l2_ae_awb_lock();
				
				//DEBUG("2: AE/AWB process, flag = 0x%x\r\n", CdspDev->ae_awb_flag);
#if 1				
				awb_ret = gp_cdsp_awb_calc_gain(awb, &p_3a_result->awb_result, CdspDev->sensor_cdsp.wb_gain); 
				
				lum = gp_cdsp_ae_get_result_lum(ae);
				if((awb_ret == AWB_FAIL) && ((CdspDev->ae_awb_flag & CDSP_LOW_LUM) != 0) && (lum <= targetY_low) && (CdspDev->sat_contr_idx >= 2))
				{
					awb_low_lum_cnt++;
					if(awb_low_lum_cnt >= 6)
					{
						if(pre_awb_ct > 45)
						{
							// could be night)
							gp_cdsp_awb_reset_wb_gain(awb, 40, CdspDev->sensor_cdsp.wb_gain);
							
							CdspDev->ae_awb_flag |= CDSP_AWB_SET_GAIN;
							
							//DBG_PRINT("AWB low light\r\n");
							awb_low_lum_cnt = 0x80000000;//-512;
							pre_awb_ct = 40;
						}
						else
						{
							awb_low_lum_cnt = 0;
						}
					}
				}
				else if(awb_ret != AWB_FAIL && awb_ret != AWB_RET && awb_low_lum_cnt > 0)
				{
					awb_low_lum_cnt = 0;
				}
				
				if(awb_ret != AWB_FAIL || awb_ret != AWB_RET)
				{
					CdspDev->ae_awb_flag |= CDSP_AWB_SET_GAIN;
				}
					
				#if 1
				if(awb_ret != AWB_RET)
				{
					//DEBUG("awb mode = 0x%x\n", awbmode);
					//if(ret != AWB_SUCCESS_CVR)
					if(awbmode == AWB_AUTO_CVR || awbmode == AWB_AUTO_CVR_DAYLIGHT || awbmode == AWB_AUTO_CVR_NIGHT)
					{
						csi_ctrl.id = V4L2_CID_EXPOSURE;
						ret = CdspDev->sensor_sd->sensor_get_ctrl(&csi_ctrl);
						if(ret < 0) {
							DEBUG("sensor_get_ctrl fail\r\n");
						}
						
						p_seInfo = (sensor_exposure_t	*) csi_ctrl.value;
						//DEBUG("SensorExposure: time = 0x%x, analogGain = 0x%x, digitalGain = 0x%x\r\n", p_seInfo->time, p_seInfo->analog_gain, p_seInfo->digital_gain);
						//DEBUG("Thr: gain = 0x%x, daylight ev idx =%d\n", CdspDev->night_gain_thr, p_seInfo->daylight_ev_idx);
					
						// Auto mode for CVR
						if(gp_cdsp_ae_is_night(ae) == 1) 
						{
							//DBG_PRINT("AWB Night\r\n");
							awbmode = AWB_AUTO_CVR_NIGHT;
							gp_cdsp_awb_set_ct_offset(awb, 0);
						}
						else if(awb_ret == AWB_FAIL) 
						{
							//DEBUG("AWB Auto 1\r\n");
							awbmode = AWB_AUTO_CVR;
						}
						else if(p_seInfo->sensor_ev_idx <= p_seInfo->daylight_ev_idx)
						{
							//DEBUG("AWB Daylight\r\n");
							awbmode = AWB_AUTO_CVR_DAYLIGHT;
							gp_cdsp_awb_set_ct_offset(awb, 0);
						}
						else 				
						{
							//DEBUG("AWB Auto 2\r\n");
							awbmode = AWB_AUTO_CVR;
							gp_cdsp_awb_set_ct_offset(awb, 0);
						}
						awbmode = AWB_AUTO_CVR;
						gp_cdsp_awb_set_mode(awb, awbmode);
						gp_cdsp_awb_calc_gain(awb, &p_3a_result->awb_result, CdspDev->sensor_cdsp.wb_gain);

						//DEBUG("colorT = %d, calc colorT = %d\n", gp_cdsp_awb_get_color_temperature(awb), gp_cdsp_awb_get_calc_color_temperature(awb));
					}
				}
				#endif
				wb_gain2.wbgain2en = ENABLE;
				wb_gain2.rgain2 = gp_cdsp_awb_get_r_gain2(awb);
				wb_gain2.bgain2 = gp_cdsp_awb_get_b_gain2(awb);
				wb_gain2.ggain2 = 64;			
				drv_l2_CdspSetWBGain2(&wb_gain2);

				{
					int rgain, bgain;
					bgain = gp_cdsp_awb_get_b_gain(CdspDev->awb_workmem);
					rgain = gp_cdsp_awb_get_r_gain(CdspDev->awb_workmem);
					//DBG_PRINT("r gain = %d, %d, b gain = %d, %d\r\n", rgain, wb_gain2.rgain2,bgain,wb_gain2.bgain2);
				}
#endif				
				drv_l2_ae_awb_unlock();
			}

			CdspDev->ae_awb_flag &= (~CDSP_AWB_CTRL_EN);
			//DEBUG("AWB process, flag = 0x%x\r\n", CdspDev->ae_awb_flag);
		}
		//DEBUG("3: AE/AWB process, flag = 0x%x\r\n", CdspDev->ae_awb_flag);
	}

	csi_ctrl.id = V4L2_CID_EXPOSURE;
	ret = CdspDev->sensor_sd->sensor_get_ctrl(&csi_ctrl);
		
	p_seInfo = (sensor_exposure_t	*) csi_ctrl.value;
	CdspDev->sensor_ev_idx = p_seInfo->sensor_ev_idx;
	DEBUG("aeawb stop: sensor_ev_idx = %d\r\n", CdspDev->sensor_ev_idx);

	CdspDev->color_temp_init_val = gp_cdsp_awb_get_color_temperature(awb);
	CdspDev->ae_awb_flag = 0;
	CdspDev->ae_awb_flag |= (CDSP_LUM_STATUS | CDSP_HIGH_LUM);

	DEBUG("\r\n\r\n=========== AE & AWB Process Stop =========\r\n\r\n");
	OSTaskDel(OS_PRIO_SELF);
	return;
}


void cdsp_yuyv_restart(INT32U g_frame_addr)
{

	drv_l1_CdspSetRawPath2(0, 0, 1); 
	//Set YUV Buffer Addr
	drv_l1_CdspSetYuvBuffA(1280, 720, g_frame_addr); 

	drv_l1_CdspSetSRAM(ENABLE, 0xA0);

	drv_l1_CdspSetClampEn(ENABLE, 1280);

	//hwCdsp_SetOutYUVFmt(YVYU);	
	R_CDSP_DATA_FORMAT &= ~0xC000;			
	R_CDSP_DATA_FORMAT |= YVYU  << 14; 

}

void save_raw10(INT32U capture_mode, INT32U g_frame_addr)
{

//	capture_mode = LUTGAMMA;
//	drv_l1_CdspSetDataSource(SDRAM_INPUT);	//Sensor data input
	
	//drv_l1_CdspSetIntEn(1,CDSP_INT_ALL); //off all Interupt
	
	//Set RAW Buffer Address
	//hwCdsp_SetRaw10Buff(1280, 720, 0, g_frame_addr);
	drv_l1_CdspSetRawBuffSize(1280,720,0,10);
	drv_l1_CdspSetRawBuff(g_frame_addr);
	
	
	
	drv_l1_CdspSetSRAM(ENABLE, 0xA0);
	drv_l1_CdspSetClampEn(ENABLE, 1280);
	
	drv_l1_IspSetDenoiseEn(DISABLE);

	//read back 
	drv_l1_CdspSetReadBackSize(0x0, 0x0, 1280, 720);
	
	switch (capture_mode)
	{
		case MODE_PURERAW:
		case MODE_LINCORR:
		case MODE_LENSCMP:	
	 		drv_l1_CdspSetRawPath2(0x01, 0x0, 0x0);   //0x240 = 0x01;	//10 bit to 16bit, From LensCmp
		break;
 	
		case MODE_WBGAIN:
			drv_l1_CdspSetWbGainEn(ENABLE);
			drv_l1_CdspSetRawPath2(0x03, 0x0, 0x0);   //0x240 = 0x03;	//10 bit to 16bit, From WBGain
		break;

		case MODE_LUTGAMMA:
			drv_l1_CdspSetWbGainEn(ENABLE);
			drv_l1_CdspSetLutGammaEn(ENABLE);
			drv_l1_CdspSetRawPath2(0x05, 0x0, 0x0);   //0x240 = 0x05	//10 bit to 16bit, From LutGamma
		break;

		default:
			DBG_PRINT("RAW PATH not selsect! \r\n");
		break;
	}
	
	//DBG_PRINT("Getting Sensor Data\r\n");		//imgsorce from front
	DBG_PRINT("_");
	//drv_l1_CdspSetDataSource(FRONT_INPUT);

}

#endif //(defined _DRV_L2_CDSP) && (_DRV_L2_CDSP == 1) 

