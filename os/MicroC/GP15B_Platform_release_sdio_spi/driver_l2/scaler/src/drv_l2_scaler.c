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
#include "drv_l1_scaler.h"
#include "drv_l2_scaler.h"
#include "gplib_mm_gplus.h"

#if (defined _DRV_L2_SCALER) && (_DRV_L2_DISP == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define FORCE_INTP_EN	0

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
#define DEBUG				DBG_PRINT
#else
#define DEBUG(...)
#endif

#define RETURN(x, msg)\
{\
	ret = x;\
	DBG_PRINT(msg);\
	goto __exit;\
}

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U ext_line_buf[SCALER_MAX];

/**
 * @brief   scale init  
 * @param   none
 * @return 	none
 */
void drv_l2_scaler_init(void)
{
	INT32U i;
	
	for(i=0; i<SCALER_MAX; i++) {
		ext_line_buf[i] = 0;
	}

	drv_l1_scaler_init(SCALER_0);
	drv_l1_scaler_init(SCALER_1);
}

/**
 * @brief   scale clip image  
 * @param   scale_dev[in]: scaler device
 * @param   wait_done[in]: wait scaler finish
 * @param   src[in]: source image
 * @param   dst[in]: destination image
 * @param   clip[in]: clip range
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_scaler_clip(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst, gpRect *clip)
{	
	// IMAGE Src, Dst;
	INT32S src_width, dst_width;
	INT32S in_format, out_format;
	ScalerFormat_t scale;
	
	switch(src->format)
	{
	//case IMG_FMT_GRAY:
	//	src_width = src->widthStep >> 1 << 1; //align2
	//	in_format = C_SCALER_CTRL_IN_Y_ONLY;
	//	break;
	case IMG_FMT_YUYV:
	case IMG_FMT_YCbYCr:
		src_width = src->widthStep >> 2 << 1; //align2
		in_format = C_SCALER_CTRL_IN_UYVY;
		break;
	case IMG_FMT_UYVY:
	case IMG_FMT_CbYCrY:
		src_width = src->widthStep >> 2 << 1; //align2
		in_format = C_SCALER_CTRL_IN_YUYV;
		break;
	default:
        return STATUS_FAIL;
	}
		
	switch(dst->format)
	{
	//case IMG_FMT_GRAY:
	//	dst_width = dst->widthStep >> 1 << 1; //align2
	//	out_format = C_SCALER_CTRL_OUT_Y_ONLY;
	//	break;
	case IMG_FMT_YUYV:
	case IMG_FMT_YCbYCr:
		dst_width = dst->widthStep >> 4 << 3; //align8
		out_format = C_SCALER_CTRL_OUT_UYVY;
		break;
	case IMG_FMT_UYVY:
	case IMG_FMT_CbYCrY:
		dst_width = dst->widthStep >> 4 << 3; //align8
		out_format = C_SCALER_CTRL_OUT_YUYV;
		break;
	default:
		return STATUS_FAIL;
	}
		
	scale.input_format = in_format;
	scale.input_width = src_width;
	scale.input_height = src->height;
	scale.input_visible_width = clip->x + clip->width;
	scale.input_visible_height = clip->y + clip->height;
	scale.input_x_offset = clip->x;
	scale.input_y_offset = clip->y;
	
	scale.input_y_addr = (INT32U)src->ptr;
	scale.input_u_addr = 0;
	scale.input_v_addr = 0;
	
	scale.output_format = out_format;
	scale.output_width = dst_width;
	scale.output_height = dst->height;
	scale.output_buf_width = dst_width;
	scale.output_buf_height = dst->height;
	scale.output_x_offset = 0;
	
	scale.output_y_addr = (INT32U)dst->ptr;
	scale.output_u_addr = 0;
	scale.output_v_addr = 0;
		
	scale.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;
	scale.scale_mode = C_SCALER_FULL_SCREEN_BY_RATIO;
	scale.digizoom_m = 10;
	scale.digizoom_n = 10;
		
	return drv_l2_scaler_trigger(scale_dev, wait_done, &scale, 0x008080);
}

/**
 * @brief   scale image  
 * @param   scale_dev[in]: scaler device
 * @param   wait_done[in]: wait scaler finish
 * @param   src[in]: source image
 * @param   dst[in]: destination image
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_scaler_once(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst)
{
	// IMAGE Src, Dst;
	INT32S src_width, dst_width;
	INT32S in_format, out_format;
	ScalerFormat_t scale;
	
	switch(src->format)
	{
	//case IMG_FMT_GRAY:
	//	src_width = src->widthStep >> 1 << 1; //align2
	//	in_format = C_SCALER_CTRL_IN_Y_ONLY;
	//	break;
	case IMG_FMT_YUYV:
	case IMG_FMT_YCbYCr:
		src_width = src->widthStep >> 2 << 1; //align2
		in_format = C_SCALER_CTRL_IN_UYVY;
		break;
	case IMG_FMT_UYVY:
	case IMG_FMT_CbYCrY:
		src_width = src->widthStep >> 2 << 1; //align2
		in_format = C_SCALER_CTRL_IN_YUYV;
		break;
	default:
        return STATUS_FAIL;
	}
		
	switch(dst->format)
	{
	//case IMG_FMT_GRAY:
	//	dst_width = dst->widthStep >> 1 << 1; //align2
	//	out_format = C_SCALER_CTRL_OUT_Y_ONLY;
	//	break;
	case IMG_FMT_YUYV:
	case IMG_FMT_YCbYCr:
		dst_width = dst->widthStep >> 4 << 3; //align8
		out_format = C_SCALER_CTRL_OUT_UYVY;
		break;
	case IMG_FMT_UYVY:
	case IMG_FMT_CbYCrY:
		dst_width = dst->widthStep >> 4 << 3; //align8
		out_format = C_SCALER_CTRL_OUT_YUYV;
		break;
	default:
		return STATUS_FAIL;
	}
		
	scale.input_format = in_format;
	scale.input_width = src_width;
	scale.input_height = src->height;
	scale.input_visible_width = src->width;
	scale.input_visible_height = src->height;
	scale.input_x_offset = 0;
	scale.input_y_offset = 0;
	
	scale.input_y_addr = (INT32U)src->ptr;
	scale.input_u_addr = 0;
	scale.input_v_addr = 0;
	
	scale.output_format = out_format;
	scale.output_width = dst_width;
	scale.output_height = dst->height;
	scale.output_buf_width = dst_width;
	scale.output_buf_height = dst->height;
	scale.output_x_offset = 0;
	
	scale.output_y_addr = (INT32U)dst->ptr;
	scale.output_u_addr = 0;
	scale.output_v_addr = 0;
		
	scale.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;
	scale.scale_mode = C_SCALER_FULL_SCREEN_BY_RATIO;
	scale.digizoom_m = 10;
	scale.digizoom_n = 10;
		
	return drv_l2_scaler_trigger(scale_dev, wait_done, &scale, 0x008080);
}

/**
 * @brief   scale wait done  
 * @param   scale_dev[in]: scaler device
 * @param   wait_done[in]: wait scaler finish
 * @param   pScale[in]: scale parameter
 * @return 	scaler status
 */
