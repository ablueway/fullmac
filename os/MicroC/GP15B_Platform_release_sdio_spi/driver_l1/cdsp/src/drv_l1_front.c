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
#include "drv_l1_cdsp.h"
#include "drv_l1_front.h"

#if (defined _DRV_L1_CDSP) && (_DRV_L1_CDSP == 1)                  
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
 
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**
 * @brief	cdsp front reset 
 * @param	none
 * @return	none
 */ 
void drv_l1_FrontReset(void)
{
	R_CDSP_FRONT_GCLK |= 0x100;
	R_CDSP_FRONT_GCLK &= ~0x100;
}

/**
 * @brief	To start Front
 * @param	none
 * @return	none
 */
void drv_l1_FrontStart(void)
{
	R_CDSP_FRONT_CTRL1 |= 0x04;
	R_CDSP_FRONT_CTRL0 |= 1 << 4;
	R_CDSP_TG_ZERO = 0x00;
	R_CDSP_FRONT_GCLK |= 0x0F;
	R_CDSP_FRONT_GCLK |= (1 << 4);
}

/**
 * @brief	To stop Front
 * @param	none
 * @return	none
 */
void drv_l1_FrontStop(void)
{
	R_CDSP_FRONT_GCLK &= ~(1 << 4);
	R_CDSP_FRONT_GCLK &= ~(0x0F);
	R_CDSP_TG_ZERO = 0x1f;
	R_CDSP_FRONT_CTRL0 &= ~(1 << 4);
}

/**
 * @brief	Select input path
 * @param	mode[in]: 0: Normal , 1: mipi
 * @return	none
 */
void drv_l1_FrontInputPathSet(INT8U path)
{
	if (path) {
		R_CDSP_MIPI_CTRL |= (1 << 12);
	} else {
		R_CDSP_MIPI_CTRL &= ~(1 << 12);
	}
}

/**
 * @brief	Select input mode
 * @param	mode[in]: 0: RAW , 1: YUV
 * @return	none
 */
void drv_l1_FrontInputModeSet(INT8U mode)
{
	if (mode) {
		R_CDSP_FRONT_CTRL3 |= (1 << 11);	//YUV input enable
		R_CDSP_FRONT_CTRL3 |= (0x03 << 5); 	//Tvreshhen, Tvreshven
	} else {
		R_CDSP_FRONT_CTRL3 &= ~(1 << 11);	//YUV input disable
		R_CDSP_FRONT_CTRL3 &= ~(0x03 << 5); //Tvreshhen, Tvreshven
	}
}

/**
 * @brief	Y/U/V sub 128 for mipi input
 * @param	ySub[in]: 0: disable, 1:enable
 * @param	uSub[in]: 0: disable, 1:enable
  * @param	vSub[in]: 0: disable, 1:enable 
 * @return	none
 */
void drv_l1_FrontYuvSubEn(INT32U ySub, INT32U uSub, INT32U vSub)
{
	R_CDSP_FRONT_CTRL3 &= ~(0x07 << 8);
	R_CDSP_FRONT_CTRL3 |= ((vSub & 0x1) << 10) | ((uSub & 0x01) << 9) | ((ySub & 0x01) << 8);
}

/**
 * @brief	Select data transfer mode
 * @param	mode[in]: 0: CCIR601 , 1: CCIR656
 * @return	none
 */
void drv_l1_FrontDataTransMode(INT8U mode)
{
	if (mode) {
		R_CDSP_FRONT_CTRL3 |= (1 << 4);
	} else {
		R_CDSP_FRONT_CTRL3 &= ~(1 << 4);
	}
}

INT32U drv_l1_FrontDataTransModeGet(void)
{
	return (R_CDSP_FRONT_CTRL3 >> 4) & 0x01;
}

/**
 * @brief	Select YUV data in sequence
 * @param	mode[in]: 0: UYVY , 1: YVYU , 2: VYUY , 3: YUYV (LSB -> MSB)
 * @return	none
 */
void drv_l1_FrontYuvOrderSet(UINT32 mode)
{
	R_CDSP_FRONT_CTRL3 &= ~(0x03);
	R_CDSP_FRONT_CTRL3 |= (mode & 0x03);
}

/**
 * @brief	Select YUV data in sequence
 * @param	hPol[in]: 0: normal , 1: inverse
 * @param	vPol[in]: 0: normal , 1: inverse
 * @param	in1den[in]: 0: normal , 1: delay 1T
 * @return	none
 */
