#ifndef __AD_KEY_DRIVER_H__
#define __AD_KEY_DRIVER_H__

#include "driver_l2.h"

typedef enum {
	ADKEY_IO1 = 1,
	ADKEY_IO2,
	ADKEY_IO3,
	ADKEY_IO4,
	ADKEY_IO5,
	ADKEY_IO6,
	ADKEY_IO7,
	ADKEY_IO8,
	ADKEY_IO1_UP,
	ADKEY_IO2_UP,
	ADKEY_IO3_UP,
	ADKEY_IO4_UP,
	ADKEY_IO5_UP,
	ADKEY_IO6_UP,
	ADKEY_IO7_UP,
	ADKEY_IO8_UP,
	ADKEY_IO1_C,
	ADKEY_IO2_C,
	ADKEY_IO3_C,
	ADKEY_IO4_C,
	ADKEY_IO5_C,
	ADKEY_IO6_C,
	ADKEY_IO7_C,
	ADKEY_IO8_C,
	ADKEY_MAX
} KEY_NUMBER_ENUM;

extern INT32S drv_l2_ad_key_init(void);
extern void drv_l2_ad_key_uninit(void);
extern INT32S drv_l2_ad_key_scan(void);
#endif