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
#include "drv_l1_sfr.h"
#include "drv_l1_wrap.h"

#if (defined _DRV_L1_WRAP) && (_DRV_L1_WRAP == 1)                 
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define WRAP_PATH_SR_EN			(1 << 0)
#define WRAP_PATH_O_EN			(1 << 2)
#define WRAP_PROTECT_EN			(1 << 3)
#define WRAP_CLIP_MODE_EN		(1 << 4)
#define WRAP_FILTER_EN			(1 << 8)
#define WRAP_BURST_8_MODE		(1 << 11)
#define WRAP_FILTER_DO_FLUSH	(1 << 15)
#define WRAP_PROTECT_STATUS		(1 << 28)
#define WRAP_BUSY_CHECK			((INT32U)1 << 31)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct 
{										// Offset
	volatile INT32U	CTRL;           	// 0x0000
	volatile INT32U	WRAP_SADDR;        	// 0x0004
	volatile INT32U	FILTER_SADDR;      	// 0x0008
	volatile INT32U	FILTER_SIZE;      	// 0x000C
	volatile INT32U	FRAME_WIDTH;      	// 0x0010
	volatile INT32U	FRAME_HEIGHT;      	// 0x0014
	volatile INT32U	CLIP_SRC_WIDTH;    	// 0x0018
	volatile INT32U	CLIP_SRC_HEIGHT;   	// 0x001C
	volatile INT32U	CLIP_START_X;    	// 0x0020
	volatile INT32U	CLIP_START_Y;   	// 0x0024
}WRAP_SFR;


static WRAP_SFR * get_WRAP_SFR_base(INT8U wrap_num)
{
	if (wrap_num == WRAP_SCA2TFT) {
		return (WRAP_SFR *)P_SCA2TFT_BASE;
	} else if (wrap_num == WRAP_CSIMUX) {
		return (WRAP_SFR *)P_CSIMUX_BASE;
	} else {
		return (WRAP_SFR *)P_CSI2SCA_BASE;
	}
}

/**
 * @brief   set wrap filter enable
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   enable[in]: enable or disable 
 * @return 	none
 */
void drv_l1_wrap_filter_enable(INT8U wrap_num, INT8U enable)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	if(enable) {
		pWrap->CTRL |= WRAP_FILTER_EN;	//enable filter
	} else {
		pWrap->CTRL &= ~WRAP_FILTER_EN;	//disable filter
	}
}

/**
 * @brief   set wrap path
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   pathSrEnable[in]: 1: Enable path-sr  0: Disable path-sr
 * @param   pathOEnable[in]: 1: Enable path-o  0: Disable path-o
 * @return 	none
 */
void drv_l1_wrap_path_set(INT8U wrap_num, INT8U pathSrEnable, INT8U pathOEnable)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	if(pathSrEnable) {
		pWrap->CTRL |= WRAP_PATH_SR_EN;
	} else {
		pWrap->CTRL &= ~WRAP_PATH_SR_EN;
	}
	
	if((wrap_num == WRAP_CSIMUX) || (wrap_num == WRAP_CSI2SCA)) {
		if(pathOEnable) {
			pWrap->CTRL |= WRAP_PATH_O_EN;
		} else {
			pWrap->CTRL &= ~WRAP_PATH_O_EN;
		}
	}

	pWrap->CTRL |= WRAP_BURST_8_MODE; // For D1 alignment	
}

/**
 * @brief   set wrap output address for path-sr and path-o 
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   addr[in]: buffer address 
 * @return 	none
 */
void drv_l1_wrap_addr_set(INT8U wrap_num, INT32U addr)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	pWrap->WRAP_SADDR = addr;
}

/**
 * @brief   set wrap filter start address  
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   addr[in]: filter start address 
 * @param   size[in]: filter access range 
 * @return 	none
 */
