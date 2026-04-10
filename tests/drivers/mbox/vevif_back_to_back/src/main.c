/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

#if !DT_NODE_EXISTS(DT_PATH(mbox_reproducer))
#error "Missing /mbox-reproducer devicetree node"
#endif

#if !defined(REPRO_SIDE_REMOTE)
#define REPRO_SIDE_REMOTE 0
#endif

#define REPRO_NODE DT_PATH(mbox_reproducer)
#define RX_MBOX_NODE DT_PHANDLE_BY_IDX(REPRO_NODE, mboxes, 0)
#define TX_MBOX_NODE DT_PHANDLE_BY_IDX(REPRO_NODE, mboxes, 1)

#if DT_NODE_HAS_COMPAT(RX_MBOX_NODE, nordic_nrf_vevif_event_rx)
#define RX_KIND "vevif-event-rx"
#elif DT_NODE_HAS_COMPAT(RX_MBOX_NODE, nordic_nrf_vevif_task_rx)
#define RX_KIND "vevif-task-rx"
#elif DT_NODE_HAS_COMPAT(RX_MBOX_NODE, nordic_nrf_bellboard_rx)
#define RX_KIND "bellboard-rx"
#else
#define RX_KIND "unknown-rx"
#endif

#if DT_NODE_HAS_COMPAT(TX_MBOX_NODE, nordic_nrf_vevif_event_tx)
#define TX_KIND "vevif-event-tx"
#elif DT_NODE_HAS_COMPAT(TX_MBOX_NODE, nordic_nrf_vevif_task_tx)
#define TX_KIND "vevif-task-tx"
#elif DT_NODE_HAS_COMPAT(TX_MBOX_NODE, nordic_nrf_bellboard_tx)
#define TX_KIND "bellboard-tx"
#else
#define TX_KIND "unknown-tx"
#endif

#if defined(CONFIG_REPRO_MODE_TASK_BURST) == defined(CONFIG_REPRO_MODE_REVERSE_BURST)
#error "Select exactly one reproducer mode"
#endif

#if defined(CONFIG_REPRO_MODE_TASK_BURST)
#define REPRO_MODE_NAME "task-burst"
#define REPRO_CONSUMER_REMOTE 1
#else
#define REPRO_MODE_NAME "reverse-burst"
#define REPRO_CONSUMER_REMOTE 0
#endif

#if REPRO_SIDE_REMOTE == REPRO_CONSUMER_REMOTE
#define REPRO_IS_CONSUMER 1
#else
#define REPRO_IS_CONSUMER 0
#endif

static const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(REPRO_NODE, rx);
static const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(REPRO_NODE, tx);

static atomic_t callback_count;

static bool wait_for_at_least(atomic_t *value, int expected, uint32_t timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	do {
		if (atomic_get(value) >= expected) {
			return true;
		}

		k_sleep(K_MSEC(1));
	} while (k_uptime_get() < deadline);

	return atomic_get(value) >= expected;
}

static int send_notification_pair(void)
{
	int ret;

	ret = mbox_send_dt(&tx_channel, NULL);
	if (ret < 0) {
		return ret;
	}

	return mbox_send_dt(&tx_channel, NULL);
}

static void rx_callback(const struct device *dev, mbox_channel_id_t channel_id,
			void *user_data, struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);
	ARG_UNUSED(data);

	atomic_inc(&callback_count);
}

static void print_banner(const char *role)
{
	printk("REPRO START board=%s role=%s mode=%s tx=%s rx=%s tx_ch=%u rx_ch=%u iterations=%d\n",
	       CONFIG_BOARD_TARGET, role, REPRO_MODE_NAME, TX_KIND, RX_KIND, tx_channel.channel_id,
	       rx_channel.channel_id, CONFIG_REPRO_ITERATIONS);
}


static int setup_consumer(void)
{
	int ret;

	if (!mbox_is_ready_dt(&rx_channel)) {
		printk("RESULT FAIL reason=rx-not-ready mode=%s role=consumer rx=%s\n",
		       REPRO_MODE_NAME, RX_KIND);
		return -ENODEV;
	}

	ret = mbox_register_callback_dt(&rx_channel, rx_callback, NULL);
	if (ret < 0) {
		printk("RESULT FAIL reason=register-callback mode=%s role=consumer ret=%d rx=%s\n",
		       REPRO_MODE_NAME, ret, RX_KIND);
		return ret;
	}

	ret = mbox_set_enabled_dt(&rx_channel, true);
	if (ret < 0) {
		printk("RESULT FAIL reason=set-enabled mode=%s role=consumer ret=%d rx=%s\n",
		       REPRO_MODE_NAME, ret, RX_KIND);
		return ret;
	}

	return 0;
}

static int setup_producer(void)
{
	if (!mbox_is_ready_dt(&tx_channel)) {
		printk("RESULT FAIL reason=tx-not-ready mode=%s role=producer tx=%s\n",
		       REPRO_MODE_NAME, TX_KIND);
		return -ENODEV;
	}

	return 0;
}

static void run_consumer(void)
{
	int ret;
	int expected = CONFIG_REPRO_ITERATIONS * 2;
	bool done;

	print_banner("consumer");

	ret = setup_consumer();
	if (ret < 0) {
		return;
	}

	done = wait_for_at_least(&callback_count, expected, CONFIG_REPRO_OVERALL_TIMEOUT_MS);
	printk("RESULT %s mode=%s role=consumer tx=%s rx=%s expected=%d observed=%d timeout_ms=%d\n",
	       done ? "PASS" : "FAIL", REPRO_MODE_NAME, TX_KIND, RX_KIND, expected,
	       atomic_get(&callback_count), CONFIG_REPRO_OVERALL_TIMEOUT_MS);

	while (true) {
		k_sleep(K_FOREVER);
	}
}

static void run_producer(void)
{
	int ret;

	print_banner("producer");

	ret = setup_producer();
	if (ret < 0) {
		return;
	}

	k_sleep(K_MSEC(CONFIG_REPRO_START_DELAY_MS));

	for (int iteration = 0; iteration < CONFIG_REPRO_ITERATIONS; iteration++) {
		ret = send_notification_pair();
		if (ret < 0) {
			printk("RESULT FAIL reason=send-error mode=%s role=producer iter=%d ret=%d tx=%s\n",
			       REPRO_MODE_NAME, iteration, ret, TX_KIND);
			return;
		}

		if (CONFIG_REPRO_ITERATION_GAP_MS > 0) {
			k_sleep(K_MSEC(CONFIG_REPRO_ITERATION_GAP_MS));
		}
	}

	printk("REPRO PRODUCER DONE mode=%s tx=%s iterations=%d\n",
		REPRO_MODE_NAME, TX_KIND, CONFIG_REPRO_ITERATIONS);

	while (true) {
		k_sleep(K_FOREVER);
	}
}

int main(void)
{
#if REPRO_IS_CONSUMER
	run_consumer();
#else
	run_producer();
#endif

	return 0;
}
