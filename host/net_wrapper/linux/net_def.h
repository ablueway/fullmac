/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifndef _LINUX_NET_DEF_H_
#define _LINUX_NET_DEF_H_

#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>

struct netdev
{
    char name[26];
    u16 mtu;
    u32 ipaddr;
    u32 netmask;
    u32 gw;
    u32 flags;    
    unsigned char hwmac[6];        
    bool add_state;
};

//typedef struct net_device *NET_DEV;
typedef struct netdev NET_DEV;
//typedef struct wireless_dev	*WIRELESS_DEV;

#define NETDEV_IF_DOWN      	(0x00U)
#define NETDEV_IF_UP        	(0x01U)
#define NETDEV_IF_LINK_UP   	(0x04U)
#define NETDEV_IF_BROADCAST   	(0x02U)

#endif /* _NET_DEF_H_ */
