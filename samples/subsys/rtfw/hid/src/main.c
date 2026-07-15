/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/hids.h>
#include <hal/nrf_grtc.h>

#include "hid_internal.h"
#include "hid_platform.h"

#define HID_REPORT_SIZE  3U
#define HID_REPORT_ID    1U
#define HID_REPORT_INDEX 0U
#define BASE_USB_HID_SPEC_VERSION 0x0101
#define LATENCY_HISTOGRAM_BINS   256U
#define LATENCY_HISTOGRAM_BIN_US 8U
#define LATENCY_REPORT_SAMPLES   128U
#define LATENCY_HISTOGRAM_RANGE_US \
	(LATENCY_HISTOGRAM_BINS * LATENCY_HISTOGRAM_BIN_US)

BT_HIDS_DEF(hids, HID_REPORT_SIZE);

static K_MUTEX_DEFINE(connection_lock);
static struct bt_conn *active_connection;
static struct k_work advertising_work;

static uint32_t latency_min_us = UINT32_MAX;
static uint32_t latency_max_us;
static uint64_t latency_sum_us;
static uint32_t latency_samples;
static uint32_t latency_histogram_overflows;
static uint32_t hids_window_drops;
static int hids_send_last_error;
static uint32_t latency_histogram[LATENCY_HISTOGRAM_BINS];

static const uint8_t report_map[] = {
	0x05, 0x01,       /* Usage Page (Generic Desktop) */
	0x09, 0x02,       /* Usage (Mouse) */
	0xA1, 0x01,       /* Collection (Application) */
	0x85, HID_REPORT_ID,
	0x09, 0x01,       /* Usage (Pointer) */
	0xA1, 0x00,       /* Collection (Physical) */
	0x05, 0x09,       /* Usage Page (Buttons) */
	0x19, 0x01,
	0x29, 0x03,
	0x15, 0x00,
	0x25, 0x01,
	0x95, 0x03,
	0x75, 0x01,
	0x81, 0x02,
	0x95, 0x01,       /* Five bits padding */
	0x75, 0x05,
	0x81, 0x01,
	0x05, 0x01,
	0x09, 0x30,       /* X */
	0x09, 0x31,       /* Y */
	0x15, 0x81,
	0x25, 0x7f,
	0x75, 0x08,
	0x95, 0x02,
	0x81, 0x06,
	0xC0,
	0xC0,
};

static const struct bt_data advertising_data[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
};

static const struct bt_data scan_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void advertising_handler(struct k_work *work)
{
	int error;

	ARG_UNUSED(work);
	error = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, advertising_data,
				ARRAY_SIZE(advertising_data), scan_data,
				ARRAY_SIZE(scan_data));
	printk("HID advertising start: %d\n", error);
}

static void connected(struct bt_conn *connection, uint8_t error)
{
	int hids_error;
	int security_error;

	if (error != 0U) {
		return;
	}

	hids_error = bt_hids_connected(&hids, connection);
	if (hids_error != 0) {
		printk("HIDS connection setup failed: %d\n", hids_error);
		return;
	}

	k_mutex_lock(&connection_lock, K_FOREVER);
	if (active_connection == NULL) {
		active_connection = bt_conn_ref(connection);
	}
	k_mutex_unlock(&connection_lock);

	security_error = bt_conn_set_security(connection, BT_SECURITY_L2);
	if (security_error != 0) {
		printk("HID security request failed: %d\n", security_error);
	}
}

static void disconnected(struct bt_conn *connection, uint8_t reason)
{
	int error;

	ARG_UNUSED(reason);
	error = bt_hids_disconnected(&hids, connection);
	if (error != 0) {
		printk("HIDS disconnection cleanup failed: %d\n", error);
	}

	k_mutex_lock(&connection_lock, K_FOREVER);
	if (active_connection == connection) {
		bt_conn_unref(active_connection);
		active_connection = NULL;
	}
	k_mutex_unlock(&connection_lock);
}

static void connection_recycled(void)
{
	k_work_submit(&advertising_work);
}

BT_CONN_CB_DEFINE(connection_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = connection_recycled,
};

static int hids_init(void)
{
	struct bt_hids_init_param parameters = {0};
	struct bt_hids_inp_rep *input_report =
		&parameters.inp_rep_group_init.reports[0];

	parameters.rep_map.data = report_map;
	parameters.rep_map.size = sizeof(report_map);
	parameters.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	parameters.info.flags =
		BT_HIDS_REMOTE_WAKE | BT_HIDS_NORMALLY_CONNECTABLE;
	input_report->size = HID_REPORT_SIZE;
	input_report->id = HID_REPORT_ID;
	parameters.inp_rep_group_init.cnt = 1U;
	parameters.is_mouse = true;

	return bt_hids_init(&hids, &parameters);
}

