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
#include "drv_l1_sfr.h"
#include "drv_l1_system.h"
#include "drv_l1_cdsp.h"
#include "drv_l1_front.h"
#include "drv_l1_interrupt.h"

#if (defined _DRV_L1_CDSP) && (_DRV_L1_CDSP == 1)                  
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CDSP_TABLE_BASEADDR		(CDSP_BASE + 0x800)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define WRITE32(addr)		(*(volatile unsigned *) (addr))

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct cdspLensData_s 
{
	volatile INT32U  LensTable[256];     /* 0x00~0xFF */        
} cdspLensData_t;

typedef struct cdspGammaData_s 
{
	volatile INT32U  GammaTable[128];   /* 0x000~0x3FF */        
} cdspGammaData_t;

typedef struct cdspEdgeLutData_s 
{
	volatile INT32U  EdgeTable[256];   /* 0x00~0xFF */        
} cdspEdgeLutData_t;

/* New ISP */
typedef struct ispLiCorData_s 
{
	volatile INT32U  LiCorTable[48]; 
} ispLiCorData_t;

typedef struct ispHr0Data_s 
{
	volatile INT32U  Hr0Table[48]; 
} ispHr0Data_t;

typedef struct ispLucData_s 
{
	volatile INT32U MaxTan8[32]; 
	volatile INT32U Slope4[16]; 
	volatile INT32U CLPoint[8];
} ispLucData_t;

typedef struct ispRadiusFile0_s 
{
	volatile INT32U Radius_File_0[512]; 
} ispRadiusFile0_t;

typedef struct ispRadiusFile1_s 
{
	volatile INT32U Radius_File_1[512]; 
} ispRadiusFile1_t;
			
typedef struct cdspNewDEdgeLutData_s 
{
	volatile INT32U  NewDEdgeTable[256];   /* 0x000~0x3FF */        
} cdspNewDEdgeLutData_t;	
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern INT32U MCLK;


/**
 * @brief	cdsp get front int status
 * @param	enable[in]: enable or disable
 * @param	path[in]: data input source, FRONT_INPUT, SDRAM_INPUT and MIPI_INPUT
 * @param	raw_fmt_en[in]: raw data format flag
 * @return	int status
 */
INT32S drv_l1_CdspSetClockTree(INT8U enable, INT32U path, INT8U raw_fmt_en)
{
	INT32U reg = R_SYSTEM_MISC_CTRL0;
	
	// disable 
	if(enable == 0) {
		// Disable CDSP Sensor Clock 
		reg &= ~MASK_SYSMISC_CTL0_SEN2CDSP_CLKO_EN;
		R_SYSTEM_MISC_CTRL0 = reg;
		return STATUS_OK;
	}
	
	//set current is 96MHz	
	R_SYSTEM_CLK_CTRL |= (1 << 3); 		
		
	// Enable CDSP Sensor Clock 
	reg |= MASK_SYSMISC_CTL0_SEN2CDSP_CLKO_EN;
	
	// CDSP Source path Swich
	switch(path)
	{
	case FRONT_INPUT:
		reg &= ~(0x03 << 2); 
		reg |= 1 << 2;
		break;
	case SDRAM_INPUT:
		reg &= ~(0x03 << 2);
		break;
	case MIPI_INPUT:
		reg &= ~(0x03 << 2); 
		reg |= 2 << 2;
		break;
	default:
		return STATUS_FAIL;
	}
	
	// CDSP Source format
	if(raw_fmt_en) {
		reg &= ~(1 << 1);	//bayer raw
	} else {
		reg |= 1 << 1;		//yuv
	}
	
	R_SYSTEM_MISC_CTRL0 = reg;
	return STATUS_OK;	
}

/***************************
    CDSP ISR & INT Handle
****************************/
/**
 * @brief	cdsp get int status
 * @param	none
 * @return	int status
 */
INT32U drv_l1_CdspGetIntStatus(void)
{
	INT32U irq = 0;
	INT32U status = R_CDSP_INT;
	INT32U enable = R_CDSP_INTEN;
	
	// clear status
	R_CDSP_INT = status;
	status &= enable;
	
	if(status & CDSP_FIFO) {
		irq |= CDSP_FIFO;
	}
	
	if(status & CDSP_FACWR) {
		irq |= CDSP_FACWR;
	}
		
	if(status & CDSP_EOF) {
		irq |= CDSP_EOF;
	}

	if(status & CDSP_OVERFOLW) {
		irq |= CDSP_OVERFOLW;
	}
		
	if(status & CDSP_AEWIN_SEND) {
		irq |= CDSP_AEWIN_SEND;
	}
		
	if(status & CDSP_AWBWIN_UPDATE) {
		irq |= CDSP_AWBWIN_UPDATE;
	}
		
	if(status & CDSP_AFWIN_UPDATE) {
		irq |= CDSP_AFWIN_UPDATE;
	}
	return irq;
}

/**
 * @brief	cdsp get front vd int status
 * @param	none
 * @return	int status
 */
INT32U drv_l1_CdspGetFrontVdIntStatus(void)
{
	INT32U irq = 0;
	INT32U status = R_CDSP_FRONT_VDRF_INT;
	INT32U enable = R_CDSP_FRONT_VDRF_INTEN;
	
	//clear status 
	R_CDSP_FRONT_VDRF_INT = status;
	status &= enable;

	if(status & FRONT_VD_RISE) {
		irq |= FRONT_VD_RISE;
	} 
		
	if(status & FRONT_VD_FALL) {
		R_CDSP_FRONT_VDRF_INT |= FRONT_VD_FALL;
		irq |= FRONT_VD_FALL;
	}	

	return irq;
}

/**
 * @brief	cdsp get front int status
 * @param	none
 * @return	int status
 */
INT32U drv_l1_CdspGetFrontIntStatus(void)
{
	INT32U irq = 0;
	INT32U status = R_CDSP_FRONT_INT;
	INT32U enable = R_CDSP_FRONT_INTEN;
	
	// clear status
	R_CDSP_FRONT_INT = status;
	status &= enable;
	 
	if(status & VDR_EQU_VDRINT_NO) {
		irq |= VDR_EQU_VDRINT_NO;
	} 
	
	if(status & VDF_EQU_VDFINT_NO) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & SERIAL_DONE) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & SNAP_DONE) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & CLR_DO_CDSP) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & EHDI_FS_EQU_INTH_NO) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & FRONT_VVALID_RISE) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & FRONT_VVALID_FALL) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & OVERFLOW_RISE) {
		irq |= VDR_EQU_VDRINT_NO;
	}
	
	if(status & OVERFLOW_FALL) {
		irq |= VDR_EQU_VDRINT_NO;
	}

	return irq;
}

/**
 * @brief	cdsp get global int status
 * @param	none
 * @return	int status
 */
INT32U drv_l1_CdspGetGlbIntStatus(void)
{
	INT32U irq = 0;
	
	if(R_CDSP_GINT & CDSP_INT_BIT) {
		irq |= CDSP_INT_BIT;
	}

	if(R_CDSP_GINT & FRONT_VD_INT_BIT) {
		irq |= FRONT_VD_INT_BIT;
	}
	
	if(R_CDSP_GINT & FRONT_INT_BIT) {
		irq |= FRONT_INT_BIT;
	}
	
	return irq;
}

/***************************
       cdsp system
****************************/
/**
 * @brief	cdsp reset 
 * @param	none
 * @return	none
 */
void drv_l1_CdspReset(void)
{
	R_CDSP_RST = 0x01;
	R_CDSP_RST = 0x00;
}

/**
 * @brief	cdsp set clock gating 
 * @param	enable[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetGatingClk(INT8U enable)
{
	if(enable) {
		R_CDSP_GATING_CLK_CTRL = 0x00;
	} else {
		R_CDSP_GATING_CLK_CTRL = 0xFFFFFFFF;
	}
}

/**
 * @brief	cdsp set trigger
 * @param	docdsp[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetRedoTriger(INT8U docdsp)
{
	if(docdsp) {
		R_CDSP_DO |= 0x01;
	} else {
		R_CDSP_DO &= ~0x01;
	}
}

/**
 * @brief	cdsp set gating valid
 * @param	gating_gamma_vld[in]: gating lutgamma valid signal if raw save to DRAM.
 * @param	gating_badpixob_vld[in]: gating badpixel valid signal if YUV Sensor input.
 * @return	none
 */
void drv_l1_CdspSetGatingValid(INT8U gating_gamma_vld, INT8U gating_badpixob_vld)
{
	R_CDSP_DATA_FORMAT &= ~(0x03 << 8);
	R_CDSP_DATA_FORMAT |= ((INT32U)gating_gamma_vld & 0x1) << 8 |
					 ((INT32U)gating_badpixob_vld & 0x1) << 9;
}

/**
 * @brief	cdsp set dark subtraction
 * @param	raw_sub_en[in]: dark subtraction enable.
 * @param	sony_jpg_en[in]: sony input as raw8 data.
 * @return	none
 */
void drv_l1_CdspSetDarkSub(INT8U raw_sub_en, INT8U sony_jpg_en)
{
	R_CDSP_DATA_FORMAT &= ~0x0880;
	R_CDSP_DATA_FORMAT |= (raw_sub_en & 0x1) << 7 |
					((INT32U)sony_jpg_en & 0x1) << 11; 
}

/**
 * @brief	cdsp set input raw format
 * @param	format[in]: input sequence, 0:Gr first, 1:R first, 2:B first, 3:Gb first
 * @return	none
 */
void drv_l1_CdspSetRawDataFormat(INT8U format)
{
	R_CDSP_IMG_TYPE = format & 0x03;
}

/**
 * @brief	cdsp set output yuv range
 * @param	signed_yuv_en[in]: 1 will do 2'component
 * @return	none
 */
void drv_l1_CdspSetYuvRange(INT8U signed_yuv_en)
{
	R_CDSP_YUV_RANGE &= ~0x07;
	R_CDSP_YUV_RANGE |= signed_yuv_en;
}	

/**
 * @brief	cdsp set fifo threshold and overflow enable
 * @param	overflowen[in]: over flow enable
 * @param	sramthd[in]: sram threshold. max is 0x1FF
 * @return	none
 */
void drv_l1_CdspSetSRAM(INT8U overflowen, INT16U sramthd)
{
	if(sramthd > 0x1FF) {
		sramthd = 0x100;
	}
	
	R_CDSP_WSRAM_THR = (sramthd & 0x1FF) | (((INT32U)overflowen & 0x1) << 11);
}

/***************************
        interrupt
****************************/
/**
 * @brief	cdsp set interrupt enable
 * @param	inten[in]: enable or disable
 * @param	bit[in]: CDSP_AFWIN_UPDATE ~ CDSP_FIFO
 * @return	none
 */
void drv_l1_CdspSetIntEn(INT8U inten, INT8U bit)
{
	if (inten) {
		R_CDSP_INT |= bit;	//clear
		R_CDSP_INTEN |= bit;	//enable
	} else {
		R_CDSP_INTEN &= ~bit;	//disable
		R_CDSP_INT |= bit;	//clear
	}
}

/**
* @brief	cdsp interrupt clear status
* @param	bit [in]: clear interrupt bit
* @return	none
*/
void drv_l1_CdspClrIntStatus(INT8U bit)
{
	R_CDSP_INT = bit;
}

/***************************
          other
****************************/
/**
 * @brief	cdsp set line interval
 * @param	line_interval[in]: line interval
 * @return	none
 */
void drv_l1_CdspSetLineInterval(INT16U line_interval)
{
	R_CDSP_LINE_INTERVAL = line_interval;
}

/**
 * @brief	cdsp set external line 
 * @param	linesize[in]: external line size
 * @param	lineblank[in]: external line blank
 * @return	none
 */
void drv_l1_CdspSetExtLine(INT16U linesize, INT16U lineblank)
{
	R_CDSP_EXT_LINE_SIZE = linesize;
	R_CDSP_EXT_BANK_SIZE = lineblank;
}

/***************************
         path set
****************************/
/**
 * @brief	cdsp set input image source 
 * @param	image_source[in]: FRONT_INPUT, SDRAM_INPUT and MIPI_INPUT
 * @return	none
 */
void drv_l1_CdspSetDataSource(INT8U image_source)
{
	// 0:front, 1:sdram, 2:mipi
	R_CDSP_DO &= ~(0x03 << 4);
	R_CDSP_DO |= (image_source << 4);
}

/**
 * @brief	cdsp raw data path set
 * @param	raw_mode [in]: raw data path set, 0:YUV out, 1:Raw data out, 3:WBgain out, 5:LutGamma out
 * @param	cap_mode [in]: 0: package 10 to 16, 1:package 8 to 16  
 * @param	yuv_mode [in]: 0:8y4u4v, 1: yuyv
 * @param	yuv_format[in]: set out sequence, 0:V0Y1U0Y0(YUYV), 1:Y1V0Y0U0(UYVY), 2:U0Y1V0Y0(YVYU), 3:U0Y1V0Y0(YVYU)
 * @return	none
 */
void drv_l1_CdspSetRawPath(INT8U raw_mode, INT8U cap_mode, INT8U yuv_mode, INT8U yuv_fmt)   
{
	R_CDSP_DATA_FORMAT &= ~0xC03F;
	R_CDSP_DATA_FORMAT |= (raw_mode & 0x07) | ((cap_mode & 0x1) << 4) | 
						  ((yuv_mode & 0x1) << 5) | ((yuv_fmt& 0x03) << 14);					
}


/**
 * @brief	cdsp raw data path set 2 (without YUV output format configuration)
 * @param	raw_mode [in]: raw data path set, 0:YUV out, 1:Raw data out, 3:WBgain out, 5:LutGamma out
 * @param	cap_mode [in]: 0: package 10 to 16, 1:package 8 to 16  
 * @param	yuv_mode [in]: 0:8y4u4v, 1: yuyv
 * @return	none
 */
void drv_l1_CdspSetRawPath2(INT8U raw_mode, INT8U cap_mode, INT8U yuv_mode)   
{
	R_CDSP_DATA_FORMAT &= ~0x03F;
	R_CDSP_DATA_FORMAT |= (raw_mode & 0x07) | ((cap_mode & 0x1) << 4) | 
						  ((yuv_mode & 0x1) << 5) ;					
}

/**
 * @brief	cdsp set YUV mux path 
 * @param	redoedge[in]: 1:YUV path, 0:YUV path6
 * @return	none
 */
void drv_l1_CdspSetYuvMuxPath(INT8U redoedge)
{
	if(redoedge) {
		R_CDSP_DO |= 0x02;	//YUV path	
	} else {
		R_CDSP_DO &= ~0x02;	//YUV path6
	}
}

/**
 * @brief	cdsp set len compensation path
 * @param	yuvlens[in]: 1:yuv path, imgcrop, yuvhavg, 0:only yuvhavg
 * @return	none
 */
void drv_l1_CdspSetLensCmpPath(INT8U yuvlens)
{
#if 0 //no this path in GP326033
	if(yuvlens) {
		R_CDSP_LENS_CMP_CTRL |= 0x20;	//YUV path2	
	} else {
		R_CDSP_LENS_CMP_CTRL &= ~0x20;	//YUV path5
	}
#endif
}

/**
 * @brief	cdsp set external line path
 * @param	extinen[in]: external line enable 
 * @param	path[in]: 
 * @return	none
 */
