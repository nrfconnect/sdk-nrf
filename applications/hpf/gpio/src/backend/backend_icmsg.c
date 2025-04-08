/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "backend.h"
#include <zephyr/ipc/ipc_service.h>

static struct ipc_ept ep;
static backend_callback_t cbck;
static volatile uint32_t bound_sem = 1;

static void ep_bound(void *priv)
{
	bound_sem = 0;
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	(void)len;
	(void)priv;

	cbck((hpf_gpio_data_packet_t *)data);
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.received = ep_recv,
	},
};

int backend_init(backend_callback_t callback)
{
	int ret = 0;
	const struct device *ipc0_instance;
	volatile uint32_t delay = 0;
	cbck = callback;

#if !defined(CONFIG_SYS_CLOCK_EXISTS)
	/* Wait a little bit for IPC service to be ready on APP side */
	while (delay < 1000) {
		delay++;
	}
#endif

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Wait for endpoint to be bound */
	while (bound_sem != 0) {
	};

	return 0;
}