INT32S drv_l2_scaler_wait_done(INT8U scale_dev, ScalerFormat_t *pScale)
{
	// scale a
	if(pScale->scale_status == C_SCALER_STATUS_STOP) {
		return C_SCALER_STATUS_STOP;
	}

	// scale done
	if(pScale->scale_status == C_SCALER_STATUS_DONE) {
		pScale->scale_status = C_SCALER_STATUS_STOP;
		drv_l2_scaler_stop(scale_dev);
		return C_SCALER_STATUS_STOP;
	} 
	
	pScale->scale_status = drv_l1_scaler_wait_idle(scale_dev);
	if(pScale->scale_status == C_SCALER_STATUS_TIMEOUT) {
		pScale->scale_status = C_SCALER_STATUS_STOP;
		drv_l2_scaler_stop(scale_dev);
		return C_SCALER_STATUS_TIMEOUT;
	} 
	
	return pScale->scale_status;
}

/**
 * @brief   scale stop 
 * @param   scale_dev[in]: scaler device
 * @return 	scaler status
 */
void drv_l2_scaler_stop(INT8U scale_dev)
{	
	drv_l1_scaler_stop(scale_dev);
	
	// free line buffer
	if(ext_line_buf[scale_dev]) {
		gp_free((void *)ext_line_buf[scale_dev]);
		ext_line_buf[scale_dev] = 0;
	}

	// hardware unlock
	drv_l1_scaler_unlock(scale_dev);
}

