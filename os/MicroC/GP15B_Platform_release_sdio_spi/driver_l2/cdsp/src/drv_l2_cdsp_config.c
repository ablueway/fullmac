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
#include "drv_l2_cdsp_config.h"
#include "drv_l2_cdsp.h"
 
#if (defined _DRV_L2_CDSP) && (_DRV_L2_CDSP == 1) 

INT32S drv_l2_cdsp_set_badpixel_ob(INT32S obval)
{
	gpCdspCtrl_t ctrl;
	gpCdspBadPixOB_t badpixel_ob;
	
	// bad pixel	
	badpixel_ob.badpixen = ENABLE;
	badpixel_ob.badpixmirr = 0x3;
	badpixel_ob.bprthr = 160;
	badpixel_ob.bpgthr =160;
	badpixel_ob.bpbthr = 160;	

	// OB
	// Auto OB
	badpixel_ob.autooben = ENABLE;
	badpixel_ob.obHOffset = 4;
	badpixel_ob.obVOffset = 4;
	badpixel_ob.obtype = 4;
	badpixel_ob.Ravg = 0;
	badpixel_ob.GRavg = 0;
	badpixel_ob.Bavg = 0;
	badpixel_ob.GBavg = 0;
	
	// WB offset
	badpixel_ob.wboffseten = ENABLE;
	badpixel_ob.boffset = 0;
	badpixel_ob.roffset = 0;
	badpixel_ob.gboffset = 0;
	badpixel_ob.groffset = 0;

	// Manu OB
	badpixel_ob.manuoben = ENABLE;
	badpixel_ob.manuob = 42;//obval;


	ctrl.id = MSG_CDSP_BADPIX_OB;
	ctrl.value = (int)&badpixel_ob;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_wb_gain(void)
{
	gpCdspCtrl_t ctrl;
	gpCdspWhtBal_t wht_bal;
	gpCdspWbGain2_t wbgain2;

	// set AWB enable
	wht_bal.wbgainen = ENABLE;	
	wht_bal.bgain = 92;
	wht_bal.gbgain = 1*64;
	wht_bal.rgain = 64;
	wht_bal.grgain = 1*64;
	wht_bal.global_gain = 1*32;
	
	ctrl.id = MSG_CDSP_WBGAIN;
	ctrl.value = (int)&wht_bal;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}

	// set WB gain2 disable
	wbgain2.wbgain2en =ENABLE;
	wbgain2.bgain2 = 64;
	wbgain2.rgain2 = 64;
	wbgain2.ggain2 = 64;
	
	ctrl.id = MSG_CDSP_WBGAIN2;
	ctrl.value = (int)&wbgain2;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	return STATUS_OK;	
}

INT32S drv_l2_cdsp_set_ae_target(INT32S ae_target, INT32S ae_target_night)
{
	gpCdspCtrl_t ctrl;
	
	// set Target Lum		
	ctrl.id = MSG_CDSP_TARGET_AE;
	ctrl.value = (int)&ae_target;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}


	ctrl.id = MSG_CDSP_TARGET_AE_NIGHT;
	ctrl.value = (int)&ae_target_night;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}

	
	return STATUS_OK;	
}

