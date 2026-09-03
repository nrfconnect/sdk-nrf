/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "timer_internal.h"

static K_MUTEX_DEFINE(timer_lock);
static struct rtfw_timer_config desired_config;
static uint32_t desired_period_us;

static int config_publish(const struct rtfw_timer_config *config)
{
	struct rtfw_command command = {
		.id = RTFW_TIMER_COMMAND_CONFIGURE,
		.data_len = sizeof(*config),
	};

	memcpy(command.data, config, sizeof(*config));
	return rtfw_submit(&command);
}

int rtfw_timer_init(rtfw_event_cb_t event_handler, void *user_data)
{
	const struct rtfw_config framework_config = {
		.command_handler = rtfw_timer_command_handler,
		.fastpath_handler = rtfw_timer_fastpath_handler,
		.pend_source_irq = rtfw_timer_pend_source_irq,
		.event_handler = event_handler,
		.event_user_data = user_data,
	};
	int error;

	desired_period_us = RTFW_TIMER_DEFAULT_PERIOD_US;
	desired_config.enabled = 0U;
	desired_config.period_ticks =
		rtfw_timer_us_to_ticks(RTFW_TIMER_DEFAULT_PERIOD_US);

	error = rtfw_init(&framework_config);
	if (error != 0) {
		return error;
	}

	rtfw_timer_fastpath_init(desired_config.period_ticks);
	return config_publish(&desired_config);
}

int rtfw_timer_enable(bool enable)
{
	struct rtfw_timer_config config;
	int error;

	k_mutex_lock(&timer_lock, K_FOREVER);
	config = desired_config;
	config.enabled = enable ? 1U : 0U;
	error = config_publish(&config);
	if (error == 0) {
		desired_config = config;
	}
	k_mutex_unlock(&timer_lock);
	return error;
}

int rtfw_timer_period_set(uint32_t period_us)
{
	struct rtfw_timer_config config;
	uint32_t period_ticks;
	int error;

	if (period_us < CONFIG_SAMPLE_RTFW_TIMER_MIN_PERIOD_US ||
	    period_us > CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US) {
		return -EINVAL;
	}
	period_ticks = rtfw_timer_us_to_ticks(period_us);
	if (period_ticks < RTFW_TIMER_MIN_PERIOD_TICKS) {
		return -EINVAL;
	}

	k_mutex_lock(&timer_lock, K_FOREVER);
	config = desired_config;
	config.period_ticks = period_ticks;
	error = config_publish(&config);
	if (error == 0) {
		desired_config = config;
		desired_period_us = period_us;
	}
	k_mutex_unlock(&timer_lock);
	return error;
}

int rtfw_timer_status_get(struct rtfw_timer_status *status)
{
	struct rtfw_status framework_status;
	struct rtfw_timer_config applied = {0};
	bool applied_valid;
	int error;

	if (status == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&timer_lock, K_FOREVER);
	error = rtfw_get_status(&framework_status);
	if (error != 0) {
		k_mutex_unlock(&timer_lock);
		return error;
	}

	status->requested_enabled = desired_config.enabled != 0U;
	status->requested_period_us = desired_period_us;
	status->requested_period_ticks = desired_config.period_ticks;

	applied_valid =
		framework_status.applied.id == RTFW_TIMER_COMMAND_CONFIGURE &&
		framework_status.applied.data_len == sizeof(applied);
	if (applied_valid) {
		memcpy(&applied, framework_status.applied.data,
		       sizeof(applied));
	}
	status->enabled =
		applied_valid && applied.enabled != 0U;
	status->period_ticks = applied_valid ? applied.period_ticks :
					      desired_config.period_ticks;
	status->period_us = rtfw_timer_ticks_to_us(status->period_ticks);
	status->pending = framework_status.pending;
	status->tick_count = rtfw_timer_tick_count_get();
	status->dropped_events = framework_status.dropped_events;
	status->max_queue_depth = framework_status.max_queue_depth;
	status->faults = framework_status.faults;
	k_mutex_unlock(&timer_lock);
	return 0;
}
