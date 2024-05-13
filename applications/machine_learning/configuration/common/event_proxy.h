/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EVENT_PROXY_H_
#define _EVENT_PROXY_H_

#include <zephyr/device.h>

struct event_proxy_config {
	const struct device *ipc;
	size_t events_cnt;
	const struct event_type **events;
};

#endif /* _EVENT_PROXY_H_ */
