/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_NET_NET_MGMT_H_
#define ZEPHYR_NET_NET_MGMT_H_

#include <stdint.h>

/* Bit macro definition */
#ifndef BIT64
#define BIT64(n) (1ULL << (n))
#endif

/* Forward declarations */
struct net_if;

/* Network management event definitions */
#define NET_EVENT_L4_CONNECTED    BIT64(0)
#define NET_EVENT_L4_DISCONNECTED BIT64(1)

/* Event callback structure - must be defined before the typedef */
struct net_mgmt_event_callback {
	void *next;  /* For linked list */
	void *handler;  /* Will be cast to proper function pointer */
	uint64_t event_mask;
};

/* Function pointer type for event handlers */
typedef void (*net_mgmt_event_handler_t)(struct net_mgmt_event_callback *cb,
					  uint64_t event, struct net_if *iface);

/* Mock function declarations for the two functions we need */
void net_mgmt_init_event_callback(struct net_mgmt_event_callback *cb,
				  net_mgmt_event_handler_t handler,
				  uint64_t mgmt_event_mask);

void net_mgmt_add_event_callback(struct net_mgmt_event_callback *cb);

#endif /* ZEPHYR_NET_NET_MGMT_H_ */
