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

/*******************************************************
    Include file
*******************************************************/
#include "drv_l1_interrupt.h"
#include "drv_l1_tv.h"
#include "drv_l1_tft.h"
#include "drv_l1_hdmi.h"
#include "drv_l1_conv420to422.h"
#include "drv_l2_display.h"
#include "gp_stdlib.h"
#if _OPERATING_SYSTEM != _OS_NONE 
#include "os.h"
#endif

#if (defined _DRV_L2_DISP) && (_DRV_L2_DISP == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define DISP_QUEUE_MAX		1
#define DISP_UPDATE_DONE	1
 
#define PPU_TV_V_BLANKING				(1 << 11)
#define PPU_TFT_V_BLANKING				(1 << 13)
#define PPU_HDMI_V_BLANKING				(1 << 19)

#define	PPU_YUYV_TYPE3					(3 << 20)
#define	PPU_YUYV_TYPE2					(2 << 20)
#define	PPU_YUYV_TYPE1					(1 << 20)
#define	PPU_YUYV_TYPE0					(0 << 20)

#define	PPU_RGBG_TYPE3					(3 << 20)
#define	PPU_RGBG_TYPE2					(2 << 20)
#define	PPU_RGBG_TYPE1					(1 << 20)
#define	PPU_RGBG_TYPE0					(0 << 20)

#define PPU_LB							(1 << 19)
#define	PPU_YUYV_MODE					(1 << 10)
#define	PPU_RGBG_MODE			        (0 << 10)

#define TFT_SIZE_1024X768               (7 << 16)
#define TFT_SIZE_800X600                (6 << 16)
#define TFT_SIZE_800X480                (5 << 16)
#define TFT_SIZE_720X480				(4 << 16)
#define TFT_SIZE_480X272				(3 << 16)
#define TFT_SIZE_480X234				(2 << 16)
#define TFT_SIZE_640X480                (1 << 16)
#define TFT_SIZE_320X240                (0 << 16)
#define TFT_SIZE_MASK					(7 << 16)

#define	PPU_YUYV_RGBG_FORMAT_MODE		(1 << 8)
#define	PPU_RGB565_MODE			        (0 << 8)

#define	PPU_FRAME_BASE_MODE			    (1 << 7)
#define	PPU_VGA_NONINTL_MODE			(0 << 5)

#define	PPU_VGA_MODE					(1 << 4)
#define	PPU_QVGA_MODE					(0 << 4)

#define BYPASS_CONV420_EN				1
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct display_s
{
#if _OPERATING_SYSTEM != _OS_NONE 
	OS_EVENT *tv_q;
	OS_EVENT *tft_q;
	OS_EVENT *hdmi_q;
	void *tv_q_buf[DISP_QUEUE_MAX];
	void *tft_q_buf[DISP_QUEUE_MAX];
	void *hdmi_q_buf[DISP_QUEUE_MAX];
	OS_EVENT *tv_sem;
	OS_EVENT *tft_sem;
	OS_EVENT *hdmi_sem;
#endif

	INT8U tv_update_flag;
	INT8U tft_update_flag;
	INT8U hdmi_update_flag;
	INT8U hdmi_en_flag;
	
	INT32U tv_color_fmt;
	INT32U tft_color_fmt;
	INT32U hdmi_color_fmt;
	INT32U tv_disp_buf;
	INT32U tft_disp_buf;
	INT32U hdmi_disp_buf;	
} display_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static display_t display, *pDisp; 
extern DispCtrl_t TFT_Param;