void drv_l1_FrontSetSyncPolarity(INT8U hPol, INT8U vPol, INT8U in1den)
{
	if(hPol) {
		R_CDSP_FRONT_CTRL1 |= (1 << 4);
	} else {
		R_CDSP_FRONT_CTRL1 &= ~(1 << 4);
	}
	
	if(vPol) {
		R_CDSP_FRONT_CTRL1 |= (1 << 5);
	} else {
		R_CDSP_FRONT_CTRL1 &= ~(1 << 5);
	}
		
	if(in1den) {
		R_CDSP_FRONT_CTRL1 |= (1 << 12);
	} else {
		R_CDSP_FRONT_CTRL1 &= ~(1 << 12);
	}
}

void drv_l1_FrontGetSyncPolarity(INT8U *hPol, INT8U *vPol, INT8U *in1den)
{
	*hPol = ((R_CDSP_FRONT_CTRL1 >> 4) & 0x01) ? 0 : 1;
	*vPol = ((R_CDSP_FRONT_CTRL1 >> 5) & 0x01) ? 0 : 1;
	*in1den = (R_CDSP_FRONT_CTRL1 >> 12) & 0x01;
}

/**
 * @brief	Gate TG input signal to zero
 * @param	tggz[in]: 0: normal , 1: gating to zero, 
 					bit[0]: Exthdi
					bit[1]: Extvdi
					bit[2]: Tvhvalidi
					bit[3]: Tvvvalidi
					bit[4]: Tvdvalidi
 * @return	none
 */
void drv_l1_FrontInputGate(UINT32 tggz)
{
	R_CDSP_TG_ZERO = tggz & 0x1FF;
}

/**
 * @brief	Enable Front interrupt control
 * @param	interrupten[in]: 0: disable , 1: enable
 * @param	vdrintnum[in]: Define How many VD's rising occurs VD Rising interrupt
 * @param	vdfintnum[in]: Define How many VD's falling occurs VD Falling interrupt
 * @param	hdfintnum[in]: Define How many HD's falling occurs HD Falling interrupt 
 * @return	none
 */
void drv_l1_FrontIntCfg(INT32U interrupten, INT32U vdrintnum, INT32U vdfintnum, INT32U hdfintnum)
{
	if (vdfintnum > 15) {
		vdfintnum = 15;
	}
	if(vdfintnum == 0) {
		vdfintnum =1;
	}
	if (vdrintnum > 15) {
		vdrintnum = 15;
	}
	if (vdrintnum == 0) {
		vdrintnum = 1;
	}

	R_CDSP_VD_RFOCC_INT = (vdfintnum << 4) | vdrintnum;
	if(hdfintnum > 4095) {
		hdfintnum = 4095;
	}
	if(hdfintnum == 0) {
		hdfintnum = 1;
	}

	R_CDSP_INTH_NUM = hdfintnum;
	R_CDSP_FRONT_INTEN = interrupten & 0xff;
	R_CDSP_FRONT_VDRF_INTEN = (interrupten >> 8) & 0x03;
}


void drv_l1_FrontVdIntEn(INT32U enable, INT32U vdtype)
{
	R_CDSP_FRONT_VDRF_INT |= vdtype;
	
	if(enable)
		R_CDSP_FRONT_VDRF_INTEN |= vdtype;
	else
		R_CDSP_FRONT_VDRF_INTEN &= (~vdtype);
}

/**
 * @brief	Clear front interrupt event
 * @param	clrintevt[in]: 0: keep value , 1: clear value 
 * @return	none
 */
void drv_l1_FrontClrIntEvent(INT32U clrintevt)
{
	R_CDSP_FRONT_INT = (INT8U)(clrintevt & 0xff);
	R_CDSP_FRONT_VDRF_INT = (UINT8)((clrintevt >> 8) & 0x03);
}

/**
 * @brief	Read front interrupt event 
 * @param	clrintevt[in]: 0: keep value , 1: clear value 
 * @return	none
 */
void drv_l1_FrontGetIntEvent(INT32U *pintevtvalue)
{
	*pintevtvalue = (R_CDSP_FRONT_VDRF_INT << 8) | (R_CDSP_FRONT_INT & 0xFF);
}

