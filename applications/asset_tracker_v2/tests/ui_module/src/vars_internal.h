/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>

#include "events/led_state_event.h"

/* Expose internal variables in UI module for testing purposes. */
extern enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_LTE_CONNECTING,
	STATE_CLOUD_CONNECTING,
	STATE_CLOUD_ASSOCIATING,
	STATE_FOTA_UPDATING,
	STATE_SHUTDOWN
} state;

extern enum sub_state_type {
	SUB_STATE_ACTIVE,
	SUB_STATE_PASSIVE,
} sub_state;

extern enum sub_sub_state_type {
	SUB_SUB_STATE_LOCATION_INACTIVE,
	SUB_SUB_STATE_LOCATION_ACTIVE
} sub_sub_state;

extern struct led_pattern {
	/* Variable used to construct a linked list of led patterns. */
	sys_snode_t header;
	/* LED state. */
	enum led_state led_state;
	/* Duration of the LED state. */
	int16_t duration_sec;
} led_pattern_list[LED_STATE_COUNT];

extern sys_slist_t pattern_transition_list;

extern void transition_list_clear(void);
