/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_TIMER_INTERNAL_H_
#define RTFW_TIMER_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/sys/util.h>
#include <rtfw/rtfw.h>

#define RTFW_TIMER_COMMAND_CONFIGURE 1U
#define RTFW_TIMER_EVENT_TICK        RTFW_EVENT_TYPE_USER_BASE
#define RTFW_TIMER_DEFAULT_PERIOD_US 500000U
#define RTFW_TIMER_MIN_PERIOD_TICKS  2U

struct rtfw_timer_config {
	uint32_t enabled;
	uint32_t period_ticks;
};

BUILD_ASSERT(sizeof(struct rtfw_timer_config) <=
	     CONFIG_RTFW_COMMAND_DATA_SIZE,
	     "RTFW command payload is too small for the TIMER configuration");
BUILD_ASSERT(CONFIG_SAMPLE_RTFW_TIMER_MIN_PERIOD_US <=
	     CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US,
	     "minimum TIMER period must not exceed maximum");
BUILD_ASSERT(RTFW_TIMER_DEFAULT_PERIOD_US >=
	     CONFIG_SAMPLE_RTFW_TIMER_MIN_PERIOD_US &&
	     RTFW_TIMER_DEFAULT_PERIOD_US <=
	     CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US,
	     "default TIMER period must be inside the accepted range");

struct rtfw_timer_status {
	bool enabled;
	uint32_t period_us;
	uint32_t period_ticks;
	bool requested_enabled;
	uint32_t requested_period_us;
	uint32_t requested_period_ticks;
	bool pending;
	uint32_t tick_count;
	uint32_t dropped_events;
	uint32_t max_queue_depth;
	uint32_t faults;
};

int rtfw_timer_init(rtfw_event_cb_t event_handler, void *user_data);
int rtfw_timer_enable(bool enable);
int rtfw_timer_period_set(uint32_t period_us);
int rtfw_timer_status_get(struct rtfw_timer_status *status);

int rtfw_timer_command_handler(const struct rtfw_command *command,
			       void *user_data);
void rtfw_timer_fastpath_handler(void *user_data);
void rtfw_timer_pend_source_irq(void *user_data);
void rtfw_timer_fastpath_init(uint32_t initial_period_ticks);
uint32_t rtfw_timer_us_to_ticks(uint32_t microseconds);
uint32_t rtfw_timer_ticks_to_us(uint32_t ticks);
uint32_t rtfw_timer_tick_count_get(void);

#endif /* RTFW_TIMER_INTERNAL_H_ */