/**
 * @brief   scale image trigger  
 * @param   scale_dev[in]: scaler device
 * @param   wait_done[in]: wait scaler finish
 * @param   pScale[in]: scale parameter
 * @param   boundary_color[in]: boundary color
 * @return 	scaler status
 */
INT32S drv_l2_scaler_trigger(INT8U scale_dev, INT8U wait_done, ScalerFormat_t *pScale, INT32U boundary_color)
{
	INT8U mode = 0;
	INT32S ret;
	INT32U tempx, tempy;

	DEBUG("%s\r\n", __func__);
	
	// times
	pScale->reserved0 = 1;

	// scaler lock
	drv_l1_scaler_lock(scale_dev);	
	
	if(pScale->input_visible_width > pScale->input_width) {
		pScale->input_visible_width = 0;
	}
	
	if(pScale->input_visible_height > pScale->input_height) {
		pScale->input_visible_height = 0;
	}
	
	if(pScale->output_width > pScale->output_buf_width) {
		pScale->output_width = pScale->output_buf_width;
	}
	
	if(pScale->output_height > pScale->output_buf_height) {
		pScale->output_height = pScale->output_buf_height;
	}
	
	// scale mode setting
	drv_l1_scaler_init(scale_dev);
	drv_l1_scaler_isr_callback_set(NULL);
	switch(pScale->scale_mode) 
	{
	case C_SCALER_FULL_SCREEN:	
		ret = drv_l1_scaler_image_pixels_set(scale_dev, pScale->input_width, pScale->input_height, pScale->output_buf_width, pScale->output_buf_height);	
		if(ret < 0) {
			RETURN(C_SCALER_START_ERR, "SCALER_START_ERR\r\n");		
		}
	#if FORCE_INTP_EN == 1	
		tempx += 0x8000;
		tempy += 0x8000;
		nRet = drv_l1_scaler_input_offset_set(tempx, tempy);
	#endif		
		break;
		
	case C_SCALER_BY_RATIO:	
		mode = 1;
	case C_SCALER_FULL_SCREEN_BY_RATIO:
		ret = drv_l1_scaler_input_pixels_set(scale_dev, pScale->input_width, pScale->input_height);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_SIZE_ERR, "SCALER_INPUT_SIZE_ERR\r\n");
		}
	
		ret = drv_l1_scaler_input_visible_pixels_set(scale_dev, pScale->input_visible_width, pScale->input_visible_height);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_SIZE_ERR, "SCALER_INPUT_VISIBLE_SIZE_ERR\r\n");
		}
		
		if(pScale->input_visible_width) {
			tempx = pScale->input_visible_width;
		} else {
			tempx = pScale->input_width;
		}
		
		if(pScale->input_visible_height) {
			tempy = pScale->input_visible_height;
		} else {
			tempy = pScale->input_height;
		}
		
		if(pScale->input_x_offset > tempx) {
			pScale->input_x_offset = 0;
		}
		
		if(pScale->input_y_offset > tempy) {
			pScale->input_y_offset = 0;
		}
		
		if(pScale->input_x_offset) {
			tempx -= pScale->input_x_offset;
		}
		
		if(pScale->input_y_offset) {
			tempy -= pScale->input_y_offset;
		}
		
		if(mode) {
			/* scale by ratio */
			tempx = (tempx << 16) / pScale->output_width;
			tempy = (tempy << 16) / pScale->output_height;
		} else {
			/* scale full screen by ratio */
			if (pScale->output_buf_height*tempx > pScale->output_buf_width*tempy) {
		      	tempx = tempy = (tempx << 16) / pScale->output_buf_width;
		      	pScale->output_height = (pScale->output_buf_height << 16) / tempx;
		    } else {
		      	tempx = tempy = (tempy << 16) / pScale->output_buf_height;
		      	pScale->output_width = (pScale->output_buf_width << 16) / tempy;
			}
		}
		
		ret = drv_l1_scaler_output_pixels_set(scale_dev, tempx, tempy, pScale->output_buf_width, pScale->output_buf_height);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_SIZE_ERR, "SCALER_OUTPUT_SIZE_ERR\r\n");
		}
	
		tempx = pScale->input_x_offset << 16;
		tempy = pScale->input_y_offset << 16;
	#if FORCE_INTP_EN == 1	
		tempx += 0x8000;
		tempy += 0x8000;
	#endif	
	
		ret = drv_l1_scaler_input_offset_set(scale_dev, tempx, tempy);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_OFFSET_ERR, "SCALER_INPUT_OFFSET_ERR\r\n");
		}
		break;
		
	case C_SCALER_FULL_SCREEN_BY_DIGI_ZOOM:
		ret = drv_l1_scaler_input_pixels_set(scale_dev, pScale->input_width, pScale->input_height);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_SIZE_ERR, "SCALER_INPUT_SIZE_ERR\r\n");
		}
		
		ret = drv_l1_scaler_input_visible_pixels_set(scale_dev, pScale->input_visible_width, pScale->input_visible_height);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_SIZE_ERR, "SCALER_INPUT_VISIBLE_SIZE_ERR\r\n");
		}
		
		/* mutiple * 100 */
		if(pScale->digizoom_n == 0) {
			pScale->digizoom_n = 10;
		}

		if(pScale->digizoom_m == 0) {
			pScale->digizoom_m = 10;
		}
		
		tempx = 100 * (pScale->output_width * pScale->digizoom_m) / (pScale->input_width * pScale->digizoom_n);
		tempx = 65536 * 100 / tempx;
		ret = drv_l1_scaler_output_pixels_set(scale_dev, tempx, tempx, pScale->output_buf_width, pScale->output_buf_height);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_SIZE_ERR, "SCALER_OUTPUT_SIZE_ERR\n");
		}
			
		tempx = pScale->output_width * (ABS(pScale->digizoom_m - pScale->digizoom_n));
		tempx >>= 1;
		tempx =	(tempx << 16) / pScale->digizoom_n;
		tempy = pScale->output_height * (ABS(pScale->digizoom_m - pScale->digizoom_n));
		tempy >>= 1;
		tempy = (tempy << 16) / pScale->digizoom_n;
		ret = drv_l1_scaler_input_offset_set(scale_dev, tempx, tempy);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_OFFSET_ERR, "SCALER_INPUT_OFFSET_ERR\r\n");
		}
		break;
	
	case C_SCALER_RATIO_USER:
		ret = drv_l1_scaler_input_pixels_set(scale_dev, pScale->input_width, pScale->input_height);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_SIZE_ERR, "SCALER_INPUT_SIZE_ERR\r\n");
		}
	
		ret = drv_l1_scaler_input_visible_pixels_set(scale_dev, pScale->input_width, pScale->input_height);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_SIZE_ERR, "SCALER_INPUT_VISIBLE_SIZE_ERR\r\n");
		}
		
		if(pScale->input_visible_width) {
			tempx = pScale->input_visible_width;
		} else {
			tempx = pScale->input_width;
		}
		
		if(pScale->input_visible_height) {
			tempy = pScale->input_visible_height;
		} else {
			tempy = pScale->input_height;
		}
				
		/* scale by ratio */
		tempx = (tempx << 16) / pScale->output_width;
		tempy = (tempy << 16) / pScale->output_height;
		
		ret = drv_l1_scaler_output_pixels_set(scale_dev, tempx, tempy, pScale->output_buf_width, pScale->output_buf_height);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_SIZE_ERR, "SCALER_OUTPUT_SIZE_ERR\r\n");
		}
	
		tempx = pScale->input_x_offset;
		tempy = pScale->input_y_offset;
	#if FORCE_INTP_EN == 1	
		tempx += 0x8000;
		tempy += 0x8000;
	#endif	
	
		ret = drv_l1_scaler_input_offset_set(scale_dev, tempx, tempy);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_OFFSET_ERR, "SCALER_OUTPUT_OFFSET_ERR\r\n");
		}	
		break;	
		
	default:
		RETURN(C_SCALER_START_ERR, "scaler mode fail!\r\n");
	}
	
	// alloc ext buffer for line buffer use
	if(pScale->output_buf_width >= 1024) {
		if(ext_line_buf[scale_dev]) {
			gp_free((void *)ext_line_buf[scale_dev]);
			ext_line_buf[scale_dev] = 0;
		}
		
		ext_line_buf[scale_dev] = (INT32U)gp_malloc_align(pScale->output_buf_width*4*2, 4);
		if(ext_line_buf[scale_dev] == 0) {
			RETURN(C_SCALER_EXT_BUF_ERR, "SCALER_EXT_BUF_ERR\r\n");
		}
		
		drv_l1_scaler_external_line_buffer_set(scale_dev, ext_line_buf[scale_dev]);
		ret = drv_l1_scaler_line_buffer_mode_set(scale_dev, C_SCALER_EXTERNAL_LINE_BUFFER);
		if(ret < 0) {
			RETURN(C_SCALER_EXT_BUF_ERR, "SCALER_EXT_BUF_ERR\r\n");
		}
	}
	
	ret = drv_l1_scaler_output_offset_set(scale_dev, pScale->output_x_offset);
	if(ret < 0) {
		RETURN(C_SCALER_OUTPUT_BUF_ERR, "SCALER_OUTPUT_BUF_ERR\r\n");
	}
	
	ret = drv_l1_scaler_input_A_addr_set(scale_dev, pScale->input_y_addr, pScale->input_u_addr, pScale->input_v_addr);
	if(ret < 0) {
		RETURN(C_SCALER_INPUT_BUF_ERR, "SCALER_INPUT_BUF_ERR\r\n");
	}
	
	ret = drv_l1_scaler_output_addr_set(scale_dev, pScale->output_y_addr, pScale->output_u_addr, pScale->output_v_addr);
	if(ret < 0) {
		RETURN(C_SCALER_OUTPUT_BUF_ERR, "SCALER_OUTPUT_BUF_ERR\r\n");
	}
	
	ret = drv_l1_scaler_input_format_set(scale_dev, pScale->input_format);
	if(ret < 0) {
		RETURN(C_SCALER_INPUT_FMT_ERR, "SCALER_INPUT_FMT_ERR\r\n");
	}
	
	ret = drv_l1_scaler_output_format_set(scale_dev, pScale->output_format);
	if(ret < 0) {
		RETURN(C_SCALER_OUTPUT_FMT_ERR, "SCALER_OUTPUT_FMT_ERR\r\n");
	}
	
	switch(pScale->fifo_mode)
	{
	case C_SCALER_CTRL_FIFO_DISABLE:
		ret = drv_l1_scaler_input_fifo_line_set(scale_dev, C_SCALER_CTRL_IN_FIFO_DISABLE);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_FIFO_ERR, "SCALER_INPUT_FIFO_ERR\r\n");
		}

		ret = drv_l1_scaler_output_fifo_line_set(scale_dev, C_SCALER_CTRL_OUT_FIFO_DISABLE);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_FIFO_ERR, "SCALER_OUTPUT_FIFO_ERR\r\n");
		}
		break;	
		
	case C_SCALER_CTRL_IN_FIFO_16LINE:
	case C_SCALER_CTRL_IN_FIFO_32LINE:
	case C_SCALER_CTRL_IN_FIFO_64LINE:
	case C_SCALER_CTRL_IN_FIFO_128LINE:
	case C_SCALER_CTRL_IN_FIFO_256LINE:
		ret = drv_l1_scaler_input_fifo_line_set(scale_dev, pScale->fifo_mode);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_FIFO_ERR, "SCALER_INPUT_FIFO_ERR\r\n");
		}
		
		ret = drv_l1_scaler_output_fifo_line_set(scale_dev, C_SCALER_CTRL_OUT_FIFO_DISABLE);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_FIFO_ERR, "SCALER_OUTPUT_FIFO_ERR\r\n");
		}
		break;

	case C_SCALER_CTRL_OUT_FIFO_16LINE:
	case C_SCALER_CTRL_OUT_FIFO_32LINE:
	case C_SCALER_CTRL_OUT_FIFO_64LINE:
		ret = drv_l1_scaler_input_fifo_line_set(scale_dev, C_SCALER_CTRL_IN_FIFO_DISABLE);
		if(ret < 0) {
			RETURN(C_SCALER_INPUT_FIFO_ERR, "SCALER_INPUT_FIFO_ERR\r\n");
		}

		ret = drv_l1_scaler_output_fifo_line_set(scale_dev, pScale->fifo_mode);
		if(ret < 0) {
			RETURN(C_SCALER_OUTPUT_FIFO_ERR, "SCALER_OUTPUT_FIFO_ERR\r\n");
		}
		break;
	}
	
	// set scale other parameters
	ret = drv_l1_scaler_out_of_boundary_color_set(scale_dev, boundary_color);
	if(ret < 0) {
		RETURN(C_SCALER_BND_COR_ERR, "SCALER_BND_COR_ERR\r\n");
	}

	// scaler start
	ret = drv_l1_scaler_status_polling(scale_dev);
	if(ret != C_SCALER_STATUS_STOP) {
		pScale->scale_status = C_SCALER_STATUS_INIT_ERR;
		RETURN(C_SCALER_START_ERR, "SCALE2_STATUS != STOP\r\n");
	}

	DEBUG("ScaleStart\r\n");
	ret = drv_l1_scaler_start(scale_dev, ENABLE);
	if(ret < 0) {
		pScale->scale_status = C_SCALER_STATUS_INIT_ERR;
		RETURN(C_SCALER_START_ERR, "C_SCALER_START_ERR\r\n");
	} else {
		pScale->scale_status = C_SCALER_STATUS_BUSY;
	}
	
	// wait done
	if(wait_done) {
		DEBUG("ScaleWaitDone\r\n");
		ret = drv_l1_scaler_wait_idle(scale_dev);
		if(ret == C_SCALER_STATUS_DONE) {
			pScale->scale_status = C_SCALER_STATUS_STOP; 
			ret = C_SCALER_STATUS_STOP;
			drv_l2_scaler_stop(scale_dev);
		} else if(ret == C_SCALER_STATUS_TIMEOUT) {
			pScale->scale_status = C_SCALER_STATUS_TIMEOUT;
			RETURN(C_SCALER_START_ERR, "C_SCALER_START_ERR\r\n");
		} 
	} 
	
	pScale->scale_status = ret;
