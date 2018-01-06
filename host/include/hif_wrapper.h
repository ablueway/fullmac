/*
 * Copyright (c) 2014 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Semiconductor Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __HIF_WRAPPER_H__
#define __HIF_WRAPPER_H__


#ifdef __linux__
#include "hwif.h"
#else
#include "ssv_drv_if.h"
#endif

#define DRV_INFO_FLAG_OS_TYPE_SHIFT				0x0
#define DRV_INFO_FLAG_OS_TYPE_MASK				0x000000FF
#define DRV_INFO_FLAG_OS_TYPE_LINUX				0x0
#define DRV_INFO_FLAG_OS_TYPE_RTOS				0x1

#define SET_DRV_INFO_FLAG_OS_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_OS_TYPE_SHIFT) & DRV_INFO_FLAG_OS_TYPE_MASK))


#define DRV_INFO_FLAG_REGISTER_TYPE_SHIFT		0x8
#define DRV_INFO_FLAG_REGISTER_TYPE_MASK		0x0000FF00
#define DRV_INFO_FLAG_REGISTER_TYPE_PASSIVE		0x0
#define DRV_INFO_FLAG_REGISTER_TYPE_ACTIVE		0x1

#define SET_DRV_INFO_FLAG_REGISTER_TYPE(flag, val)	\
	(flag = ((val << DRV_INFO_FLAG_REGISTER_TYPE_SHIFT) & DRV_INFO_FLAG_REGISTER_TYPE_MASK))


union ssv_drv_info
{
	struct 
	{
		u32 os_type:8;
		u32 register_type:8;
		u32 rsvd:16;
	} fields;
	u32	flags;
};

struct unified_drv_ops {
	union ssv_drv_info drv_info;

#ifdef __linux__
	struct ssv6xxx_hwif_ops drv_ops;
#else
	struct ssv6xxx_drv_ops drv_ops;
#endif
};

#endif /* __HIF_WRAPPER_H__ */
