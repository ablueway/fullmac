/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifndef _NETSTACK_DEF_H_
#define _NETSTACK_DEF_H_

#include "lwip/netif.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#define NETDEV_IF_DOWN      0
#define NETDEV_IF_UP        NETIF_FLAG_UP
#define NETDEV_IF_LINK_UP   NETIF_FLAG_LINK_UP
#define NETDEV_IF_BROADCAST   NETIF_FLAG_BROADCAST

typedef enum _wifi_apmode_sec_s
{
    WIFI_APMODE_SEC_OPEN,
	WIFI_APMODE_SEC_WEP, 	
    WIFI_APMODE_SEC_WPA,//AES
    WIFI_APMODE_SEC_WPA2//AES
} wifi_apmode_sectype_t;

int wwd_softap_start( char *ssid, char *password, unsigned char security, unsigned char channel );
int wwd_softap_stop(unsigned char force);
//void netstack_set_rate_mask(char *mask);
//void netstack_set_rc_resent(bool resent);

#endif //#ifndef _NETSTACK_DEF_H_