static void ppu_isr(void)
{
	INT32U enable = R_PPU_IRQ_EN;	
	INT32U status = R_PPU_IRQ_STATUS;

	R_PPU_IRQ_STATUS = status;
	enable &= status;
#if _DRV_L1_TV == 1
	if(enable & PPU_TV_V_BLANKING) {
		if(pDisp->tv_update_flag) {
			R_TV_FBI_ADDR = pDisp->tv_disp_buf;
			pDisp->tv_update_flag = 0;
		#if _OPERATING_SYSTEM != _OS_NONE 
			OSQPost(pDisp->tv_q, (void *)DISP_UPDATE_DONE);
		#endif
		}
	}
#endif
#if _DRV_L1_TFT == 1
	if(enable & PPU_TFT_V_BLANKING) {
		if(pDisp->tft_update_flag) {
			R_TFT_FBI_ADDR = pDisp->tft_disp_buf;
			pDisp->tft_update_flag = 0;
		#if _OPERATING_SYSTEM != _OS_NONE 
			OSQPost(pDisp->tft_q, (void *)DISP_UPDATE_DONE);
		#endif
		}
	}
#endif
#if _DRV_L1_HDMI == 1
	if(enable & PPU_HDMI_V_BLANKING) {
		if(pDisp->hdmi_update_flag == 1) {
			if(pDisp->hdmi_color_fmt == DISP_FMT_GP420) {
				drv_l1_conv420_input_A_addr_set(pDisp->hdmi_disp_buf);
			}
			
			R_TV_FBI_ADDR = pDisp->hdmi_disp_buf;
			pDisp->hdmi_update_flag = 0;
		#if _OPERATING_SYSTEM != _OS_NONE 
			OSQPost(pDisp->hdmi_q, (void *)DISP_UPDATE_DONE);
		#endif
		}
		
		drv_l1_conv420_reset();	
		
		if(pDisp->hdmi_en_flag == 1) {
			// hdmi start
			if(pDisp->hdmi_color_fmt == DISP_FMT_GP420) {
				// use conv420to422 module
				drv_l1_conv420_start();
			}
		} else if(pDisp->hdmi_en_flag == 0xFF) {
			// hdmi stop
			pDisp->hdmi_en_flag = 0;
			drv_l1_conv420_uninit();
		}
	}
#endif
}

static void ppu_set_size(INT32U disp_dev, INT16U width, INT16U height)
{
	INT32U reg = R_PPU_ENABLE;
	
	reg &= ~TFT_SIZE_MASK; 
	if(disp_dev == DISDEV_TV_QVGA) {
		reg &= ~PPU_VGA_MODE;
	} else if(disp_dev == DISDEV_TV_VGA) {
		reg |= PPU_VGA_MODE;
	} else if(disp_dev == DISDEV_TV_D1) {
		reg &= ~PPU_VGA_MODE;
		reg |= TFT_SIZE_720X480;
	} else {
		reg &= ~PPU_VGA_MODE;
		reg |= PPU_LB;
		if(width == 320 && height == 240) {
			reg |= TFT_SIZE_320X240;
		} else if(width == 480 && height == 234) {
			reg |= TFT_SIZE_480X234;
		} else if(width == 480 && height == 272) {
			reg |= TFT_SIZE_480X272;
		} else if(width == 640 && height == 484) {
			reg |= TFT_SIZE_640X480;
		} else if(width == 720 && height == 484) {
			reg |= TFT_SIZE_720X480;
		} else if(width == 800 && height == 480) {
			reg |= TFT_SIZE_800X480;
		} else if(width == 800 && height == 600) {
			reg |= TFT_SIZE_800X600;
		} else if(width == 1024 && height == 768) {
			reg |= TFT_SIZE_1024X768;
		} else {
			R_FREE_SIZE	= ((width & 0x7FF) << 16) | (height & 0x7FF);
		}
	}
	
	R_PPU_ENABLE = reg;
}

