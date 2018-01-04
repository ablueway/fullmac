/******************************************************
* sensor_ov7670_csi.c
*
* Purpose: OV7670 sensor driver for CSI path
*
* Author: Eugene Hsu
*
* Date: 2014/08/25
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
*******************************************************/

/*******************************************************
    Include file
*******************************************************/
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

#if (defined _SENSOR_OV7670_CSI) && (_SENSOR_OV7670_CSI == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define OV7670_ID		0x42
 
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct regval8_s 
{
	INT8U reg_num;
	INT8U value;
} regval8_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if SCCB_MODE == SCCB_GPIO
	static void *ov7670_handle;
#elif SCCB_MODE == SCCB_HW_I2C
	static drv_l1_i2c_bus_handle_t ov7670_handle; 
#endif

/************************
 MCLK= 24Mhz;        
 PCLK=   Mhz         
 Frame width= 640
 Frame Height= 480  
*************************/
/* 300K YUV 15fps */
static const regval8_t OV7670_YUV_CONFIG[] = 
{
		{0x11, 0x02},	 // 30fps
        
		//{0x11, 0x04},    // 27fps	
		//{0x11, 0x06},    // 14.5fps
		//{0x11, 0x09},    // 13.5fps
		//{0x11, 0x08},    // 15fps		
		//{0x11, 0x06},    // 10fps
		//{0x11, 0x09},    // 6.7fps
			
		{0x3a, 0x04},
		{0x12, 0x00},
		{0x17, 0x13},
		{0x18, 0x01},
		{0x32, 0xb6},
		{0x19, 0x02},
		{0x1a, 0x7a},
		{0x03, 0x0a},
		{0x0c, 0x00},
		{0x3e, 0x00},
		{0x70, 0x3a},
		{0x71, 0x35},
		{0x72, 0x11},
		{0x73, 0xf0},
		{0xa2, 0x02},

		{0x7a, 0x24},
		{0x7b, 0x04},
		{0x7c, 0x0a},
		{0x7d, 0x17},
		{0x7e, 0x32},
		{0x7f, 0x3f},
		{0x80, 0x4c},
		{0x81, 0x58},
		{0x82, 0x64},
		{0x83, 0x6f},
		{0x84, 0x7a},
		{0x85, 0x8c},
		{0x86, 0x9e},
		{0x87, 0xbb},
		{0x88, 0xd2},
		{0x89, 0xe5},

		{0x13, 0xe0},
		{0x00, 0x00},
		{0x10, 0x00},
		{0x0d, 0x40},
		{0x14, 0x18},
		{0xa5, 0x02},
		{0xab, 0x03},
		{0x24, 0x95},
		{0x25, 0x33},
		{0x26, 0xe3},
		{0x9f, 0x78},
		{0xa0, 0x68},
		{0xa1, 0x03},
		{0xa6, 0xd8},
		{0xa7, 0xd8},
		{0xa8, 0xf0},
		{0xa9, 0x90},
		{0xaa, 0x94},
		{0x13, 0xe5},
                         
		{0x0e, 0x61},
		{0x0f, 0x4b},
		{0x16, 0x02},
		{0x1e, 0x3f}, 	//Flip & Mirror
		{0x21, 0x02},
		{0x22, 0x91},
		{0x29, 0x07},
		{0x33, 0x0b},
		{0x35, 0x0b},
		{0x37, 0x1d},
		{0x38, 0x71},
		{0x39, 0x2a},
		{0x3c, 0x78},
		{0x4d, 0x40},
		{0x4e, 0x20},
		{0x69, 0x00},
		
		/* 30FPS */
		{0x6b, 0x8a},	// pclk*6	
		{0x2d, 0x40},	// dummy byte
		{0x2e, 0x00},
		
		/* 27 FPS */	
		//{0x6b, 0xca},	// pclk*8
		
		/* 15 FPS */	
		//{0x6b, 0xca},	// pclk*8
		
		/* 10 FPS */
		//{0x6b, 0x4a},	// pclk*4
		
		/* 7 FPS */
		//{0x6b, 0x4a},	// pclk*4
		
		{0x74, 0x10},
		{0x8d, 0x4f},
		{0x8e, 0x00},
		{0x8f, 0x00},
		{0x90, 0x00},
		{0x91, 0x00},
                                 
		{0x96, 0x00},
		{0x9a, 0x80},
		{0xb0, 0x84},
		{0xb1, 0x0c},
		{0xb2, 0x0e},
		{0xb3, 0x82},
		{0xb8, 0x0a},
                         
		{0x43, 0x0a},
		{0x44, 0xf0},
		{0x45, 0x44},
		{0x46, 0x7a},
		{0x47, 0x27},
		{0x48, 0x3c},
		{0x59, 0xbc},
		{0x5a, 0xde},
		{0x5b, 0x54},
		{0x5c, 0x8a},
		{0x5d, 0x4b},
		{0x5e, 0x0f},
		{0x6c, 0x0a},
		{0x6d, 0x55},
		{0x6e, 0x11},
		{0x6f, 0x9e},
                         
		{0x6a, 0x40},
		{0x01, 0x40},
		{0x02, 0x40},
		{0x13, 0xe7},

		{0x4f, 0x80},
		{0x50, 0x80},
		{0x51, 0x00},
		{0x52, 0x22},
		{0x53, 0x5e},
		{0x54, 0x80},
		{0x58, 0x9e},
                         
		{0x62, 0x08},
		{0x63, 0x8f},
		{0x65, 0x00},
		{0x64, 0x08},
		{0x94, 0x08},
		{0x95, 0x0c},
		{0x66, 0x05},
                         
		{0x41, 0x08},
		{0x3f, 0x00},
		{0x75, 0x05},
		{0x76, 0xe1},
		{0x4c, 0x00},
		{0x77, 0x01},
		{0x3d, 0xc2},
		{0x4b, 0x09},
		{0xc9, 0x60},
		{0x41, 0x38},
		{0x56, 0x40},
                         
		{0x34, 0x11},
		
		{0x3b, 0xca},	//enable night mode
		//{0x3b, 0x4a},	//disable night mode

		{0xa4, 0x88},
		{0x96, 0x00},
		{0x97, 0x30},
		{0x98, 0x20},
		{0x99, 0x30},
		{0x9a, 0x84},
		{0x9b, 0x29},
		{0x9c, 0x03},
		{0x9d, 0x98},
		{0x9e, 0x7f},
		{0x78, 0x04},
                         
		{0x79, 0x01},
		{0xc8, 0xf0},
		{0x79, 0x0f},
		{0xc8, 0x00},
		{0x79, 0x10},
		{0xc8, 0x7e},
		{0x79, 0x0a},
		{0xc8, 0x80},
		{0x79, 0x0b},
		{0xc8, 0x01},
		{0x79, 0x0c},
		{0xc8, 0x0f},
		{0x79, 0x0d},
		{0xc8, 0x20},
		{0x79, 0x09},
		{0xc8, 0x80},
		{0x79, 0x02},
		{0xc8, 0xc0},
		{0x79, 0x03},
		{0xc8, 0x40},
		{0x79, 0x05},
		{0xc8, 0x30},
		{0x79, 0x26},
		
//		{0x3b, 0x00},    // 60Hz = 0x00, 50Hz = 0x08
		{0x3b, 0x08},    // 60Hz = 0x00, 50Hz = 0x08
//		{0x6b, 0x4a},
//		{0x6b, 0x0a},

/*
		{0x11, 0x81},    // 15fps
		{0x6b, 0x0a},
		{0x92, 0x40},
		{0x9d, 0x55},
		{0x9e, 0x47},
		{0xa5, 0x02},
		{0xab, 0x03},
*/
		{0xFF, 0xFF}
};

