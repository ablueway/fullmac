#ifndef __drv_l2_SCALER_H__
#define __drv_l2_SCALER_H__

#include "driver_l2.h"
#include "define.h"

// scaler mode
#define C_SCALER_FULL_SCREEN				1
#define C_SCALER_BY_RATIO					2
#define C_SCALER_FULL_SCREEN_BY_RATIO		3
#define C_SCALER_FULL_SCREEN_BY_DIGI_ZOOM	4
#define C_SCALER_RATIO_USER					5

// scale error 
#define C_SCALER_START_ERR					(-1)
#define C_SCALER_INPUT_SIZE_ERR				(-2)
#define C_SCALER_INPUT_OFFSET_ERR			(-3)
#define C_SCALER_INPUT_BUF_ERR				(-4)
#define C_SCALER_INPUT_FMT_ERR				(-5)
#define C_SCALER_INPUT_FIFO_ERR				(-6)
#define C_SCALER_OUTPUT_SIZE_ERR			(-7)
#define C_SCALER_OUTPUT_OFFSET_ERR			(-8)
#define C_SCALER_OUTPUT_BUF_ERR				(-9)
#define C_SCALER_OUTPUT_FMT_ERR				(-10)
#define C_SCALER_OUTPUT_FIFO_ERR			(-11)
#define C_SCALER_EXT_BUF_ERR				(-12)
#define C_SCALER_BND_COR_ERR				(-13)

typedef struct ScalerFormat_s
{
	/* img input para */
	INT32U input_format;			/* input format*/
	INT16U input_width;				/* 1~0x1FFF, input image x size */
	INT16U input_height;			/* 1~0x1FFF, input image y size */
	INT16U input_visible_width;		/* 0~0x1FFF, 0 is disable, clip x size*/
	INT16U input_visible_height;	/* 0~0x1FFF, 0 is disable, clip y size */
	INT32U input_x_offset;			/* 0~0x1FFF, x start offset in effective area */
	INT32U input_y_offset;			/* 0~0x1FFF, y start offset in effective area */
	
	INT32U input_y_addr;			/* input y addr, must be 4-align */
	INT32U input_u_addr;			/* input u addr, must be 4-align */
	INT32U input_v_addr;			/* input v addr, must be 4-align */
	
	INT32U output_format; 			/* output format*/
	INT16U output_width; 			/* 1~0x1FFF, must be 8-align, but YUV444/YUV422/YUV420 is 16-align, YUV411 is 32-align */
	INT16U output_height;			/* 1~0x1FFF */
	INT16U output_buf_width;		/* 1~0x1FFF, must be 8-align, but YUV444/YUV422/YUV420 is 16-align, YUV411 is 32-align */
	INT16U output_buf_height;		/* 1~0x1FFF */
	INT16U output_x_offset;			/* 0~0x1FFF, must be 8-align, skip x size after every line output */
	INT16U reserved0;
	
	INT32U output_y_addr;			/* output y addr, must be 4-align */
	INT32U output_u_addr;			/* output u addr, must be 4-align */
	INT32U output_v_addr;			/* output v addr, must be 4-align */
	
	/* scale para */
	INT32U fifo_mode;				/* FIFO in or FIFO out mode */
	INT8U  scale_mode;				/* C_SCALER_FULL_SCREEN / C_SCALER_FIT_RATIO.... */
	INT8U  digizoom_m;				/* digital zoom, ratio =  m/n */
	INT8U  digizoom_n;	
	INT8U  scale_status;			/* scale status */
} ScalerFormat_t;

extern void drv_l2_scaler_init(void);
extern INT32S drv_l2_scaler_clip(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst, gpRect *clip);
extern INT32S drv_l2_scaler_once(INT8U scale_dev, INT8U wait_done, gpImage *src, gpImage *dst);
extern INT32S drv_l2_scaler_wait_done(INT8U scale_dev, ScalerFormat_t *pScale);
extern void drv_l2_scaler_stop(INT8U scale_dev);
extern INT32S drv_l2_scaler_trigger(INT8U scale_dev, INT8U wait_done, ScalerFormat_t *pScale, INT32U boundary_color);
extern INT32S drv_l2_scaler_retrigger(INT8U scale_dev, ScalerFormat_t *pScale);
#endif