static void ppu_set_color_mode(INT32U color_mode)
{
	INT32U reg = R_PPU_ENABLE;
	
	reg &= ~(3 << 20);
	switch(color_mode)
	{
	case DISP_FMT_RGB565:
		reg &= ~PPU_YUYV_RGBG_FORMAT_MODE;  //RGB565 mode
		reg &= ~PPU_YUYV_MODE;				//RGB565
		break;
	
	case DISP_FMT_BGRG:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg &= ~PPU_YUYV_MODE;				//RGBG 	
		reg |= PPU_RGBG_TYPE0;
		break;
		
	case DISP_FMT_GBGR:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg &= ~PPU_YUYV_MODE;				//RGBG 	
		reg |= PPU_RGBG_TYPE1;
		break;	
	
	case DISP_FMT_RGBG:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg &= ~PPU_YUYV_MODE;				//RGBG 	
		reg |= PPU_RGBG_TYPE2;
		break;
		
	case DISP_FMT_GRGB:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg &= ~PPU_YUYV_MODE;				//RGBG 	
		reg |= PPU_RGBG_TYPE3;
		break;
	
	case DISP_FMT_VYUV:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg |= PPU_YUYV_MODE;				//YUYV 	
		reg |= PPU_YUYV_TYPE0;
		break;	
		
	case DISP_FMT_YVYU:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg |= PPU_YUYV_MODE;				//YUYV 	
		reg |= PPU_YUYV_TYPE1;
		break;			
	
	case DISP_FMT_UYVY:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg |= PPU_YUYV_MODE;				//YUYV 	
		reg |= PPU_YUYV_TYPE2;
		break;	
	
	case DISP_FMT_YUYV:
		reg |= PPU_YUYV_RGBG_FORMAT_MODE;  
		reg |= PPU_YUYV_MODE;				//YUYV 	
		reg |= PPU_YUYV_TYPE3;
		break;		
	}
	
	R_PPU_ENABLE = reg;
}

static void ppu_set_enable(INT32U enable)
{
	if(enable) {
		R_PPU_ENABLE |= PPU_FRAME_BASE_MODE;
	} else {
		R_PPU_ENABLE = 0;
		R_PPU_IRQ_EN = 0;
		R_PPU_IRQ_STATUS = 0xFFFF;
	}
}

/**
 * @brief   init display 
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_init(void)
{
	INT32S ret;
	
	if(pDisp != 0) {
		ret = STATUS_OK;
		goto __exit;
	}
	
#if _DRV_L1_TV == 1
	tv_init();
#endif
#if _DRV_L1_TFT == 1
	drv_l1_tft_init();
#endif
	ppu_set_enable(DISABLE);

	pDisp = &display;
	gp_memset((INT8S *)pDisp, 0x00, sizeof(display_t));	
	
#if _OPERATING_SYSTEM != _OS_NONE
#if _DRV_L1_TV == 1
	pDisp->tv_q = OSQCreate(&pDisp->tv_q_buf[0], DISP_QUEUE_MAX);
	if(pDisp->tv_q == 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}

	pDisp->tv_sem = OSSemCreate(1);
	if(pDisp->tv_sem == 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}
#endif

#if _DRV_L1_TFT == 1
	pDisp->tft_q = OSQCreate(&pDisp->tft_q_buf[0], DISP_QUEUE_MAX);
	if(pDisp->tft_q == 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}
	
	pDisp->tft_sem = OSSemCreate(1);
	if(pDisp->tft_sem == 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}
#endif

#if _DRV_L1_HDMI == 1
	pDisp->hdmi_q = OSQCreate(&pDisp->hdmi_q_buf[0], DISP_QUEUE_MAX);
	if(pDisp->hdmi_q == 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}
	
	pDisp->hdmi_sem = OSSemCreate(1);
	if(pDisp->hdmi_sem == 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}
#endif
#endif
	
	vic_irq_register(VIC_PPU, ppu_isr);
	vic_irq_enable(VIC_PPU);
	ret = STATUS_OK;
__exit:
	if(ret < 0) {
		drv_l2_display_uninit();
	}	
	return ret;
}

/**
 * @brief   uninit display 
 * @param   none
 * @return 	none
 */
