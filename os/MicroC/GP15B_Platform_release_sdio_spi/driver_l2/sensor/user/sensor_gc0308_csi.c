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
#include "driver_l1.h"
#include "drv_l1_sfr.h"
#include "drv_l1_system.h"
#include "drv_l1_csi.h"
#include "drv_l1_timer.h"
#include "drv_l1_i2c.h"
#include "drv_l1_power.h"
#include "drv_l1_interrupt.h"
#include "drv_l2_sensor.h"
#include "drv_l2_sccb.h"

#if (defined _SENSOR_GC0308_CSI) && (_SENSOR_GC0308_CSI == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define GC0308_ID	0x42

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if SCCB_MODE == SCCB_GPIO
	static void *gc0308_handle;
#elif SCCB_MODE == SCCB_HW_I2C
	static drv_l1_i2c_bus_handle_t gc0308_handle; 
#endif


static INT32S gc0308_sccb_open(void)
{
#if SCCB_MODE == SCCB_GPIO
	gc0308_handle = drv_l2_sccb_open(GC0308_ID, 8, 8);
	if(gc0308_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_init();
	gc0308_handle.slaveAddr = GC0308_ID;
	gc0308_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void gc0308_sccb_close(void)
{
#if SCCB_MODE == SCCB_GPIO
	if(gc0308_handle) {
		drv_l2_sccb_close(gc0308_handle);
		gc0308_handle = NULL;
	}	
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_uninit();
	gc0308_handle.slaveAddr = 0;
	gc0308_handle.clkRate = 0;
#endif
}

static INT32S gc0308_sccb_write(INT8U reg, INT8U value)
{
#if SCCB_MODE == SCCB_GPIO
	return drv_l2_sccb_write(gc0308_handle, reg, value);

#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[2];
	
	data[0] = reg;
	data[1] = value;
	return drv_l1_i2c_bus_write(&gc0308_handle, data, 2);
#endif
}

#if 0
static INT32S gc0308_sccb_read(INT8U reg, INT8U *value)
{
#if SCCB_MODE == SCCB_GPIO
	INT16U data;
	
	if(drv_l2_sccb_read(gc0308_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[1];
	
	data[0] = reg;
	if(drv_l1_i2c_bus_write(&gc0308_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	if(drv_l1_i2c_bus_read(&gc0308_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	*value = data[0];
#endif
	return STATUS_OK;
}
#endif

static INT32S gc0308_init(void)
{
	//640x480 init registers code.                                         
	gc0308_sccb_write(0xfe, 0x80);
	
	gc0308_sccb_write(0xfe, 0x00);  // set page0
	
	gc0308_sccb_write(0xd2, 0x10);  // close AEC
	gc0308_sccb_write(0x22, 0x55);  // close AWB
	
	gc0308_sccb_write(0x5a, 0x56);
	gc0308_sccb_write(0x5b, 0x40);
	gc0308_sccb_write(0x5c, 0x4a);			
	
	gc0308_sccb_write(0x22, 0x57); // Open AWB
	
#if 0
	gc0308_sccb_write(0x01, 0xce);
	gc0308_sccb_write(0x02, 0x70);
	gc0308_sccb_write(0x0f, 0x00);
	gc0308_sccb_write(0xe2, 0x00);
	gc0308_sccb_write(0xe3, 0x96);
	gc0308_sccb_write(0xe4, 0x02);
	gc0308_sccb_write(0xe5, 0x58);
	gc0308_sccb_write(0xe6, 0x02);
	gc0308_sccb_write(0xe7, 0x58);
	gc0308_sccb_write(0xe8, 0x02);
	gc0308_sccb_write(0xe9, 0x58);
	gc0308_sccb_write(0xea, 0x0c);
	gc0308_sccb_write(0xeb, 0xbe);
	gc0308_sccb_write(0xec, 0x20);
#else
	gc0308_sccb_write(0x01, 0x6a);
	gc0308_sccb_write(0x02, 0x70);
	gc0308_sccb_write(0x0f, 0x00);
	gc0308_sccb_write(0xe2, 0x00);
	gc0308_sccb_write(0xe3, 0x96);
	gc0308_sccb_write(0xe4, 0x02);
	gc0308_sccb_write(0xe5, 0x58);
	gc0308_sccb_write(0xe6, 0x02);
	gc0308_sccb_write(0xe7, 0x58);
	gc0308_sccb_write(0xe8, 0x02);
	gc0308_sccb_write(0xe9, 0x58);
	gc0308_sccb_write(0xea, 0x04);
	gc0308_sccb_write(0xeb, 0xb0);
	gc0308_sccb_write(0xec, 0x20);
#endif
	
	gc0308_sccb_write(0x05, 0x00);
	gc0308_sccb_write(0x06, 0x00);
	gc0308_sccb_write(0x07, 0x00);
	gc0308_sccb_write(0x08, 0x00);
	gc0308_sccb_write(0x09, 0x01);
	gc0308_sccb_write(0x0a, 0xe8);
	gc0308_sccb_write(0x0b, 0x02);
	gc0308_sccb_write(0x0c, 0x88);
	gc0308_sccb_write(0x0d, 0x02);
	gc0308_sccb_write(0x0e, 0x02);
	gc0308_sccb_write(0x10, 0x22);
	gc0308_sccb_write(0x11, 0xfd);
	gc0308_sccb_write(0x12, 0x0a);
	gc0308_sccb_write(0x13, 0x00);

	gc0308_sccb_write(0x14, 0x13);// change direction  10:normal , 11:H SWITCH,12: V SWITCH, 13:H&V SWITCH

	gc0308_sccb_write(0x15, 0x0a);
	gc0308_sccb_write(0x16, 0x05);
	gc0308_sccb_write(0x17, 0x01);
	gc0308_sccb_write(0x18, 0x44);
	gc0308_sccb_write(0x19, 0x44);
	gc0308_sccb_write(0x1a, 0x1e);
	gc0308_sccb_write(0x1b, 0x00);
	gc0308_sccb_write(0x1c, 0xc1);
	gc0308_sccb_write(0x1d, 0x08);
	gc0308_sccb_write(0x1e, 0x60);
	gc0308_sccb_write(0x1f, 0x16); //pad drv ,00 03 13 1f 3f james remarked

	gc0308_sccb_write(0x20, 0xff);
	gc0308_sccb_write(0x21, 0xf8);
	gc0308_sccb_write(0x22, 0x57);
	gc0308_sccb_write(0x24, 0xa2); 	// YUV
	//gc0308_sccb_write(0x24, 0xa6);  // RGB
	gc0308_sccb_write(0x25, 0x0f);

	//output sync_mode       
	gc0308_sccb_write(0x26, 0x02);//vsync  maybe need changed, value is 0x02
	gc0308_sccb_write(0x2f, 0x01);
	gc0308_sccb_write(0x30, 0xf7);
	gc0308_sccb_write(0x31, 0x50);
	gc0308_sccb_write(0x32, 0x00);
	gc0308_sccb_write(0x39, 0x04);
	gc0308_sccb_write(0x3a, 0x18);
	gc0308_sccb_write(0x3b, 0x20);
	gc0308_sccb_write(0x3c, 0x00);
	gc0308_sccb_write(0x3d, 0x00);
	gc0308_sccb_write(0x3e, 0x00);
	gc0308_sccb_write(0x3f, 0x00);
	gc0308_sccb_write(0x50, 0x10);
	gc0308_sccb_write(0x53, 0x82);
	gc0308_sccb_write(0x54, 0x80);
	gc0308_sccb_write(0x55, 0x80);
	gc0308_sccb_write(0x56, 0x82);

	gc0308_sccb_write(0x57, 0x80);  // R
	gc0308_sccb_write(0x58, 0x80);  // G
	gc0308_sccb_write(0x59, 0x80);  // B

	gc0308_sccb_write(0x8b, 0x40);
	gc0308_sccb_write(0x8c, 0x40);
	gc0308_sccb_write(0x8d, 0x40);
	gc0308_sccb_write(0x8e, 0x2e);
	gc0308_sccb_write(0x8f, 0x2e);
	gc0308_sccb_write(0x90, 0x2e);
	gc0308_sccb_write(0x91, 0x3c);
	gc0308_sccb_write(0x92, 0x50);
	gc0308_sccb_write(0x5d, 0x12);
	gc0308_sccb_write(0x5e, 0x1a);
	gc0308_sccb_write(0x5f, 0x24);
	gc0308_sccb_write(0x60, 0x07);
	gc0308_sccb_write(0x61, 0x15);
	gc0308_sccb_write(0x62, 0x08);
	gc0308_sccb_write(0x64, 0x03);
	gc0308_sccb_write(0x66, 0xe8);
	gc0308_sccb_write(0x67, 0x86);
	gc0308_sccb_write(0x68, 0xa2);
	gc0308_sccb_write(0x69, 0x18);
	gc0308_sccb_write(0x6a, 0x0f);
	gc0308_sccb_write(0x6b, 0x00);
	gc0308_sccb_write(0x6c, 0x5f);
	gc0308_sccb_write(0x6d, 0x8f);
	gc0308_sccb_write(0x6e, 0x55);
	gc0308_sccb_write(0x6f, 0x38);
	gc0308_sccb_write(0x70, 0x15);
	gc0308_sccb_write(0x71, 0x33);
	gc0308_sccb_write(0x72, 0xdc);
	gc0308_sccb_write(0x73, 0x80);
	gc0308_sccb_write(0x74, 0x02);
	gc0308_sccb_write(0x75, 0x3f);
	gc0308_sccb_write(0x76, 0x02);
	gc0308_sccb_write(0x77, 0x36);
	gc0308_sccb_write(0x78, 0x88);
	gc0308_sccb_write(0x79, 0x81);
	gc0308_sccb_write(0x7a, 0x81);
	gc0308_sccb_write(0x7b, 0x22);
	gc0308_sccb_write(0x7c, 0xff);
	gc0308_sccb_write(0x93, 0x48);
	gc0308_sccb_write(0x94, 0x00);
	gc0308_sccb_write(0x95, 0x05);
	gc0308_sccb_write(0x96, 0xe8);
	gc0308_sccb_write(0x97, 0x40);
	gc0308_sccb_write(0x98, 0xf0);
	gc0308_sccb_write(0xb1, 0x38);
	gc0308_sccb_write(0xb2, 0x38);
	gc0308_sccb_write(0xbd, 0x38);
	gc0308_sccb_write(0xbe, 0x36);
	gc0308_sccb_write(0xd0, 0xc9);
	gc0308_sccb_write(0xd1, 0x10);
	//gc0308_sccb_write(0xd2, 0x90);
	//gc0308_sccb_write(0xd3, 0x80);
	gc0308_sccb_write(0xd3, 0xA0);		 // ?תצ
	gc0308_sccb_write(0xd5, 0xf2);
	gc0308_sccb_write(0xd6, 0x16);
	gc0308_sccb_write(0xdb, 0x92);
	gc0308_sccb_write(0xdc, 0xa5);
	gc0308_sccb_write(0xdf, 0x23);
	gc0308_sccb_write(0xd9, 0x00);
	gc0308_sccb_write(0xda, 0x00);
	gc0308_sccb_write(0xe0, 0x09);
	gc0308_sccb_write(0xec, 0x20);
	gc0308_sccb_write(0xed, 0x04);
	gc0308_sccb_write(0xee, 0xa0);
	gc0308_sccb_write(0xef, 0x40);
	gc0308_sccb_write(0x80, 0x03);
	gc0308_sccb_write(0x80, 0x03);
	
#if 1	//smallest gamma curve
	gc0308_sccb_write(0x9F, 0x0B); 
	gc0308_sccb_write(0xA0, 0x16); 
	gc0308_sccb_write(0xA1, 0x29); 
	gc0308_sccb_write(0xA2, 0x3C); 
	gc0308_sccb_write(0xA3, 0x4F); 
	gc0308_sccb_write(0xA4, 0x5F); 
	gc0308_sccb_write(0xA5, 0x6F); 
	gc0308_sccb_write(0xA6, 0x8A); 
	gc0308_sccb_write(0xA7, 0x9F); 
	gc0308_sccb_write(0xA8, 0xB4); 
	gc0308_sccb_write(0xA9, 0xC6); 
	gc0308_sccb_write(0xAA, 0xD3); 
	gc0308_sccb_write(0xAB, 0xDD);  
	gc0308_sccb_write(0xAC, 0xE5);  
	gc0308_sccb_write(0xAD, 0xF1); 
	gc0308_sccb_write(0xAE, 0xFA); 
	gc0308_sccb_write(0xAF, 0xFF); 	
#elif 0		
	gc0308_sccb_write(0x9F, 0x0E); 
	gc0308_sccb_write(0xA0, 0x1C); 
	gc0308_sccb_write(0xA1, 0x34); 
	gc0308_sccb_write(0xA2, 0x48); 
	gc0308_sccb_write(0xA3, 0x5A); 
	gc0308_sccb_write(0xA4, 0x6B); 
	gc0308_sccb_write(0xA5, 0x7B); 
	gc0308_sccb_write(0xA6, 0x95); 
	gc0308_sccb_write(0xA7, 0xAB); 
	gc0308_sccb_write(0xA8, 0xBF);
	gc0308_sccb_write(0xA9, 0xCE); 
	gc0308_sccb_write(0xAA, 0xD9); 
	gc0308_sccb_write(0xAB, 0xE4);  
	gc0308_sccb_write(0xAC, 0xEC); 
	gc0308_sccb_write(0xAD, 0xF7); 
	gc0308_sccb_write(0xAE, 0xFD); 
	gc0308_sccb_write(0xAF, 0xFF); 
#elif 0	
	gc0308_sccb_write(0x9F, 0x10);
	gc0308_sccb_write(0xA0, 0x20);
	gc0308_sccb_write(0xA1, 0x38);
	gc0308_sccb_write(0xA2, 0x4E);
	gc0308_sccb_write(0xA3, 0x63);
	gc0308_sccb_write(0xA4, 0x76);
	gc0308_sccb_write(0xA5, 0x87);
	gc0308_sccb_write(0xA6, 0xA2);
	gc0308_sccb_write(0xA7, 0xB8);
	gc0308_sccb_write(0xA8, 0xCA);
	gc0308_sccb_write(0xA9, 0xD8);
	gc0308_sccb_write(0xAA, 0xE3);
	gc0308_sccb_write(0xAB, 0xEB);
	gc0308_sccb_write(0xAC, 0xF0);
	gc0308_sccb_write(0xAD, 0xF8);
	gc0308_sccb_write(0xAE, 0xFD);
	gc0308_sccb_write(0xAF, 0xFF);
#elif 0		
	gc0308_sccb_write(0x9F, 0x14); 
	gc0308_sccb_write(0xA0, 0x28); 
	gc0308_sccb_write(0xA1, 0x44); 
	gc0308_sccb_write(0xA2, 0x5D); 
	gc0308_sccb_write(0xA3, 0x72); 
	gc0308_sccb_write(0xA4, 0x86); 
	gc0308_sccb_write(0xA5, 0x95); 
	gc0308_sccb_write(0xA6, 0xB1); 
	gc0308_sccb_write(0xA7, 0xC6); 
	gc0308_sccb_write(0xA8, 0xD5); 
	gc0308_sccb_write(0xA9, 0xE1); 
	gc0308_sccb_write(0xAA, 0xEA); 
	gc0308_sccb_write(0xAB, 0xF1); 
	gc0308_sccb_write(0xAC, 0xF5); 
	gc0308_sccb_write(0xAD, 0xFB); 
	gc0308_sccb_write(0xAE, 0xFE); 
	gc0308_sccb_write(0xAF, 0xFF);
#else	// largest gamma curve
	gc0308_sccb_write(0x9F, 0x15); 
	gc0308_sccb_write(0xA0, 0x2A); 
	gc0308_sccb_write(0xA1, 0x4A); 
	gc0308_sccb_write(0xA2, 0x67); 
	gc0308_sccb_write(0xA3, 0x79); 
	gc0308_sccb_write(0xA4, 0x8C); 
	gc0308_sccb_write(0xA5, 0x9A); 
	gc0308_sccb_write(0xA6, 0xB3); 
	gc0308_sccb_write(0xA7, 0xC5); 
	gc0308_sccb_write(0xA8, 0xD5); 
	gc0308_sccb_write(0xA9, 0xDF); 
	gc0308_sccb_write(0xAA, 0xE8); 
	gc0308_sccb_write(0xAB, 0xEE); 
	gc0308_sccb_write(0xAC, 0xF3); 
	gc0308_sccb_write(0xAD, 0xFA); 
	gc0308_sccb_write(0xAE, 0xFD); 
	gc0308_sccb_write(0xAF, 0xFF);
#endif	
	
	gc0308_sccb_write(0xc0, 0x00);
	gc0308_sccb_write(0xc1, 0x10);
	gc0308_sccb_write(0xc2, 0x1C);
	gc0308_sccb_write(0xc3, 0x30);
	gc0308_sccb_write(0xc4, 0x43);
	gc0308_sccb_write(0xc5, 0x54);
	gc0308_sccb_write(0xc6, 0x65);
	gc0308_sccb_write(0xc7, 0x75);
	gc0308_sccb_write(0xc8, 0x93);
	gc0308_sccb_write(0xc9, 0xB0);
	gc0308_sccb_write(0xca, 0xCB);
	gc0308_sccb_write(0xcb, 0xE6);
	gc0308_sccb_write(0xcc, 0xFF);
	gc0308_sccb_write(0xf0, 0x02);
	gc0308_sccb_write(0xf1, 0x01);
	gc0308_sccb_write(0xf2, 0x01);
	gc0308_sccb_write(0xf3, 0x30);
	gc0308_sccb_write(0xf9, 0x9f);
	gc0308_sccb_write(0xfa, 0x78);

	//---------------------------------------------------------------
	gc0308_sccb_write(0xfe, 0x01);  //set page 1

	gc0308_sccb_write(0x00, 0xf5);
	gc0308_sccb_write(0x02, 0x1a);
	gc0308_sccb_write(0x0a, 0xa0);
	gc0308_sccb_write(0x0b, 0x60);
	gc0308_sccb_write(0x0c, 0x08);
	gc0308_sccb_write(0x0e, 0x4c);
	gc0308_sccb_write(0x0f, 0x39);
	
	gc0308_sccb_write(0x11, 0x3f);
	//gc0308_sccb_write(0x11 ,0x37);  // bit3 = 0

	gc0308_sccb_write(0x12, 0x72);
	gc0308_sccb_write(0x13, 0x13);
	gc0308_sccb_write(0x14, 0x42);
	gc0308_sccb_write(0x15, 0x43);
	gc0308_sccb_write(0x16, 0xc2);
	gc0308_sccb_write(0x17, 0xa8);
	gc0308_sccb_write(0x18, 0x18);
	gc0308_sccb_write(0x19, 0x40);
	gc0308_sccb_write(0x1a, 0xd0);
	gc0308_sccb_write(0x1b, 0xf5);
	gc0308_sccb_write(0x70, 0x40);
	gc0308_sccb_write(0x71, 0x58);
	gc0308_sccb_write(0x72, 0x30);
	gc0308_sccb_write(0x73, 0x48);
	gc0308_sccb_write(0x74, 0x20);
	gc0308_sccb_write(0x75, 0x60);
	gc0308_sccb_write(0x77, 0x20);
	gc0308_sccb_write(0x78, 0x32);
	gc0308_sccb_write(0x30, 0x03);
	gc0308_sccb_write(0x31, 0x40);
	gc0308_sccb_write(0x32, 0xe0);
	gc0308_sccb_write(0x33, 0xe0);
	gc0308_sccb_write(0x34, 0xe0);
	gc0308_sccb_write(0x35, 0xb0);
	gc0308_sccb_write(0x36, 0xc0);
	gc0308_sccb_write(0x37, 0xc0);
	gc0308_sccb_write(0x38, 0x04);
	gc0308_sccb_write(0x39, 0x09);
	gc0308_sccb_write(0x3a, 0x12);
	gc0308_sccb_write(0x3b, 0x1C);
	gc0308_sccb_write(0x3c, 0x28);
	gc0308_sccb_write(0x3d, 0x31);
	gc0308_sccb_write(0x3e, 0x44);
	gc0308_sccb_write(0x3f, 0x57);
	gc0308_sccb_write(0x40, 0x6C);
	gc0308_sccb_write(0x41, 0x81);
	gc0308_sccb_write(0x42, 0x94);
	gc0308_sccb_write(0x43, 0xA7);
	gc0308_sccb_write(0x44, 0xB8);
	gc0308_sccb_write(0x45, 0xD6);
	gc0308_sccb_write(0x46, 0xEE);
	gc0308_sccb_write(0x47, 0x0d);

	gc0308_sccb_write(0xfe, 0x00);  

	gc0308_sccb_write(0xd2, 0x90); // Open AEC at last.  

	/////////////////////////////////////////////////////////
	//-----------Update the registers -------------//
	///////////////////////////////////////////////////////////

	gc0308_sccb_write(0xfe, 0x00);//set Page0

	gc0308_sccb_write(0x10, 0x26);                               
	gc0308_sccb_write(0x11, 0x0d);// fd,modified by mormo 2010/07/06                               
	gc0308_sccb_write(0x1a, 0x2a);// 1e,modified by mormo 2010/07/06                                  

	gc0308_sccb_write(0x1c, 0x49); // c1,modified by mormo 2010/07/06                                 
	gc0308_sccb_write(0x1d, 0x9a); // 08,modified by mormo 2010/07/06                                 
	gc0308_sccb_write(0x1e, 0x61); // 60,modified by mormo 2010/07/06                                 
	//gc0308_sccb_write(0x1f, 0x16); // io driver current 
	
	gc0308_sccb_write(0x3a, 0x20);

	gc0308_sccb_write(0x50, 0x14);// 10,modified by mormo 2010/07/06                               
	gc0308_sccb_write(0x53, 0x80);                                
	gc0308_sccb_write(0x56, 0x80);

	gc0308_sccb_write(0x8b, 0x20); //LSC                                 
	gc0308_sccb_write(0x8c, 0x20);                                
	gc0308_sccb_write(0x8d, 0x20);                                
	gc0308_sccb_write(0x8e, 0x14);                                
	gc0308_sccb_write(0x8f, 0x10);                                
	gc0308_sccb_write(0x90, 0x14);                                

	gc0308_sccb_write(0x94, 0x02);                                
	gc0308_sccb_write(0x95, 0x07);                                
	gc0308_sccb_write(0x96, 0xe0);                                

	gc0308_sccb_write(0xb1, 0x40); // YCPT                                 
	gc0308_sccb_write(0xb2, 0x40);                                
	gc0308_sccb_write(0xb3, 0x40);
	gc0308_sccb_write(0xb6, 0xe0);

	gc0308_sccb_write(0xd0, 0xcb); // AECT  c9,modifed by mormo 2010/07/06                                
	gc0308_sccb_write(0xd3, 0x48); // 80,modified by mormor 2010/07/06                           

	gc0308_sccb_write(0xf2, 0x02);                                
	gc0308_sccb_write(0xf7, 0x12);
	gc0308_sccb_write(0xf8, 0x0a);

	gc0308_sccb_write(0xfe, 0x01);//set  Page1

	gc0308_sccb_write(0x02, 0x20);
	gc0308_sccb_write(0x04, 0x10);
	gc0308_sccb_write(0x05, 0x08);
	gc0308_sccb_write(0x06, 0x20);
	gc0308_sccb_write(0x08, 0x0a);

	gc0308_sccb_write(0x0e, 0x44);                                
	gc0308_sccb_write(0x0f, 0x32);
	gc0308_sccb_write(0x10, 0x41);                                
	gc0308_sccb_write(0x11, 0x37);                                
	gc0308_sccb_write(0x12, 0x22);                                
	gc0308_sccb_write(0x13, 0x19);                                
	gc0308_sccb_write(0x14, 0x44);                                
	gc0308_sccb_write(0x15, 0x44);

	gc0308_sccb_write(0x19, 0x50);                                
	gc0308_sccb_write(0x1a, 0xd8); 

	gc0308_sccb_write(0x32, 0x10); 

	gc0308_sccb_write(0x35, 0x00);                                
	gc0308_sccb_write(0x36, 0x80);                                
	gc0308_sccb_write(0x37, 0x00); 
	
	//-----------Update the registers end---------//
	return gc0308_sccb_write(0xfe, 0x00);// set back for page1
}

/**
 * @brief   gc0308 initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void gc0308_csi_init(void)
{
	/* Turn on LDO 2.8V for CSI sensor */
	drv_l1_power_ldo_28_ctrl(1, LDO_LDO28_2P8V);
	
	/* set horizontal/vertical scale ratio to 0 */
	R_CSI_TG_HRATIO = 0;
	R_CSI_TG_VRATIO = 0;
	
	/* Sensor field 0 vertical latch start register */
	R_CSI_TG_VL0START = 0x0000;
	/* *P_Sensor_TG_V_L1Start = 0x0000 */
	R_CSI_TG_VL1START = 0x0000;
	/* Sensor horizontal start register */
	R_CSI_TG_HSTART = 0x0000;

	/* reset control 0/1 */
	R_CSI_TG_CTRL0 = 0;
	R_CSI_TG_CTRL1 = 0;

	/* register CSI interrupt */
	vic_irq_register(VIC_CSI, drv_l1_csi_isr);

	/* mclk output */
	drv_l2_sensor_set_mclkout(gc0308_sensor_csi_ops.info[0].mclk);
	
	// request sccb
	gc0308_sccb_open();	
	DBG_PRINT("Sensor GC0308 csi interface init completed\r\n");
}	

/**
 * @brief   gc0308 un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void gc0308_csi_uninit(void)
{
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE);
	
	// csi disable
	R_CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CSIEN;	//disable sensor controller
	R_CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOEN;	//disable sensor clock out
	
	// release sccb
	gc0308_sccb_close();

	// Turn off LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(0, LDO_LDO28_2P8V);
}	

/**
 * @brief   gc0308 stream start function
 * @param   info index
 *			
 * @return 	none
 */
void gc0308_csi_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	/* enable CSI output clock for SCCB */
	drv_l2_sensor_set_mclkout(gc0308_sensor_csi_ops.info[index].mclk);
	
	/* set start frame buffer address */	
	if(bufA) {
		drv_l1_csi_set_buf(bufA);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", gc0308_sensor_csi_ops.name);
		gc0308_sensor_csi_ops.stream_stop();
	}	

	/* Set Horizontal/Vertical counter reset, Vertical counter incresase selection, sensor owner inerrupt enable, capture/preview mode */
	R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HRST | MASK_CSI_CTRL0_VADD | MASK_CSI_CTRL0_VRST | MASK_CSI_CTRL0_OWN_IRQ | MASK_CSI_CTRL0_CAP;
		
	/* Set delay clock 244[0:3] */
	R_CSI_TG_CTRL1 |= DELAY_1CLOCK;
	
	R_CSI_TG_CTRL1 |= MASK_CSI_CTRL1_RGB1555;

	/* According to the specific information, set CSI register */
	/* set input interface */
	if(gc0308_sensor_csi_ops.info[index].interface_mode == MODE_CCIR_HREF) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HREF;
	}
	
	/* set input & output format */
	if(gc0308_sensor_csi_ops.info[index].input_format == V4L2_PIX_FMT_VYUY) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVIN | MASK_CSI_CTRL0_YUVTYPE;
	}	

	if(gc0308_sensor_csi_ops.info[index].output_format == V4L2_PIX_FMT_VYUY) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVOUT;
	}	
	
	/* Set sensor's registers via SCCB */
	if(gc0308_init() < 0) {
		DBG_PRINT("gc0308 init fail!!!\r\n");
	}

	DBG_PRINT("%s = %d\r\n", __func__, index);
	switch(index)
	{
	case 0:
		R_CSI_TG_HWIDTH = gc0308_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = gc0308_sensor_csi_ops.info[index].target_h;	
		R_CSI_TG_HRATIO = 0;		// No Scale
		R_CSI_TG_VRATIO = 0;		// No Scale	
		break;
		
	case 1:	
		R_CSI_TG_HWIDTH = gc0308_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = gc0308_sensor_csi_ops.info[index].target_h * 2;
		R_CSI_TG_HRATIO = 0x0102;	// Scale to 1/2
		R_CSI_TG_VRATIO = 0x0102;	// Scale to 1/2
		break;
	
	case 2:			
		R_CSI_TG_HWIDTH = gc0308_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = gc0308_sensor_csi_ops.info[index].target_h * 2;
		R_CSI_TG_HRATIO = 0x0104;	// Scale to 1/4
		R_CSI_TG_VRATIO = 0x0104;	// Scale to 1/4
		break;
		
	default:
		while(1);	
	}

	/* csi long burst width need 32 align */
	if(gc0308_sensor_csi_ops.info[index].target_w & 0x1F) {
		R_CSI_SEN_CTRL &= ~(1 << 7);
	} else {
		R_CSI_SEN_CTRL |= 1 << 7;
	}
	
	/* Enable interrupt */
	vic_irq_enable(VIC_CSI);
	
	/* Clear sensor interrupt status */
	R_TGR_IRQ_STATUS = 0x3F;
	
	/* enable frame end interrupt */
	R_TGR_IRQ_EN |= MASK_CSI_FRAME_END_FLAG | MASK_CSI_FIFO_OVERFLOW_FLAG;
	
	/* enable CSI module */
	R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CSIEN;
}

/**
 * @brief   gc0308 stream stop function
 * @param   none
 * @return 	none
 */ 
void gc0308_csi_stream_stop(void)
{
	/* Disable interrupt */
	vic_irq_disable(VIC_CSI);
	
	/* Disable sensor interrupt */
	R_TGR_IRQ_EN &= ~MASK_CSI_FRAME_END_ENABLE;

	/* Disable CSI module */
	R_CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CSIEN;

	/* Disable sensor output clock*/
	R_CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOEN;	

	/* Clear sensor interrupt status */
	R_TGR_IRQ_STATUS = 0x3F;
}	

/**
 * @brief   gc0308 get info function
 * @param   none
 * @return 	pointer to sensor information data
 */ 
drv_l2_sensor_info_t* gc0308_csi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
		return NULL;
	else
		return (drv_l2_sensor_info_t*)&gc0308_sensor_csi_ops.info[index];
}
	
/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t gc0308_sensor_csi_ops =
{
	SENSOR_GC0308_CSI_NAME,				/* sensor name */
	gc0308_csi_init,
	gc0308_csi_uninit,
	gc0308_csi_stream_start,
	gc0308_csi_stream_stop,
	gc0308_csi_get_info,
	{
		/* 1rd info */	
		{
			MCLK_24M,					/* CSI clock */
			V4L2_PIX_FMT_VYUY,			/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			CSI_SENSOR_FPS_30,			/* FPS in sensor */
			WIDTH_640,					/* target width */
			HEIGHT_480, 				/* target height */
			WIDTH_640,					/* sensor width */
			HEIGHT_480, 				/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 2nd info */	
		{
			MCLK_24M,					/* CSI clock */
			V4L2_PIX_FMT_VYUY,			/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			CSI_SENSOR_FPS_30,			/* FPS in sensor */
			WIDTH_320,					/* target width */
			HEIGHT_240, 				/* target height */
			WIDTH_640,					/* sensor width */
			HEIGHT_480, 				/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 3st info */
		{
			MCLK_24M,					/* CSI clock */
			V4L2_PIX_FMT_VYUY,			/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			CSI_SENSOR_FPS_30,			/* FPS in sensor */
			160,						/* target width */
			120, 						/* target height */
			WIDTH_640,					/* sensor width */
			HEIGHT_480, 				/* sensor height */
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_HREF,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		}
	}
};
#endif //(defined _SENSOR_GC03080_CSI) && (_SENSOR_GC03080_CSI == 1)