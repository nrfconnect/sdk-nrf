/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/radio_notification_cb.h>
#include <bluetooth/hci_vs_sdc.h>

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)
#include <hal/nrf_rtc.h>
static uint32_t sys_clock_start_to_bt_clk_start_us;
#endif

static uint32_t configured_prepare_distance_us;
static const struct bt_radio_notification_conn_cb *registered_cb;

static struct bt_conn *conn_pointers[CONFIG_BT_MAX_CONN];
static struct k_timer timers[CONFIG_BT_MAX_CONN];
static struct k_work work_queue_items[CONFIG_BT_MAX_CONN];

static uint32_t conn_interval_us_get(const struct bt_conn *conn)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info)) {
		return 0;
	}

	return BT_CONN_INTERVAL_TO_US(info.le.interval);
}

static void work_handler(struct k_work *work)
{
	struct bt_conn_info info;
	uint8_t index = ARRAY_INDEX(work_queue_items, work);
	struct bt_conn *conn = conn_pointers[index];

	if (bt_conn_get_info(conn, &info)) {
		return;
	}

	if (info.state == BT_CONN_STATE_CONNECTED) {
		registered_cb->prepare(conn);
	} else {
		k_timer_stop(&timers[index]);
	}
}

static void timer_handler(struct k_timer *timer)
{
	uint8_t conn_index = ARRAY_INDEX(timers, timer);

	k_work_submit(&work_queue_items[conn_index]);
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t subevent_code;
	struct bt_conn *conn;
	sdc_hci_subevent_vs_conn_anchor_point_update_report_t *evt = NULL;

	subevent_code = net_buf_simple_pull_u8(buf);

	switch (subevent_code) {
	case SDC_HCI_SUBEVENT_VS_CONN_ANCHOR_POINT_UPDATE_REPORT:
		evt = (void *)buf->data;
		break;
	default:
		return false;
	}

	conn = bt_hci_conn_lookup_handle(evt->conn_handle);
	if (!conn) {
		return true;
	}

	uint32_t conn_interval_us = conn_interval_us_get(conn);
	uint8_t conn_index = bt_conn_index(conn);

	conn_pointers[conn_index] = conn;

	bt_conn_unref(conn);
	conn = NULL;

	uint64_t timer_trigger_us =
		evt->anchor_point_us + conn_interval_us - configured_prepare_distance_us;

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)
	timer_trigger_us -= sys_clock_start_to_bt_clk_start_us;
#endif

	/* Start/Restart a timer triggering every conn_interval_us from the last anchor point. */
	k_timer_start(&timers[conn_index], K_TIMEOUT_ABS_US(timer_trigger_us),
		      K_USEC(conn_interval_us));

	return true;
}

int bt_radio_notification_conn_cb_register(const struct bt_radio_notification_conn_cb *cb,
					   uint32_t prepare_distance_us)
{
	if (registered_cb) {
		return -EALREADY;
	}

	int err;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		return err;
	}

	sdc_hci_cmd_vs_conn_anchor_point_update_event_report_enable_t enable_params = {
		.enable = true
	};

	err = hci_vs_sdc_conn_anchor_point_update_event_report_enable(&enable_params);
	if (err) {
		return err;
	}

	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		k_work_init(&work_queue_items[i], work_handler);
		k_timer_init(&timers[i], timer_handler, NULL);
	}

	registered_cb = cb;
	configured_prepare_distance_us = prepare_distance_us;

	return 0;
}

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)
static int set_sys_clock_start_to_bt_clk_start_us(void)
{
	/* To obtain the system clock from the bt clock timestamp
	 * we find the offset between RTC0 and RTC1.
	 * We achieve this by reading out the counter value of both of them.
	 * When the diff between those two have been equal twice, we know the
	 * time difference in ticks.
	 */

	uint32_t sync_attempts = 10;
	int32_t ticks_difference = 0;

	while (sync_attempts > 0) {
		sync_attempts--;

		int32_t diff_measurement_1 =
			nrf_rtc_counter_get(NRF_RTC0) - nrf_rtc_counter_get(NRF_RTC1);

		/* We need to wait half an RTC tick to ensure we are not measuring
		 * the diff between the two RTCs at the point in time where their
		 * values are transitioning.
		 */
		k_busy_wait(15);

		int32_t diff_measurement_2 =
			nrf_rtc_counter_get(NRF_RTC0) - nrf_rtc_counter_get(NRF_RTC1);

		ticks_difference = diff_measurement_1;
		if (diff_measurement_1 == diff_measurement_2) {
			break;
		}
	}

	const uint64_t rtc_ticks_in_femto_units = 30517578125UL;

	__ASSERT_NO_MSG(ticks_difference >= 0);

	sys_clock_start_to_bt_clk_start_us =
		((ticks_difference * rtc_ticks_in_femto_units) / 1000000000UL);
	return 0;
}

SYS_INIT(set_sys_clock_start_to_bt_clk_start_us, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