void drv_l2_display_uninit(void)
{
	if(pDisp == 0) {
		return;
	}
	
#if _DRV_L1_TV == 1
	tv_disable();
#endif

#if _DRV_L1_TFT == 1
	drv_l1_tft_init();
#endif

#if _DRV_L1_HDMI == 1
	drvl1_hdmi_exit();
#endif
	ppu_set_enable(DISABLE);

#if _OPERATING_SYSTEM != _OS_NONE	
	if(pDisp->tv_q) {
		INT8U err;
		
		OSQFlush(pDisp->tv_q);
		OSQDel(pDisp->tv_q, OS_DEL_ALWAYS, &err);
		pDisp->tv_q = 0;
	}
	
	if(pDisp->tft_q) {
		INT8U err;
		
		OSQFlush(pDisp->tft_q);
		OSQDel(pDisp->tft_q, OS_DEL_ALWAYS, &err);
		pDisp->tft_q = 0;
	}
	
	if(pDisp->hdmi_q) {
		INT8U err;
		
		OSQFlush(pDisp->hdmi_q);
		OSQDel(pDisp->hdmi_q, OS_DEL_ALWAYS, &err);
		pDisp->hdmi_q = 0;
	}
	
	if(pDisp->tv_sem) {
		INT8U err;
		
		OSSemDel(pDisp->tv_sem, OS_DEL_ALWAYS, &err);
		pDisp->tv_sem = 0;
	}
	
	if(pDisp->tft_sem) {
		INT8U err;
		
		OSSemDel(pDisp->tft_sem, OS_DEL_ALWAYS, &err);
		pDisp->tft_sem = 0;
	}
	
	if(pDisp->hdmi_sem) {
		INT8U err;
		
		OSSemDel(pDisp->hdmi_sem, OS_DEL_ALWAYS, &err);
		pDisp->hdmi_sem = 0;
	}
#endif		
	
	vic_irq_unregister(VIC_PPU);
	vic_irq_disable(VIC_PPU);
	pDisp = 0;
}

