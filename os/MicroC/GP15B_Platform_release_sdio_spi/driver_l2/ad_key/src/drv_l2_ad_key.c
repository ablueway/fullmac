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
#include "drv_l2_ad_key.h"
#include "drv_l2_system_timer.h"
#include "drv_l1_adc.h"

#if (defined _DRV_L2_KEYSCAN) && (_DRV_L2_KEYSCAN == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_AD_KEY_QUEUE_MAX		5
 
#define C_AD_KEY_CH				ADC_LINE_0 
#define C_AD_KEY_DEBOUNCE		16//4			// 4 * 1 / 128 = 32ms
#define C_AD_KEY_START_REPEAT	96			// 96 * 1 / 128 = 0.75s
#define C_AD_KEY_REPEAT_COUNT	12			// 12 * 1 / 128 = 93ms
#define C_INVALID_KEY			0xffffffff

// 10bit valid value
#if (BOARD_TYPE == BOARD_DVP_GPB51PG)
	#define C_AD_VALUE_0			40
	#define C_AD_VALUE_1			100 + 40
	#define C_AD_VALUE_2			250 + 40
	#define C_AD_VALUE_3			430 + 40
	#define C_AD_VALUE_4			540 + 40
	#define C_AD_VALUE_5			760 + 40
	#define C_AD_VALUE_6			890 + 40
	#define C_AD_VALUE_7			900 + 40
	#define C_AD_VALUE_8			1024
#elif (BOARD_TYPE == BOARD_EMU_GPB51PG)
	#define C_AD_VALUE_0			40
	#define C_AD_VALUE_1			100 + 40
	#define C_AD_VALUE_2			250 + 40
	#define C_AD_VALUE_3			330 + 40
	#define C_AD_VALUE_4			440 + 40
	#define C_AD_VALUE_5			560 + 40
	#define C_AD_VALUE_6			690 + 40
	#define C_AD_VALUE_7			800 + 40
	#define C_AD_VALUE_8			1024
#endif
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum {
	RB_KEY_DOWN,
	RB_KEY_UP,
	RB_KEY_REPEAT,
	RB_KEY_MAX
} RB_KEY_TYPES_ENUM; 

typedef struct AdKey_s
{
	INT8U  flag;
    INT8U  key_code;
    INT8U  key_type;
    INT8U  RSD;
} AdKey_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void ad_key_timer_isr(void);
static void ad_key_adc_isr(INT16U ad_val);
 
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static OS_EVENT *ad_key_q;
static void *ad_key_q_buf[C_AD_KEY_QUEUE_MAX];

static INT8U ad_key_is_down;
static INT8U ad_key_start_repeat;
static INT8U ad_key_repeat_count;
static INT32U ad_key_count;
static INT32U ad_key_value;


/**
 * @brief   ad_key_initial
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_ad_key_init(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	if(ad_key_q == 0) {
		ad_key_q = OSQCreate(ad_key_q_buf, C_AD_KEY_QUEUE_MAX);
		if(ad_key_q == 0) {
			return STATUS_FAIL;
		}
	}
#endif
	
	ad_key_count = 0;
	ad_key_is_down = 0;
    ad_key_value = C_INVALID_KEY;
    
	ad_key_start_repeat = C_AD_KEY_START_REPEAT;
	ad_key_repeat_count = C_AD_KEY_REPEAT_COUNT;
	
	drv_l1_adc_init();
	drv_l1_adc_manual_ch_set(C_AD_KEY_CH);
	drv_l1_adc_manual_callback_set(ad_key_adc_isr);
	
	drv_l2_system_timer_registe_isr(ad_key_timer_isr);
	return STATUS_OK;
}

/**
 * @brief   ad_key_uninitial
 * @param   none
 * @return 	none
 */
void drv_l2_ad_key_uninit(void)
{
	drv_l2_system_timer_release_isr(ad_key_timer_isr);
#if _OPERATING_SYSTEM == _OS_UCOS2	
	if(ad_key_q) {
		INT8U err;
		OSQFlush(ad_key_q);
		OSQDel(ad_key_q, OS_DEL_ALWAYS, &err);
		ad_key_q = 0;
	}
#endif
}