INT32S drv_l2_cdsp_set_aeawb_window(INT16U width, INT16U height)
{
	int t1, t2;
	gpCdspCtrl_t ctrl;
	gpCdspAWB_t awb_win;
	gpCdspAE_t ae;
	gpCdspRawWin_t raw_win;
	
	// set RAW Win
	raw_win.aeawb_src = 1;
	raw_win.subsample = 0;	// 0:disable, 1:1/2, 2:1/4 subsample

	#if 1
	t1 = width;
	while(t1 > 1024)
	{
		raw_win.subsample++;
		t1 >>= 1;
	}
	if(raw_win.subsample > 2) raw_win.subsample = 2;

	t1 = t1 >> 3;
	t2 = 128;
	while(t1 < t2)
	{
		t2 >>= 1;
	}
	if((t1 - t2) <= (t1 >> 2))
		raw_win.hwdsize = t2;//(int)((width - (width >> 3)) / 8) & 0x1f8; // aligment 8bytes
	else
		raw_win.hwdsize = t1 - 2;

	t1 = height >> 3;
	t2 = 128;
	while(t1 < t2)
	{
		t2 >>= 1;
	}
	if((t1 - t2) <= (t1 >> 2))
		raw_win.vwdsize = t2;//(int)((height - (height >> 3)) / 8) & 0x1f8; // aligment 8bytes
	else
		raw_win.vwdsize = t1 - 2;

	
	if(raw_win.hwdsize == 0) raw_win.hwdsize = 4;
	if(raw_win.vwdsize == 0) raw_win.vwdsize = 4;
	raw_win.hwdoffset = ((width >> raw_win.subsample)  - raw_win.hwdsize*8) >> 1;
	raw_win.vwdoffset = (height - raw_win.vwdsize*8) >> 1;
	//if(raw_win.hwdoffset <= 0) raw_win.hwdoffset = 0;
	//if(raw_win.vwdoffset <= 0) raw_win.vwdoffset = 0;
	raw_win.hwdsize >>= 1;
	raw_win.vwdsize >>= 1;
	raw_win.hwdoffset >>= 1;
	raw_win.vwdoffset >>= 1;
	#else
	raw_win.subsample = 1;
	raw_win.hwdsize = 32;
	raw_win.vwdsize = 32;
	raw_win.hwdoffset = 32;
	raw_win.vwdoffset = 26;
	#endif
	DBG_PRINT("RAW_Win Set: subsample = %d, size[%d, %d],  offset[%d, %d]\r\n", raw_win.subsample, raw_win.hwdsize, raw_win.vwdsize, raw_win.hwdoffset, raw_win.vwdoffset);
	
	ctrl.id = MSG_CDSP_RAW_WIN;
	ctrl.value = (int)&raw_win;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	// set AWB win
	awb_win.awb_win_en = ENABLE;
	awb_win.awb_win_hold = DISABLE;
	awb_win.awbclamp_en = DISABLE;//ENABLE;
	awb_win.sindata = 0*64;
	awb_win.cosdata = 1*64;
	awb_win.awbwinthr = 200;
	awb_win.Ythr0 = 50;
	awb_win.Ythr1 = 100;
	awb_win.Ythr2 = 150;	
	awb_win.Ythr3 = 200;
	
	awb_win.UL1N1 = -3;
	awb_win.UL1P1 = 9;
	awb_win.VL1N1 = -3;
	awb_win.VL1P1 = 9;
	
	awb_win.UL1N2 = -7;
	awb_win.UL1P2 = 8;
	awb_win.VL1N2 = -6;
	awb_win.VL1P2 = 8;
	
	awb_win.UL1N3 = -12;
	awb_win.UL1P3 = 8;	
	awb_win.VL1N3 = -9;
	awb_win.VL1P3 = 8;
	
	ctrl.id = MSG_CDSP_AWB_WIN;
	ctrl.value = (int)&awb_win;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}

	// set AE
	ae.ae_win_en = ENABLE;
	ae.ae_win_hold = 0;
	ae.phaccfactor = raw_win.hwdsize;
	ae.pvaccfactor = raw_win.vwdsize;
	ae.ae_meter = CDSP_AE_METER_CENTER_WEIGHTED_AVG_CVR;// CDSP_AE_METER_SPOT; //CDSP_AE_METER_CENTER_WEIGHTED_AVG;
	//ae.ae_meter = CDSP_AE_METER_AVG;
	
	ctrl.id = MSG_CDSP_AE_WIN;
	ctrl.value = (int)&ae;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_gamma(void)
{
	gpCdspCtrl_t ctrl;
	gpCdspGamma_t lut_gamma;
	
	// set gamma
	lut_gamma.lut_gamma_en = ENABLE;	
	ctrl.id = MSG_CDSP_LUT_GAMMA;
	ctrl.value = (int)&lut_gamma;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_wb_offset(INT32S wb_offset_day, INT32S wb_offset_night)
{
	gpCdspCtrl_t ctrl;
	int offset;
	
	offset = wb_offset_day;
	ctrl.id = MSG_CDSP_WB_OFFSET_DAY;
	ctrl.value = (int)&offset;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	offset = wb_offset_night;
	ctrl.id = MSG_CDSP_WB_OFFSET_NIGHT;
	ctrl.value = (int)&offset;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_intp(INT32U hi_thr, INT32U low_thr)
{
	gpCdspCtrl_t ctrl;
	gpCdspIntpl_t intpl;

	intpl.rawspecmode = MODE_NORMAL;
	intpl.int_hi_thr = hi_thr;
	intpl.int_low_thr = low_thr;

	ctrl.id = MSG_CDSP_INTERPOLATION;
	ctrl.value = (int)&intpl;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_colormatrix(void)
{
	gpCdspCtrl_t ctrl;
	gpCdspCorMatrix_t ColorMatrix;
	
	ColorMatrix.colcorren = ENABLE;	
	ctrl.id = MSG_CDSP_COLOR_MATRIX;
	ctrl.value = (int)&ColorMatrix;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_histgm(INT32U hi_thr, INT32U low_thr)
{
	gpCdspCtrl_t ctrl;
	gpCdspHistgm_t histgm;

	histgm.his_en = ENABLE;
	histgm.hishighthr = hi_thr;
	histgm.hislowthr = low_thr;

	ctrl.id = MSG_CDSP_HISTGM;
	ctrl.value = (int)&histgm;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_suppression(void)
{
	gpCdspCtrl_t ctrl;
	gpCdspSuppression_t suppression;
	
	// set suppression
	suppression.suppressen = DISABLE;
	suppression.suppr_mode = 0;
	suppression.denoisen = DISABLE;
	suppression.lowyen = DISABLE;
	suppression.denoisethrl = 0x0;
	suppression.denoisethrwth= 0x0;
	suppression.yhtdiv= 0x0;
	ctrl.id = MSG_CDSP_SUPPRESSION;
	ctrl.value = (int)&suppression;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_edge(INT32S sharpness)
{
	gpCdspCtrl_t ctrl;
	gpCdspEdge_t edge;
	
	edge.edgeen = ENABLE;
	edge.eluten = ENABLE;
	edge.edge_table = 0;
	edge.ampga = sharpness;
	edge.edgedomain = 0x0;
	edge.lhmode = 0;
	edge.lhdiv = 1;
	edge.lhtdiv = 0;
	edge.lhcoring = 1;
	edge.Qcnt = 0;
	edge.Qthr = 255;
	edge.lf00 = 0x9;
	edge.lf01 = 0;
	edge.lf02 = 0x9;
	edge.lf10 = 0;
	edge.lf11 = 0x4;
	edge.lf12 = 0;
	edge.lf20 = 0x9;
	edge.lf21 = 0;
	edge.lf22 = 0x9;
		
	ctrl.id = MSG_CDSP_EDGE;
	ctrl.value = (int)&edge;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_new_denoise(INT32S sharpness)
{
	gpCdspCtrl_t ctrl;
	gpCdspNewDenoise_t NewDenoise;

	NewDenoise.newdenoiseen = ENABLE;
	NewDenoise.ndmirvsel = 0;
	NewDenoise.ndmiren = 0xD;
	
	NewDenoise.ndedgeen = ENABLE;
	NewDenoise.ndeluten = ENABLE;
	NewDenoise.ndedge_table = 0;
	NewDenoise.ndampga = sharpness;
	NewDenoise.ndlf00 = 0x9;
	NewDenoise.ndlf01 = 0;
	NewDenoise.ndlf02 = 0x9;
	NewDenoise.ndlf10 = 0;
	NewDenoise.ndlf11 = 0x4;
	NewDenoise.ndlf12 = 0;
	NewDenoise.ndlf20 = 0x9;
	NewDenoise.ndlf21 = 0;
	NewDenoise.ndlf22 = 0x9;
	NewDenoise.ndlhdiv = 2;
	NewDenoise.ndlhtdiv = 1;
	NewDenoise.ndlhcoring = 0x10;
	NewDenoise.ndlhmode = 0;

	ctrl.id = MSG_CDSP_NEWDENOISE;
	ctrl.value = (int)&NewDenoise;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

//nickfu 20150514
//INT32S drv_l2_cdsp_set_saturation_day(INT32U y_offset, INT32U u_offset, INT32U v_offset, INT32U y_scale, INT32U u_scale, INT32U v_scale)
INT32S drv_l2_cdsp_set_saturation_day(INT32S y_offset, INT32S u_offset, INT32S v_offset, INT32S y_scale, INT32S u_scale, INT32S v_scale)
{
	gpCdspCtrl_t ctrl;
	gpCdspSatHue_t sat_hue;

	// set  Saturation/Constrast enhancement	
	sat_hue.YbYcEn = ENABLE;
	sat_hue.u_huecosdata = 0x40;
	sat_hue.u_huesindata = 0x00;
	sat_hue.v_huecosdata = 0x40;
	sat_hue.v_huesindata = 0x00;
	
	// daylight
	sat_hue.y_offset = y_offset; // -128 ~ +127	
	sat_hue.u_offset = u_offset; // -128 ~ +127,   +: more blue,  -: more yellow/green	
	sat_hue.v_offset = v_offset; //  -128 ~ +127,   +: more red,  -: more blue/green
	
	sat_hue.y_scale = y_scale; // contrast
	sat_hue.u_scale = u_scale; // blud
	sat_hue.v_scale = v_scale; // red
	
	ctrl.id = MSG_CDSP_SAT_HUE_DAY;
	ctrl.value = (int)&sat_hue;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_saturation_night(INT32S y_offset, INT32S u_offset, INT32S v_offset, INT32S y_scale, INT32S u_scale, INT32S v_scale)
{
	gpCdspCtrl_t ctrl;
	gpCdspSatHue_t sat_hue;

	// set  Saturation/Constrast enhancement	
	sat_hue.YbYcEn = ENABLE;
	sat_hue.u_huecosdata = 0x40;
	sat_hue.u_huesindata = 0x00;
	sat_hue.v_huecosdata = 0x40;
	sat_hue.v_huesindata = 0x00;
	
	// daylight
	sat_hue.y_offset = y_offset; // -128 ~ +127	
	sat_hue.u_offset = u_offset; // -128 ~ +127,   +: more blue,  -: more yellow/green	
	sat_hue.v_offset = v_offset; //  -128 ~ +127,   +: more red,  -: more blue/green
	
	sat_hue.y_scale = y_scale; // contrast
	sat_hue.u_scale = u_scale; // blud
	sat_hue.v_scale = v_scale; // red
	
	ctrl.id = MSG_CDSP_SAT_HUE_NIGHT;
	ctrl.value = (int)&sat_hue;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_backlight_dt(INT32U en)
{
	gpCdspCtrl_t ctrl;
	int val = en;

	ctrl.id = MSG_CDSP_BACKLIGHT_DETECT;
	ctrl.value = (int)&val;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_awb_mode(INT32U mode)
{
	gpCdspCtrl_t ctrl;
	AWB_MODE wbmode = mode;

	ctrl.id = MSG_CDSP_WB_MODE;
	ctrl.value = (int)&wbmode;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_ev_val(INT32S ev)
{
	gpCdspCtrl_t ctrl;
	int ev_val = ev;

	ctrl.id = MSG_CDSP_EV;
	ctrl.value = (int)&ev_val;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_raw_special_mode(INT32U mode)
{
	gpCdspCtrl_t ctrl;

	ctrl.id = MSG_CDSP_RAW_SPEF;
	ctrl.value = (int)&mode;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_iso(INT32U iso)
{
	gpCdspCtrl_t ctrl;
	int iso_val;

	switch(iso)
	{
		case 100: 
			iso_val = 1;
			break;
		case 200: 
			iso_val = 2;
			break;
		case 400: 
			iso_val = 4;
			break;
		case 800: 
			iso_val = 8;
			break;
		default:
			iso_val = ISO_AUTO;
			break;
	}

	ctrl.id = MSG_CDSP_ISO;
	ctrl.value = (int)&iso_val;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_yuv_scale(INT16U dst_w, INT16U dst_h)
{
	gpCdspScalePara_t cdspScale;
	gpCdspCtrl_t ctrl;
	
	cdspScale.hscale_en = DISABLE;
	cdspScale.hscale_mode = 1;
	cdspScale.dst_hsize = dst_w;
	
	cdspScale.crop_en = DISABLE;
	cdspScale.crop_hoffset = 0;
	cdspScale.crop_voffset = 0;
	cdspScale.crop_hsize = dst_w;
	cdspScale.crop_vsize = dst_h;
	
	cdspScale.yuvhscale_en = ENABLE;
	cdspScale.yuvvscale_en = ENABLE;
	cdspScale.yuvhscale_mode = 1; // filter	
	cdspScale.yuvvscale_mode = 1; // filter
	cdspScale.yuv_dst_hsize = dst_w;
	cdspScale.yuv_dst_vsize = dst_h;

	cdspScale.img_rb_h_size = dst_w;
	cdspScale.img_rb_v_size = dst_h;

	ctrl.id = MSG_CDSP_SCALE_CROP;
	ctrl.value = (int)&cdspScale;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S drv_l2_cdsp_set_linear_correction(void)
{
	gpCdspLinCorr_t cdspLincorr;
	gpCdspCtrl_t ctrl;
	
	cdspLincorr.lincorren = ENABLE;
	
	ctrl.id = MSG_CDSP_LINCORR;
	ctrl.value = (int)&cdspLincorr;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32U drv_l2_cdsp_set_lens_compensation(void)
{
	gpCdspLensCmp_t cdspLensCmp;
	gpCdspCtrl_t ctrl;
	
	cdspLensCmp.lcen = ENABLE;
	
	ctrl.id = MSG_CDSP_LENS_CMP;
	ctrl.value = (int)&cdspLensCmp;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}


int drv_l2_cdsp_set_edge_ampga(int edge_day, int edge_night)
{
	gpCdspCtrl_t ctrl;
	
	ctrl.id = MSG_CDSP_EDGE_AMPGA_DAY;
	ctrl.value = (int)&edge_day;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return -1;
	}


	ctrl.id = MSG_CDSP_EDGE_AMPGA_NIGHT;
	ctrl.value = (int)&edge_night;
	if(drv_l2_cdsp_set_ctrl(&ctrl) < 0) {
		DBG_PRINT("VIDIOC_S_CTRL fail!!!\r\n");
		return -1;
	}
	
	return 0;
}
void drv_l2_cdsp_enable(INT16U target_w, INT16U target_h, gpCdspFav_t fav)
{
//	int ob_val = 0;
//	int targetY = 0x30; //0x2c;
//	int _awb_mode = AWB_AUTO_CVR;
//	int _sharpness = 3; // 0: soft, 1:standard, 2: sharp, 3: more sharp
//	int _ev = 6; 	  	// 0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0 
//	int _color_mode = MODE_NORMAL;
//	int _iso_mode = ISO_AUTO;

	DBG_PRINT("%s\r\n", __func__);
	
	drv_l2_cdsp_set_badpixel_ob(fav.ob_val);
	drv_l2_cdsp_set_wb_gain();

	drv_l2_cdsp_set_ae_target(fav.ae_target, fav.ae_target_night);
	drv_l2_cdsp_set_aeawb_window(target_w, target_h);
	drv_l2_cdsp_set_gamma();

	drv_l2_cdsp_set_wb_offset(0, 0);

	drv_l2_cdsp_set_intp(240, 32);
	
	drv_l2_cdsp_set_colormatrix();

	drv_l2_cdsp_set_histgm(200, 25);
		
	drv_l2_cdsp_set_suppression();
	drv_l2_cdsp_set_edge(fav.day_edge);
	drv_l2_cdsp_set_new_denoise(fav.night_edge);//

	drv_l2_cdsp_set_edge_ampga(fav.day_edge, fav.night_edge);
	
	drv_l2_cdsp_set_saturation_day(fav.day_hue_y_offset, fav.day_hue_u_offset, fav.day_hue_v_offset, fav.day_sat_y_scale, fav.day_sat_u_scale, fav.day_sat_v_scale);		//hue & satuaration & contrast calibration for day
	drv_l2_cdsp_set_saturation_night(fav.night_hue_y_offset, fav.night_hue_u_offset, fav.night_hue_v_offset, fav.night_sat_y_scale, fav.night_sat_u_scale, fav.night_sat_v_scale);		//hue & satuaration & contrast calibration for day
	//drv_l2_cdsp_set_saturation_night(0, 0, 0, 0x20, 0x20, 0x20);	//hue & satuaration & contrast calibration for night

	drv_l2_cdsp_set_backlight_dt(ENABLE);
	
	drv_l2_cdsp_set_awb_mode(fav.awb_mode);

	drv_l2_cdsp_set_ev_val(fav.ev);
	
	drv_l2_cdsp_raw_special_mode(fav.color_mode);
	
	drv_l2_cdsp_set_iso(fav.iso);

	#if N_STEP==3
	DBG_PRINT("Step 3 no LinCorr LenCmp\r\n");
	#endif
	
	#if N_STEP==4
	DBG_PRINT("Step 4 \r\n");
	drv_l2_cdsp_set_lens_compensation();
	#endif
	
	#if N_STEP==5
	DBG_PRINT("Step 5 \r\n");
	drv_l2_cdsp_set_linear_correction();
	drv_l2_cdsp_set_lens_compensation();
	
	#endif
	DBG_PRINT("cdsp set enable finish\r\n");
} 

#endif
 
 
 