#ifndef __drv_l2_CDSP_CONFIG_H__
#define __drv_l2_CDSP_CONFIG_H__

#include "driver_l2.h"
#include "drv_l2_cdsp.h"

extern INT32S drv_l2_cdsp_set_badpixel_ob(INT32S obval);
extern INT32S drv_l2_cdsp_set_wb_gain(void);
extern INT32S drv_l2_cdsp_set_ae_target(INT32S ae_target, INT32S ae_target_night);
extern INT32S drv_l2_cdsp_set_aeawb_window(INT16U width, INT16U height);
extern INT32S drv_l2_cdsp_set_gamma(void);
extern INT32S drv_l2_cdsp_set_wb_offset(INT32S wb_offset_day, INT32S wb_offset_night);
extern INT32S drv_l2_cdsp_set_intp(INT32U hi_thr, INT32U low_thr);
extern INT32S drv_l2_cdsp_set_colormatrix(void);
extern INT32S drv_l2_cdsp_set_histgm(INT32U hi_thr, INT32U low_thr);
extern INT32S drv_l2_cdsp_set_suppression(void);
extern INT32S drv_l2_cdsp_set_edge(INT32S sharpness);
extern INT32S drv_l2_cdsp_set_new_denoise(INT32S sharpness);
extern INT32S drv_l2_cdsp_set_saturation_day(INT32S y_offset, INT32S u_offset, INT32S v_offset, INT32S y_scale, INT32S u_scale, INT32S v_scale);
extern INT32S drv_l2_cdsp_set_saturation_night(INT32S y_offset, INT32S u_offset, INT32S v_offset, INT32S y_scale, INT32S u_scale, INT32S v_scale);
extern INT32S drv_l2_cdsp_set_backlight_dt(INT32U en);
extern INT32S drv_l2_cdsp_set_awb_mode(INT32U mode);
extern INT32S drv_l2_cdsp_set_ev_val(INT32S ev);
extern INT32S drv_l2_cdsp_raw_special_mode(INT32U mode);
extern INT32S drv_l2_cdsp_set_iso(INT32U iso);
extern INT32S drv_l2_cdsp_set_yuv_scale(INT16U dst_w, INT16U dst_h);

extern void drv_l2_cdsp_enable(INT16U target_w, INT16U target_h, gpCdspFav_t fav);
#endif	/*__drv_l2_CDSP_H__*/