/**
 * @brief   ad_key_uninitial
 * @param   none
 * @return 	key number, KEY_NUMBER_ENUM
 */
INT32S drv_l2_ad_key_scan(void)
{
	INT8U err;
	INT32U ADKey;
	
	ADKey = (INT32U)OSQPend(ad_key_q, 0, &err);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
		
	return ADKey;
}

static void ad_key_timer_isr(void)
{
	drv_l1_adc_manual_sample_start();	
}

static void ad_key_adc_isr(INT16U ad_val)
{
	INT32U key, post_key=0;
	AdKey_t key_para;
	
	ad_val >>= 6;
	//DBG_PRINT("ad_val = %d\r\n", ad_val);
	
	if(ad_val < C_AD_VALUE_0) {
 		key = C_INVALID_KEY;	// no key pressed
 	} else if(ad_val < C_AD_VALUE_1) {
 		key = ADKEY_IO1;
 	} else if(ad_val < C_AD_VALUE_2) {
		key = ADKEY_IO2;
	} else if(ad_val < C_AD_VALUE_3) {
		key = ADKEY_IO3;
	} else if(ad_val < C_AD_VALUE_4) {
		key = ADKEY_IO4;
 	} else if(ad_val < C_AD_VALUE_5) {
		key = ADKEY_IO5;
	} else if(ad_val < C_AD_VALUE_6) {
		key = ADKEY_IO6;
	} else if(ad_val < C_AD_VALUE_7) {
		key = ADKEY_IO7;
	} else if(ad_val < C_AD_VALUE_8) {
		key = ADKEY_IO8;	
	} else {
		key = C_INVALID_KEY;	// no key pressed
	}

	if(key == C_INVALID_KEY) {
		if (ad_key_is_down) {
			ad_key_is_down = 0;			
			key_para.key_code = ad_key_value;
			key_para.key_type = RB_KEY_UP;
			goto __post_key;		
		}
		
		ad_key_count = 0;
		ad_key_value = key;
		return;
	}
	
	if(key == ad_key_value) {
		ad_key_count += 1;
		if(ad_key_count == C_AD_KEY_DEBOUNCE) {
			ad_key_is_down = 1;		
			key_para.key_code = key;
			key_para.key_type = RB_KEY_DOWN;
			goto __post_key;		
	    } else if(ad_key_count == ad_key_start_repeat) {
			ad_key_count -= ad_key_repeat_count;
			key_para.key_code = key;
			key_para.key_type = RB_KEY_REPEAT;
			goto __post_key;
		}
	} else {
		ad_key_count = 0;
		ad_key_value = key;
	}
	
	return;
	
__post_key:
	switch(key_para.key_code)
	{
	case ADKEY_IO1:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO1;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO1_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO1_C;
		}
		break;
	case ADKEY_IO2:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO2;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO2_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO2_C;
		}
		break;
	case ADKEY_IO3:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO3;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO3_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO3_C;
		}
		break;	
	case ADKEY_IO4:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO4;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO4_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO4_C;
		}
		break;	
	case ADKEY_IO5:
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO5;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO5_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO5_C;
		}
		break;
	case ADKEY_IO6:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO6;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO6_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO6_C;
		}	
		break;
	case ADKEY_IO7:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO7;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO7_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO7_C;
		}
		break;	
	case ADKEY_IO8:	
		if(key_para.key_type == RB_KEY_DOWN) {
			post_key = ADKEY_IO8;
		} else if(key_para.key_type == RB_KEY_UP) {
			post_key = ADKEY_IO8_UP;
		} else if(key_para.key_type == RB_KEY_REPEAT) {
			post_key = ADKEY_IO8_C;
		}
		break;	
	}		

#if _OPERATING_SYSTEM == _OS_UCOS2		
	OSQPost(ad_key_q, (void *)post_key);
#endif		
}
#endif
