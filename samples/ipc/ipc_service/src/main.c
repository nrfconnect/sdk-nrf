/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host, LOG_LEVEL_INF);

#define BASE_MESSAGE_LEN 40
#define MESSAGE_LEN 100

struct data_packet {
	uint8_t message;
	uint8_t data[MESSAGE_LEN];
};


static K_SEM_DEFINE(bound_sem, 0, 1);

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	uint8_t received_val = *((uint8_t *)data);
	static uint8_t expected_val;
	static uint16_t expected_len = BASE_MESSAGE_LEN;

	static unsigned long long cnt;
	static unsigned int stats_every;
	static uint32_t start;

	if (start == 0) {
		start = k_uptime_get_32();
	}

	if ((received_val != expected_val) || (len != expected_len)) {
		printk("Unexpected message\n");
	}

	expected_val++;
	expected_len++;

	cnt += len;

	if (stats_every++ > 5000) {
		/* Print throuhput [Bytes/s]. Use printk not to overload CPU with logger.
		 * Sample never reaches lower priority thread because of high throughput
		 * (100% cpu load) so logging would not be able to handle messages in
		 * deferred mode (immediate mode would be heavier than printk).
		 */
		printk("%llu\n", (1000*cnt)/(k_uptime_get_32() - start));
		stats_every = 0;
	}

	if (expected_len > sizeof(struct data_packet)) {
		expected_len = BASE_MESSAGE_LEN;
	}
}

static struct ipc_ept_cfg ep_cfg = {
	.name = "ep0",
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};

int main(void)
{
	const struct device *ipc0_instance;
	struct data_packet msg = {.message = 0};
	struct ipc_ept ep;
	int ret;

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_INF("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure");
		return ret;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	uint16_t mlen = BASE_MESSAGE_LEN;

	while (true) {
		ret = ipc_service_send(&ep, &msg, mlen);
		if (ret == -ENOMEM) {
			/* No space in the buffer. Retry. */
			continue;
		} else if (ret < 0) {
			LOG_ERR("send_message(%d) failed with ret %d", msg.message, ret);
			break;
		}

		msg.message++;

		mlen++;
		if (mlen > sizeof(struct data_packet)) {
			mlen = BASE_MESSAGE_LEN;
		}

		/* Quasi minimal busy wait time which allows to continuosly send
		 * data without -ENOMEM error code. The purpose is to test max
		 * throughput. Determined experimentally.
		 */
		if (CONFIG_APP_IPC_SERVICE_SEND_INTERVAL < 1000) {
			k_busy_wait(CONFIG_APP_IPC_SERVICE_SEND_INTERVAL);
		} else {
			k_msleep(CONFIG_APP_IPC_SERVICE_SEND_INTERVAL/1000);
		}
	}

	return 0;
}