static void latency_record(uint32_t edge_timestamp_us)
{
	uint32_t latency_us =
		nrf_grtc_sys_counter_low_get(NRF_GRTC) - edge_timestamp_us;
	uint32_t histogram_index = MIN(latency_us / LATENCY_HISTOGRAM_BIN_US,
				       LATENCY_HISTOGRAM_BINS - 1U);

	latency_min_us = MIN(latency_min_us, latency_us);
	latency_max_us = MAX(latency_max_us, latency_us);
	latency_sum_us += latency_us;
	latency_samples++;
	latency_histogram[histogram_index]++;
	if (latency_us >= LATENCY_HISTOGRAM_RANGE_US) {
		latency_histogram_overflows++;
	}

	if (latency_samples == LATENCY_REPORT_SAMPLES) {
		struct rtfw_status framework_status = {0};
		uint32_t cumulative = 0U;
		uint32_t median_index = 0U;
		uint32_t median_lb_us;
		int status_error;

		for (; median_index < LATENCY_HISTOGRAM_BINS; median_index++) {
			cumulative += latency_histogram[median_index];
			if (cumulative >= DIV_ROUND_UP(latency_samples, 2U)) {
				break;
			}
		}
		median_lb_us = median_index * LATENCY_HISTOGRAM_BIN_US;
		status_error = rtfw_get_status(&framework_status);
		printk("HID edge->HIDS submit us: avg=%llu median_lb=%u min=%u "
		       "max=%u jitter=%u hist_overflows=%u "
		       "hids_window_drops=%u last_hids_err=%d "
		       "rtfw_drops=%u rtfw_max_depth=%u "
		       "rtfw_faults=0x%08x rtfw_status_err=%d\n",
		       latency_sum_us / latency_samples, median_lb_us,
		       latency_min_us, latency_max_us,
		       latency_max_us - latency_min_us,
		       latency_histogram_overflows, hids_window_drops,
		       hids_send_last_error, framework_status.dropped_events,
		       framework_status.max_queue_depth, framework_status.faults,
		       status_error);

		latency_min_us = UINT32_MAX;
		latency_max_us = 0U;
		latency_sum_us = 0U;
		latency_samples = 0U;
		latency_histogram_overflows = 0U;
		hids_window_drops = 0U;
		hids_send_last_error = 0;
		memset(latency_histogram, 0, sizeof(latency_histogram));
	}
}

static void rtfw_event_handler(const struct rtfw_event *event, void *user_data)
{
	struct bt_conn *connection = NULL;
	uint8_t report[HID_REPORT_SIZE] = {0};
	int error;

	ARG_UNUSED(user_data);
	if (event->type != RTFW_HID_EVENT_EDGE) {
		return;
	}

	k_mutex_lock(&connection_lock, K_FOREVER);
	if (active_connection != NULL) {
		connection = bt_conn_ref(active_connection);
	}
	k_mutex_unlock(&connection_lock);

	if (connection == NULL) {
		return;
	}

	report[0] = event->value != 0U ? 1U : 0U;
	report[1] = event->value != 0U ? 8U : 0U;
	error = bt_hids_inp_rep_send(&hids, connection, HID_REPORT_INDEX,
				     report, sizeof(report), NULL);
	if (error != 0) {
		hids_window_drops++;
		hids_send_last_error = error;
	}
	latency_record(event->timestamp);
	bt_conn_unref(connection);
}

static int fastpath_enable(void)
{
	struct rtfw_hid_config config = {.enabled = 1U};
	struct rtfw_command command = {
		.id = RTFW_HID_COMMAND_CONFIGURE,
		.data_len = sizeof(config),
	};

	memcpy(command.data, &config, sizeof(config));
	return rtfw_submit(&command);
}

int main(void)
{
	const struct rtfw_config framework_config = {
		.command_handler = rtfw_hid_command_handler,
		.fastpath_handler = rtfw_hid_fastpath_handler,
		.pend_source_irq = rtfw_hid_pend_source_irq,
		.event_handler = rtfw_event_handler,
	};
	int error;

	printk("Starting event-driven RTFW HID/GPIOTE backend\n");
	error = rtfw_init(&framework_config);
	if (error != 0) {
		return error;
	}
	rtfw_hid_fastpath_init();

	error = hids_init();
	if (error != 0) {
		return error;
	}
	error = bt_enable(NULL);
	if (error != 0) {
		return error;
	}
	(void)settings_load();

	k_work_init(&advertising_work, advertising_handler);
	k_work_submit(&advertising_work);

	error = fastpath_enable();
	if (error != 0) {
		return error;
	}

	return 0;
}