void drv_l1_CdspSetExtLinePath(INT8U extinen, INT8U path)
{
	if(extinen){
		R_CDSP_INP_MIRROR_CTRL |= 0x30;		//B[4]:Suggest it set 1
	} else {
		R_CDSP_INP_MIRROR_CTRL &= ~0x20;
	}

	if(path) {
		R_CDSP_INP_MIRROR_CTRL |= 0x40;
	} else {
		R_CDSP_INP_MIRROR_CTRL &= ~0x40;
	}
}

/**
 * @brief	cdsp set line control
 * @param	ybufen[in]: 0: raw data, 1: YUV data	
 * @return	none
 */
void drv_l1_CdspSetLineCtrl(INT8U ybufen)
{
	if(ybufen) {
		R_CDSP_YUV_CTRL |= 0x80;	//sol1, YUV data	
	} else {
		R_CDSP_YUV_CTRL &= ~0x80;	//sol2, raw data
	}
}

/***************************
    cdsp dma buffer set
****************************/
/**
 * @brief	cdsp set dma YUV buffer a 
 * @param	width[in]: width
 * @param	height[in]: height
 * @param	buffer_addr[in]: buffer address
 * @return	none
 */
void drv_l1_CdspSetYuvBuffA(INT16U width, INT16U height, INT32U buffer_addr)
{
	R_CDSP_DMA_YUVABUF_HVSIZE =  (width & 0xFFF) | (((INT32U)height&0xFFF) << 12);
	R_CDSP_DMA_YUVABUF_SADDR = buffer_addr;
}

/**
 * @brief	cdsp set dma YUV buffer a 
 * @param	width[in]: width
 * @param	height[in]: height
 * @param	buffer_addr[in]: buffer address
 * @return	none
 */
void drv_l1_CdspSetYuvBuffB(INT16U width, INT16U height, INT32U buffer_addr)
{
	R_CDSP_DMA_YUVBBUF_HVSIZE =  (width & 0xFFF) | (((INT32U)height&0xFFF) << 12);
	R_CDSP_DMA_YUVBBUF_SADDR = buffer_addr;
}

/**
 * @brief	cdsp set dma raw buffer size
 * @param	width[in]: width
 * @param	height[in]: height
 * @param	hoffset[in]: h offset
 * @param	rawbit[in]: raw8 or raw10
 * @return	none
 */
void drv_l1_CdspSetRawBuffSize(INT16U width, INT16U height, INT16U hoffset, INT8U rawbit)
{
	R_CDSP_DMA_RAWBUF_HVSIZE =  (((INT32U)width*rawbit/16) & 0xFFF) | 
								((((INT32U)height*rawbit/16) & 0xFFF) << 12);
	R_CDSP_DMA_RAWBUF_HOFFSET = hoffset;
}

/**
 * @brief	cdsp set dma raw buffer address 
 * @param	buffer_addr[in]: buffer address
 * @return	none
 */
void drv_l1_CdspSetRawBuff(INT32U buffer_addr) 
{
	R_CDSP_DMA_RAWBUF_SADDR = buffer_addr;	
}

/**
 * @brief	cdsp set read back size
 * @param	hoffset[in]: h offset
 * @param	voffset[in]: v offset
 * @param	hsize[in]: width
 * @param	vsize[in]: height
 * @return	none
 */
void drv_l1_CdspSetReadBackSize(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize)
{
	R_CDSP_WDRAM_HOFFSET = 0;//hoffset;
	R_CDSP_WDRAM_VOFFSET = 0;//voffset;
	
	R_CDSP_RB_HOFFSET = hoffset;
	R_CDSP_RB_VOFFSET = voffset;
	
	//R_CDSP_RB_HSIZE = ((hsize*rawbit)/8);
	//R_CDSP_RB_VSIZE = ((vsize*rawbit)/8);
	R_CDSP_RB_HSIZE = hsize;
	R_CDSP_RB_VSIZE = vsize;
}

/**
 * @brief	cdsp set buffer operation mode
 * @param	buffer_mode[in]: RD_A_WR_A ~ AUTO_SWITCH
 * @return	none
 */
void drv_l1_CdspSetDmaConfig(INT8U buffer_mode)
{
	if(buffer_mode == RD_A_WR_A) {
		R_CDSP_DMA_COFG = 0x00;
	} else if(buffer_mode == RD_A_WR_B) {
		R_CDSP_DMA_COFG = 0x01;
	} else if(buffer_mode == RD_B_WR_B) {
		R_CDSP_DMA_COFG = 0x03;
	} else if(buffer_mode == RD_B_WR_A) {
		R_CDSP_DMA_COFG = 0x02;
	} else {
		R_CDSP_DMA_COFG = 0x10;
	}
}

/**
 * @brief	cdsp set fifo mode enable
 * @param	frcen[in]: fifo enable
 * @param	fifo_en[in]: auto switch fifo
 * @return	none
 */
void drv_l1_CdspSetFIFOMode(INT8U frcen, INT8U fifo_en)
{
	if(fifo_en){
		R_CDSP_DMA_COFG |= (0x01 << 5) | ((frcen & 0x1) << 4);
	} else {
		R_CDSP_DMA_COFG &= ~((0x01 << 5) | (0x01 << 4));
	}	
}

/**
 * @brief	cdsp set fifo size
 * @param	line_mode[in]: 8, 16, 32, 64
 * @return	none
 */
void drv_l1_CdspSetFifoLine(INT8U line_mode)
{
	R_CDSP_DMA_COFG &= ~(0x03 << 6);
	R_CDSP_DMA_COFG |= (line_mode & 0x3) << 6;
}

/***************************
       clamp set
****************************/
/**
 * @brief	cdsp set clamp width
 * @param	clamphsizeen[in]: clamp enable
 * @param	clamphsize[in]: clamp width
 * @return	none
 */
void drv_l1_CdspSetClampEn(INT8U clamphsizeen, INT16U clamphsize)
{
	/* Enable clamping */
	R_CDSP_CLAMP_SETTING = ((INT32U)clamphsizeen & 0x1) << 12 | clamphsize; 	
}

/**
 * @brief	cdsp set pre RB clamp
 * @param	pre_rb_clamp[in]: 0x00 ~ 0xFF
 * @return	none
 */
void drv_l1_CdspSetPreRBClamp(INT8U pre_rb_clamp)
{
	R_CDSP_INP_QTHR &= ~(0xFF << 8);
	R_CDSP_INP_QTHR |= ((INT32U)pre_rb_clamp) << 8;
}

INT32U drv_l1_CdspGetPreRBClamp(void)
{
	return (R_CDSP_INP_QTHR & 0xFF);
}

/**
 * @brief	cdsp set R B clamp enable
 * @param	rbclampen[in]: R B clamp enable
 * @param	rbclamp[in]: R B clamp value
 * @return	none
 */
void drv_l1_CdspSetRBClamp(INT8U rbclampen, INT8U rbclamp)
{
	R_CDSP_RB_CLAMP_CTRL = ((INT32U)rbclampen << 8) | rbclamp;
}

void drv_l1_CdspGetRBClamp(INT8U *rbclampen, INT8U *rbclamp)
{
	*rbclampen = (R_CDSP_RB_CLAMP_CTRL >> 8) & 0x01;
	*rbclamp = R_CDSP_RB_CLAMP_CTRL & 0xFF;
}

/**
* @brief	cdsp uv division set
* @param	uvDiven [in]: un div function enable
* @return	none
*/
void drv_l1_CdspSetUvDivideEn(INT8U uvDiven)
{
	if(uvDiven) {
		R_CDSP_YUV_CTRL |= 0x01;
	} else {
		R_CDSP_YUV_CTRL &= ~0x01;
	}
}

INT32U gpHalCdspGetUvDivideEn(void)
{
	return (R_CDSP_YUV_CTRL & 0x01);
}

/**
 * @brief	cdsp set UV scale 
 * @param	pDiv[in]: UV scale parameter pointer
 * @return	none
 */
void drv_l1_CdspSetUvDivide(uv_divide_t *pDiv)
{	
	R_CDSP_UVSCALE_COND1 = ((INT32U)pDiv->YT1 << 16) | 
							((INT32U)pDiv->YT2 << 8) | pDiv->YT3;
	R_CDSP_UVSCALE_COND0 = ((INT32U)pDiv->YT4 << 16) |
							((INT32U)pDiv->YT5 << 8) | pDiv->YT6;
}

/**
* @brief	cdsp uv division get
* @param	UVDivide [out]: y value Tn
* @return	none
*/
void drv_l1_CdspGetUvDivide(uv_divide_t *UVDivide)
{
	UVDivide->YT1 = (R_CDSP_UVSCALE_COND1 >> 16) & 0xFF;
	UVDivide->YT2 = (R_CDSP_UVSCALE_COND1 >> 8) & 0xFF;
	UVDivide->YT3 = R_CDSP_UVSCALE_COND1 & 0xFF;
	UVDivide->YT4 = (R_CDSP_UVSCALE_COND0 >> 16) & 0xFF;
	UVDivide->YT5 = (R_CDSP_UVSCALE_COND0 >> 8) & 0xFF;
	UVDivide->YT6 = R_CDSP_UVSCALE_COND0 & 0xFF;
}

/**
 * @brief	cdsp set Q count threshold 
 * @param	Qthr[in]: threshold value
 * @param	PreRBclamp[in]: pre R B clamp value.
 * @return	none
 */
void drv_l1_CdspSetQcntThr(INT16U Qthr, INT16U PreRBclamp)
{
	R_CDSP_INP_QTHR = ((PreRBclamp & 0xFF) << 8) | (Qthr & 0xFF);
}

/***************************
       bad pixel set
****************************/
/**
 * @brief	cdsp set bad pixel 
 * @param	badpixen[in]: bad pixel enable
 * @param	badpixmirrlen[in]: bad pixel mirror enable, bit0: right , bit1: left
 * @return	none
 */
void drv_l1_CdspSetBadPixelEn(INT8U badpixen, INT8U badpixmirrlen)
{
	R_CDSP_BADPIX_CTRL = (badpixmirrlen & 0x3) | ((badpixen & 0x01) << 3);
}

void drv_l1_CdspGetBadPixelEn(INT8U *badpixen, INT8U *badpixmirrlen)
{
	*badpixmirrlen = R_CDSP_BADPIX_CTRL & 0x03;
	*badpixen = (R_CDSP_BADPIX_CTRL >> 3) & 0x01;	
}

/**
 * @brief	cdsp set bad pixel threshold value
 * @param	bprthr[in]: r channel threshold for bad pixel
 * @param	bpgthr[in]: g channel threshold for bad pixel
 * @param	bpbthr[in]: b channel threshold for bad pixel
 * @return	none
 */
void drv_l1_CdspSetBadPixel(INT8U bprthr, INT8U bpgthr, INT8U bpbthr)
{	
	R_CDSP_BADPIX_CTHR = (bprthr & 0xFF) |
						(((INT32U)bpgthr & 0xFF) << 8) |
						(((INT32U)bpbthr & 0xFF) << 16);
}

void drv_l1_CdspGetBadPixel(INT8U *bprthr, INT8U *bpgthr, INT8U *bpbthr)
{	
	*bprthr = R_CDSP_BADPIX_CTHR & 0xFF;
	*bpgthr = (R_CDSP_BADPIX_CTHR >> 8) & 0xFF;
	*bpbthr = (R_CDSP_BADPIX_CTHR >> 16) & 0xFF;
}

/***************************
     optical black set
****************************/
/**
 * @brief	cdsp set manual optical black 
 * @param	manuoben[in]: Optical black manual mode enable
 * @param	manuob[in]: Manual subtracted value of optical black (sign value) -1024~1023
 * @return	none
 */
void drv_l1_CdspSetManuOB(INT8U manuoben, INT16U manuob)
{
	R_CDSP_OPB_CTRL = (manuob & 0x7FF)|(((INT32U)manuoben & 0x1) << 15);	
}

void drv_l1_CdspGetManuOB(INT8U *manuoben, INT16U *manuob)
{
	*manuob = R_CDSP_OPB_CTRL & 0x7FF;
	*manuoben = (R_CDSP_OPB_CTRL >> 15) & 0x01;
}

/**
 * @brief	cdsp set auto optical black 
 * @param	autooben[in]: Optical black auto mode enable
 * @param	obtype[in]: Optical black accumulation block type
 * @param	obtype[in]: Optical black subtraction using current frame accumulation value
 * @return	none
 */
void drv_l1_CdspSetAutoOB(INT8U autooben, INT8U obtype, INT8U curfob)
{
	R_CDSP_OPB_TYPE = (obtype & 0x7) | ((autooben & 0x1) << 3) | ((curfob & 0x1) << 4); 	
}

void drv_l1_CdspGetAutoOB(INT8U *autooben, INT8U *obtype, INT16U *obHOffset, INT16U *obVOffset)
{
	*autooben = (R_CDSP_OPB_TYPE >> 3) & 0x01;
	*obtype = R_CDSP_OPB_TYPE & 0x07;
	*obHOffset = R_CDSP_OPB_HOFFSET;
	*obVOffset = R_CDSP_OPB_VOFFSET;
}

/**
 * @brief	cdsp set optical black h and v offset 
 * @param	obHOffset[in]: optical black h offset 
 * @param	obVOffset[in]: optical black v offset 
 * @return	none
 */
void drv_l1_CdspSetOBHVoffset(INT16U obHOffset, INT16U obVOffset)
{
	R_CDSP_OPB_HOFFSET = obHOffset & 0xFFF;
	R_CDSP_OPB_VOFFSET = obVOffset & 0xFFF;
}

/**
 * @brief	cdsp get optical black average value 
 * @param	ob_avg_r[out]: Optical black R average value
 * @param	ob_avg_gr[out]: Optical black Gr average value
 * @param	ob_avg_b[out]: Optical black B average value
 * @param	ob_avg_gb[out]: Optical black Gb average value
 * @return	none
 */
void drv_l1_CdspGetAutoOBAvg(INT16U *ob_avg_r, INT16U *ob_avg_gr, INT16U *ob_avg_b, INT16U *ob_avg_gb)
{
	*ob_avg_r = R_CDSP_OPB_RAVG;
	*ob_avg_gr = R_CDSP_OPB_GRAVG;
	*ob_avg_b = R_CDSP_OPB_BAVG;
	*ob_avg_gb = R_CDSP_OPB_GBAVG;
}

/***************************
     lens compensation
****************************/
/**
 * @brief	cdsp set lens compensation table
 * @param	plensdata[in]: table
 * @return	none
 */
void drv_l1_CdspSetLensCmpTable(INT16U *plensdata)
{
	INT32U i;
	cdspLensData_t *pLensData = (cdspLensData_t *)(CDSP_TABLE_BASEADDR);

	R_CDSP_MACRO_CTRL = 0x0101;	
	for(i=0; i<256; i++) {
		pLensData->LensTable[i] = *plensdata++;
	}
	
	R_CDSP_MACRO_CTRL = 0x00; /* Select None RAM */
}