static INT32S ov7670_sccb_open(void)
{
#if SCCB_MODE == SCCB_GPIO
	ov7670_handle = drv_l2_sccb_open(OV7670_ID, 8, 8);
	if(ov7670_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_init();
	ov7670_handle.slaveAddr = OV7670_ID;
	ov7670_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void ov7670_sccb_close(void)
{
#if SCCB_MODE == SCCB_GPIO
	if(ov7670_handle) {
		drv_l2_sccb_close(ov7670_handle);
		ov7670_handle = NULL;
	}	
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_uninit();
	ov7670_handle.slaveAddr = 0;
	ov7670_handle.clkRate = 0;
#endif
}

static INT32S ov7670_sccb_write(INT8U reg, INT8U value)
{
#if SCCB_MODE == SCCB_GPIO
	return drv_l2_sccb_write(ov7670_handle, reg, value);

#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[2];
	
	data[0] = reg;
	data[1] = value;
	return drv_l1_i2c_bus_write(&ov7670_handle, data, 2);
#endif
}

#if 0
static INT32S ov7670_sccb_read(INT8U reg, INT8U *value)
{
#if SCCB_MODE == SCCB_GPIO
	INT16U data;
	
	if(drv_l2_sccb_read(ov7670_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[1];
	
	data[0] = reg;
	if(drv_l1_i2c_bus_write(&ov7670_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	if(drv_l1_i2c_bus_read(&ov7670_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	*value = data[0];
#endif
	return STATUS_OK;
}
#endif

static INT32S ov7670_sccb_write_table(regval8_t *pTable)
{
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}
		
		if(ov7670_sccb_write(pTable->reg_num, pTable->value) < 0) {
			DBG_PRINT("sccb write fail.\r\n");
			continue;
		}
		pTable++;
	}
	return STATUS_OK;
}

/**
 * @brief   ov7670 initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void ov7670_csi_init(void)
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
	drv_l2_sensor_set_mclkout(ov7670_sensor_csi_ops.info[0].mclk);
	
	/* request SCCB */
	ov7670_sccb_open();	
	DBG_PRINT("Sensor OV7670 csi interface init completed\r\n");
}	

/**
 * @brief   ov7670 un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
void ov7670_csi_uninit(void)
{
	// disable mclk 
	drv_l2_sensor_set_mclkout(MCLK_NONE);
	
	// csi disable
	R_CSI_TG_CTRL0 &= ~MASK_CSI_CTRL0_CSIEN;	//disable sensor controller
	R_CSI_TG_CTRL1 &= ~MASK_CSI_CTRL1_CLKOEN;	//disable sensor clock out
	
	// release SCCB
	ov7670_sccb_close();

	// Turn off LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(0, LDO_LDO28_2P8V);
}	

/**
 * @brief   ov7670 stream start function
 * @param   info index
 *			
 * @return 	none
 */
void ov7670_csi_stream_start(INT32U index, INT32U bufA, INT32U bufB)
{
	/* enable CSI output clock for SCCB */
	drv_l2_sensor_set_mclkout(ov7670_sensor_csi_ops.info[index].mclk);
	
	/* set start frame buffer address */	
	if(bufA) {
		drv_l1_csi_set_buf(bufA);
	} else {
		DBG_PRINT("Input frame buffer address error, fail to start %s\r\n", ov7670_sensor_csi_ops.name);
		ov7670_sensor_csi_ops.stream_stop();
	}	

	/* Set Horizontal/Vertical counter reset, Vertical counter incresase selection, sensor owner inerrupt enable, capture/preview mode */
	R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HRST | MASK_CSI_CTRL0_VADD | MASK_CSI_CTRL0_VRST | MASK_CSI_CTRL0_OWN_IRQ | MASK_CSI_CTRL0_CAP;
		
	/* Set delay clock 244[0:3] */
	R_CSI_TG_CTRL1 |= DELAY_1CLOCK;
	
	R_CSI_TG_CTRL1 |= MASK_CSI_CTRL1_RGB1555;

	/* According to the specific information, set CSI register */
	/* set input interface */
	if(ov7670_sensor_csi_ops.info[index].interface_mode == MODE_CCIR_HREF) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_HREF;
	}
	
	/* set input & output format */
	if(ov7670_sensor_csi_ops.info[index].input_format == V4L2_PIX_FMT_VYUY) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVIN | MASK_CSI_CTRL0_YUVTYPE;
	}	

	if(ov7670_sensor_csi_ops.info[index].output_format == V4L2_PIX_FMT_VYUY) {
		R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_YUVOUT;
	}

	/* Set sensor's registers via SCCB */
	if(ov7670_sccb_write_table((regval8_t *)OV7670_YUV_CONFIG) < 0) {
		DBG_PRINT("ov7670 init fail!!!\r\n");
	}
	
	DBG_PRINT("%s = %d\r\n", __func__, index);
	switch(index)
	{
	case 0:
		R_CSI_TG_HWIDTH = ov7670_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = ov7670_sensor_csi_ops.info[index].target_h;	
		R_CSI_TG_HRATIO = 0;		// No Scale
		R_CSI_TG_VRATIO = 0;		// No Scale	
		break;
		
	case 1:	
		R_CSI_TG_HWIDTH = ov7670_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = ov7670_sensor_csi_ops.info[index].target_h * 2;
		R_CSI_TG_HRATIO = 0x0102;	// Scale to 1/2
		R_CSI_TG_VRATIO = 0x0102;	// Scale to 1/2
		break;
	
	case 2:
		R_CSI_TG_HWIDTH = ov7670_sensor_csi_ops.info[index].target_w;
		R_CSI_TG_VHEIGHT = ov7670_sensor_csi_ops.info[index].target_h * 2;
		R_CSI_TG_HRATIO = 0x0104;	// Scale to 1/4
		R_CSI_TG_VRATIO = 0x0104;	// Scale to 1/4
		break;
		
	default:
		while(1);	
	}
	
	/* csi long burst width need 32 align */
	if(ov7670_sensor_csi_ops.info[index].target_w & 0x1F) {
		R_CSI_SEN_CTRL &= ~(1 << 7);
	} else {
		R_CSI_SEN_CTRL |= 1 << 7;
	}
	
	/* Enable interrupt */
	vic_irq_enable(VIC_CSI);
	
	/* Clear sensor interrupt status */
	R_TGR_IRQ_STATUS = 0x3F;
	
	/* enable frame end interrupt */
	R_TGR_IRQ_EN |= MASK_CSI_FRAME_END_FLAG|MASK_CSI_FIFO_OVERFLOW_FLAG;
	
	/* enable CSI module */
	R_CSI_TG_CTRL0 |= MASK_CSI_CTRL0_CSIEN;
}

/**
 * @brief   ov7670 stream stop function
 * @param   none
 * @return 	none
 */ 
void ov7670_csi_stream_stop(void)
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
 * @brief   ov7670 get info function
 * @param   none
 * @return 	pointer to sensor information data
 */ 
drv_l2_sensor_info_t* ov7670_csi_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1))
		return NULL;
	else
		return (drv_l2_sensor_info_t*)&ov7670_sensor_csi_ops.info[index];
}
	
/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t ov7670_sensor_csi_ops =
{
	SENSOR_OV7670_CSI_NAME,				/* sensor name */
	ov7670_csi_init,
	ov7670_csi_uninit,
	ov7670_csi_stream_start,
	ov7670_csi_stream_stop,
	ov7670_csi_get_info,
	{
		/* 1nd info */	
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
		/* 2st info */
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
			120,		 				/* target height */
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
#endif //(defined _SENSOR_OV7670_CSI) && (_SENSOR_OV7670_CSI == 1)