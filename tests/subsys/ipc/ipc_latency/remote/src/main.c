/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

#define BOUND_TIMEOUT_MS 5000

const struct device *ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
static struct ipc_ept test_endpoint;

K_SEM_DEFINE(endpoint_bound_sem, 0, 1);

static void ep_bound_callback(void *priv)
{
	k_sem_give(&endpoint_bound_sem);
}

static void ep_unbound_callback(void *priv)
{
	printk("IPC endpoint unbounded\n");
}

static void ep_recv_callback(const void *data, size_t len, void *priv)
{
	int ret;

	ret = ipc_service_send(&test_endpoint, data, len);
	if (ret < 0) {
		printk("Failed to send data back: %d\n", ret);
	}
}

static void ep_error_callback(const char *message, void *priv)
{
	printk("IPC data error: %s\n", message);
}

static struct ipc_ept_cfg test_ep_cfg = {
	.name = "ep0",
	.cb = {
		.bound = ep_bound_callback,
		.unbound = ep_unbound_callback,
		.received = ep_recv_callback,
		.error = ep_error_callback,
	},
};

int main(void)
{

	int ret;

	printk("Hello\n");
	k_msleep(50);

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		printk("IPC instance open failed\n");
		return -1;
	}

	printk("Instance opened\n");

	ret = ipc_service_register_endpoint(ipc0_instance, &test_endpoint, &test_ep_cfg);
	if (ret != 0) {
		printk("IPC endpoint register failed\n");
		return -1;
	}
	printk("Endpoint registered\n");

	printk("Waiting for binding\n");
	ret = k_sem_take(&endpoint_bound_sem, K_MSEC(BOUND_TIMEOUT_MS));
	if (ret != 0) {
		printk("IPC timeout occurred while waiting for endpoint bound\n");
		return -1;
	}
	printk("Binding done\n");

	k_sleep(K_FOREVER);

	return 0;
}
