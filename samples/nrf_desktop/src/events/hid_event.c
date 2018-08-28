/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "hid_event.h"

static void print_event(const struct event_header *eh)
{
}


EVENT_TYPE_DEFINE(hid_keyboard_event, print_event, NULL);
EVENT_TYPE_DEFINE(hid_mouse_xy_event, print_event, NULL);
EVENT_TYPE_DEFINE(hid_mouse_wp_event, print_event, NULL);
EVENT_TYPE_DEFINE(hid_mouse_button_event, print_event, NULL);
