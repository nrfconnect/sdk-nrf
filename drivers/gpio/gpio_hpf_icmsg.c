/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/ipc/ipc_service.h>

#include "gpio_hpf.h"

#define IPC_BOUND_TIMEOUT_MS 100
#define IPC_BOUND_RETRY_COUNT 10
#define IPC_BOUND_RETRY_DELAY_MS 10

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

static int wait_for_endpoint_bound(void)
{
#if defined(CONFIG_MULTITHREADING)
	return k_sem_take(&bound_sem, K_MSEC(IPC_BOUND_TIMEOUT_MS));
#else
#if defined(CONFIG_SYS_CLOCK_EXISTS)
	uint32_t start = k_uptime_get_32();
#else
	uint32_t repeat = IPC_BOUND_TIMEOUT_MS * 1000; /* Convert ms to us. */
#endif

	while (bound_sem != 0U) {
#if defined(CONFIG_SYS_CLOCK_EXISTS)
		if ((k_uptime_get_32() - start) > IPC_BOUND_TIMEOUT_MS) {
			return -ETIMEDOUT;
		}
#else
		repeat--;
		if (repeat == 0U) {
			return -ETIMEDOUT;
		}
#endif
		k_sleep(K_USEC(1));
	}

	return 0;
#endif
}

static int register_endpoint_with_retry(const struct device *ipc0_instance)
{
	int ret;

	for (int attempt = 0; attempt < IPC_BOUND_RETRY_COUNT; attempt++) {
#if defined(CONFIG_MULTITHREADING)
		k_sem_reset(&bound_sem);
#else
		bound_sem = 1;
#endif

		ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
		if (ret < 0) {
			return ret;
		}

		ret = wait_for_endpoint_bound();
		if (ret == 0) {
			return 0;
		}

		ret = ipc_service_deregister_endpoint(&ep);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(IPC_BOUND_RETRY_DELAY_MS));
	}

	return -ETIMEDOUT;
}

int gpio_send(hpf_gpio_data_packet_t *msg)
{
	if (ipc_service_send(&ep, (void *)msg, sizeof(hpf_gpio_data_packet_t)) ==
	    sizeof(hpf_gpio_data_packet_t)) {
		return 0;
	} else {
		return -EIO;
	}
}

int gpio_hpf_init(const struct device *port)
{
	const struct device *ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	int ret = ipc_service_open_instance(ipc0_instance);

	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	return register_endpoint_with_retry(ipc0_instance);
}
