/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/ipc/ipc_service.h>

#include "gpio_nrfe.h"

#if defined(CONFIG_MULTITHREADING)
static K_SEM_DEFINE(bound_sem, 0, 1);
#else
static volatile uint32_t bound_sem = 1;
#endif

static void ep_bound(void *priv)
{
#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&bound_sem);
#else
	bound_sem = 0;
#endif
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.received = NULL,
	},
};

static struct ipc_ept ep;

int gpio_send(nrfe_gpio_data_packet_t *msg)
{
	if (ipc_service_send(&ep, (void *)msg, sizeof(nrfe_gpio_data_packet_t)) ==
	    sizeof(nrfe_gpio_data_packet_t)) {
		return 0;
	} else {
		return -EIO;
	}
}

int gpio_nrfe_init(const struct device *port)
{
	const struct device *ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	int ret = ipc_service_open_instance(ipc0_instance);

	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_MULTITHREADING)
	k_sem_take(&bound_sem, K_FOREVER);
#else
	while (bound_sem != 0) {
	}
#endif

	return 0;
}
