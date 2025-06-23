/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/net_mgmt.h>

/* Simple stub implementations for the net_mgmt functions we need to mock */

void net_mgmt_init_event_callback(struct net_mgmt_event_callback *cb,
				  net_mgmt_event_handler_t handler,
				  uint64_t mgmt_event_mask)
{
	/* Mock implementation - do nothing */
	(void)cb;
	(void)handler;
	(void)mgmt_event_mask;
}

void net_mgmt_add_event_callback(struct net_mgmt_event_callback *cb)
{
	/* Mock implementation - do nothing */
	(void)cb;
}
