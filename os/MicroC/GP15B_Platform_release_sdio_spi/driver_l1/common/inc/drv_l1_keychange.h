#ifndef __drv_l1_KEYCHANGE_H__
#define __drv_l1_KEYCHANGE_H__

/****************************************************
*		include file								*
****************************************************/
#include "driver_l1_cfg.h"
#include "project.h"
#include "Arm.h"

/****************************************************
*		register 									*
****************************************************/
#define R_KEYCH_INT  	(*((volatile INT32U *) 0xD0100024))
#define R_KEYCH_LATCH  	(*((volatile INT32U *) 0xC0000110))
#define R_KEYCH_ENABLE  (*((volatile INT32U *) 0xC000017C))


/****************************************************
*		constant 									*
****************************************************/

/*
// Register 0xD0100024 bit definition
#define KEYCHG7_INT_FLAG  1<<15
#define KEYCHG6_INT_FLAG  1<<13
#define KEYCHG5_INT_FLAG  1<<11
#define KEYCHG4_INT_FLAG  1<<9
#define KEYCHG3_INT_FLAG  1<<7
#define KEYCHG2_INT_FLAG  1<<5
#define KEYCHG1_INT_FLAG  1<<3
#define KEYCHG0_INT_FLAG  1<<1

#define KEYCHG7_INT_EN    1<<14
#define KEYCHG6_INT_EN    1<<12
#define KEYCHG5_INT_EN    1<<10
#define KEYCHG4_INT_EN    1<<8
#define KEYCHG3_INT_EN    1<<6
#define KEYCHG2_INT_EN    1<<4
#define KEYCHG1_INT_EN    1<<2
#define KEYCHG0_INT_EN    1<<0
*/

// keychange number
#define KEYCHG_INT_7    7
#define KEYCHG_INT_6    6
#define KEYCHG_INT_5    5
#define KEYCHG_INT_4    4
#define KEYCHG_INT_3    3
#define KEYCHG_INT_2    2
#define KEYCHG_INT_1    1
#define KEYCHG_INT_0    0

#define KEYCHG_INT_MAX  8

/****************************************************
*		function 									*
*****************************************************/

void drv_l1_keychange_init(void);
INT8S drv_l1_keychange_int_set(INT8U byKeyChgNum, INT8U bEnable, void (*keychange_isr)(void));
void drv_l1_keychange_latch(void);

#endif 		/* __drv_l1_KEYCHANGE_H__ */


