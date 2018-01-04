/******************************************************
* drv_l2_sensor.c
*
* Purpose: Sensor L2 driver
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
#include "drv_l1_sfr.h"
#include "drv_l1_gpio.h"
#include "drv_l1_csi.h"
#include "drv_l2_sensor.h"
#include "driver_l2.h"

#if (defined _DRV_L2_SENSOR) && (_DRV_L2_SENSOR == 1)
/******************************************************
    Definitions and variable declaration
*******************************************************/
static drv_l2_sensor_ops_t* sensor_obj[MAX_SENSOR_NUM];

/**********************************************
*	Extern variables and functions
**********************************************/

/*********************************************
*	Variables declaration
*********************************************/

/*****************************************************
    Utility functions
******************************************************/

/**
 * @brief   Register sensor operation function table
 * @param   sensor : operation function table
 * @return 	none
 */
void drv_l2_sensor_register_ops(drv_l2_sensor_ops_t* sensor)
{
	INT32U i;
	
	for(i=0; i<MAX_SENSOR_NUM; i++) {
		if(sensor_obj[i] == 0) {
			sensor_obj[i] = sensor;
			break;
		}
	}
	
	if(i == MAX_SENSOR_NUM) {
		DBG_PRINT("sensor_obj not find the empty when register ops\r\n");
	}
}	

/**
 * @brief   Get sensor operation function table
 * @param   index : sensor index
 * @return 	sensor operation function table
 */
drv_l2_sensor_ops_t* drv_l2_sensor_get_ops(INT32U index)
{
	if(index > MAX_SENSOR_NUM) {
		DBG_PRINT("Index[%d] is wrong when get ops\r\n", index);
		return NULL;
	}
		
	return sensor_obj[index];
}	

/**
 * @brief   Sensor driver layer 2 initialization function
 * @param   none
 * @return 	none
 */
void drv_l2_sensor_init(void)
{
	INT32U i;
	/* reset sensor_obj pointer to NULL */
	for(i=0; i<MAX_SENSOR_NUM; i++) {
		sensor_obj[i] = NULL;
	}	
	
	/* register sensor ops for specified sensor */
#if (defined _SENSOR_OV7670_CSI) && (_SENSOR_OV7670_CSI == 1)	
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov7670_sensor_csi_ops);	
#endif
#if (defined _SENSOR_GC0308_CSI) && (_SENSOR_GC0308_CSI == 1)	
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&gc0308_sensor_csi_ops);	
#endif
#if (defined _SENSOR_OV3640_CSI) && (_SENSOR_OV3640_CSI == 1)	
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov3640_sensor_csi_ops);	
#endif
#if (defined _SENSOR_OV7670_CDSP) && (_SENSOR_OV7670_CDSP == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov7670_cdsp_ops);
#endif
#if (defined _SENSOR_OV9712_CDSP) && (_SENSOR_OV9712_CDSP == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov9712_cdsp_ops);
#endif
#if (defined _SENSOR_OV3640_CSI_MIPI) && (_SENSOR_OV3640_CSI_MIPI == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov3640_csi_mipi_ops);
#endif
#if (defined _SENSOR_OV3640_CDSP_MIPI) && (_SENSOR_OV3640_CDSP_MIPI == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov3640_cdsp_mipi_ops);
#endif
#if (defined _SENSOR_SOI_H22_CDSP_MIPI) && (_SENSOR_SOI_H22_CDSP_MIPI == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&soi_h22_cdsp_mipi_ops);
#endif		
#if (defined _TVIN_TVP5150_CSI) && (_TVIN_TVP5150_CSI == 1)	
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&tvp5150_csi_ops);	
#endif
#if (defined _SENSOR_GC1004_CDSP_MIPI) && (_SENSOR_GC1004_CDSP_MIPI == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&gc1004_cdsp_mipi_ops);
#endif		
#if (defined _SENSOR_GC1004_CDSP) && (_SENSOR_GC1004_CDSP == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&gc1004_cdsp_ops);
#endif		
#if (defined _SENSOR_OV8865_CDSP_MIPI) && (_SENSOR_OV8865_CDSP_MIPI == 1)
	drv_l2_sensor_register_ops((drv_l2_sensor_ops_t*)&ov8865_cdsp_mipi_ops);
#endif
}	

/**
* @brief	select internal data path 
* @param	sen_path: csi or cdsp,  
* @return	none
*/
void drv_l2_sensor_set_path(INT32U sen_path)
{
	if (sen_path == 1) {
		R_SYSTEM_MISC_CTRL4 |= (1<<4);
	} else {
		R_SYSTEM_MISC_CTRL4 &= ~(1<<4);
	}
}

/**
 * @brief   set csi input source path
 * @param   from_mipi_en[in]: 0: from sensor, 1: from mipi
 * @return 	none
 */
void drv_l2_sensor_set_csi_source(INT32U from_mipi_en)
{
	if(from_mipi_en == 0) {
		R_SYSTEM_MISC_CTRL0 &= ~(1 << 0);	//form sensor interface	
	} else {
		R_SYSTEM_MISC_CTRL0 |= (1 << 0);	//form mipi interface
	}
}

/**
* @brief	select mclk out clock speed
* @param	mclk_sel:   
* @return	speed
*/
INT32S drv_l2_sensor_set_mclkout(INT32U mclk_sel)
{
	INT32U mclk_out;
	
	// set mclk
	switch(mclk_sel)
	{
	case MCLK_NONE:
		R_CSI_TG_CTRL1 &= ~(1 << 7);
		R_SYSTEM_CTRL &= ~(1 << 8);
		return 0;

	case MCLK_13_5M:
		R_SYSTEM_CTRL |= (1 << 14) | (1 << 11) | (1 << 8); 
		R_CSI_TG_CTRL1 &= ~(1 << 11);		
		mclk_out = 13500000;
		break;	

	case MCLK_27M:
		R_SYSTEM_CTRL &= ~(1 << 14);
		R_SYSTEM_CTRL |= (1 << 11) | (1 << 8); 
		R_CSI_TG_CTRL1 &= ~(1 << 11);		
		mclk_out = 27000000;
		break;	

	case MCLK_24M:
		// 0xD000000C[14][11][8] set 1, from XCKGEN 24Mhz
		R_SYSTEM_CTRL |= (1 << 14) | (1 << 11) | (1 << 8); 
		R_CSI_TG_CTRL1 |= (1 << 11);
		mclk_out = 24000000;
		break;

	case MCLK_48M:
		R_SYSTEM_CTRL &= ~(1 << 14);
		R_SYSTEM_CTRL |= (1 << 11) | (1 << 8); 
		R_CSI_TG_CTRL1 |= (1 << 11);
		mclk_out = 48000000;
		break;

	default:
		return 0;
	}
	
	// Enable CSI MCLK out	
	R_CSI_TG_CTRL1 |= (1 << 7);
	return mclk_out;
}
#endif //_DRV_L2_SENSOR
