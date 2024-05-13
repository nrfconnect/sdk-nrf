/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "event_proxy.h"

/* This configuration file is included only once from event_proxy module and
 * holds information about the proxy channels and subscribed events.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} event_proxy_def_include_once;

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <caf/events/sensor_data_aggregator_event.h>


static const struct event_type *remote_events[] = {
	_EVENT_ID(sensor_data_aggregator_release_buffer_event),
};

static const struct event_proxy_config cfg = {
	.ipc = DEVICE_DT_GET(DT_NODELABEL(ipc1)),
	.events_cnt = ARRAY_SIZE(remote_events),
	.events = remote_events,
};

static const struct event_proxy_config *proxy_configs[CONFIG_EVENT_MANAGER_PROXY_CH_COUNT] = {
	&cfg,
};
