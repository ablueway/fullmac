#ifndef __drv_l1_CACHE_H__
#define __drv_l1_CACHE_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

#define C_CACHE_SIZE					(8*1024)
#define C_CACHE_BANK_NUM				4
#define C_CACHE_AREA_START				0x00000000
#define C_CACHE_AREA_END				0xF80382FF

#define C_CACHE_INDEX_MASK				0x000007F0
#define C_CACHE_INDEX_SHIFT				4
#define C_CACHE_TAG_MASK				0x3FFFF800
#define C_CACHE_TAG_SHIFT				11

// Cache Valid_Lock_Tag register
#define C_CACHE_VLT_TAG_MASK			0x0007FFFF
#define C_CACHE_VLT_LOCK				0x00080000
#define C_CACHE_VLT_VALID				0x00100000
#define C_CACHE_VLT_INDEX_SHIFT			4
#define C_CACHE_VLT_BANK_SHIFT			20

// Cache control register
#define C_CACHE_CTRL_EN					0x00000001
#define C_CACHE_CTRL_WRITE_BUFFER_EN	0x00000002
#define C_CACHE_CTRL_WRITE_BACK_EN		0x00000004

// Cache config register
#define C_CACHE_CFG_BURST_EN			0x00000001
#define C_CACHE_CFG_SINGLE_EN			0x00000002
#define C_CACHE_CFG_VLT_EN				0x00000004

// Invalid cache line register
#define C_CACHE_INV_LINE_INVALID		0x00000001
#define C_CACHE_INV_LINE_DRAIN			0x00000002
#define C_CACHE_INV_LINE_INDEX_MASK		0x000007F0
#define C_CACHE_INV_LINE_BANK_SHIFT		2
#define C_CACHE_INV_LINE_EN				0x80000000

// Cache lockdown register
#define C_CACHE_LOCK_TAG_INDEX_MASK		0x3FFFFFF0
#define C_CACHE_LOCK_BANK_SHIFT			2
#define C_CACHE_LOCK_EN					0x80000000

// Drain write buffer register
#define C_CACHE_WRITE_BUF_DRAIN_EN		0x00000001

// Drain cache line register
#define C_CACHE_DRAIN_LINE_INDEX_MASK	0x000007F0
#define C_CACHE_DRAIN_LINE_BANK_SHIFT	2
#define C_CACHE_DRAIN_LINE_EN			0x80000002

// Invalid cache bank register
#define C_CACHE_INV_BANK_INVALID		0x00000001
#define C_CACHE_INV_BANK_DRAIN			0x00000002
#define C_CACHE_INV_BANK_BANK_SHIFT		2
#define C_CACHE_INV_BANK_EN				0x80000000

// Invalid cache range register
#define C_CACHE_INV_RANGE_INVALID		0x00000001
#define C_CACHE_INV_RANGE_DRAIN			0x00000002
#define C_CACHE_INV_RANGE_SIZE_MAX		0x3FFFFFF0
#define C_CACHE_INV_RANGE_ADDR_MASK		0x3FFFFFF0
#define C_CACHE_INV_RANGE_EN			0x80000000
/*
Function Name	cache_lockdown
Description	Lock down the specified area into cache
Header File	drv_l1_cache.h
Syntax	INT32S cache_lockdown(INT32U addr, INT32U size)
Parameters	addr: start address of the specified area
			size: size of the specified area
Return Values	0: Success
			-1: Failed
*/
extern INT32S cache_lockdown(INT32U addr, INT32U size);
/*
Function Name	cache_drain_line
Description	Drain the specified line
Header File	drv_l1_cache.h
Syntax	INT32S cache_drain_line(INT32U bank, INT32U addr)
Parameters	bank: the specified bank in cache
			addr: the specified line in cache
Return Values	0: Success
			-1: Failed
*/
extern INT32S cache_drain_range(INT32U addr, INT32U size);
/*
Function Name	cache_invalid_range
Description	Invalidate the specified area in cache
Header File	drv_l1_cache.h
Syntax	INT32S cache_invalid_range(INT32U addr, INT32U size)
Parameters	addr: start address of the specified area
			size: size of the specified area
Return Values	0: Success
			-1: Failed
*/
extern INT32S cache_invalid_range(INT32U addr, INT32U size);
/*
Function Name	cache_burst_mode_enable
Description	Enable burst mode
Header File	drv_l1_cache.h
Syntax	INT32S cache_burst_mode_enable(void)
Parameters	None
Return Values	0: Success
-1: Failed
*/
extern INT32S cache_burst_mode_enable(void);
/*
Function Name	cache_burst_mode_disable
Description	Disable burst mode
Header File	drv_l1_cache.h
Syntax	void cache_burst_mode_disable(void)
Parameters	None
Return Values	None
*/
extern void cache_burst_mode_disable(void);
/*
Function Name	cache_init
Description	Initialize Cache driver
Header File	drv_l1_cache.h
Syntax	void cache_init(void)
Parameters	None
Return Values	None
*/
extern void cache_init(void);
/*
Function Name	cache_enable
Description	Enable the Cache
Header File	drv_l1_cache.h
Syntax	void cache_enable(void)
Parameters	None
Return Values	None
*/
extern void cache_enable(void);
/*
Function Name	cache_disable
Description	Disable the Cache
Header File	drv_l1_cache.h
Syntax	void cache_disable(void)
Parameters	None
Return Values	None
*/
extern void cache_disable(void);
/*
Function Name	cache_write_back_enable
Description	Enable write back mode
Header File	drv_l1_cache.h
Syntax	void cache_write_back_enable(void)
Parameters	None
Return Values	None
*/
extern void cache_write_back_enable(void);
/*
Function Name	cache_write_back_disable
Description	Disable write back mode
Header File	drv_l1_cache.h
Syntax	void cache_write_back_disable(void)
Parameters	None
Return Values	None
*/
extern void cache_write_back_disable(void);


#endif		// __drv_l1_CACHE_H__