/**
 * @brief	Trigger front to snap image
 * @param	number[in]: Number of images to snap (1~16) 
 * @return	none
 */
void drv_l1_FrontSnapTrigger(INT32U number)
{
	if(number == 0) {
		number = 1;
	}
	
	if(number >= 16) {
		number = 0;
	}
		
	R_CDSP_SNAP_CTRL = (1 << 12) | ((number & 0x0F) << 8);
}

/**
 * @brief	Control flah light
 * @param	width[in]: pulse width, (unit 2 pixel clock, 16 bits)
 * @param	linenum[in]: the number of lines after vd falling edge to output pulse
 * @param	mode[in]: Immediate mode , 1: Synchronize-to-linenum mode
 * @return	none
 */
void drv_l1_FrontSetFlash(INT32U width, INT32U linenum, INT32U mode)
{
	R_CDSP_FLASH_WIDTH = width;
	if (linenum > 0x1fff) {
		linenum = 0x1ff;
	}
	
	R_CDSP_FLASH_TRIG_NUM = linenum;
	if(mode) {
		R_CDSP_FRONT_CTRL0 |= (1 << 8);
	} else {
		R_CDSP_FRONT_CTRL0 &= ~(1 << 8);
	}
} 

/**
 * @brief	Trigger flah light
 * @param	none
 * @return	none
 */
void drv_l1_FrontFlashTrigger(void)
{
	R_CDSP_FRONT_CTRL0 |= 1 << 11;
} 

/**
 * @brief	Adjust offset/size of image from sensor
 * @param	roi[in]: region of image 
 * @param	syncVdEn[in]: 0: immediate change value ,  1: synchronizeto the next Vsync
 * @return	none
 */
void drv_l1_FrontSetRoi(gpHalFrontRoi_t roi, INT8U syncVdEn)
{
	if(syncVdEn) {
		R_CDSP_FRONT_CTRL2 |= (0x03 << 4);
	} else {
		R_CDSP_FRONT_CTRL2 &= ~(0x03 << 4);
	}

	//front size set
	R_CDSP_FRAME_H_SETTING = ((roi.hOffset & 0xFFFF) << 16) | (roi.hSize & 0x0FFF);
	R_CDSP_FRAME_V_SETTING = ((roi.vOffset & 0xFFFF) << 16) | (roi.vSize & 0x0FFF);
	
	//mipi size set
	R_CDSP_MIPI_HVSIZE = ((roi.vSize & 0x0FFF) << 12) | (roi.hSize & 0x0FFF);
	R_CDSP_MIPI_HVOFFSET = ((roi.vOffset & 0x0FFF) << 12) | (roi.hOffset & 0x0FFF);
}

void drv_l1_FrontGetRoi(gpHalFrontRoi_t* roi)
{
	roi->hSize = R_CDSP_FRAME_H_SETTING & 0x0FFF;
	roi->hOffset = R_CDSP_FRAME_H_SETTING >> 16;
	roi->vSize = R_CDSP_FRAME_V_SETTING & 0x0FFF;
	roi->vOffset = R_CDSP_FRAME_V_SETTING >> 16;
} 

/**
 * @brief	cdsp front set update enable
 * @param	VdUpdate[in]: 
 * @return	none
 */
void drv_l1_FrontVdUpdateEn(INT32U enable)
{
	if(enable) {
		R_CDSP_FRONT_CTRL0 |= (1 << 8);	
	} else {
		R_CDSP_FRONT_CTRL0 &= ~(1 << 8);
	}
} 


void drv_l1_FrontHdCntSet(UINT32 value)
{
	R_CDSP_FRONT_DUMMY = value;
}


/**
 * @brief	cdsp front set update enable
 * @param	vdflag = 1(VD_RISE)  or 2 (VD_FALL)
 * @return	1: vd enable  ,  0: else
 */
INT32U drv_l1_FrontVdIsEnable(INT32U vdflag)
{
	int ret;

	ret = R_CDSP_FRONT_VDRF_INT & R_CDSP_FRONT_VDRF_INTEN & vdflag;
	
	if(ret != 0)
	{
		R_CDSP_FRONT_VDRF_INT |= vdflag; // clear event
		//drv_l1_FrontHdCntSet(0x0000);
		
		return 1;
	}

	return 0;
}




/**
 * @brief	Reshape internal sync signal 
 * @param	reshapeCtl[in]: resharp control
 * @return	none
 */
