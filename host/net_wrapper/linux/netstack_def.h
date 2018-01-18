/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifndef _NETSTACK_DEF_H_
#define _NETSTACK_DEF_H_

/* TODO: aaron: change the linux socket */
#if 0
#include "lwip/netif.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#define NETDEV_IF_DOWN      0
#define NETDEV_IF_UP        NETIF_FLAG_UP
#define NETDEV_IF_LINK_UP   NETIF_FLAG_LINK_UP
#define NETDEV_IF_BROADCAST   NETIF_FLAG_BROADCAST
#endif

#endif //#ifndef _NETSTACK_DEF_H_
