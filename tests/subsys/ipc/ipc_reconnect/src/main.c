/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/random/random.h>

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <nrf53_cpunet_mgmt.h>
#endif

#define IPC_TIMEOUT_MS		       8000
#define SEND_DEAD_TIME_MS	       1000
#define TEST_DATA_LEN		       4
#define TEST_MESSAGES_COUNT	       3

const struct device *const ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
static uint8_t *rx_data;

K_SEM_DEFINE(endpoint_bound_sem, 0, 1);
K_SEM_DEFINE(recv_data_ready_sem, 0, 1);

static void prepare_test_data(uint8_t *test_data, size_t len)
{
	for (int i = 0; i < len; i++) {
		test_data[i] = sys_rand8_get();
	}
}

static void ep_bound_callback(void *priv)
{
	k_sem_give(&endpoint_bound_sem);

}

static void ep_unbound_callback(void *priv)
{

}

static void ep_recv_callback(const void *data, size_t len, void *priv)
{
	memcpy(rx_data, data, len);
	k_sem_give(&recv_data_ready_sem);
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

static void configure_ipc(struct ipc_ept *endpoint)
{
	int ret;

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		printk("IPC instance open failed\n");
	} else {
		printk("Instance opened\n");
	}

	ret = ipc_service_register_endpoint(ipc0_instance, endpoint, &test_ep_cfg);
	if (ret) {
		printk("IPC endpoint register failed\n");
	} else {
		printk("Endpoint registered\n");
	}

	printk("Waiting for binding\n");
	ret = k_sem_take(&endpoint_bound_sem, K_MSEC(IPC_TIMEOUT_MS));
	if (ret) {
		printk("IPC timeout occurred while waiting for endpoint bound\n");
	} else {
		printk("IPC endpoint bound\n");
	}

	k_msleep(10);
}

static void unconfigure_ipc(struct ipc_ept *endpoint)
{

	int ret;

	ret = ipc_service_deregister_endpoint(endpoint);
	if (ret) {
		printk("Failed to deregister IPC endpoint: %d\n", ret);
	} else {
		printk("IPC endpoint deregistered\n");
	}

	ret = ipc_service_close_instance(ipc0_instance);
	if (ret) {
		printk("Failed to close IPC instance: %d\n", ret);
	} else {
		printk("IPC instance closed\n");
	}
}

static void test_transmission(struct ipc_ept *endpoint)
{

	int ret;
	uint8_t *test_data = (uint8_t *)k_malloc(TEST_DATA_LEN);

	rx_data = (uint8_t *)k_malloc(TEST_DATA_LEN);

	prepare_test_data(test_data, TEST_DATA_LEN);

	for (int i = 0; i < TEST_MESSAGES_COUNT; i++) {
		ret = ipc_service_send(endpoint, (void *)test_data, TEST_DATA_LEN);
		if (ret < 0) {
			printk("Failed to send message over IPC: %d\n", ret);
		} else {
			printk("Message %u sent\n", i);
		}

		if (k_sem_take(&recv_data_ready_sem, K_MSEC(IPC_TIMEOUT_MS))) {
			printk("IPC data receive timeout\n");
		} else {
			printk("Message %u received\n", i);
		}

		if (memcmp(test_data, rx_data, TEST_DATA_LEN)) {
			printk("Message %u: TX data != RX data\n", i);
		}

		k_msleep(SEND_DEAD_TIME_MS);
	}

	k_free(test_data);
	k_free(rx_data);
}

int main(void)
{
	struct ipc_ept test_endpoint;

	printk("Hello\n");
	k_msleep(100);

	configure_ipc(&test_endpoint);
	test_transmission(&test_endpoint);
	printk("Disconnecting\n");
	unconfigure_ipc(&test_endpoint);
	printk("Reconnecting\n");
	configure_ipc(&test_endpoint);
	test_transmission(&test_endpoint);

	return 0;
}
