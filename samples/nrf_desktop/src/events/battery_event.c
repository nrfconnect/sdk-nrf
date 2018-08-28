/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "battery_event.h"

static void print_event(const struct event_header *eh)
{
}

EVENT_TYPE_DEFINE(battery_event, print_event, NULL);
