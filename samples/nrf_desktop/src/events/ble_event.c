/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "ble_event.h"

const void *PEER_DISCONNECTED = "disconnected";
const void *PEER_CONNECTED = "connected";
const void *PEER_SECURED = "secured";

static void print_event(const struct event_header *eh)
{
	struct ble_peer_event *event = cast_ble_peer_event(eh);

	printk("conn_id=%p %s", event->conn_id, (const char *)(event->state));
}

EVENT_TYPE_DEFINE(ble_peer_event, print_event);

EVENT_TYPE_DEFINE(ble_interval_event, NULL);