__exit:	
	if(ret < 0) {
		DBG_PRINT("ScalerFail[%d]!\r\n", scale_dev);
		DBG_PRINT("InWH[%d, %d]\r\n", pScale->input_width, pScale->input_height);
		DBG_PRINT("InVisWH[%d, %d]\r\n", pScale->input_visible_width, pScale->input_visible_height);
		DBG_PRINT("InXYOffset[%x, %x]\r\n", pScale->input_x_offset, pScale->input_y_offset);
		DBG_PRINT("outWH[%d, %d]\r\n", pScale->output_width, pScale->output_height);
		DBG_PRINT("outBufWH[%d, %d]\r\n", pScale->output_buf_width, pScale->output_buf_height);
		DBG_PRINT("format[0x%x, 0x%x]\r\n", pScale->input_format, pScale->output_format);
		DBG_PRINT("outXOffset = 0x%x\r\n", pScale->output_x_offset);
		DBG_PRINT("InYAddr = 0x%x\r\n", pScale->input_y_addr);
		DBG_PRINT("InUAddr = 0x%x\r\n", pScale->input_u_addr);
		DBG_PRINT("InVAddr = 0x%x\r\n", pScale->input_v_addr);
		DBG_PRINT("OutYAddr = 0x%x\r\n", pScale->output_y_addr);
		DBG_PRINT("OutUAddr = 0x%x\r\n", pScale->output_u_addr);
		DBG_PRINT("OutVAddr = 0x%x\r\n", pScale->output_v_addr);
		DBG_PRINT("ScaleMode = 0x%x\r\n", pScale->scale_mode);	
		drv_l2_scaler_stop(scale_dev);
	}
	
	return ret;
}

