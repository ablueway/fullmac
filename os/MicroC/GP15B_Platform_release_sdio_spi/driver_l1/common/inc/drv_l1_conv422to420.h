#ifndef __DRV_L1_CONV422TO420_H__
#define __DRV_L1_CONV422TO420_H__

#include "driver_l1.h"

#define CONV422_FRAME_END		(1 << 3)
#define CONV422_IRQ_BUF_B		(1 << 2)
#define CONV422_IRQ_BUF_A		(1 << 1)
#define CONV422_CHANGE_FIFO		(1 << 0)

typedef enum
{
    CONV422_FMT_YUYV,                 
    CONV422_FMT_YVYU,
    CONV422_FMT_UYVY,
    CONV422_FMT_VYUY
} CONV422_YUV_FORMAT;

typedef enum
{
    CONV422_FIFO_MODE,                 
    CONV422_FRAME_MODE
} CONV422_MODE;

typedef enum
{
    CONV422_FMT_420,                 
    CONV422_FMT_422
} CONV422_FORMAT;

// Extern APIs
extern void drv_l1_conv422_init(void);
extern void drv_l1_conv422_reset(void);
extern void drv_l1_conv422_input_fmt_set(INT32U format);
extern void drv_l1_conv422_bypass_enable(INT32U enable);
extern void drv_l1_conv422_mode_set(INT32U mode);
extern void drv_l1_conv422_output_format_set(INT32U mode);
extern void drv_l1_conv422_irq_enable(INT32U enable);

extern void drv_l1_conv422_output_A_addr_set(INT32U addr);
extern INT32U drv_l1_conv422_output_A_addr_get(void);
extern void drv_l1_conv422_output_B_addr_set(INT32U addr);
extern INT32U drv_l1_conv422_output_B_addr_get(void);

extern void drv_l1_conv422_input_pixels_set(INT32U inWidth, INT32U inHeight);
extern void drv_l1_conv422_fifo_line_set(INT32U line);
extern void drv_l1_conv422_register_callback(void (*isr_handle)(INT32U event));
#endif		// __DRV_L1_CONV422TO420_H__

