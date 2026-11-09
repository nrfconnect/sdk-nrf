/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Replaces the Zephyr pinctrl API, which is not available on native_sim. */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_

#include <stdint.h>

#define PINCTRL_STATE_DEFAULT 0U
#define PINCTRL_STATE_SLEEP   1U

struct pinctrl_dev_config {
	uint8_t unused;
};

#define PINCTRL_DT_DEFINE(node_id) static struct pinctrl_dev_config fake_pinctrl_dev_config

#define PINCTRL_DT_DEV_CONFIG_GET(node_id) (&fake_pinctrl_dev_config)

int pinctrl_apply_state(const struct pinctrl_dev_config *config, uint8_t id);

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_ */