/**
 * @brief   scale image retrigger for fifo mode use 
 * @param   scale_dev[in]: scaler device
 * @param   pScale[in]: scale parameter
 * @return 	scaler status
 */
INT32S drv_l2_scaler_retrigger(INT8U scale_dev, ScalerFormat_t *pScale)
{
	INT32S ret;
	
	DEBUG("%s\r\n", __func__);
	if(pScale->fifo_mode == C_SCALER_CTRL_FIFO_DISABLE) {
		return C_SCALER_STATUS_STOP;
	}
	
	if(pScale->scale_status == C_SCALER_STATUS_STOP) {
		return C_SCALER_STATUS_STOP;
	}
	
	if(pScale->scale_status == C_SCALER_STATUS_DONE) {
		pScale->scale_status = C_SCALER_STATUS_STOP;
		ret = C_SCALER_STATUS_STOP;
		drv_l2_scaler_stop(scale_dev);
		goto __exit;
	}

	/* times */
	pScale->reserved0++;

	/* scale restart */
	ret = drv_l1_scaler_restart(scale_dev);
	if(ret < 0) {
		pScale->scale_status = C_SCALER_STATUS_STOP;
		drv_l2_scaler_stop(scale_dev);
		RETURN(C_SCALER_START_ERR, "ScalerRestartFail\r\n");
	} else if(ret == 1) {
		/* already finish */
		DEBUG("ScalerAlreadyDone\r\n");
		pScale->scale_status = C_SCALER_STATUS_STOP;
		ret = C_SCALER_STATUS_STOP;
		drv_l2_scaler_stop(scale_dev);
		goto __exit;
	} 

	/* waiting for done */
	DEBUG("ScalerWaitFifo\r\n");
	ret = drv_l1_scaler_wait_idle(scale_dev);
	if(ret == C_SCALER_STATUS_DONE) {
		pScale->scale_status = C_SCALER_STATUS_STOP;
		ret = C_SCALER_STATUS_STOP;
		drv_l2_scaler_stop(scale_dev);
	} else if(ret == C_SCALER_STATUS_TIMEOUT) {
		RETURN(C_SCALER_STATUS_TIMEOUT, "SCALER_STATUS_TIMEOUT\r\n");
	} else {
		pScale->scale_status = ret;
	}
	
__exit:
	if(ret < 0) {
		DBG_PRINT("ScalerFail[%d]!\r\n", scale_dev);
		DBG_PRINT("InWH[%d, %d]\r\n", pScale->input_width, pScale->input_height);
		DBG_PRINT("InVisWH[%d, %d]\r\n", pScale->input_visible_width, pScale->input_visible_height);
		DBG_PRINT("InXYOffset[%x, %x]\r\n", pScale->input_x_offset, pScale->input_y_offset);
		DBG_PRINT("outWH[%d, %d]\r\n", pScale->output_width, pScale->output_height);
		DBG_PRINT("outBufWH[%d, %d]\r\n", pScale->output_buf_width, pScale->output_buf_height);
		DBG_PRINT("format[0x%x, 0x%x]\r\n", pScale->input_format, pScale->output_format);
		DBG_PRINT("outXOffset = 0x%x\r\n", pScale->output_x_offset);
		DBG_PRINT("InYAddr = 0x%x\r\n", pScale->input_y_addr);
		DBG_PRINT("InUAddr = 0x%x\r\n", pScale->input_u_addr);
		DBG_PRINT("InVAddr = 0x%x\r\n", pScale->input_v_addr);
		DBG_PRINT("OutYAddr = 0x%x\r\n", pScale->output_y_addr);
		DBG_PRINT("OutUAddr = 0x%x\r\n", pScale->output_u_addr);
		DBG_PRINT("OutVAddr = 0x%x\r\n", pScale->output_v_addr);
		DBG_PRINT("ScaleMode = 0x%x\r\n", pScale->scale_mode);	
		drv_l2_scaler_stop(scale_dev);
	}
	
	return ret;
}

#endif //(defined _DRV_L2_DISP) && (_DRV_L2_DISP == 1)