/**
 * @brief	cdsp set lens compensation enable
 * @param	lcen[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetLensCmpEn(INT8U lcen)
{
	if(lcen) {
		R_CDSP_LENS_CMP_CTRL |= (1 << 4);		
	} else {
		R_CDSP_LENS_CMP_CTRL &= ~(1 << 4);
	}			
}

INT32U drv_l1_CdspGetLensCmpEn(void)
{
	return ((R_CDSP_LENS_CMP_CTRL >> 4) & 0x01);
}

/**
* @brief	cdsp lens compensation postion
* @param	centx [in]: X center 
* @param	centy [in]: Y center
* @param	xmoffset [in]: X offset
* @param	ymoffset [in]: Y offset
* @return	none
*/
void drv_l1_CdspSetLensCmpPos(INT16U centx, INT16U centy, INT16U xmoffset, INT16U ymoffset) 
{
	R_CDSP_IM_XCENT = centx;
	R_CDSP_IM_YCENT = centy;
	R_CDSP_LENS_CMP_XOFFSET_MAP &= ~0xFFF;
	R_CDSP_LENS_CMP_XOFFSET_MAP |= xmoffset;	
	R_CDSP_LENS_CMP_YOFFSET_MAP = ymoffset;
}

void drv_l1_CdspGetLensCmpPos(INT16U *centx, INT16U *centy, INT16U *xmoffset, INT16U *ymoffset) 
{
	*centx = R_CDSP_IM_XCENT;
	*centy = R_CDSP_IM_YCENT;
	*xmoffset = R_CDSP_LENS_CMP_XOFFSET_MAP & 0xFFF;	
	*ymoffset = R_CDSP_LENS_CMP_YOFFSET_MAP;
}

/**
* @brief	cdsp lens compensation 
* @param	stepfactor [in]: step unit between entries of len shading compensation LUT
* @param	xminc [in]: X increase step
* @param	ymoinc [in]: Y increase step odd line
* @param	ymeinc [in]: Y increase step even lin
* @return	none
*/
void drv_l1_CdspSetLensCmp(INT8U stepfactor, INT8U xminc, INT8U ymoinc, INT8U ymeinc) 
{
	R_CDSP_LENS_CMP_CTRL &= ~0x07;
	R_CDSP_LENS_CMP_CTRL |= (stepfactor & 0x07);

	R_CDSP_LENS_CMP_XOFFSET_MAP &= ~(0x0F << 12);	
	R_CDSP_LENS_CMP_XOFFSET_MAP = (((INT16U)xminc & 0x0F) << 12);	
	R_CDSP_LENS_CMP_YINSTEP_MAP = (((INT16U)ymeinc & 0x0F) << 4) | (ymoinc & 0x0F);
}

void drv_l1_CdspGetLensCmp(INT8U *stepfactor, INT8U *xminc, INT8U *ymoinc, INT8U *ymeinc) 
{
	*stepfactor = R_CDSP_LENS_CMP_CTRL & 0x07;
	*xminc = (R_CDSP_LENS_CMP_XOFFSET_MAP >> 12) & 0x0F;
	*ymeinc	= (R_CDSP_LENS_CMP_YINSTEP_MAP >> 4) & 0x0F;
	*ymoinc = R_CDSP_LENS_CMP_YINSTEP_MAP & 0x0F;	
}

/***************************
    New ISP Macro Table
****************************/
/**
 * @brief	cdsp set table
 * @param	plicordata[in]: table
 * @return	none
 */
void drv_l1_IspSetLiCor(INT8U *plicordata)
{
	INT32U i;
	ispLiCorData_t *pLicordata = (ispLiCorData_t *)(CDSP_TABLE_BASEADDR);

	R_CDSP_MACRO_CTRL = 0x1414;	//gp15b
	for(i=0; i<48; i++) {
		pLicordata->LiCorTable[i] = *plicordata++;
	}
	
	R_CDSP_MACRO_CTRL = 0x00; /* Select None RAM */
}

/**
 * @brief	cdsp set table
 * @param	value[in]: table, 1280*24
 * @return	none
 */