void drv_l1_FrontSetReshape(gpHalFrontReshape_t reshapeCtl)
{
	R_CDSP_FRONT_CTRL3 &= ~(0x03 << 5);	//Tvreshhen, Tvreshven = 0
	R_CDSP_FRONT_CTRL3 &= ~(0x0F);		//Hreshen, Vreshen = 0

	if(reshapeCtl.hReshapeEn){
		R_CDSP_HSYNC_FREDGE = ((reshapeCtl.hFall & 0x0FFF) << 16) | (reshapeCtl.hRise & 0x0FFF);
	}
	
	if(reshapeCtl.vReshapeEn){
		R_CDSP_VSYNC_FREDGE = ((reshapeCtl.vFall & 0x0FFF) << 16) | (reshapeCtl.vRise & 0x0FFF);
	}
	
	if (reshapeCtl.mode == 0){ //RAW
		R_CDSP_FRONT_CTRL3 |= ((reshapeCtl.vBackOffEn & 0x01) << 3) | ((reshapeCtl.vReshapeClkType & 0x01) << 2) |
							((reshapeCtl.vReshapeEn & 0x01) << 1) | (reshapeCtl.hReshapeEn & 0x01);
	} else{ //YUV	
		R_CDSP_FRONT_CTRL3 |= ((reshapeCtl.vBackOffEn & 0x01) << 3) | ((reshapeCtl.vReshapeClkType & 0x01) << 2) |
							((reshapeCtl.vReshapeEn & 0x01) << 1) | (reshapeCtl.hReshapeEn & 0x01);					
		R_CDSP_FRONT_CTRL3 |= 0x03 << 5;
	}
} 

/**
 * @brief	cdsp front generate signal
 * @param	siggen_en[in]:
 * @param	sig_clr_bar[in]:  
 * @param	sigmode[in]:  
 * @return	none
 */
void drv_l1_FrontSetSiggen(INT8U siggen_en, INT8U sig_clr_bar, INT32U sigmode)
{
	R_CDSP_SIG_GEN = (sigmode & 0x0F) |
					 ((sig_clr_bar & 0x03) << 5) |
					 ((siggen_en & 0x1) << 7);
} 

/**
 * @brief	cdsp front wait vd
 * @param	mode[in]:
 * @param	number[in]:    
 * @return	none
 */
void drv_l1_FrontWaitVd(INT32U mode, INT32U number)
{
	INT32U i;
	INT32U timeOutCnt = 0x000FFFFF; 
	INT32U tmp0;

	if(mode) {
		for(i=0;i<number;i++) {
			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x02;  //vsync
				timeOutCnt--;
			} while(tmp0 == 0x02 && timeOutCnt > 0);

			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x02; //vsync 
				timeOutCnt--;
			} while(tmp0 == 0x00 && timeOutCnt > 0);
		}
	}
	else {
		for(i=0;i<number;i++) {
    	
			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x02; 
				timeOutCnt--;
			} while(tmp0 == 0x00 && timeOutCnt > 0);

			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x02; 
				timeOutCnt--;
			} while(tmp0 == 0x02 && timeOutCnt > 0);
		}
	} 
} 

/**
 * @brief	cdsp front wait v valid
 * @param	mode[in]:
 * @param	number[in]:    
 * @return	none
 */
INT32U drv_l1_FrontWaitVValid(INT32U mode, INT32U number)
{
	INT32U i;
	INT32U timeOutCnt = 0x000FFFFF,tmp0;
	
	if(mode) {
		for(i=0;i<number;i++) {    	
			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x08; //v vaild sync
				timeOutCnt--;
			} while(tmp0 == 0x08 && timeOutCnt > 0);

			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x08; 		//v vaild sync
				timeOutCnt--;
			} while(tmp0 == 0x00 && timeOutCnt > 0);
		}
	}
	else {
		for(i=0;i<number;i++) { 	
			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x08; 
				timeOutCnt--;
			} while(tmp0 == 0x00 && timeOutCnt > 0);

			timeOutCnt = 0x000FFFFF;
			do {
				tmp0 = R_CDSP_SEN_CTRL_SIG & 0x08; 
				timeOutCnt--;
			} while(tmp0 == 0x08 && timeOutCnt > 0);

		}
	}
	return 0;
} 
#endif //(defined _DRV_L1_CDSP) && (_DRV_L1_CDSP == 1) 