/**
 * @brief   display start 
 * @param   disp_dev[in]: display device, see DISP_DEV.
 * @param   color_mode[in]: display color mode, DISP_FMT  
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_start(DISP_DEV disp_dev, DISP_FMT color_mode)
{
	switch(disp_dev)
	{
	#if _DRV_L1_TV == 1
	case DISDEV_TV_QVGA:
		pDisp->tv_color_fmt = color_mode;
		
		tv_start(TVSTD_NTSC_J, TV_QVGA, TV_NON_INTERLACE);
		
		ppu_set_size(DISDEV_TV_QVGA, 320, 240);
		ppu_set_color_mode(color_mode);
		ppu_set_enable(ENABLE);	
		R_PPU_IRQ_STATUS = PPU_TV_V_BLANKING;
		R_PPU_IRQ_EN |= PPU_TV_V_BLANKING;
		break;
		
	case DISDEV_TV_VGA:
		pDisp->tv_color_fmt = color_mode;
		
		tv_start(TVSTD_NTSC_J, TV_HVGA, TV_INTERLACE);
		
		ppu_set_size(DISDEV_TV_VGA, 640, 480);
		ppu_set_color_mode(color_mode);
		ppu_set_enable(ENABLE);	
		R_PPU_IRQ_STATUS = PPU_TV_V_BLANKING;
		R_PPU_IRQ_EN |= PPU_TV_V_BLANKING;
		break;
		
	case DISDEV_TV_D1:
		pDisp->tv_color_fmt = color_mode;
		
		tv_start(TVSTD_NTSC_J, TV_D1, TV_INTERLACE);
		
		ppu_set_size(DISDEV_TV_D1, 740, 480);
		ppu_set_color_mode(color_mode);	
		ppu_set_enable(ENABLE);	
		R_PPU_IRQ_STATUS = PPU_TV_V_BLANKING;
		R_PPU_IRQ_EN |= PPU_TV_V_BLANKING;
		break;
	#endif
	#if _DRV_L1_TFT == 1
	case DISDEV_TFT:
		pDisp->tft_color_fmt = color_mode;
		
		TFT_Param.init();
		
		ppu_set_size(DISDEV_TFT, TFT_Param.width, TFT_Param.height);
		ppu_set_color_mode(color_mode);	
		ppu_set_enable(ENABLE);
		R_PPU_IRQ_STATUS = PPU_TFT_V_BLANKING;
		R_PPU_IRQ_EN |= PPU_TFT_V_BLANKING;
		break;
	#endif
	#if _DRV_L1_HDMI == 1
	case DISDEV_HDMI_480P:
		pDisp->hdmi_en_flag = 1;
		pDisp->hdmi_color_fmt = color_mode;
		if(color_mode == DISP_FMT_GP420) {
			// use ConvGP420toYUV422 module
			drv_l1_conv420_init();
			drv_l1_conv420_reset();
			drv_l1_conv420_path(CONV420_TO_TV_HDMI);
			drv_l1_conv420_output_fmt_set(CONV420_FMT_YUYV);
			drv_l1_conv420_input_pixels_set(480);
			drv_l1_conv420_convert_enable(ENABLE);
			drv_l1_conv420_start();
		}
		else {
			// bypass ConvGP420toYUV422 module
			drv_l1_conv420_uninit();
		}
			
		drvl1_hdmi_init(2, 44100);
		drvl1_hdmi_audio_ctrl(1);
		drvl1_hdmi_dac_mute(1);	
		
		if(color_mode == DISP_FMT_GP420) {
			ppu_set_color_mode(DISP_FMT_YUYV);	
		} else {
			ppu_set_color_mode(color_mode);
		}
		
		ppu_set_size(DISDEV_HDMI_480P, 720, 480);
		ppu_set_enable(ENABLE);
		R_PPU_IRQ_STATUS = PPU_HDMI_V_BLANKING;
		R_PPU_IRQ_EN |= PPU_HDMI_V_BLANKING;
		break;
		
	case DISDEV_HDMI_720P:
		pDisp->hdmi_en_flag = 1;
		pDisp->hdmi_color_fmt = color_mode;
		if(color_mode == DISP_FMT_GP420) {
			// use ConvGP420toYUV422 module
			drv_l1_conv420_init();
			drv_l1_conv420_reset();
			drv_l1_conv420_path(CONV420_TO_TV_HDMI);
			drv_l1_conv420_output_fmt_set(CONV420_FMT_YUYV);
			drv_l1_conv420_input_pixels_set(1280);
			drv_l1_conv420_convert_enable(ENABLE);
			drv_l1_conv420_start();
		}
		else {
			// bypass Conv GP420 to YUV422
			drv_l1_conv420_uninit();
		}
		
		drvl1_hdmi_init(6, 44100);
		drvl1_hdmi_audio_ctrl(1);
		drvl1_hdmi_dac_mute(1);	
		
		if(color_mode == DISP_FMT_GP420) {
			ppu_set_color_mode(DISP_FMT_YUYV);	
		} else {
			ppu_set_color_mode(color_mode);
		}
		
		ppu_set_size(DISDEV_HDMI_720P, 1280, 720);
		ppu_set_enable(ENABLE);
		R_PPU_IRQ_STATUS = PPU_HDMI_V_BLANKING;
		R_PPU_IRQ_EN |= PPU_HDMI_V_BLANKING;
		break;
	#endif
	default:
		return STATUS_FAIL;
	}	
	
	return STATUS_OK;
}

/**
 * @brief   display stop
 * @param   disp_dev[in]: display device, see DISP_DEV. 
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_stop(DISP_DEV disp_dev)
{
	switch(disp_dev)
	{
	#if _DRV_L1_TV == 1
	case DISDEV_TV_QVGA:
	case DISDEV_TV_VGA:
	case DISDEV_TV_D1:
		tv_disable();
		R_PPU_IRQ_EN &= ~PPU_TV_V_BLANKING;
		R_PPU_IRQ_STATUS = PPU_TV_V_BLANKING;
		break;
	#endif
	#if _DRV_L1_TFT == 1
	case DISDEV_TFT:
		drv_l1_tft_en_set(DISABLE);		
		R_PPU_IRQ_EN &= ~PPU_TFT_V_BLANKING;
		R_PPU_IRQ_STATUS = PPU_TFT_V_BLANKING;
		break;
	#endif
	#if _DRV_L1_HDMI == 1
	case DISDEV_HDMI_480P:
	case DISDEV_HDMI_720P:
		if(pDisp->hdmi_en_flag) {
			INT8U err;
			
			OSSemPend(pDisp->hdmi_sem, 0, &err);
			pDisp->hdmi_en_flag = 0xFF;
			while(pDisp->hdmi_en_flag) {
				OSTimeDly(1);
			}
			
			R_PPU_IRQ_EN &= ~PPU_HDMI_V_BLANKING;
			R_PPU_IRQ_STATUS = PPU_HDMI_V_BLANKING;
			OSSemPost(pDisp->hdmi_sem);
		}
		drvl1_hdmi_exit();
		break;
	#endif
	default:
		return STATUS_FAIL;
	}	
	
	return STATUS_OK;
}

/**
 * @brief   display get size
 * @param   disp_dev[in]: display device  
 * @param   width[out]: width
 * @param   height[out]: height  
 * @return 	none
 */
