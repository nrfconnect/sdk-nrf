/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/drivers/gpio.h>

#ifdef CONFIG_TEST_EXTRA_STACK_SIZE
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#else
#define STACKSIZE (1024)
#endif
#define NUMBER_OF_MESSAGES_TO_SEND 20
#define IPC_SEND_DEAD_TIME_MS	   1
#define IPC_BOUND_TIMEOUT_MS	   5000

K_THREAD_STACK_DEFINE(ipc0_stack, STACKSIZE);

LOG_MODULE_REGISTER(host, LOG_LEVEL_INF);

static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

struct payload {
	unsigned long cnt;
	unsigned long size;
	uint8_t data[];
};

struct payload *p_payload;

static K_SEM_DEFINE(bound_sem, 0, 1);

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	uint8_t received_val = *((uint8_t *)data);
	static uint8_t expected_val;

	if ((received_val != expected_val) || (len != CONFIG_APP_IPC_SERVICE_MESSAGE_LEN)) {
		printk("Unexpected message received_val: %d , expected_val: %d\n", received_val,
		       expected_val);
		printk("Test should not enter here\n");
		__ASSERT_NO_MSG(1 == 0);
	}

	expected_val++;
}

static struct ipc_ept_cfg ep_cfg = {
	.name = "ep0",
	.cb = {
		.bound = ep_bound,
		.received = ep_recv,
	},
};

int main(void)
{
	const struct device *ipc0_instance;
	struct ipc_ept ep;
	int ret;
	unsigned long last_cnt = 0;
	unsigned long delta = 0;
	int test_repetitions = 3;

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret, "Error: GPIO Device not ready");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(ret == 0, "Could not configure led GPIO");

	p_payload = (struct payload *)k_malloc(CONFIG_APP_IPC_SERVICE_MESSAGE_LEN);
	if (!p_payload) {
		printk("k_malloc() failure\n");
		__ASSERT_NO_MSG(1 == 0);
	}

	memset(p_payload->data, 0xA5, CONFIG_APP_IPC_SERVICE_MESSAGE_LEN - sizeof(struct payload));

	p_payload->size = CONFIG_APP_IPC_SERVICE_MESSAGE_LEN;
	p_payload->cnt = 0;

	printk("IPC-service %s demo started\n", CONFIG_BOARD_TARGET);

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_INF("ipc_service_open_instance() failure (%d)", ret);
		__ASSERT_NO_MSG(ret == 0);
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure (%d)", ret);
		__ASSERT_NO_MSG(ret == 0);
	}

	ret = k_sem_take(&bound_sem, K_MSEC(IPC_BOUND_TIMEOUT_MS));
	if (ret < 0) {
		printf("k_sem_take() failure (%d)", ret);
		__ASSERT_NO_MSG(ret == 0);
	}
	k_msleep(CONFIG_TEST_START_DELAY_MS);

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis enabled\n");
	while (test_repetitions--)
#else
	while (test_repetitions)
#endif
	{
		printk("Hello\n");
		printk("Send data over IPC\n");

		for (int counter = 0; counter < NUMBER_OF_MESSAGES_TO_SEND; counter++) {
			ret = ipc_service_send(&ep, p_payload, CONFIG_APP_IPC_SERVICE_MESSAGE_LEN);
			if (ret == -ENOMEM) {
				/* No space in the buffer. Retry. */
				continue;
			} else if (ret < 0) {
				printk("send_message(%ld) failed with ret %d\n", p_payload->cnt,
				       ret);
				__ASSERT_NO_MSG(ret == 0);
			}
			k_msleep(IPC_SEND_DEAD_TIME_MS);

			delta = p_payload->cnt - last_cnt;
			printk("Δpkt: %ld (%ld B/pkt) | throughput: %ld bit/s\n", delta,
			       p_payload->size, delta * CONFIG_APP_IPC_SERVICE_MESSAGE_LEN * 8);
			last_cnt = p_payload->cnt;
			p_payload->cnt++;
		}

		printk("Go to sleep (s2ram)\n");
		gpio_pin_set_dt(&led, 0);
		k_msleep(CONFIG_SLEEP_TIME_MS);
		gpio_pin_set_dt(&led, 1);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}
