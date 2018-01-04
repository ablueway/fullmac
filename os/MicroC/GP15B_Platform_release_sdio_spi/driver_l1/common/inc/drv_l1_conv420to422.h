#ifndef __DRV_L1_CONV420TO422_H__
#define __DRV_L1_CONV420TO422_H__

#include "driver_l1.h"

#define CONV420_IRQ_FRAME_END	(1 << 2)
#define CONV420_IRQ_FIFO_B		(1 << 1)
#define CONV420_IRQ_FIFO_A		(1 << 0)

typedef enum
{
	CONV420_TO_TV_HDMI,
	CONV420_TO_SCALER0
} CONV420_PATH;

typedef enum
{
    CONV420_IDLE_BUF_A,                 
    CONV420_IDLE_BUF_B,
    CONV420_IDLE_BUF_NON
} CONV420_IDLE_BUF;

typedef enum
{
	CONV420_FMT_YUYV,
	CONV420_FMT_YVYU,
	CONV420_FMT_UYVY,
	CONV420_FMT_VYUY
} CONV420_FORMAT;

// Conv420To422 Extern APIs
extern void drv_l1_conv420_init(void);
extern void drv_l1_conv420_uninit(void);
extern void drv_l1_conv420_convert_enable(INT32U enable);
extern void drv_l1_conv420_path(INT32U path);
extern void drv_l1_conv420_output_fmt_set(INT32U fmt);
extern void drv_l1_conv420_start(void);
extern void drv_l1_conv420_reset(void);
extern void drv_l1_conv420_fifo_irq_enable(INT32U enable);

extern void drv_l1_conv420_input_A_addr_set(INT32U addr);
extern INT32U drv_l1_conv420_input_A_addr_get(void);
extern void drv_l1_conv420_input_B_addr_set(INT32U addr);
extern INT32U drv_l1_conv420_input_B_addr_get(void);

extern INT32S drv_l1_conv420_input_pixels_set(INT32U inWidth);
extern INT32S drv_l1_conv420_fifo_line_set(INT32U lineCount);
extern void drv_l1_register_callback(void (*isr_handle)(INT32U event));
#endif		// __DRV_L1_CONV420TO422_H__
