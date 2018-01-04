#ifndef DRV_L2_SCCB_H
#define DRV_L2_SCCB_H
#include "driver_l2.h"

extern void *drv_l2_sccb_open(INT8U ID, INT8U RegBits, INT8U DataBits);
extern void drv_l2_sccb_close(void *handle);
extern INT32S drv_l2_sccb_write(void *handle, INT16U addr, INT16U data);
extern INT32S drv_l2_sccb_read(void *handle, INT16U addr, INT16U *data);

#endif  //DRV_L2_SCCB_H
