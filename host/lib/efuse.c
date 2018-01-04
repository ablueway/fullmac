/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <os.h>
#include <log.h>
#include <dev.h>
#include <ssv_dev.h>
#include "efuse.h"

u32 read_chip_id()
{
    return ssv_hal_read_chip_id();
}


void read_efuse_macaddr(u8 *mcdr)
{
	ssv_hal_read_efuse_mac(mcdr);
}