void drv_l1_wrap_filter_addr_set(INT8U wrap_num, INT32U addr, INT32U size)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);
	
	if(wrap_num == WRAP_CSIMUX) {
		pWrap->FILTER_SADDR = addr;
		pWrap->FILTER_SIZE = size;	
	}
}

/**
 * @brief   set wrap flush internal point and load output address
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @return 	none
 */
void drv_l1_wrap_filter_flush(INT8U wrap_num)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	pWrap->CTRL |= WRAP_FILTER_DO_FLUSH;
}

/**
 * @brief   wait wrap busy
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   wait_idle[in]: 1: wait until idle, 0: get current status
 * @return 	busy status, 1: busy, 0:idle
 */
INT32S drv_l1_wrap_check_busy(INT8U wrap_num, INT32U wait_idle)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);
	
	if(wait_idle == 0) {
		if(pWrap->CTRL & WRAP_BUSY_CHECK) {
			return 1;
		}
		
		return 0;
	}
	
	while(pWrap->CTRL & WRAP_BUSY_CHECK);
	return 0;
}

/**
 * @brief   set wrap protect enable
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_wrap_protect_enable(INT8U wrap_num, INT8U enable)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);
	
	if(wrap_num == WRAP_CSI2SCA) {
		return;
	}
	
	if(enable) {
		pWrap->CTRL |= WRAP_PROTECT_EN;
	} else {
		pWrap->CTRL &= ~WRAP_PROTECT_EN;
	}
}

/**
 * @brief   set wrap protect size
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   inWidth[in]: input width
 * @param   inHeight[in]: input height
 * @return 	busy status, 1: busy, 0:idle
 */
void drv_l1_wrap_protect_pixels_set(INT8U wrap_num, INT32U inWidth, INT32U inHeight)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	if(wrap_num == WRAP_CSI2SCA) {
		return;
	}

	pWrap->FRAME_WIDTH = ((inWidth >> 1)-1);
	pWrap->FRAME_HEIGHT = (inHeight-1);
}

/**
 * @brief   set wrap clip mode enable
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_wrap_clip_mode_enable(INT8U wrap_num, INT8U enable)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	if(wrap_num == WRAP_CSIMUX) {
		if(enable) {
			pWrap->CTRL |= WRAP_CLIP_MODE_EN;
		} else {
			pWrap->CTRL &= ~WRAP_CLIP_MODE_EN;
		}
	}
}

/**
 * @brief   set wrap clip mode source size
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   inWidth[in]: input width
 * @param   inHeight[in]: input height
 * @return 	none
 */
void drv_l1_wrap_clip_source_pixels_set(INT8U wrap_num, INT32U inWidth, INT32U inHeight)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);
	
	if(wrap_num == WRAP_CSIMUX) {
		pWrap->CLIP_SRC_WIDTH = inWidth;
		pWrap->CLIP_SRC_HEIGHT = inHeight;
	}
}

/**
 * @brief   set wrap clip mode start pixel
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @param   inStartX[in]: start x postion
 * @param   inStartY[in]: start y postion
 * @return 	none
 */
void drv_l1_wrap_clip_start_pixels_set(INT8U wrap_num, INT32U inStartX, INT32U inStartY)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	if(wrap_num == WRAP_CSIMUX) {
		pWrap->CLIP_START_X = inStartX;
		pWrap->CLIP_START_Y = inStartY;
	}
}

/**
 * @brief   get wrap protect status
 * @param   wrap_num[in]: wrap device number, see WRAP_NUM
 * @return 	protect status, 0: normal run, 1: underrun 
 */
INT32S wrap_protect_status_get(INT8U wrap_num)
{
	WRAP_SFR* pWrap = get_WRAP_SFR_base(wrap_num);

	if(pWrap->CTRL & WRAP_PROTECT_STATUS) {
		return STATUS_FAIL;
	} else {	
		return STATUS_OK;
	}
}

#endif //(defined _DRV_L1_WRAP) && (_DRV_L1_WRAP == 1)

