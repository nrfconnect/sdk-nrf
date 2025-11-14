/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/ipc/ipc_service.h>

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <nrf53_cpunet_mgmt.h>
#endif

#define IPC_TIMEOUT_MS		       5000
#define TEST_TIMER_COUNT_TIME_LIMIT_MS 500
#define MEASUREMENT_REPEATS	       10
#define SEND_DEAD_TIME_MS	       10

const struct device *const ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));
static uint8_t *rx_data;

K_SEM_DEFINE(endpoint_bound_sem, 0, 1);
K_SEM_DEFINE(recv_data_ready_sem, 0, 1);

static void prepare_test_data(uint8_t *test_data, size_t len)
{
	for (int i = 0; i < len; i++) {
		test_data[i] = i;
	}
}

static uint64_t get_maximal_allowed_ping_pong_time_us(size_t test_message_len)
{
	uint64_t maximal_allowed_ping_pong_time_us;

#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
	switch (test_message_len) {
	case 1:
		maximal_allowed_ping_pong_time_us = 130;
		break;
	case 16:
		maximal_allowed_ping_pong_time_us = 140;
		break;
	case 32:
		maximal_allowed_ping_pong_time_us = 150;
		break;
	case 64:
		maximal_allowed_ping_pong_time_us = 160;
		break;
	case 128:
		maximal_allowed_ping_pong_time_us = 170;
		break;
	case 256:
		maximal_allowed_ping_pong_time_us = 180;
		break;
	case 512:
		maximal_allowed_ping_pong_time_us = 200;
		break;
	default:
		maximal_allowed_ping_pong_time_us = 220;
		break;
	}
#else
	switch (test_message_len) {
	case 1:
		maximal_allowed_ping_pong_time_us = 190;
		break;
	case 16:
		maximal_allowed_ping_pong_time_us = 200;
		break;
	case 32:
		maximal_allowed_ping_pong_time_us = 210;
		break;
	case 64:
		maximal_allowed_ping_pong_time_us = 220;
		break;
	case 128:
		maximal_allowed_ping_pong_time_us = 230;
		break;
	case 256:
		maximal_allowed_ping_pong_time_us = 240;
		break;
	case 512:
		maximal_allowed_ping_pong_time_us = 250;
		break;
	default:
		maximal_allowed_ping_pong_time_us = 300;
		break;
	}
#endif
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		maximal_allowed_ping_pong_time_us *= 1.8;
	}
	return maximal_allowed_ping_pong_time_us;
}

void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

static void ep_bound_callback(void *priv)
{
	k_sem_give(&endpoint_bound_sem);
}

static void ep_unbound_callback(void *priv)
{
	TC_PRINT("IPC endpoint unbounded\n");
}

static void ep_recv_callback(const void *data, size_t len, void *priv)
{
	memcpy(rx_data, data, len);
	k_sem_give(&recv_data_ready_sem);
}

static void ep_error_callback(const char *message, void *priv)
{
	TC_PRINT("IPC data error: %s\n", message);
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
	zassert_false((ret < 0) && (ret != -EALREADY), "IPC instance open failed\n");
	TC_PRINT("Instance opened\n");

	ret = ipc_service_register_endpoint(ipc0_instance, endpoint, &test_ep_cfg);
	zassert_ok(ret, "IPC endpoint register failed\n");
	TC_PRINT("Endpoint registered\n");

	TC_PRINT("Waiting for binding\n");
	ret = k_sem_take(&endpoint_bound_sem, K_MSEC(IPC_TIMEOUT_MS));
	zassert_ok(ret, "IPC timeout occurred while waiting for endpoint bound\n");
	TC_PRINT("Binding done\n");

	k_msleep(10);
}

static void test_ipc_latency(struct ipc_ept *endpoint, size_t test_message_len)
{
	int ret;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint64_t maximal_allowed_ping_pong_time_us;
	uint8_t *test_data = (uint8_t *)k_malloc(test_message_len);

	rx_data = (uint8_t *)k_malloc(test_message_len);

	maximal_allowed_ping_pong_time_us = get_maximal_allowed_ping_pong_time_us(test_message_len);
	prepare_test_data(test_data, test_message_len);
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	for (uint32_t test_counter = 0; test_counter < MEASUREMENT_REPEATS; test_counter++) {
		counter_reset(tst_timer_dev);
		counter_start(tst_timer_dev);
		ret = ipc_service_send(endpoint, (void *)test_data, test_message_len);
		if (ret < 0) {
			TC_PRINT("Failed to send message over IPC: %d\n", ret);
		}
		zassert_ok(k_sem_take(&recv_data_ready_sem, K_MSEC(IPC_TIMEOUT_MS)),
			   "IPC data receive timeout\n");
		counter_get_value(tst_timer_dev, &tst_timer_value);
		counter_stop(tst_timer_dev);

		zassert_mem_equal(test_data, rx_data, test_message_len, "TX data != RX data\n");

		timer_value_us[test_counter] = counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[test_counter];

		k_msleep(SEND_DEAD_TIME_MS);
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Measured IPC ping-pong time for %u bytes [us]: %llu\n", test_message_len,
		 average_timer_value_us);
	TC_PRINT("Maximal allowed ping-pong time for %u bytes [us]: %llu\n", test_message_len,
		 maximal_allowed_ping_pong_time_us);
	zexpect_true(average_timer_value_us < maximal_allowed_ping_pong_time_us,
		     "Measured IPC ping-pong latency is over the specified limit\n");

	k_free(test_data);
	k_free(rx_data);
}

ZTEST(ipc_latency, test_ipc_ping_pong_latency_app_rad)
{
	struct ipc_ept test_endpoint;

	configure_ipc(&test_endpoint);

	test_ipc_latency(&test_endpoint, 1);
	test_ipc_latency(&test_endpoint, 16);
	test_ipc_latency(&test_endpoint, 32);
	test_ipc_latency(&test_endpoint, 64);
	test_ipc_latency(&test_endpoint, 128);
#if defined(CONFIG_IPC_SERVICE_BACKEND_ICBMSG)
	test_ipc_latency(&test_endpoint, 256);
	test_ipc_latency(&test_endpoint, 512);
#endif
}

void *test_setup(void)
{
	TC_PRINT("Hello\n");
	k_msleep(100);

	return NULL;
}

ZTEST_SUITE(ipc_latency, NULL, test_setup, NULL, NULL, NULL);
