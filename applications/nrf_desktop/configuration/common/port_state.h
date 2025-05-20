/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PORT_STATE_H_
#define _PORT_STATE_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pin_state {
	uint32_t pin;
	uint32_t val;
};

struct port_state {
	const struct device    *port;
	const struct pin_state *ps;
	size_t                  ps_count;
};

#ifdef __cplusplus
}
#endif

#endif /* _PORT_STATE_H_ */