void drv_l2_display_get_size(DISP_DEV disp_dev, INT16U *width, INT16U *height)
{
	switch(disp_dev)
	{
	case DISDEV_TV_QVGA:
		*width = 320;
		*height = 240;
		break;
		
	case DISDEV_TV_VGA:
		*width = 640;
		*height = 480;
		break;	
		
	case DISDEV_TV_D1:
		*width = 720;
		*height = 480;
		break;		
	
	case DISDEV_TFT:
		*width = TFT_Param.width;
		*height = TFT_Param.height;
		break;
	
	case DISDEV_HDMI_480P:
		*width = 720;
		*height = 480;
		break;		
	
	case DISDEV_HDMI_720P:
		*width = 1280;
		*height = 720;
		break;
				
	default:
		*width = 0;
		*height = 0;	
	}
}	

/**
 * @brief   display get format 
 * @param   disp_dev[in]: display device
 * @return 	display format
 */
INT32U drv_l2_display_get_fmt(DISP_DEV disp_dev)
{
	switch(disp_dev)
	{
	case DISDEV_TV_QVGA:	
	case DISDEV_TV_VGA:
	case DISDEV_TV_D1:
		return pDisp->tv_color_fmt;
	
	case DISDEV_TFT:
		return pDisp->tft_color_fmt;
	
	case DISDEV_HDMI_480P:
	case DISDEV_HDMI_720P:
		return pDisp->hdmi_color_fmt;
		
	default:
		return DISP_FMT_MAX;	
	}
}

/**
 * @brief   display update frame buffer 
 * @param   buffer[in]: frame buffer address 
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_display_update(DISP_DEV disp_dev, INT32U buffer)
{
	INT8U err;
	INT32U message;
	
	if(buffer == 0) {
		return STATUS_FAIL;
	} 
	
	switch(disp_dev)
	{
	#if _DRV_L1_TV == 1
	case DISDEV_TV_QVGA:	
	case DISDEV_TV_VGA:
	case DISDEV_TV_D1:
		pDisp->tv_disp_buf = buffer;
		pDisp->tv_update_flag = 1;
	#if _OPERATING_SYSTEM != _OS_NONE
		OSSemPend(pDisp->tv_sem, 0, &err);			
		message = (INT32U)OSQPend(pDisp->tv_q, 100, &err);
		OSSemPost(pDisp->tv_sem);
		
		if((message != DISP_UPDATE_DONE) || (err != OS_NO_ERR)) {
			return STATUS_FAIL;
		}
	#else
		while(pDisp->tv_update_flag);
	#endif
		break;
	#endif
	#if _DRV_L1_TFT == 1
	case DISDEV_TFT:
		pDisp->tft_disp_buf = buffer;
		pDisp->tft_update_flag = 1;
	#if _OPERATING_SYSTEM != _OS_NONE
		OSSemPend(pDisp->tft_sem, 0, &err);		
		message = (INT32U)OSQPend(pDisp->tft_q, 100, &err);
		OSSemPost(pDisp->tft_sem);
		
		if((message != DISP_UPDATE_DONE) || (err != OS_NO_ERR)) {
			return STATUS_FAIL;
		}
	#else
		while(pDisp->tft_update_flag);
	#endif
		break;
	#endif
	#if _DRV_L1_HDMI == 1
	case DISDEV_HDMI_480P:
	case DISDEV_HDMI_720P:
		pDisp->hdmi_disp_buf = buffer;
		pDisp->hdmi_update_flag = 1;
	#if _OPERATING_SYSTEM != _OS_NONE
		OSSemPend(pDisp->hdmi_sem, 0, &err);		
		message = (INT32U)OSQPend(pDisp->hdmi_q, 100, &err);
		OSSemPost(pDisp->hdmi_sem);
		
		if((message != DISP_UPDATE_DONE) || (err != OS_NO_ERR)) {
			return STATUS_FAIL;
		}
	#else
		while(pDisp->hdmi_update_flag);
	#endif
		break;
	#endif
	default:
		return DISDEV_MAX;	
	}
	
	return STATUS_OK;
}
	
#endif //(defined _DRV_L2_DISP) && (_DRV_L2_DISP == 1)
