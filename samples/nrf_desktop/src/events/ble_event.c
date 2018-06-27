/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "ble_event.h"

static void print_event(const struct event_header *eh)
{
}

EVENT_TYPE_DEFINE(ble_peer_event, print_event);
