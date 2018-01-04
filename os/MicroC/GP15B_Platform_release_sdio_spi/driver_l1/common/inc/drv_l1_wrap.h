#ifndef __drv_l1_WRAP_H__
#define __drv_l1_WRAP_H__

#include "driver_l1.h"

// arbiter dummy address
#define DUMMY_BUFFER_ADDRS	0xF8500000

typedef enum
{
    WRAP_SCA2TFT,                 
    WRAP_CSIMUX,                 
    WRAP_CSI2SCA,
    WRAP_MAX
} WRAP_NUM;

// Wrap extern APIs
extern void drv_l1_wrap_filter_enable(INT8U wrap_num, INT8U enable);

extern void drv_l1_wrap_path_set(INT8U wrap_num, INT8U pathSrEnable, INT8U pathOEnable);
extern void drv_l1_wrap_addr_set(INT8U wrap_num, INT32U addr);
extern void drv_l1_wrap_filter_addr_set(INT8U wrap_num, INT32U addr, INT32U size);
extern void drv_l1_wrap_filter_flush(INT8U wrap_num);
extern INT32S drv_l1_wrap_check_busy(INT8U wrap_num, INT32U wait_idle);

extern void drv_l1_wrap_protect_enable(INT8U wrap_num, INT8U enable);
extern void drv_l1_wrap_protect_pixels_set(INT8U wrap_num, INT32U inWidth, INT32U inHeight);
extern void drv_l1_wrap_clip_mode_enable(INT8U wrap_num, INT8U enable);
extern void drv_l1_wrap_clip_source_pixels_set(INT8U wrap_num, INT32U inWidth, INT32U inHeight);
extern void drv_l1_wrap_clip_start_pixels_set(INT8U wrap_num, INT32U inStartX, INT32U inStartY);
extern INT32S wrap_protect_status_get(INT8U wrap_num);
#endif		// __drv_l1_WRAP_H__