void drv_l1_IspSetHr0(INT32U value)	
{
	INT32S i;

	R_CDSP_MACRO_CTRL = 0x1515;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x5515;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x9515;
	for(i=0; i<256; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x00;
}

/**
 * @brief	cdsp set table
 * @param	value[in]: table, 1280*24
 * @return	none
 */
void drv_l1_IspSetHr1(INT32U value)	
{
	INT32S i;

	R_CDSP_MACRO_CTRL = 0x1616;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x5616;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x9616;
	for(i=0; i<256; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x00;
}

/**
 * @brief	cdsp set table
 * @param	value[in]: table, 1280*24
 * @return	none
 */
void drv_l1_IspSetHr2(INT32U value)
{
	INT32S i;

	R_CDSP_MACRO_CTRL = 0x1717;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x5717;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x9717;
	for(i=0; i<256; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x00;
}

/**
 * @brief	cdsp set table
 * @param	value[in]: table, 1280*24
 * @return	none
 */
void drv_l1_IspSetHr3(INT32U value)
{
	INT32S i;

	R_CDSP_MACRO_CTRL = 0x1818;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x5818;
	for(i=0; i<512; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x9818;
	for(i=0; i<256; i++) {
		WRITE32(CDSP_TABLE_BASEADDR+i*4) = value;
	}	
	
	R_CDSP_MACRO_CTRL = 0x00;
}

//("../model/MaxTan8_File.txt","../model/slope4_File.txt","../model/CLPoint_File.txt");
/**
 * @brief	GP_ISP_luc_MaxTan8_Slop_CLP_Table 
 * @param	pmaxtan8data[in]: table 
 * @param	pslopdata[in]: table 
 * @param	pclpdata[in]: table 
 * @return	none
 */
void drv_l1_IspSetLucMaxTan8SlopCLP(INT16U *pmaxtan8data, INT16U *pslopdata, INT16U *pclpdata)
{
	INT32U i;
	ispLucData_t *pLucdata = (ispLucData_t *)(CDSP_TABLE_BASEADDR);

	R_CDSP_MACRO_CTRL = 0x1C1C;	
	for(i=0; i<32; i++) {
		pLucdata->MaxTan8[i] = *pmaxtan8data++;
	}
	
	for(i=0; i<16; i++) {
		pLucdata->Slope4[i] = *pslopdata++;
	}
	
	for(i=0; i<8; i++) {
		pLucdata->CLPoint[i] = *pclpdata++;
	}

	R_CDSP_MACRO_CTRL = 0x00; /* Select None RAM */
}

/**
 * @brief	GP_ISP_Luc_Radius_Table
 * @param	pradusf0data[in]: table 
 * @return	none
 */
void drv_l1_IspSetRadusF0(INT16U *pradusf0data)
{
	INT32U i;
	ispRadiusFile0_t *pRadusF0data = (ispRadiusFile0_t *)(CDSP_TABLE_BASEADDR);

	R_CDSP_MACRO_CTRL = 0x1919;	
	for(i=0; i<512; i++) {
		pRadusF0data->Radius_File_0[i] = *pradusf0data++;
	}
	
	R_CDSP_MACRO_CTRL = 0x00; /* Select None RAM */
}

/**
 * @brief	GP_ISP_Luc_Radius_Table
 * @param	pradusf1data[in]: table 
 * @return	none
 */
void drv_l1_IspSetRadusF1(INT16U *pradusf1data)
{
	INT32U i;
	ispRadiusFile1_t *pRadusF1data = (ispRadiusFile1_t *)(CDSP_TABLE_BASEADDR);

	R_CDSP_MACRO_CTRL = 0x1A1A;	//gp15b
	for(i=0; i<512; i++) {
		pRadusF1data->Radius_File_1[i] = *pradusf1data++;
	}
	
	R_CDSP_MACRO_CTRL = 0x00; /* Select None RAM */
}

/**
 * @brief	cdsp set isp lens compensation
 * @param	lcen[in]: enable
 * @param	stepfactor[in]: step factor 
 * @return	none
 */
void drv_l1_IspSetLenCmp(INT32U lcen,INT32U stepfactor)
{
	R_ISP_HR_LUT_CTRL &= ~0xFF;
	R_ISP_HR_LUT_CTRL = (stepfactor & 0x1F) | ((lcen & 0x1) << 7); 				
}

/***************************
      raw h scaler
****************************/
/**
 * @brief	cdsp set raw h scale enable
 * @param	hscale_en[in]: h scale enable
 * @param	hscale_mode[in]: 0: drop, 1: filter
 * @return	none
 */
void drv_l1_CdspSetRawHScaleEn(INT8U hscale_en, INT8U hscale_mode)
{
	R_CDSP_SCLDW_CTRL &= ~0x09;
	R_CDSP_SCLDW_CTRL |= (hscale_mode & 0x1) | ((hscale_en & 0x1) << 3);
}

/**
 * @brief	cdsp set raw h scale 
 * @param	src_hsize[in]: input h size
 * @param	dst_hsize[in]: output h size
 * @return	none
 */
void drv_l1_CdspSetRawHScale(INT32U src_hsize, INT32U dst_hsize)
{
	INT32U factor;

	if (dst_hsize >= src_hsize) {
		R_CDSP_HRAW_SCLDW_FACTOR = 0x0000;
	} else {
		factor = (dst_hsize << 16) / (src_hsize) + 1;
		R_CDSP_HSCALE_EVEN_PVAL = (factor/2)+0x8000;
		R_CDSP_HSCALE_ODD_PVAL = factor;
		R_CDSP_HRAW_SCLDW_FACTOR = factor;
	}

	//reflected at next vaild vd edge
	//R_CDSP_SCALE_FACTOR_CTRL |= 0x01;
}

/***************************
     white balance set
****************************/
/**
 * @brief	cdsp set white balance gain enable
 * @param	wbgainen[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetWbGainEn(INT8U wbgainen)
{
	if(wbgainen) {
		R_CDSP_YUVSPEC_MODE |= (1 << 7);	
	} else {
		R_CDSP_YUVSPEC_MODE &= ~(1 << 7);
	}

	//reflected at vd update
	//R_CDSP_YUVSPEC_MODE |= 0x10; 
}

/**
 * @brief	cdsp set white balance gain
 * @param	r_gain[in]: R gain
 * @param	gr_gain[in]: GR gain
 * @param	b_gain[in]: B gain
 * @param	gb_gain[in]: GB gain
 * @return	none
 */
void drv_l1_CdspSetWbGain(INT16U r_gain, INT16U gr_gain, INT16U b_gain, INT16U gb_gain)
{
	R_CDSP_WHBAL_RSETTING &= ~0x1FF;
	R_CDSP_WHBAL_RSETTING |= r_gain & 0x1FF;
	R_CDSP_WHBAL_GRSETTING &= ~0x1FF;
	R_CDSP_WHBAL_GRSETTING |= gr_gain & 0x1FF;
	R_CDSP_WHBAL_BSETTING &= ~0x1FF;
	R_CDSP_WHBAL_BSETTING |= b_gain & 0x1FF;
	R_CDSP_WHBAL_GBSETTING &= ~0x1FF;
	R_CDSP_WHBAL_GBSETTING |= gb_gain & 0x1FF;

	
}

/**
 * @brief	cdsp set white balance offset enable
 * @param	wboffseten[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetWbOffsetEn(INT8U wboffseten)
{
	if(wboffseten == 0) {
		R_CDSP_YUVSPEC_MODE |= (1 << 6);
	} else {
		R_CDSP_YUVSPEC_MODE &= ~(1 << 6);
	}
	
	//reflected at vd update
	//R_CDSP_YUVSPEC_MODE |= 0x10; 
}

INT32U drv_l1_CdspGetWbOffsetEn(void)
{
	return ((R_CDSP_YUVSPEC_MODE >> 6) & 0x01);
}

/**
 * @brief	cdsp set white balance offset
 * @param	roffset[in]: R offset 
 * @param	groffset[in]: GR offset 
 * @param	boffset[in]: B offset
 * @param	gboffset[in]: GB offset
 * @return	none
 */
void drv_l1_CdspSetWbOffset(INT8U roffset, INT8U groffset, INT8U boffset, INT8U gboffset)
{
	R_CDSP_WHBAL_RSETTING &= ~(0xFF << 12);
	R_CDSP_WHBAL_RSETTING |= ((INT32U)roffset & 0xFF) << 12;
	R_CDSP_WHBAL_GRSETTING &= ~(0xFF << 12);
	R_CDSP_WHBAL_GRSETTING |= ((INT32U)groffset & 0xFF) << 12;
	R_CDSP_WHBAL_BSETTING &= ~(0xFF << 12);
	R_CDSP_WHBAL_BSETTING |= ((INT32U)boffset & 0xFF) << 12;
	R_CDSP_WHBAL_GBSETTING &= ~(0xFF << 12);
	R_CDSP_WHBAL_GBSETTING |= ((INT32U)gboffset & 0xFF) << 12;
}

/**
 * @brief	cdsp get white balance gain
 * @param	pwbgainen[out]: enable or disable
 * @param	prgain[out]: R gain 
 * @param	pgrgain[out]: GR gain 
 * @param	pbgain[out]: B gain
 * @param	pgbgain[out]: GB gain
 * @return	none
 */
INT32U drv_l1_CdspGetWbGain(INT16U *prgain, INT16U *pgrgain, INT16U *pbgain, INT16U *pgbgain)
{
	*prgain = R_CDSP_WHBAL_RSETTING & 0x1FF;
	*pgrgain = R_CDSP_WHBAL_GRSETTING & 0x1FF;
	*pbgain = R_CDSP_WHBAL_BSETTING & 0x1FF;
	*pgbgain = R_CDSP_WHBAL_GBSETTING & 0x1FF;
	return ((R_CDSP_YUVSPEC_MODE >> 7) & 0x01);
}

/**
 * @brief	cdsp get white balance offset
 * @param	pwboffseten[out]: enable or disable
 * @param	proffset[out]: R offset 
 * @param	pgroffset[out]: GR offset 
 * @param	pboffset[out]: B offset
 * @param	pgboffset[out]: GB offset
 * @return	none
 */
void drv_l1_CdspGetWbOffset(INT8U *pwboffseten, INT8S *proffset, INT8S *pgroffset, INT8S *pboffset,	INT8S *pgboffset)
{
	*pwboffseten = (R_CDSP_YUVSPEC_MODE >> 6) & 0x01;
	*proffset = (R_CDSP_WHBAL_RSETTING >> 12) & 0xFF;
	*pgroffset =(R_CDSP_WHBAL_GRSETTING >> 12) & 0xFF;
	*pboffset = (R_CDSP_WHBAL_BSETTING >> 12) & 0xFF;
	*pgboffset = (R_CDSP_WHBAL_GBSETTING >> 12) & 0xFF;
}

/***************************
     global gain
****************************/
/**
 * @brief	cdsp set global gain value
 * @param	global_gain[in]: gain
 * @return	none
 */
void drv_l1_CdspSetGlobalGain(INT8U global_gain)
{
	R_CDSP_GLOBAL_GAIN = global_gain & 0xFF;
}

/**
 * @brief	cdsp get global gain value
 * @param	none
 * @return	gain value
 */
INT32U drv_l1_CdspGetGlobalGain(void)
{
	return R_CDSP_GLOBAL_GAIN;
} 

/***************************
          gain2
****************************/
/**
 * @brief	cdsp set white balance gain2 enable
 * @param	wbgain2en[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetWbGain2En(INT8U wbgain2en)
{
	//vaild when input format is raw.
	if(wbgain2en) {
		R_CDSP_AWB_WIN_BGAIN2 |= (1 << 11);
	} else {
		R_CDSP_AWB_WIN_BGAIN2 &= ~(1 << 11);
	}
}

/**
 * @brief	cdsp set white balance gain2
 * @param	rgain2[in]: R gain
 * @param	ggain2[in]: G gain
 * @param	bgain2[in]: B gain
 * @return	none
 */
void drv_l1_CdspSetWbGain2(INT16U rgain2, INT16U ggain2, INT16U bgain2)
{
	//vaild when input format is raw.
	R_CDSP_AWB_WIN_RGGAIN2 = (rgain2 & 0x1FF) | (((INT32U)ggain2 & 0x1FF) << 12);
	R_CDSP_AWB_WIN_BGAIN2 &= (~0x1FF);
	R_CDSP_AWB_WIN_BGAIN2 |= bgain2 & 0x1FF;
}

/**
 * @brief	cdsp set white balance gain2
 * @param	wbgain2en[in]: enable or disable
 * @param	rgain2[in]: R gain
 * @param	ggain2[in]: G gain
 * @param	bgain2[in]: B gain
 * @return	none
 */
void drv_l1_CdspGetWbGain2(INT8U *pwbgain2en, INT16U *prgain, INT16U *pggain, INT16U *pbgain)
{
	*pwbgain2en = (R_CDSP_AWB_WIN_RGGAIN2 >> 11) & 0x01; 
	*prgain = R_CDSP_AWB_WIN_RGGAIN2 & 0x1FF;
	*pggain = (R_CDSP_AWB_WIN_RGGAIN2>>12) & 0x1FF;
	*pbgain = R_CDSP_AWB_WIN_BGAIN2 & 0x1FF;
}

/***************************
         gamma set
****************************/
/**
 * @brief	cdsp set gamma table
 * @param	pGammaTable[in]: table
 * @return	none
 */
void drv_l1_CdspSetGammaTable(INT32U *pGammaTable)
{
	INT16U i;
	cdspGammaData_t *pGammaData = (cdspGammaData_t *)(CDSP_TABLE_BASEADDR);
	
	R_CDSP_MACRO_CTRL = 0x0202;
	for(i=0; i<128; i++) { 
		pGammaData->GammaTable[i] = *pGammaTable++;
	}
	
	R_CDSP_MACRO_CTRL = 0;
}

/**
 * @brief	cdsp set lut gamma enable 
 * @param	lut_gamma_en[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetLutGammaEn(INT8U lut_gamma_en)
{
	if(lut_gamma_en) {
		R_CDSP_YUVSPEC_MODE |= 0x100;
	} else {
		R_CDSP_YUVSPEC_MODE &= ~0x100;
	}
}

INT32U drv_l1_CdspGetLutGammaEn(void)
{
	return ((R_CDSP_YUVSPEC_MODE >> 8) & 0x01);	
}

/***************************
      interpolation set
****************************/
/**
 * @brief	cdsp set interpolation and mirror
 * @param	intplmirhlen[in]: Interpolation h/v/t/d mirror 2 pixels
 * @param	intplmirvsel[in]: Interpolation vertical down mirror position selection, suggest set 1
 * @param	intplcnt2sel[in]: Intpl cnt2 ini sel 0: 3'h0 1: 3'h7
 * @return	none
 */
void drv_l1_CdspSetIntplMirEn(INT8U intplmirhlen, INT8U intplmirvsel, INT8U intplcnt2sel)
{
	R_CDSP_INP_MIRROR_CTRL &= ~0x9F;
	R_CDSP_INP_MIRROR_CTRL |= ((intplcnt2sel & 0x1) << 7) |
						((intplmirvsel & 0x1) << 4) |
						(intplmirhlen & 0x0F);
						
}

/**
 * @brief	cdsp set Interpolation threshold
 * @param	int_low_thr[in]: Interpolation low threshold
 * @param	int_hi_thr[in]: Interpolation hight threshold 
 * @return	none
 */
void drv_l1_CdspSetIntplThr(INT8U int_low_thr, INT8U int_hi_thr)
{
	R_CDSP_INP_DENOISE_THR = ((INT16U)int_hi_thr << 8) | int_low_thr;
}

void drv_l1_CdspGetIntplThr(INT8U *int_low_thr, INT8U *int_hi_thr)
{
	*int_low_thr = R_CDSP_INP_DENOISE_THR & 0xFF;
	*int_hi_thr = (R_CDSP_INP_DENOISE_THR >> 8) & 0xFF;
}

/***************************
  edge in intpolation set
****************************/
/**
 * @brief	cdsp set edge enable
 * @param	edgeen[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetEdgeEn(INT8U edgeen)
{
	if(edgeen) {
		R_CDSP_INP_EDGE_CTRL |= 0x01;
	} else {
		R_CDSP_INP_EDGE_CTRL &= ~0x01;
	}
}

/**
 * @brief	cdsp get edge enable
 * @param	none
 * @return	status
 */
INT32U drv_l1_CdspGetEdgeEn(void)
{
	return (R_CDSP_INP_EDGE_CTRL & 0x01);
}

/**
 * @brief	cdsp set edge filter matrix
 * @param	pEdgeFilter[in]: edge filter matrix
 * @return	none
 */
void drv_l1_CdspSetEdgeFilter(edge_filter_t *pEdgeFilter) 
{
	//vaild when R_INP_EDGE_CTRL.bit0 = 1
	R_CDSP_HPF_LCOEF0 = ((pEdgeFilter->LPF02 & 0x0F) << 8) | ((pEdgeFilter->LPF01 & 0x0F) << 4) | (pEdgeFilter->LPF00 & 0x0F);
	R_CDSP_HPF_LCOEF1 = ((pEdgeFilter->LPF12 & 0x0F) << 8) | ((pEdgeFilter->LPF11 & 0x0F) << 4) | (pEdgeFilter->LPF10 & 0x0F);
	R_CDSP_HPF_LCOEF2 = ((pEdgeFilter->LPF22 & 0x0F) << 8) | ((pEdgeFilter->LPF21 & 0x0F) << 4) | (pEdgeFilter->LPF20 & 0x0F);
}

/**
 * @brief	cdsp get edge filter matrix
 * @param	pEdgeFilter[in]: edge filter matrix
 * @return	none
 */
void drv_l1_CdspGetEdgeFilter(edge_filter_t *pEdgeFilter) 
{
	pEdgeFilter->LPF00 = R_CDSP_HPF_LCOEF0 & 0x0F;
	pEdgeFilter->LPF01 = (R_CDSP_HPF_LCOEF0 >> 4) & 0x0F;
	pEdgeFilter->LPF02 = (R_CDSP_HPF_LCOEF0 >> 8) & 0x0F;
	pEdgeFilter->LPF10 = R_CDSP_HPF_LCOEF1 & 0x0F;
	pEdgeFilter->LPF11 = (R_CDSP_HPF_LCOEF1 >> 4) & 0x0F;
	pEdgeFilter->LPF12 = (R_CDSP_HPF_LCOEF1 >> 8) & 0x0F;
	pEdgeFilter->LPF20 = R_CDSP_HPF_LCOEF2 & 0x0F;
	pEdgeFilter->LPF21 = (R_CDSP_HPF_LCOEF2 >> 4) & 0x0F;
	pEdgeFilter->LPF22 = (R_CDSP_HPF_LCOEF2 >> 8) & 0x0F;
}

/**
 * @brief	cdsp set edge LCoring
 * @param	lhdiv[in]: L edge enhancement edge value scale for Lcoring path
 * @param	lhtdiv[in]: L edge enhancement edge value scale for Clamp path
 * @param	lhcoring[in]: L core ring threshold value as calculating edge data
 * @param	lhmode[in]: For switch high-pass filter path. 0: default, 1: user define
 * @return	none
 */
void drv_l1_CdspSetEdgeLCoring(INT8U lhdiv, INT8U lhtdiv, INT8U lhcoring,INT8U lhmode)
{
	R_CDSP_LH_DIV_CTRL = (lhdiv & 0x07) | ((lhtdiv& 0x07) << 4) |
						(((INT32U)lhcoring & 0x1FF) << 8) |
						(((INT32U)lhmode & 0x1) << 16); 
}

/**
 * @brief	cdsp get edge LCoring
 * @param	lhdiv[out]: L edge enhancement edge value scale for Lcoring path
 * @param	lhtdiv[out]: L edge enhancement edge value scale for Clamp path
 * @param	lhcoring[out]: L core ring threshold value as calculating edge data
 * @param	lhmode[out]: For switch high-pass filter path. 0: default, 1: user define
 * @return	none
 */
void drv_l1_CdspGetEdgeLCoring(INT8U *lhdiv, INT8U *lhtdiv, INT8U *lhcoring, INT8U *lhmode)
{
	*lhdiv = R_CDSP_LH_DIV_CTRL & 0x07;
	*lhtdiv = (R_CDSP_LH_DIV_CTRL >> 4) & 0x07; 
	*lhcoring = (R_CDSP_LH_DIV_CTRL >> 8) & 0x1FF;
	*lhmode = (R_CDSP_LH_DIV_CTRL >> 16) & 0x01;
}

/**
 * @brief	cdsp set edge amplifier
 * @param	ampga[in]: 0: x1 1:x2 2:x3 3:x4
 * @return	none
 */
void drv_l1_CdspSetEdgeAmpga(INT8U ampga)
{
	R_CDSP_INP_EDGE_CTRL &= ~(0x03 << 6);
	R_CDSP_INP_EDGE_CTRL |= (ampga & 0x03) << 6 ;
}

/**
 * @brief	cdsp get edge amplifer gain
 * @param	none
 * @return	gain
 */
INT32U drv_l1_CdspGetEdgeAmpga(void)
{
	return (R_CDSP_INP_EDGE_CTRL >> 6) & 0x3;
}

/**
 * @brief	cdsp set edge domain
 * @param	edgedomain[in]: 0: add edge on Y value, 1: add edge on RGB value
 * @return	none
 */
void drv_l1_CdspSetEdgeDomain(INT8U edgedomain)
{
	R_CDSP_INP_EDGE_CTRL &= ~(1 << 2);
	R_CDSP_INP_EDGE_CTRL |= (edgedomain & 0x1) << 2;
}

/**
 * @brief	cdsp get edge domain
 * @param	none
 * @return	domain
 */
INT32U drv_l1_CdspGetEdgeDomain(void)
{
	return (R_CDSP_INP_EDGE_CTRL >> 2) & 0x1;
}

/**
 * @brief	cdsp edge Q threshold set
 * @param	Qthr [in]: edge threshold 
 * @return	none
 */
void drv_l1_CdspSetEdgeQthr(INT8U Qthr)
{
	R_CDSP_INP_QTHR &= ~0xFF;
	R_CDSP_INP_QTHR |= (Qthr & 0xFF);
}

/**
 * @brief	cdsp get edge Q summary 
 * @param	none
 * @return	q value
 */
INT32U drv_l1_CdspGetEdgeQCnt(void)
{
	return R_CDSP_INP_QCNT;
}

/***************************
    edge lut table set
****************************/
/**
 * @brief	cdsp set edge lut table
 * @param	pLutEdgeTable[in]: table
 * @return	none
 */
void drv_l1_CdspSetEdgeLutTable(INT8U *pLutEdgeTable)
{
	INT32U i;
	cdspEdgeLutData_t *pEdgeLutData = (cdspEdgeLutData_t *)(CDSP_TABLE_BASEADDR);
	
	R_CDSP_MACRO_CTRL = 0x0404;
	for(i = 0; i < 256; i++) { 
		pEdgeLutData->EdgeTable[i] = *pLutEdgeTable++;
	}
	
	R_CDSP_MACRO_CTRL = 0x00;
}

/**
 * @brief	cdsp set edge lut enable
 * @param	eluten[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetEdgeLutTableEn(INT8U eluten)
{
	if(eluten) {
		R_CDSP_INP_EDGE_CTRL |= 0x10;
	} else {
		R_CDSP_INP_EDGE_CTRL &= ~0x10;
	}
}

/**
 * @brief	cdsp get edge lut enable
 * @param	none 
 * @return	status
 */
INT32U drv_l1_CdspGetEdgeLutTableEn(void)
{
	return (R_CDSP_INP_EDGE_CTRL >> 4) & 0x01;
}

/***************************
    color matrix set
****************************/
/**
* @brief	cdsp color matrix enable
* @param	colcorren [in]: color matrix enable
* @return	none
*/
void drv_l1_CdspSetColorMatrixEn(INT8U colcorren)
{
	if(colcorren) {		
		R_CDSP_CC_COF4 |= (1 << 11);
	} else {		
		R_CDSP_CC_COF4 &= ~(1 << 11);	
	}
}

INT32U drv_l1_CdspGetColorMatrixEn(void)
{
	return ((R_CDSP_CC_COF4 >> 11) & 0x1);
}

/**
 * @brief	cdsp set color matrix enable
 * @param	pCM[in]: color matrix pointer
 * @return	none
 */
void drv_l1_CdspSetColorMatrix(color_matrix_t *pCM)
{
	R_CDSP_CC_COF0 = ((pCM->CcCof01 & 0x3FF) << 12) | (pCM->CcCof00 & 0x3FF);
	R_CDSP_CC_COF1 = ((pCM->CcCof10 & 0x3FF) << 12) | (pCM->CcCof02 & 0x3FF);
	R_CDSP_CC_COF2 = ((pCM->CcCof12 & 0x3FF) << 12) | (pCM->CcCof11 & 0x3FF);
	R_CDSP_CC_COF3 = ((pCM->CcCof21 & 0x3FF) << 12) | (pCM->CcCof20 & 0x3FF);
	R_CDSP_CC_COF4 &= ~(0x3FF);
	R_CDSP_CC_COF4 |= (pCM->CcCof22 & 0x3FF);
}

/**
 * @brief	cdsp get color matrix enable
 * @param	pCM[in]: color matrix pointer
 * @return	none
 */
void drv_l1_CdspGetColorMatrix(color_matrix_t *pCM)
{	
	pCM->CcCof00 = R_CDSP_CC_COF0 & 0x3FF;
	pCM->CcCof01 = (R_CDSP_CC_COF0 >> 12) & 0x3FF;
	pCM->CcCof02 = R_CDSP_CC_COF1 & 0x3FF;
	pCM->CcCof10 = (R_CDSP_CC_COF1 >> 12) & 0x3FF;
	pCM->CcCof11 = R_CDSP_CC_COF2 & 0x3FF;
	pCM->CcCof12 = (R_CDSP_CC_COF2 >> 12) & 0x3FF;	
	pCM->CcCof20 = R_CDSP_CC_COF3 & 0x3FF;
	pCM->CcCof21 = (R_CDSP_CC_COF3 >> 12) & 0x3FF;
	pCM->CcCof22 = R_CDSP_CC_COF4 & 0x3FF;
}

/***************************
    raw special mode set
****************************/
/**
 * @brief	cdsp set special mode in raw format
 * @param	rawspecmode[in]: 0 ~ 6
 * @return	none
 */
void drv_l1_CdspSetRawSpecMode(INT8U rawspecmode)
{
	if(rawspecmode > 6)
		return;
	
	R_CDSP_RGB_SPEC_ROT_MODE &= ~0x0F;
	R_CDSP_RGB_SPEC_ROT_MODE |= rawspecmode & 0x07;	
	
	//reflected at vd ypdate 
	//R_CDSP_RGB_SPEC_ROT_MODE |= 0x08;
}

INT32U drv_l1_CdspGetRawSpecMode(void)
{
	return (R_CDSP_RGB_SPEC_ROT_MODE & 0x07);	
}

/***************************
  YUV insert & coring set
****************************/
/**
 * @brief	cdsp set YUV444 insert enable
 * @param	yuvinserten[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetYuv444InsertEn(INT8U yuvinserten)
{
	if(yuvinserten) {
		R_CDSP_YUV_CTRL |= 0x08;
	} else {
		R_CDSP_YUV_CTRL &= ~0x08;
	}
}

INT32U drv_l1_CdspGetYuv444InsertEn(void)
{
	return ((R_CDSP_YUV_CTRL >> 3) & 0x01);
}

/**
 * @brief	cdsp set YUV Coring
 * @param	y_corval_coring[in]: Y subtraction | Y Coring
 * @param	u_corval_coring[in]: U subtraction | U Coring
 * @param	v_corval_coring[in]: V subtraction | V Coring
 * @return	none
 */
void drv_l1_CdspSetYuvCoring(INT8U y_corval_coring, INT8U u_corval_coring, INT8U v_corval_coring)
{
	R_CDSP_YUV_CORING_SETTING = (y_corval_coring & 0xFF) |
								((u_corval_coring & 0xFF) << 8) | 
								((v_corval_coring & 0xFF) << 16);	 							
}

void drv_l1_CdspGetYuvCoring(INT8U *y_corval_coring, INT8U *u_corval_coring, INT8U *v_corval_coring)
{
	*y_corval_coring = R_CDSP_YUV_CORING_SETTING & 0xFF;
	*u_corval_coring = (R_CDSP_YUV_CORING_SETTING >> 8) & 0xFF;
	*v_corval_coring = (R_CDSP_YUV_CORING_SETTING >> 16) & 0xFF;	 							
}

/***************************
       crop set
****************************/
/**
 * @brief	cdsp set crop windows enable
 * @param	hvEnable[in]: crop enable
 * @return	none
 */
void drv_l1_CdspSetCropEn(INT8U hvEnable)
{
	if(hvEnable) {
		hvEnable = 0x3;
		if(R_CDSP_IMCROP_HOFFSET == 0) {
			hvEnable &= ~0x01;
		}

		if(R_CDSP_IMCROP_VOFFSET == 0) {
			hvEnable &= ~0x02;
		}

		R_CDSP_IMCROP_CTRL = hvEnable; 
	} else {
		R_CDSP_IMCROP_CTRL = 0;
	}
	
	//reflected at vd update
	//R_IMCROP_CTRL |= 0x10;
	
	//crop sync zoomfactor
	//R_IMCROP_CTRL |= 0x20;
}

/**
 * @brief	cdsp set crop windows
 * @param	hoffset[in]: x offset
 * @param	voffset[in]: y offset
 * @param	hsize[in]: width
 * @param	vsize[in]: height
 * @return	none
 */
void drv_l1_CdspSetCrop(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize)
{
	R_CDSP_IMCROP_HOFFSET = hoffset;
	R_CDSP_IMCROP_VOFFSET = voffset;
	R_CDSP_IMCROP_HSIZE = hsize;
	R_CDSP_IMCROP_VSIZE = vsize;
}

/**
 * @brief	cdsp get crop windows
 * @param	hvEnable[out]: crop enable
 * @param	hoffset[out]: x offset
 * @param	voffset[out]: y offset
 * @param	hsize[out]: width
 * @param	vsize[out]: height
 * @return	none
 */
void drv_l1_CdspGetCrop(INT8U *hvEnable, INT32U *hoffset, INT32U *voffset, INT32U *hsize, INT32U *vsize)
{   
	*hvEnable = R_CDSP_IMCROP_CTRL;
	*hoffset = R_CDSP_IMCROP_HOFFSET;
	*voffset = R_CDSP_IMCROP_VOFFSET;
	*hsize = R_CDSP_IMCROP_HSIZE;
	*vsize = R_CDSP_IMCROP_VSIZE;
}

/***************************
       YUV havg set
****************************/
/**
 * @brief	cdsp set YUV h average
 * @param	yuvhavgmiren[in]: mirror enable
 * @param	ytype[in]: 0: disable 1: 3 tap 2: 5 tap
 * @param	utype[in]: 0: disable 1: 3 tap 2: 5 tap
 * @param	vtype[in]: 0: disable 1: 3 tap 2: 5 tap
 * @return	none
 */
void drv_l1_CdspSetYuvHAvg(INT8U yuvhavgmiren, INT8U ytype, INT8U utype, INT8U vtype)
{
	R_CDSP_YUV_AVG_LPF_TYPE = (ytype & 0x03) | ((utype & 0x03) << 2) | ((vtype & 0x03) << 4) |	
							(((INT16U)yuvhavgmiren & 0x03) << 8);	
}

void drv_l1_CdspGetYuvHAvg(INT8U *yuvhavgmiren, INT8U *ytype, INT8U *utype, INT8U *vtype)
{
	*yuvhavgmiren = (R_CDSP_YUV_AVG_LPF_TYPE >> 8) & 0x03;
	*ytype = (R_CDSP_YUV_AVG_LPF_TYPE >> 0) & 0x03;
	*utype = (R_CDSP_YUV_AVG_LPF_TYPE >> 2) & 0x03;
	*vtype = (R_CDSP_YUV_AVG_LPF_TYPE >> 4) & 0x03;
}

/***************************
    YUV special mode set
****************************/
/**
 * @brief	cdsp set YUV special mode
 * @param	yuvspecmode[in]: 0 ~ 7
 * @return	none
 */
void drv_l1_CdspSetYuvSpecMode(INT8U yuvspecmode)
{
	R_CDSP_YUVSPEC_MODE &= ~0x07;
	R_CDSP_YUVSPEC_MODE |= yuvspecmode & 0x07;	
}

INT32U drv_l1_CdspGetYuvSpecMode(void)
{
	return (R_CDSP_YUVSPEC_MODE & 0x07);
}

/**
 * @brief	cdsp set binarize mode threshold
 * @param	binarthr[in]: threshold
 * @return	none
 */
void drv_l1_CdspSetBinarizeModeThr(INT8U binarthr)
{
	//vaild when special mode = 2, (binarize)
	R_CDSP_YUVSP_EFFECT_BTHR = binarthr;	
}

INT32U drv_l1_CdspGetBinarizeModeThr(void)
{
	return R_CDSP_YUVSP_EFFECT_BTHR;	
}

/***************************
 Y brightness & contrast set
****************************/
/**
 * @brief	cdsp set brightness and contrast
 * @param	YbYcEn[in]: enable or disable 
 * @param	yb[in]: y brightness 
 * @param	yc[in]: y contrast 
 * @return	none
 */
void drv_l1_CdspSetBriCont(INT8U YbYcEn, INT32U yb,INT32U yc)
{
	if(YbYcEn == 0) {
		R_CDSP_YUV_RANGE &= ~0x10;
		return;
	}

	// vaild when yuv special mode = 0x3
	R_CDSP_YUVSP_EFFECT_SCALER &= ~0xFF;
	R_CDSP_YUVSP_EFFECT_SCALER |= yc & 0xFF;
	R_CDSP_YUVSP_EFFECT_OFFSET &= ~0xFF;
	R_CDSP_YUVSP_EFFECT_OFFSET |= yb & 0xFF;

	//enable y brightness and contrast adj
	R_CDSP_YUV_RANGE |= 0x10;
}

/**
* @brief	cdsp yuv special mode brightness and contrast adjust enable
* @param	YbYcEn [in]: enable y brightness and contrast adjust 
* @return	none
*/
void drv_l1_CdspSetBriContEn(INT8U YbYcEn)
{
	/* vaild when yuv special mode = 0x3 */
	if(YbYcEn) { 
		R_CDSP_YUV_RANGE |= 0x10;
	} else {
		R_CDSP_YUV_RANGE &= ~0x10;
	}	
}

/**
 * @brief	cdsp get brightness and contrastenable
 * @param	none 
 * @return	status
 */
INT32U drv_l1_CdspGetBriContEn(void)
{
	return (R_CDSP_YUV_RANGE >> 4) & 0x01;
}

/**
 * @brief	cdsp set brightness and saturation
 * @param	y_offset[in]: Y brightnedd
 * @param	u_offset[in]: U saturation 
 * @param	v_offset[in]: V saturation 
 * @return	none
 */
void drv_l1_CdspSetYuvSPEffOffset(INT8S y_offset, INT8S u_offset, INT8S v_offset)
{
	R_CDSP_YUVSP_EFFECT_OFFSET = y_offset | (((INT32U)u_offset & 0xFF) << 8) |
								(((INT32U)v_offset & 0xFF) << 16);
}

/**
 * @brief	cdsp get brightness and saturation
 * @param	y_offset[out]: Y brightnedd
 * @param	u_offset[out]: U saturation 
 * @param	v_offset[out]: V saturation 
 * @return	none
 */
void drv_l1_CdspGetYuvSPEffOffset(INT8S *y_offset, INT8S *u_offset, INT8S *v_offset)
{
	*y_offset = R_CDSP_YUVSP_EFFECT_OFFSET & 0xFF;
	*u_offset = (R_CDSP_YUVSP_EFFECT_OFFSET >> 8) & 0xFF;
	*v_offset = (R_CDSP_YUVSP_EFFECT_OFFSET >> 16) & 0xFF;
}

/**
 * @brief	cdsp set contrast and saturation
 * @param	y_scale[in]: Y contrast
 * @param	u_scale[in]: U saturation 
 * @param	v_scale[in]: V saturation 
 * @return	none
 */
void drv_l1_CdspSetYuvSPEffScale(INT8U y_scale, INT8U u_scale, INT8U v_scale)
{
	R_CDSP_YUVSP_EFFECT_SCALER = y_scale | (((INT32U)u_scale & 0xFF) << 8) |
								(((INT32U)v_scale & 0xFF) << 16);
}

/**
 * @brief	cdsp get contrast and saturation
 * @param	y_scale[out]: Y contrast
 * @param	u_scale[out]: U saturation 
 * @param	v_scale[out]: V saturation 
 * @return	none
 */
void drv_l1_CdspGetYuvSPEffScale(INT8U *y_scale, INT8U *u_scale, INT8U *v_scale)
{
	*y_scale = R_CDSP_YUVSP_EFFECT_SCALER;
	*u_scale = (R_CDSP_YUVSP_EFFECT_SCALER >> 8) & 0xFF;
	*v_scale = (R_CDSP_YUVSP_EFFECT_SCALER >> 16) & 0xFF;
}

/***************************
   UV saturation & hue set
****************************/
/**
 * @brief	cdsp set saturation and hue
 * @param	uv_sat_scale[in]: uv scale gain
 * @param	uoffset[in]: U offset
 * @param	voffset[in]: V offset 
 * @param	u_huesindata[in]: hue sin data
 * @param	u_huecosdata[in]: hue cos data
 * @param	v_huesindata[in]: hue sin data
 * @param	v_huecosdata[in]: hue cos data  
 * @return	none
 */
void drv_l1_CdspSetSatHue(INT32U uv_sat_scale, INT32U uoffset, INT32U voffset, 
								  INT32U u_huesindata, INT32U u_huecosdata, INT32U v_huesindata, INT32U v_huecosdata)
{	
	R_CDSP_HUE_ROT_U = (u_huecosdata << 8) | u_huesindata;
	R_CDSP_HUE_ROT_V = (v_huecosdata << 8) | v_huesindata;
	R_CDSP_YUVSP_EFFECT_SCALER &= ~0xFFFF00;
	R_CDSP_YUVSP_EFFECT_SCALER |= (uv_sat_scale << 16) | (uv_sat_scale << 8);
	R_CDSP_YUVSP_EFFECT_OFFSET &= ~0xFFFF00;
	R_CDSP_YUVSP_EFFECT_OFFSET |= (voffset << 16) | (uoffset << 8);
}

/**
 * @brief	cdsp set hue
 * @param	u_huesindata[in]: hue sin data
 * @param	u_huecosdata[in]: hue cos data
 * @param	v_huesindata[in]: hue sin data
 * @param	v_huecosdata[in]: hue cos data  
 * @return	none
 */
void drv_l1_CdspSetYuvSPHue(INT16U u_huesindata, INT16U u_huecosdata, INT16U v_huesindata, INT16U v_huecosdata)
{
	R_CDSP_HUE_ROT_U = (u_huecosdata << 8) | u_huesindata;
	R_CDSP_HUE_ROT_V = (v_huecosdata << 8) | v_huesindata;
}

/**
 * @brief	cdsp set hue
 * @param	u_huesindata[out]: hue sin data
 * @param	u_huecosdata[out]: hue cos data
 * @param	v_huesindata[out]: hue sin data
 * @param	v_huecosdata[out]: hue cos data  
 * @return	none
 */
void drv_l1_CdspGetYuvSPHue(INT8S *u_huesindata, INT8S *u_huecosdata,	INT8S *v_huesindata, INT8S *v_huecosdata)
{
	*u_huesindata = R_CDSP_HUE_ROT_U & 0xFF;
	*u_huecosdata = (R_CDSP_HUE_ROT_U >> 8) & 0xFF;
	*v_huesindata = R_CDSP_HUE_ROT_V & 0xFF;
	*v_huecosdata = (R_CDSP_HUE_ROT_V >> 8) & 0xFF;
}

/***************************
      yuv h & v scale
****************************/
/**
 * @brief	cdsp set yuv h scale enable
 * @param	yuvhscale_en[in]: enable or disable
 * @param	yuvhscale_mode[in]: 0: drop, 1: filter 
 * @return	none
 */
void drv_l1_CdspSetYuvHScaleEn(INT8U yuvhscale_en, INT8U yuvhscale_mode)
{
	R_CDSP_SCLDW_CTRL &= ~0x900;
	R_CDSP_SCLDW_CTRL |= (((INT32U)yuvhscale_mode & 0x1) << 8)|(((INT32U)yuvhscale_en & 0x1) << 11);

	//reflected at next vaild vd edge
	//R_CDSP_SCALE_FACTOR_CTRL = 0x01; 
}

/**
 * @brief	cdsp set yuv v scale enable
 * @param	vscale_en[in]: enable or disable
 * @param	vscale_mode[in]: 0: drop, 1: filter 
 * @return	none
 */
void drv_l1_CdspSetYuvVScaleEn(INT8U vscale_en, INT8U vscale_mode)
{
	R_CDSP_SCLDW_CTRL &= ~0x90;
	R_CDSP_SCLDW_CTRL |= (((INT32U)vscale_mode & 0x1) << 4)|((INT32U)vscale_en & 0x1) << 7;
	
	//reflected at next vaild vd edge
	//R_CDSP_SCALE_FACTOR_CTRL = 0x01;      	 
}

/**
 * @brief	cdsp set yuv h scale factor
 * @param	hscaleaccinit[in]: init value
 * @param	yuvhscalefactor[in]: scale factor 
 * @return	none
 */
void drv_l1_CdspSetYuvHScale(INT16U hscaleaccinit, INT16U yuvhscalefactor)
{
	R_CDSP_HSCALE_ACC_INIT = hscaleaccinit;
	R_CDSP_HYUV_SCLDW_FACTOR = yuvhscalefactor;
}

/**
 * @brief	cdsp set yuv v scale factor
 * @param	vscaleaccinit[in]: init value
 * @param	yuvvscalefactor[in]: scale factor 
 * @return	none
 */
void drv_l1_CdspSetYuvVScale(INT16U vscaleaccinit, INT16U yuvvscalefactor)
{
	R_CDSP_VSCALE_ACC_INIT = vscaleaccinit;
	R_CDSP_VYUV_SCLDW_FACTOR = yuvvscalefactor;
}

/***************************
      Suppression set
****************************/
/**
 * @brief	cdsp set Suppression enable
 * @param	suppressen[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetUvSupprEn(INT8U suppressen)
{
	if(suppressen) {
		R_CDSP_YUV_CTRL |= 0x10;
	} else {
		R_CDSP_YUV_CTRL &= ~0x10;
	}
}

/**
 * @brief	cdsp set Suppression
 * @param	yuvsupmirvsel[in]: 1: cnt3eq2 , 0: cnt3eq1;
 * @param	fstextsolen[in]: Enable first sol when extend 2 lines
 * @param	yuvsupmiren[in]: Enable UV suppression r/l/d/t mirror
 * @return	none
 */
void drv_l1_CdspSetUvSuppr(INT8U yuvsupmirvsel, INT8U fstextsolen, INT8U yuvsupmiren)
{	
	R_CDSP_YUV_RANGE &= 0x20FF;
	R_CDSP_YUV_RANGE |= ((yuvsupmiren & 0x0F) << 8) | 
						(((INT16U)yuvsupmirvsel & 0x1) << 12) | 
						(((INT16U)fstextsolen & 0x1) << 13);
}

/***************************
Y edge in uv suppression set
****************************/
/**
 * @brief	cdsp edge source set
 * @param	posyedgeen [in]: 0:raw, 1: YUV
 * @return	none
 */
void drv_l1_CdspSetEdgeSrc(INT8U posyedgeen)
{
	if(posyedgeen) {
		R_CDSP_INP_EDGE_CTRL |= 0x02;
	} else {
		R_CDSP_INP_EDGE_CTRL &= ~0x02;
	}
}

/**
 * @brief	cdsp edge source get
 * @param	none
 * @return	posyedgeen: 0:raw, 1: YUV
 */
INT32U drv_l1_CdspGetEdgeSrc(void)
{
	return ((R_CDSP_INP_EDGE_CTRL >> 1) & 0x01);
}

/***************************
Y denoise in suppression set
****************************/
/**
 * @brief	cdsp set Y denoise enable
 * @param	denoisen[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetYDenoiseEn(INT8U denoisen)
{
	if(denoisen) {
		R_CDSP_YUV_CTRL |= 0x20;
	} else {
		R_CDSP_YUV_CTRL &= ~0x20;
	}
}

INT32U drv_l1_CdspGetYDenoiseEn(void)
{
	return ((R_CDSP_YUV_CTRL >> 1) & 0x01);
}

/**
 * @brief	cdsp set Y denoise
 * @param	denoisethrl[in]: Y denoise low threshold
 * @param	denoisethrwth[in]: Y denoise threshold bandwidth.
 * @param	yhtdiv[in]: Ydenoise divider
 * @return	none
 */
void drv_l1_CdspSetYDenoise(INT8U denoisethrl,	INT8U denoisethrwth, INT8U yhtdiv)
{
	R_CDSP_DENOISE_SETTING = (((INT16U)yhtdiv & 0x07) << 12) |
							 (((INT16U)denoisethrwth & 0x07) << 8) | (denoisethrl & 0xFF);
}

void drv_l1_CdspGetYDenoise(INT8U *denoisethrl,	INT8U *denoisethrwth, INT8U *yhtdiv)
{
	*denoisethrl = R_CDSP_DENOISE_SETTING & 0xFF;
	*denoisethrwth = (R_CDSP_DENOISE_SETTING >> 8) & 0x07;
	*yhtdiv = (R_CDSP_DENOISE_SETTING >> 12) & 0x07;
}

/***************************
  Y LPF in suppression set
****************************/
/**
 * @brief	cdsp set Y low pass filter enable
 * @param	lowyen[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetYLPFEn(INT8U lowyen)
{
	if(lowyen) {
		R_CDSP_YUV_CTRL |= 0x40;
	} else {
		R_CDSP_YUV_CTRL &= ~0x40;
	}
}

INT32U drv_l1_CdspGetYLPFEn(void)
{
	return ((R_CDSP_YUV_CTRL >> 2) & 0x01);
}

/***************************
        New ISP
****************************/
/**
 * @brief	cdsp set image size
 * @param	hsize[in]: width
 * @param	vsize[in]: height
 * @return	none
 */
void drv_l1_IspSetImageSize(INT16U hsize, INT16U vsize)
{
	R_ISP_IMSIZE_CROSSTALK_WEIGHT = (hsize & 0xFFF) | ((INT32U)vsize << 16);
}

/***************************
  linearity correction set
****************************/
/**
 * @brief	cdsp set linearity correction
 * @param	rgb_tbl[in]: table
 * @param	len[in]: length
 * @return	none
 */
void drv_l1_CdspSetLinCorrTable(INT8U *rgb_tbl)
{
	int i, val;
	INT32U reg_tmp;
	INT8U *p_rgb_tbl;
	volatile INT32U *p_sram_hw;
	
	reg_tmp = R_CDSP_MACRO_CTRL;
	R_CDSP_MACRO_CTRL = 0x1414;
		
	p_rgb_tbl = rgb_tbl;
	p_sram_hw = (INT32U *)CDSP_TABLE_BASEADDR;
	for(i = 0 ; i < 48 ; i++) {
		val = *p_rgb_tbl++;
		*p_sram_hw++ = val;
	}
	
	R_CDSP_MACRO_CTRL = 0;
	R_CDSP_MACRO_CTRL = reg_tmp;
}

/**
 * @brief	cdsp set linearity correction enable
 * @param	lincorr_en[in]: enable or disable
 * @return	none
 */
void drv_l1_IspSetLinCorrEn(INT8U lincorr_en)
{
	R_ISP_LI_HR_CTRL |= lincorr_en;
}

/**
 * @brief	cdsp set dectect bad pixel 
 * @param	algorithm_sel[in]: 
 * @return	none
 */
void drv_l1_IspSetDbpcSel(INT8U algorithm_sel)
{
	if (algorithm_sel == 0){
		R_ISP_LI_HR_CTRL &= ~(1<<4);
	} else {
		//R_ISP_LI_HR_CTRL |= (1<<4);	//2014-01-21  only DPC default mode.(ISP_hr_dpc_sel = 0) has been tested.
	}
}

/**
 * @brief	cdsp set dectect bad pixel 
 * @param	dpc_en[in]: enable or disable
 * @param	algorithm_sel[in]: 
 * @return	none
 */
void drv_l1_IspSetDpcEn(INT8U dpc_en,INT8U algorithm_sel)
{
	if (dpc_en == 1){						//Open bad pixel algorithm should be enable matsumoto algorithm
		R_ISP_LI_HR_CTRL |= (1<<5);
		if (algorithm_sel == 0) {
			R_ISP_LI_HR_CTRL &= ~(1<<4);	//matsumoto san perposal
		} else {
			R_ISP_LI_HR_CTRL |= (1<<4);		//Missing bad pixel
		}
	} else {
		R_ISP_LI_HR_CTRL &= ~(1<<5);
	}	
}

/**
 * @brief	cdsp set dectect bad pixel threshold
 * @param	sel_mode[in]:
 * @param	DPCth1[in]: 
 * @param	DPCth2[in]: 
 * @param	DPCth3[in]:  
 * @return	none
 */
void drv_l1_IspSetDpcRcvModeSelThr(INT8U sel_mode, INT16U DPCth1, INT16U DPCth2, INT16U DPCth3)
{
	R_ISP_LI_HR_CTRL &= ~0x80000;
	R_ISP_LI_HR_CTRL |= sel_mode << 19;	//second mode non function
	
	R_ISP_HR_DEPIXCANCEL_THOLD = (DPCth1 & 0x1FF)|((DPCth2 & 0x1FF) << 12);
	if (DPCth3 >4) {
		R_ISP_HR_DEPIXCANCEL_THOLD |= 0x4 << 24;	//max is 4
	} else {
		R_ISP_HR_DEPIXCANCEL_THOLD |= (DPCth3 & 0x7) << 24;
	}

}

/**
 * @brief	cdsp set smooth factor
 * @param	DPCn[in]: factor 64:smooth; 192:sharpen
 * @return	none
 */
void drv_l1_IspSetSmoothFactor(INT16U DPCn)	
{
	R_ISP_LI_HR_CTRL |= (((INT32U)DPCn & 0x1FF)<<8);
	R_ISP_LI_HR_CTRL &= ~0x80000;	//set second mode,but 
}

/**
 * @brief	cdsp set cross talk Gb and Gr enable
 * @param	ctk_en[in]: 
 * @param	ctk_gbgr[in]: 
 * @return	none
 */
void drv_l1_IspSetCrostlkGbGrEn(INT8U ctk_en, INT8U ctk_gbgr)
{
	if (ctk_en == 1) {
		R_ISP_LI_HR_CTRL |= 1 << 7;
	} else {
		R_ISP_LI_HR_CTRL &= ~(1 << 7);
	}
	
	R_ISP_LI_HR_CTRL |= ctk_gbgr <<20;
		
}

/**
 * @brief	cdsp set cross talk hold
 * @param	ctk_thr1[in]: 
 * @param	ctk_thr2[in]:
 * @param	ctk_thr3[in]:
 * @param	ctk_thr4[in]: 
 * @return	none
 */
void drv_l1_IspSetCrostlkThold(INT16U ctk_thr1, INT16U ctk_thr2, INT16U ctk_thr3, INT16U ctk_thr4)
{
	R_ISP_HR_CROSSTALK_THOLD = (ctk_thr1 & 0xFFFF) |
								(((INT32U)ctk_thr2 & 0xFFFF) << 8) |
								(((INT32U)ctk_thr3 & 0xFFFF) << 16) |
								(((INT32U)ctk_thr4 & 0xFFFF) << 24);
}

/**
 * @brief	cdsp set cross talk weight
 * @param	ctkw1[in]: 
 * @param	ctkw2[in]:
 * @param	ctkw3[in]:
 * @return	none
 */
void drv_l1_IspSetCrostlkWeight(INT16U ctkw1, INT16U ctkw2, INT16U ctkw3)
{
	R_ISP_HRR_DENOISE_CROSSTALK_WEIGHT |= (((INT32U)ctkw1 & 0xFF) << 16) |
										  (((INT32U)ctkw2 & 0xFF) << 20) |
										  (((INT32U)ctkw3 & 0xFF) << 24);
}

/**
 * @brief	cdsp set denoise enable
 * @param	denoise_en[in]: enable or disable
 * @return	none
 */
void drv_l1_IspSetDenoiseEn(INT8U denoise_en)
{
	if (denoise_en == 1) {
		R_ISP_LI_HR_CTRL |= 1 << 6;
	} else {
		R_ISP_LI_HR_CTRL &= ~(1 << 6);
	}
		
}

/**
 * @brief	cdsp set denoise threshold
 * @param	denoise_en[in]: enable or disable
 * @return	none
 */
void drv_l1_IspSetDenoiseThold(INT16U rdn_thr1,INT16U rdn_thr2,INT16U rdn_thr3,INT16U rdn_thr4)
{
	R_ISP_HR_DENOISE_THOLD = (rdn_thr1 & 0xFFFF) |
							 (((INT32U)rdn_thr2 & 0xFFFF) << 8) |
							 (((INT32U)rdn_thr3 & 0xFFFF) << 16) |
							 (((INT32U)rdn_thr4 & 0xFFFF) << 24);
}

/**
 * @brief	cdsp set denoise weight
 * @param	rdnw1[in]:
 * @param	rdnw2[in]: 
 * @param	rdnw3[in]:  
 * @return	none
 */
void drv_l1_IspSetDenoiseWeight(INT16U rdnw1, INT16U rdnw2, INT16U rdnw3)
{
	R_ISP_HRR_DENOISE_CROSSTALK_WEIGHT |= ((INT32U)rdnw1 & 0xFF) |
										  (((INT32U)rdnw2 & 0xFF) << 4) |
										  (((INT32U)rdnw3 & 0xFF) << 8);
}

/**
 * @brief	cdsp new denoise enable
 * @param	newdenoiseen [in]: new denoise enable, effective when raw data input.
 * @return	none
 */
void drv_l1_CdspSetNewDenoiseEn(INT8U newdenoiseen)
{
	if(newdenoiseen) {
		R_Ndenoise_CTRL |= 0x01;
	} else {
		R_Ndenoise_CTRL &= ~0x01;
	}
}

/**
 * @brief	get cdsp new denoise
 * @param	
 * @return	status
 */
INT32U drv_l1_CdspGetNewDenoiseEn(void)
{
	return (R_Ndenoise_CTRL & 0x01);
}

/**
 * @brief	cdsp new denoise set
 * @param	ndmirvsel [in]: 1:cnt3eq2, 0:cnt3eq1
 * @param	ndmiren [in]: new denoise enable, bit0:top, bit1:down, bit2:left, bit3:right
 * @return	none
 */
void drv_l1_CdspSetNewDenoise(INT8U ndmirvsel, INT8U ndmiren)
{
	ndmirvsel &= 0x1;
	ndmiren &= 0x0F;
	R_Ndenoise_CTRL &= ~((0x0F << 4)|(0x1 << 1));
	R_Ndenoise_CTRL |= (ndmiren << 4)|(ndmirvsel << 1);
}

/**
 * @brief	get cdsp new denoise
 * @param	ndmirvsel [out]: 1:cnt3eq2, 0:cnt3eq1
 * @param	ndmiren [out]: new denoise enable, bit0:top, bit1:down, bit2:left, bit3:right
 * @return	none
 */
void drv_l1_CdspGetNewDenoise(INT8U *ndmirvsel, INT8U *ndmiren)
{
	*ndmirvsel = (R_Ndenoise_CTRL >> 1) & 0x01;
	*ndmiren = (R_Ndenoise_CTRL >> 4) & 0x0F;
}

/**
 * @brief	cdsp new denoise edge enable
 * @param	ndedgeen [in]: new denoise edge enable
 * @param	ndeluten [in]: new denoise edge lut enable
 * @return	none
 */
void drv_l1_CdspSetNdEdgeEn(INT8U ndedgeen,INT8U ndeluten)
{
	if(ndedgeen) {
		R_Ndenoise_CTRL |= 0x100;
	} else {
		R_Ndenoise_CTRL &= ~0x100;
	}
	
	if(ndeluten) {
		R_Ndenoise_CTRL |= 0x1000;
	} else {
		R_Ndenoise_CTRL &= ~0x1000;
	}
}

/**
 * @brief	get cdsp new denoise edge enable
 * @param	ndedgeen [out]: new denoise edge enable
 * @param	ndeluten [out]: new denoise edge lut enable
 * @return	none
 */
void drv_l1_CdspGetNdEdgeEn(INT8U *ndedgeen, INT8U *ndeluten)
{
	*ndedgeen = (R_Ndenoise_CTRL >> 8) & 0x01;
	*ndeluten = (R_Ndenoise_CTRL >> 12) & 0x01;
}

/**
* @brief	cdsp new denoise edge HPF matrix set
* @param	pNdEdgeFilter [in]: table
* @return	none
*/
void drv_l1_CdspSetNdEdgeFilter(edge_filter_t *pNdEdgeFilter)
{
	R_Ndenoise_Ledge_Cof0 = pNdEdgeFilter->LPF00 & 0x0F;
	R_Ndenoise_Ledge_Cof0 |= (pNdEdgeFilter->LPF01 & 0x0F) << 4;
	R_Ndenoise_Ledge_Cof0 |= (pNdEdgeFilter->LPF02 & 0x0F) << 8;

	R_Ndenoise_Ledge_Cof1 = pNdEdgeFilter->LPF10 & 0x0F;
	R_Ndenoise_Ledge_Cof1 |= (pNdEdgeFilter->LPF11 & 0x0F) << 4;
	R_Ndenoise_Ledge_Cof1 |= (pNdEdgeFilter->LPF12 & 0x0F) << 8;

	R_Ndenoise_Ledge_Cof2 = pNdEdgeFilter->LPF20 & 0x0F;
	R_Ndenoise_Ledge_Cof2 |= (pNdEdgeFilter->LPF21 & 0x0F) << 4;
	R_Ndenoise_Ledge_Cof2 |= (pNdEdgeFilter->LPF22 & 0x0F) << 8;
}

/**
* @brief	cdsp new denoise edge HPF matrix get
* @param	pNdEdgeFilter [in]: table
* @return	none
*/
void drv_l1_CdspGetNdEdgeFilter(edge_filter_t *pNdEdgeFilter)

{
	pNdEdgeFilter->LPF00 = R_Ndenoise_Ledge_Cof0 & 0x0F;
	pNdEdgeFilter->LPF01 = (R_Ndenoise_Ledge_Cof0 >> 4) & 0x0F;
	pNdEdgeFilter->LPF02 = (R_Ndenoise_Ledge_Cof0 >> 8) & 0x0F;

	pNdEdgeFilter->LPF10 = (R_Ndenoise_Ledge_Cof1) & 0x0F;
	pNdEdgeFilter->LPF11 = (R_Ndenoise_Ledge_Cof1 >> 4) & 0x0F;
	pNdEdgeFilter->LPF12 = (R_Ndenoise_Ledge_Cof1 >> 8) & 0x0F;

	pNdEdgeFilter->LPF20 = R_Ndenoise_Ledge_Cof2 & 0x0F;
	pNdEdgeFilter->LPF21 = (R_Ndenoise_Ledge_Cof2 >> 4) & 0x0F;
	pNdEdgeFilter->LPF22 = (R_Ndenoise_Ledge_Cof2 >> 8) & 0x0F;
}

/**
* @brief	cdsp new denoise edge scale set
* @param	ndlhdiv [in]: L edge enhancement edge vale scale
* @param	ndlhtdiv [in]: L edge enhancement edge vale scale
* @param	ndlhcoring [in]: L core ring threshold
* @param	ndlhmode [in]: 1: default matrix, 0: enable paroramming matrix
* @return	none
*/
void drv_l1_CdspSetNdEdgeLCoring(INT8U ndlhdiv, INT8U ndlhtdiv, INT8U ndlhcoring)
{
	INT8U lh, lht;

	lh = lht = 0;
	while(1) // lhdiv=2^lh
	{
		ndlhdiv >>= 1;
		if(ndlhdiv)
			lh++;
		else 
			break;

		if(lh >= 7)
			break;
	}
	
	while(1) // lhtdiv=2^lht
	{
		ndlhtdiv >>= 1;
		if(ndlhtdiv) 
			lht++;
		else 
			break;

		if(lht >= 7)
			break;
	}
	
	if(lh > 7) lh = 7;
	if(lht> 7) lht = 7;
	R_Ndenoise_Ledge_Set |= (lht << 12)|(lh << 8);
	R_Ndenoise_Ledge_Set |= (ndlhcoring & 0x1FF);
}

/**
* @brief	cdsp new denoise edge scale get
* @param	ndlhdiv [out]: L edge enhancement edge vale scale
* @param	ndlhtdiv [out]: L edge enhancement edge vale scale
* @param	ndlhcoring [out]: L core ring threshold
* @return	none
*/
void drv_l1_CdspGetNdEdgeLCoring(INT8U *ndlhdiv, INT8U *ndlhtdiv, INT8U *ndlhcoring)
{
	*ndlhdiv = 1 << ((R_Ndenoise_Ledge_Set >> 8) & 0x07);
	*ndlhtdiv = 1 << ((R_Ndenoise_Ledge_Set >> 12) & 0x07);
	*ndlhcoring = R_Ndenoise_Ledge_Set & 0xFF;
}

/**
* @brief	cdsp new denoise edge amp set
* @param	ndampga [in]: 0:1, 1:2, 2:3, 3:4
* @return	none
*/
void drv_l1_CdspSetNdEdgeAmpga(INT8U ndampga)
{
	ndampga &= 0x03;
	R_Ndenoise_CTRL |= (ndampga << 14);
}

/**
* @brief	cdsp new denoise edge amp get
* @param	ndampga [out]: 0:1, 1:2, 2:3, 3:4
* @return	gain
*/
INT32U drv_l1_CdspGetNdEdgeAmpga(void)
{
	return ((R_Ndenoise_CTRL >> 14) & 0x3);
}

/***************************
   auto focuse / af set
****************************/
/**
 * @brief	cdsp set auto focuse enable
 * @param	af_en[in]: enable or disable
 * @param	af_win_hold[in]: af value hold enable
 * @return	none
 */
void drv_l1_CdspSetAFEn(INT8U af_en, INT8U af_win_hold)
{
	R_CDSP_AF_WIN_CTRL &= ~0x90000;
	R_CDSP_AF_WIN_CTRL |= (((INT32U)af_win_hold & 0x1) << 16)|(((INT32U)af_en & 0x1) << 19); 	
}

void drv_l1_CdspGetAFEn(INT8U *af_en, INT8U *af_win_hold)
{
	*af_en = (R_CDSP_AF_WIN_CTRL >> 19) & 0x01;
	*af_win_hold = (R_CDSP_AF_WIN_CTRL >> 16) & 0x01;
}

/**
 * @brief	cdsp set auto focuse windows 1
 * @param	hoffset[in]: x position
 * @param	voffset[in]: y position
 * @param	hsize[in]: width
 * @param	vsize[in]: height 
 * @return	none
 */
void drv_l1_CdspSetAfWin1(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize)
{
	R_CDSP_AF_WIN1_HVOFFSET = (hoffset & 0xFFF) | (((INT32U)voffset & 0xFFF) << 12); 
	R_CDSP_AF_WIN1_HVSIZE = (hsize & 0xFFF) | (((INT32U)vsize & 0xFFF) << 12);
}

/**
 * @brief	cdsp set auto focuse windows 2
 * @param	hoffset[in]: x position
 * @param	voffset[in]: y position
 * @param	hsize[in]: width
 * @param	vsize[in]: height 
 * @return	none
 */
void drv_l1_CdspSetAfWin2(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize)
{
	// size 0: 256, 1:512, 2:1024, 3:64, 4:2048
	hoffset >>= 2;	// offset unit is 4 pixel
	voffset >>= 2;
	R_CDSP_AF_WIN2_HVOFFSET = (hoffset & 0x3FF) | (((INT32U)voffset & 0x3FF) << 12); 
	R_CDSP_AF_WIN_CTRL &= ~0xFF;
	R_CDSP_AF_WIN_CTRL |=  (hsize & 0xF) | ((vsize & 0xF) << 4);
}

/**
 * @brief	cdsp set auto focuse windows 3
 * @param	hoffset[in]: x position
 * @param	voffset[in]: y position
 * @param	hsize[in]: width
 * @param	vsize[in]: height 
 * @return	none
 */
void drv_l1_CdspSetAfWin3(INT16U hoffset, INT16U voffset, INT16U hsize, INT16U vsize)
{
	// size 0: 256, 1:512, 2:1024, 3:64, 4:2048
	hoffset >>= 2;	// offset unit is 4 pixel
	voffset >>= 2;
	R_CDSP_AF_WIN3_HVOFFSET = (hoffset & 0x3FF) | (((INT32U)voffset & 0x3FF) << 12); 
	R_CDSP_AF_WIN_CTRL &= ~0xFF00;
	R_CDSP_AF_WIN_CTRL |= ((hsize & 0xF) << 8) | ((vsize & 0xF) << 12);
}

/**
* @brief	cdsp get af window1 statistics
* @param	windows_no[in]: window index number
* @param	af_value[out]: 
* @return	SUCCESS/ERROR
*/
INT32S drv_l1_CdspGetAFWinVlaue(INT8U windows_no, af_windows_value_t *af_value)
{
	if(windows_no == 1) {
		af_value->h_value_l = R_CDSP_AF_WIN1_HVALUE_L;
		af_value->h_value_h = R_CDSP_AF_WIN1_HVALUE_H;
		af_value->v_value_l = R_CDSP_AF_WIN1_VVALUE_L;
		af_value->v_value_h = R_CDSP_AF_WIN1_VVALUE_H;
	} else if(windows_no == 2) {
		af_value->h_value_l = R_CDSP_AF_WIN2_HVALUE_L;
		af_value->h_value_h = R_CDSP_AF_WIN2_HVALUE_H;
		af_value->v_value_l = R_CDSP_AF_WIN2_VVALUE_L;
		af_value->v_value_h = R_CDSP_AF_WIN2_VVALUE_H;	
	} else if(windows_no == 3) {
		af_value->h_value_l = R_CDSP_AF_WIN3_HVALUE_L;
		af_value->h_value_h = R_CDSP_AF_WIN3_HVALUE_H;
		af_value->v_value_l = R_CDSP_AF_WIN3_VVALUE_L;
		af_value->v_value_h = R_CDSP_AF_WIN3_VVALUE_H;
	} else {
		return -1;
	}
	return 0;
}

/***************************
auto white balance / awb set
****************************/
/**
 * @brief	cdsp set auto white balance enable
 * @param	awb_win_en[in]: enable or disable
 * @param	awb_win_hold[in]: value hold enable
 * @return	none
 */
void drv_l1_CdspSetAWBEn(INT8U awb_win_en, INT8U awb_win_hold)
{
	R_CDSP_AE_AWB_WIN_CTRL &= ~0x0804;
	R_CDSP_AE_AWB_WIN_CTRL |= ((awb_win_hold & 0x1) << 2) | (((INT16U)awb_win_en& 0x1) << 11); 
}

/**
 * @brief	cdsp set auto white balance
 * @param	awbclamp_en[in]: awb clamp enable or disable
 * @param	sindata[in]: sin data
 * @param	cosdata[in]: cos data
 * @param	awbwinthr[in]: awb threshold
 * @return	none
 */
void drv_l1_CdspSetAWB(INT8U awbclamp_en, INT8U sindata, INT8U cosdata, INT8U awbwinthr)
{
	R_CDSP_AWB_WIN_CTRL = (sindata & 0xFF) |
						  (((INT32U)cosdata  & 0xFF)<< 8) |
						  (((INT32U)awbwinthr & 0xFF) << 16) |
						  (((INT32U)awbclamp_en & 0x1) << 24);
}

/**
 * @brief	cdsp set auto white balance threshold
 * @param	Ythr0[in]: lum0clamp
 * @param	Ythr1[in]: lum1clamp
 * @param	Ythr2[in]: lum2clamp
 * @param	Ythr3[in]: lum3clamp
 * @return	none
 */
void drv_l1_CdspSetAwbYThr(INT8U Ythr0, INT8U Ythr1, INT8U Ythr2, INT8U Ythr3)
{
	R_CDSP_AWB_SPECWIN_Y_THR = Ythr0|(Ythr1 << 8)|(Ythr2 << 16)|(Ythr3 << 24);
}

/**
 * @brief	cdsp set auto white balance threshold
 * @param	UVthr[in]: AWB UV threshold
 * @return	none
 */
void drv_l1_CdspSetAwbUVThr(awb_uv_thr_t *UVthr)
{	
	R_CDSP_AWB_SPECWIN_UV_THR1 = (UVthr->UL1N1 & 0xFF) | 
								 (((INT32S)UVthr->UL1P1 & 0xFF) << 8) | 
								 (((INT32S)UVthr->VL1N1 & 0xFF) << 16) | 
								 (((INT32S)UVthr->VL1P1 & 0xFF) << 24);

	R_CDSP_AWB_SPECWIN_UV_THR2 = (UVthr->UL1N2 & 0xFF) | 
								 (((INT32S)UVthr->UL1P2 & 0xFF) << 8) | 
								 (((INT32S)UVthr->VL1N2 & 0xFF) << 16) | 
								 (((INT32S)UVthr->VL1P2 & 0xFF) << 24);

	R_CDSP_AWB_SPECWIN_UV_THR3 = (UVthr->UL1N3 & 0xFF) | 
								 (((INT32S)UVthr->UL1P3 & 0xFF) << 8) | 
								 (((INT32S)UVthr->VL1N3 & 0xFF) << 16) | 
								 (((INT32S)UVthr->VL1P3 & 0xFF) << 24);
}

/***************************
   auto expore / ae set
****************************/
/**
 * @brief	cdsp set ae and awb source
 * @param	raw_en[in]: 1:raw(from awblinectr), 0:rgb(poswb) ae/awb window
 * @return	none
 */
void drv_l1_CdspSetAeAwbSrc(INT8U raw_en)
{
	if(raw_en) {
		R_CDSP_AE_AWB_WIN_CTRL |= (1 << 12);
	} else {
		R_CDSP_AE_AWB_WIN_CTRL &= ~(1 << 12);
	}
}

/**
 * @brief	cdsp get ae and awb source
 * @param	none
 * @return	source
 */
INT32U drv_l1_CdspGetAeAwbSrc(void)
{
	return ((R_CDSP_AE_AWB_WIN_CTRL >> 12) & 0x01);
}

/**
 * @brief	cdsp set ae and awb subsample
 * @param	subample[in]: 0: disable, 1: 1/2, 2: 1/4 subsample
 * @return	none
 */
void drv_l1_CdspSetAeAwbSubSample(INT8U subample)
{
	R_CDSP_AE_AWB_WIN_CTRL &= ~(0x03 << 13);
	R_CDSP_AE_AWB_WIN_CTRL |= ((INT32U)subample &0x3) << 13;  
}

/**
 * @brief	cdsp get ae and awb sub sample
 * @param	none
 * @return	source
 */
INT32U drv_l1_CdspGetAeAwbSubSample(void)
{
	return (((R_CDSP_AE_AWB_WIN_CTRL >>13) & 0x03));
}

/**
 * @brief	cdsp set ae enable
 * @param	ae_win_en[in]: enable or disable
 * @param	ae_win_hold[in]: value hold enable
 * @return	none
 */
void drv_l1_CdspSetAEEn(INT8U ae_win_en, INT8U ae_win_hold)
{
	R_CDSP_AE_AWB_WIN_CTRL &= ~0x0111;
	R_CDSP_AE_AWB_WIN_CTRL |= (ae_win_hold & 0x1) |
						(((INT16U)ae_win_en& 0x1) << 8);
	//reflected at next vaild vd edge 
	//R_CDSP_AE_AWB_WIN_CTRL |= 0x10;

	/* vdupdate */
	//R_CDSP_FRONT_CTRL0 |= (1<<4);
}

void drv_l1_CdspGetAEEn(INT8U *ae_win_en, INT8U *ae_win_hold)
{
	*ae_win_en = (R_CDSP_AE_AWB_WIN_CTRL >> 8) & 0x01;
	*ae_win_hold = R_CDSP_AE_AWB_WIN_CTRL & 0x01;
}

/**
 * @brief	cdsp get ae active buffer number 
 * @param	none
 * @return	0: ae a buffer ready, 1: ae b buffer ready
 */
INT32U drv_l1_CdspGetAEActBuff(void)
{
	if((R_CDSP_AE_AWB_WIN_CTRL >>15) & 0x01) {
		return 1;	/* buffer b active */
	} else {
		return 0;	/* buffer a active */
	}
}

/**
 * @brief	cdsp set ae windows
 * @param	phaccfactor[in]: h accumulator normalize factor
 * @param	pvaccfactor[in]: v accumulator normalize factor
 * @return	none
 */
void drv_l1_CdspSetAEWin(INT8U phaccfactor, INT8U pvaccfactor)
{
	INT8U h_factor, v_factor;
	
	if(phaccfactor <= 4) h_factor = 0;
	else if(phaccfactor <= 8) h_factor = 1;
	else if(phaccfactor <= 16) h_factor = 2;
	else if(phaccfactor <= 32) h_factor = 3;
	else if(phaccfactor <= 64) h_factor = 4;
	else if(phaccfactor <= 128) h_factor = 5;
	else h_factor = 5;

	if(pvaccfactor <= 4) v_factor = 0;
	else if(pvaccfactor <= 8) v_factor = 1;
	else if(pvaccfactor <= 16) v_factor = 2;
	else if(pvaccfactor <= 32) v_factor = 3;
	else if(pvaccfactor <= 64) v_factor = 4;
	else if(pvaccfactor <= 128) v_factor = 5;
	else v_factor = 5;

	DBG_PRINT( "\r\nAE win factor = [0x%x, 0x%x]\r\n", h_factor, v_factor);		
	R_CDSP_AE_WIN_SIZE = (v_factor << 4)|h_factor;

	//R_CDSP_AE_WIN_SIZE = phaccfactor & 0x07 | (pvaccfactor & 0x07) << 4;
	
	//reflected at next vaild vd edge
	//R_AEW_SIZE |= 0x100;			
}

/**
 * @brief	cdsp set ae buffer address
 * @param	winaddra[in]: ae buffer a address
 * @param	winaddrb[in]: ae buffer b address
 * @return	none
 */
void drv_l1_CdspSetAEBuffAddr(INT32U winaddra,INT32U winaddrb)
{
	R_CDSP_AE_WIN_ABUFADDR = winaddra;
	R_CDSP_AE_WIN_BBUFADDR = winaddrb;
}

/**
 * @brief	cdsp set rgb windows
 * @param	hwdoffset[in]: x position
 * @param	vwdoffset[in]: y position
 * @param	hwdsize[in]: width
 * @param	vwdsize[in]: height
 * @return	none
 */
void drv_l1_CdspSetRGBWin(INT16U hwdoffset, INT16U vwdoffset, INT16U hwdsize, INT16U vwdsize)
{
	R_CDSP_RGB_WINH_CTRL = (hwdoffset << 12) | hwdsize;
	R_CDSP_RGB_WINV_CTRL = (vwdoffset << 12) | vwdsize;
}

/**
 * @brief	cdsp get rgb windows
 * @param	hwdoffset[out]: x position
 * @param	vwdoffset[out]: y position
 * @param	hwdsize[out]: width
 * @param	vwdsize[out]: height
 * @return	none
 */
void drv_l1_CdspGetRGBWin(INT16U *hwdoffset, INT16U *vwdoffset, INT16U *hwdsize, INT16U *vwdsize)
{
	*hwdoffset = (R_CDSP_RGB_WINH_CTRL >> 12) & 0xFFF;
	*hwdsize = R_CDSP_RGB_WINH_CTRL & 0x3FF;
	*vwdoffset = (R_CDSP_RGB_WINV_CTRL >> 12) & 0xFFF;
	*vwdsize = R_CDSP_RGB_WINV_CTRL & 0x3FF;
}

/**
 * @brief	cdsp set test windows display
 * @param	AeWinTest[in]: ae window display
 * @param	AfWinTest[in]: af window display
 * @return	none
 */
void drv_l1_CdspSet3ATestWinEn(INT8U AeWinTest, INT8U AfWinTest)
{
	if(AfWinTest == 1 || AeWinTest == 1)
		R_CDSP_AEF_WIN_TEST = 0x09;
	else
		R_CDSP_AEF_WIN_TEST =  ((AfWinTest & 0x1) << 3) | (AeWinTest & 0x1);
}

/***************************
       histgm set
****************************/
/**
 * @brief	cdsp set histgm enable
 * @param	his_en[in]: enable or disable
 * @param	his_hold_en[in]: value hold enable
 * @return	none
 */
void drv_l1_CdspSetHistgmEn(INT8U his_en, INT8U his_hold_en)
{
	R_CDSP_HISTGM_CTRL &= ~0x10000;
	R_CDSP_HISTGM_CTRL |= (INT32U)(his_en & 0x1) << 16;
	R_CDSP_AE_AWB_WIN_CTRL &= ~0x2;
	R_CDSP_AE_AWB_WIN_CTRL |= (his_hold_en & 0x1) << 1;		
}

/**
 * @brief	cdsp set histgm threshold
 * @param	hislowthr[in]: low value
 * @param	hishighthr[in]: high value
 * @return	none
 */
void drv_l1_CdspSetHistgm(INT8U hislowthr, INT8U hishighthr)
{
	R_CDSP_HISTGM_CTRL &= ~0xFFFF;
	R_CDSP_HISTGM_CTRL |= hislowthr | ((INT16U)hishighthr << 8);
}

/**
 * @brief	cdsp get histgm threshold
 * @param	hislowthr[out]: low value
 * @param	hishighthr[out]: high value
 * @return	none
 */
void drv_l1_CdspGetHistgm(INT32U *hislowcnt, INT32U *hishighcnt)
{
	*hislowcnt = R_CDSP_HISTGM_LCNT;
	*hishighcnt = R_CDSP_HISTGM_HCNT;
}

/***************************
       awb sum set
****************************/
/**
 * @brief	cdsp get awb cnt
 * @param	section [in]: index = 1, 2, 3
 * @param	sumcnt [out]: count get
 * @return	SUCCESS/ERROR
 */
INT32S drv_l1_CdspGetAwbSumCnt(INT8U section, INT32U *sumcnt)
{
	//volatile INT32U *ptr;
	
	if(section == 1) {		
		*sumcnt = R_CDSP_SUM_CNT1;
		//*sumcnt = ((UINT32)(pCdspReg2->cdspAwbSumCnt1[3]&0x03)<<24)|((UINT32)pCdspReg2->cdspAwbSumCnt1[2]<<16)|((UINT32)pCdspReg2->cdspAwbSumCnt1[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumCnt1[0];
	} else if(section == 2) {
		//*sumcnt = ((UINT32)(pCdspReg2->cdspAwbSumCnt2[3]&0x03)<<24)|((UINT32)pCdspReg2->cdspAwbSumCnt2[2]<<16)|((UINT32)pCdspReg2->cdspAwbSumCnt2[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumCnt2[0];
		*sumcnt = R_CDSP_SUM_CNT2;
	} else if(section == 3) {
		//*sumcnt = ((UINT32)(pCdspReg2->cdspAwbSumCnt3[3]&0x03)<<24)|((UINT32)pCdspReg2->cdspAwbSumCnt3[2]<<16)|((UINT32)pCdspReg2->cdspAwbSumCnt3[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumCnt3[0];
		*sumcnt = R_CDSP_SUM_CNT3;
	} else {
		return -1;
	}
	return 0;
}

/**
 * @brief	cdsp get awb g
 * @param	section [in]: index = 1, 2, 3
 * @param	sumgl [out]: sum g1 low 
 * @param	sumgl [out]: sum g1 high 
 * @return	SUCCESS/ERROR
 */
INT32S drv_l1_CdspGetAwbSumG(INT8U section, INT32U *sumgl, INT32U *sumgh)
{
	if(section == 1) {
		*sumgl = R_CDSP_SUM_G1_L;
		*sumgh = R_CDSP_SUM_G1_H;
	} else if(section == 2) {
		*sumgl = R_CDSP_SUM_G2_L;
		*sumgh = R_CDSP_SUM_G2_H;
	} else if(section == 3) {
		*sumgl = R_CDSP_SUM_G3_L;
		*sumgh = R_CDSP_SUM_G3_H;
	} else {
		return -1;
	}
	return 0;
}

/**
 * @brief	cdsp get awb rg
 * @param	section [in]: section = 1, 2, 3
 * @param	sumrgl [out]: sum rg low 
 * @param	sumrgl [out]: sum rg high 
 * @return	SUCCESS/ERROR
 */
INT32S drv_l1_CdspGetAwbSumRG(INT8U section, INT32U *sumrgl, INT32S *sumrgh)
{
	//volatile INT32U *ptr;
	
	if(section == 1) {		
		//*sumrgl = ((UINT32)pCdspReg2->cdspAwbSumRg1L[3]<<24)|((UINT32)pCdspReg2->cdspAwbSumRg1L[2] << 16)
		//	| ((UINT32)pCdspReg2->cdspAwbSumRg1L[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumRg1L[0];
		//*sumrgh = (SINT32)(pCdspReg2->cdspAwbSumRg1H&0x07);//<<16)|((UINT32)pCdspReg2->cdspAwbSumRg1L[3]<<8)|((UINT32)pCdspReg2->cdspAwbSumRg1L[2]);
		*sumrgl = R_CDSP_SUM_RG1_L;
		*sumrgh = R_CDSP_SUM_RG1_H;
	} else if(section == 2) {
		//*sumrgl = ((UINT32)pCdspReg2->cdspAwbSumRg2L[3]<<24)|((UINT32)pCdspReg2->cdspAwbSumRg2L[2]<<16)
		//	| ((UINT32)pCdspReg2->cdspAwbSumRg2L[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumRg2L[0];
		//*sumrgh = (SINT32)(pCdspReg2->cdspAwbSumRg2H&0x07);//<<16)|((UINT32)pCdspReg2->cdspAwbSumRg2L[3]<<8)|((UINT32)pCdspReg2->cdspAwbSumRg2L[2]);
		*sumrgl = R_CDSP_SUM_RG2_L;
		*sumrgh = R_CDSP_SUM_RG2_H;
	} else if(section == 3) {
		//*sumrgl = ((UINT32)pCdspReg2->cdspAwbSumRg3L[3]<<24)|((UINT32)pCdspReg2->cdspAwbSumRg3L[2] << 16) 
		//	|((UINT32)pCdspReg2->cdspAwbSumRg3L[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumRg3L[0];
		//*sumrgh = (SINT32)(pCdspReg2->cdspAwbSumRg3H&0x07);//<<16)|((UINT32)pCdspReg2->cdspAwbSumRg3L[3]<<8)|((UINT32)pCdspReg2->cdspAwbSumRg3L[2]);
		*sumrgl = R_CDSP_SUM_RG3_L;
		*sumrgh = R_CDSP_SUM_RG3_H;
	} else {
		return -1;
	}

	
	return 0;
}

/**
 * @brief	cdsp get awb bg
 * @param	section [in]: section = 1, 2, 3
 * @param	sumbgl [out]: sum bg low 
 * @param	sumbgl [out]: sum bg high 
 * @return	SUCCESS/ERROR
 */
INT32S drv_l1_CdspGetAwbSumBG(INT8U section, INT32U *sumbgl, INT32S *sumbgh)
{
	//volatile INT32U *ptr;
	
	if(section == 1) {
		//*sumbgl = ((UINT32)pCdspReg2->cdspAwbSumBg1L[3]<<24)|((UINT32)pCdspReg2->cdspAwbSumBg1L[2] << 16) 
		//	| ((UINT32)pCdspReg2->cdspAwbSumBg1L[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumBg1L[0];
		//*sumbgh = (SINT32)(pCdspReg2->cdspAwbSumBg1H&0x07);//<<16)|;
		*sumbgl = R_CDSP_SUM_BG1_L;
		*sumbgh = R_CDSP_SUM_BG1_H;
	} else if(section == 2) {
		//*sumbgl = ((UINT32)pCdspReg2->cdspAwbSumBg2L[3]<<24)|((UINT32)pCdspReg2->cdspAwbSumBg2L[2] << 16)
		//	| ((UINT32)pCdspReg2->cdspAwbSumBg2L[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumBg2L[0];
		//*sumbgh = (SINT32)(pCdspReg2->cdspAwbSumBg2H&0x07);//<<16)|
		*sumbgl = R_CDSP_SUM_BG2_L;
		*sumbgh = R_CDSP_SUM_BG2_H;
	} else if(section == 3) {
		//*sumbgl = ((UINT32)pCdspReg2->cdspAwbSumBg3L[3]<<24)|((UINT32)pCdspReg2->cdspAwbSumBg3L[2] << 16) 
		//	| ((UINT32)pCdspReg2->cdspAwbSumBg3L[1]<<8)|(UINT32)pCdspReg2->cdspAwbSumBg3L[0];
		//*sumbgh = (SINT32)(pCdspReg2->cdspAwbSumBg3H&0x07);//<<16)|
		*sumbgl = R_CDSP_SUM_BG3_L;
		*sumbgh = R_CDSP_SUM_BG3_H;
	} else {
		return -1;
	}
	
	return 0;
}

/***************************
   Motion Detection set
****************************/
/**
 * @brief	cdsp set motion detect enable
 * @param	enable[in]: enable or disable
 * @return	none
 */
void drv_l1_CdspSetMDEn(INT8U enable)
{
	INT32U threshold;
	INT16U width; 
	INT32U working_buf;
	
	working_buf = R_CDSP_MD_DMA_SADDR;
	width = R_CDSP_MD_HSIZE;
	threshold = R_CDSP_MD_CTRL;
	
	if (threshold && (width>16) && working_buf) 
	{
		R_CDSP_MD_CTRL |= enable;
	}
}

/**
 * @brief	cdsp set motion detect 
 * @param	enable[in]: enable or disable
 * @param	threshold[in]: threshold value
 * @param	width[in]: width
 * @param	working_buf[in]: work buffer address 
 * @return	none
 */
void drv_l1_CdspSetMD(INT8U enable, INT8U threshold, INT16U width, INT32U working_buf)
{
	INT32U value;
	
	value = R_CDSP_MD_CTRL & ~0x7F01;
	if (enable && (width>16) && working_buf) {
		value |= 0x1;
	}	
	value |= (threshold & 0x7F) << 8;
		
	R_CDSP_MD_DMA_SADDR = working_buf;
	R_CDSP_MD_HSIZE = width;
	R_CDSP_MD_CTRL = value;
}

/**
 * @brief	cdsp get motion detect result 
 * @param	none
 * @return	address
 */
INT32U drv_l1_CdspGetResult(void)
{
	return R_CDSP_MD_DIFF_DMA_SADDR;
}

/**
 * @brief	cdsp set new denoise table
 * @param	new denoise Table[in]: table
 * @return	none
 */
void drv_l1_CdspSetNewDEdgeLut(INT8U *pLutNewDEdgeTable)
{
	INT32S i;
	cdspNewDEdgeLutData_t *pNewDEdgeLutData = (cdspNewDEdgeLutData_t *)(CDSP_TABLE_BASEADDR);
	
	R_CDSP_MACRO_CTRL = 0x1010;
	for(i = 0; i < 256; i++) 
		pNewDEdgeLutData->NewDEdgeTable[i] = *pLutNewDEdgeTable++;
		
	R_CDSP_MACRO_CTRL = 0x00;
}
#endif //(defined _DRV_L1_CDSP) && (_DRV_L1_CDSP == 1) 
