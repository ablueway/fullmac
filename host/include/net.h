/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifndef _NET_H_
#define _NET_H_

#include "host_config.h"
#include <net_def.h>


typedef struct st_dhcps_info{
	/* start,end are in host order: we need to compare start <= ip <= end */
	u32 start_ip;              /* start address of leases, in host order */
	u32 end_ip;                /* end of leases, in host order */
	u32 max_leases;            /* maximum number of leases (including reserved addresses) */

    u32 subnet;
    u32 gw;
    u32 dns;

	u32 auto_time;             /* how long should udhcpd wait before writing a config file.
			                         * if this is zero, it will only write one on SIGUSR1 */
	u32 decline_time;          /* how long an address is reserved if a client returns a
			                         * decline message */
	u32 conflict_time;         /* how long an arp conflict offender is leased for */
	u32 offer_time;            /* how long an offered address is reserved */
	u32 max_lease_sec;         /* maximum lease time (host order) */
	u32 min_lease_sec;         /* minimum lease time a client can request */
}dhcps_info;

typedef struct st_dhcpdipmac
{
    u32 ip;
    u8 mac[6];
    u8 reserved[2];
}dhcpdipmac;

struct netstack_ip_addr {
  u32 addr;
};

#endif /* _NET_H_ */