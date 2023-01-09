/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include "sdc_hci_vs.h"

#include "chmap_filter.h"

#define MODULE ble_qos
#include <caf/events/module_state_event.h>
#include "ble_event.h"
#include "config_event.h"
#include "hid_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_QOS_LOG_LEVEL);

#define INVALID_BLACKLIST 0xFFFF

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
# if DT_PROP(DT_NODELABEL(usbd), num_in_endpoints) < 4
# error Too few USB IN Endpoints enabled. \
	Modify appropriate dts.overlay to increase num-in-endpoints to 4 or more
# endif
#endif

#ifndef ROUNDED_DIV
#define ROUNDED_DIV(a, b)  (((a) + ((b) / 2)) / (b))
#endif /* ROUNDED_DIV */

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
#define THREAD_STACK_SIZE		     \
	(CONFIG_DESKTOP_BLE_QOS_STACK_SIZE + \
	 CONFIG_DESKTOP_BLE_QOS_STATS_PRINT_STACK_SIZE)
#else
#define THREAD_STACK_SIZE CONFIG_DESKTOP_BLE_QOS_STACK_SIZE
#endif

#define THREAD_PRIORITY K_PRIO_PREEMPT(K_LOWEST_APPLICATION_THREAD_PRIO)

#define MAX_KEY_LEN 20

static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

struct params_ble {
	uint16_t sample_count_min;
	uint8_t min_channel_count;
	int16_t weight_crc_ok;
	int16_t weight_crc_error;
	uint16_t ble_block_threshold;
	uint8_t eval_max_count;
	uint16_t eval_duration;
	uint16_t eval_keepout_duration;
	uint16_t eval_success_threshold;
} __packed;

struct params_wifi {
	int16_t wifi_rating_inc;
	int16_t wifi_present_threshold;
	int16_t wifi_active_threshold;
} __packed;

struct params_chmap {
	uint8_t chmap[CHMAP_BLE_BITMASK_SIZE];
} __packed;

struct params_blacklist {
	uint16_t wifi_chn_bitmask;
} __packed;

static uint8_t chmap_instance_buf[CHMAP_FILTER_INST_SIZE] __aligned(CHMAP_FILTER_INST_ALIGN);
static struct chmap_instance *chmap_inst;
static uint8_t current_chmap[CHMAP_BLE_BITMASK_SIZE] = CHMAP_BLE_BITMASK_DEFAULT;
static atomic_t processing;
static atomic_t new_blacklist;
static atomic_t params_updated;
static struct chmap_filter_params filter_params;
static struct k_mutex data_access_mutex;

BUILD_ASSERT(sizeof(struct bt_hci_cp_le_set_host_chan_classif) ==
	     sizeof(struct params_chmap));
BUILD_ASSERT(sizeof(current_chmap) == sizeof(struct params_chmap));
BUILD_ASSERT(THREAD_PRIORITY >= CONFIG_BT_HCI_TX_PRIO);

static void ble_qos_thread_fn(void);

static const struct device *const cdc_dev =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(ncs_ble_qos_uart));
static uint32_t cdc_dtr;

enum ble_qos_opt {
	BLE_QOS_OPT_BLACKLIST,
	BLE_QOS_OPT_CHMAP,
	BLE_QOS_OPT_PARAM_BLE,
	BLE_QOS_OPT_PARAM_WIFI,

	BLE_QOS_OPT_COUNT
};

static const char * const opt_descr[] = {
	[BLE_QOS_OPT_BLACKLIST] = "blacklist",
	[BLE_QOS_OPT_CHMAP] = "chmap",
	[BLE_QOS_OPT_PARAM_BLE]	= "param_ble",
	[BLE_QOS_OPT_PARAM_WIFI] = "param_wifi"
};


static void update_blacklist(const uint8_t *blacklist)
{
	atomic_set(&new_blacklist, sys_get_le16(blacklist));
}

static void update_parameters(const uint8_t *qos_ble_params,
			      const uint8_t *qos_wifi_params)
{
	size_t pos;

	k_mutex_lock(&data_access_mutex, K_FOREVER);

	if (qos_ble_params != NULL) {
		pos = 0;

		filter_params.maintenance_sample_count =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.maintenance_sample_count);

		filter_params.min_channel_count = qos_ble_params[pos];
		pos += sizeof(filter_params.min_channel_count);

		filter_params.ble_weight_crc_ok =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.ble_weight_crc_ok);

		filter_params.ble_weight_crc_error =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.ble_weight_crc_error);

		filter_params.ble_block_threshold =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.ble_block_threshold);

		filter_params.eval_max_count = qos_ble_params[pos];
		pos += sizeof(filter_params.eval_max_count);

		filter_params.eval_duration =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.eval_duration);

		filter_params.eval_keepout_duration =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.eval_keepout_duration);

		filter_params.eval_success_threshold =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(filter_params.eval_success_threshold);
	}

	if (qos_wifi_params != NULL) {
		pos = 0;

		filter_params.wifi_rating_inc =
			sys_get_le16(&qos_wifi_params[pos]);
		pos += sizeof(filter_params.wifi_rating_inc);

		filter_params.wifi_present_threshold =
			sys_get_le16(&qos_wifi_params[pos]);
		pos += sizeof(filter_params.wifi_present_threshold);

		filter_params.wifi_active_threshold =
			sys_get_le16(&qos_wifi_params[pos]);
		pos += sizeof(filter_params.wifi_active_threshold);
	}

	atomic_set(&params_updated, true);
	k_mutex_unlock(&data_access_mutex);
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, opt_descr[BLE_QOS_OPT_BLACKLIST])) {
		uint8_t data[sizeof(struct params_blacklist)];

		ssize_t len = read_cb(cb_arg, data, sizeof(data));

		if ((len != sizeof(data)) || (len != len_rd)) {
			LOG_ERR("Can't read option %s from storage",
				opt_descr[BLE_QOS_OPT_BLACKLIST]);
			return len;
		}

		update_blacklist(data);

	} else if (!strcmp(key, opt_descr[BLE_QOS_OPT_CHMAP])) {
		LOG_ERR("Chmap is not stored in settings");
		__ASSERT_NO_MSG(false);

	} else if (!strcmp(key, opt_descr[BLE_QOS_OPT_PARAM_BLE])) {
		uint8_t data[sizeof(struct params_ble)];

		ssize_t len = read_cb(cb_arg, data, sizeof(data));

		if ((len != sizeof(data)) || (len != len_rd)) {
			LOG_ERR("Can't read option %s from storage",
				opt_descr[BLE_QOS_OPT_PARAM_BLE]);
			return len;
		}

		update_parameters(data, NULL);

	} else if (!strcmp(key, opt_descr[BLE_QOS_OPT_PARAM_WIFI])) {
		uint8_t data[sizeof(struct params_wifi)];

		ssize_t len = read_cb(cb_arg, data, sizeof(data));

		if ((len != sizeof(data)) || (len != len_rd)) {
			LOG_ERR("Can't read option %s from storage",
				opt_descr[BLE_QOS_OPT_PARAM_WIFI]);
			return len;
		}

		update_parameters(NULL, data);
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(ble_qos, MODULE_NAME, NULL, settings_set, NULL,
			       NULL);

static void send_uart_data(const struct device *cdc_dev, const uint8_t *str, int str_len)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	int sent = uart_fifo_fill(cdc_dev, str, str_len);

	if (sent != str_len) {
		LOG_WRN("Sent %d of %d bytes", sent, str_len);
	}
#else
	/* uart_fifo_fill is not declared if CONFIG_UART_INTERRUPT_DRIVEN
	 * is disabled. send_uart_data should not be called in that case.
	 */
	ARG_UNUSED(cdc_dev);
	ARG_UNUSED(str);
	ARG_UNUSED(str_len);
	__ASSERT_NO_MSG(false);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

static void ble_chn_stats_print(bool update_channel_map)
{
	char str[64];
	int str_len, part_len;
	int16_t chn_rating;
	uint8_t chn_freq, chn_state;

	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE)) {
		return;
	}

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		int err;
		uint32_t cdc_val;

		/* Repeated to monitor CDC state */
		err = uart_line_ctrl_get(cdc_dev,
					 UART_LINE_CTRL_DTR,
					 &cdc_val);
		if (!err) {
			cdc_dtr = cdc_val;
		}

		if (!cdc_dtr) {
			return;
		}
	}

	/* Note: if UART FIFO overflows, expect a warning printout: */
	/* "<wrn> usb_cdc_acm: Ring buffer full (...)" */
	/* USB_CDC_ACM_RINGBUF_SIZE can be increased to prevent an overflow */

	if (update_channel_map) {
		str_len = snprintf(
			str,
			sizeof(str),
			"[%08" PRIu32 "]Channel map update\n",
			(uint32_t)k_cycle_get_32());
		if (str_len <= 0 || str_len > sizeof(str)) {
			LOG_ERR("Encoding error");
			return;
		}
		send_uart_data(cdc_dev, (uint8_t *)str, str_len);
	}

	/* Channel state information print format: */
	/* BT_INFO={idx0_freq:idx0_state:idx0_rating, ... } */

	str_len = snprintf(str, sizeof(str), "BT_INFO={");
	if (str_len <= 0 || str_len > sizeof(str)) {
		LOG_ERR("Encoding error");
		return;
	}
	send_uart_data(cdc_dev, (uint8_t *)str, str_len);

	str_len = 0;
	for (uint8_t i = 0; i < CHMAP_BLE_CHANNEL_COUNT; i++) {
		chmap_filter_chn_info_get(
			chmap_inst,
			i,
			&chn_state,
			&chn_rating,
			&chn_freq);
		part_len = snprintf(
			&str[str_len],
			sizeof(str) - str_len,
			"%" PRIu8 ":%" PRIu8 ":%" PRId16 ",",
			chn_freq,
			chn_state,
			chn_rating);
		str_len += part_len;
		if (part_len <= 0 || str_len > sizeof(str)) {
			LOG_ERR("Encoding error");
			return;
		}

		if (str_len >= ((sizeof(str) * 2) / 3)) {
			send_uart_data(cdc_dev, (uint8_t *)str, str_len);
			str_len = 0;
		}
	}
	part_len = snprintf(&str[str_len], sizeof(str) - str_len, "}\n");
	str_len += part_len;
	if (part_len <= 0 || str_len > sizeof(str)) {
		LOG_ERR("Encoding error");
		return;
	}
	send_uart_data(cdc_dev, (uint8_t *)str, str_len);
}

static void hid_pkt_stats_print(uint32_t ble_recv)
{
	static uint32_t prev_ble;
	static uint32_t prev_timestamp;
	uint32_t timestamp;
	uint32_t time_diff, rate_diff;
	uint32_t ble_rate;
	char str[64];
	int str_len;

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		if (!cdc_dtr) {
			return;
		}
	}

	timestamp = k_cycle_get_32();
	if (timestamp == prev_timestamp) {
		LOG_WRN("Too high printout rate");
		return;
	}

	time_diff = prev_timestamp < timestamp ?
		    (timestamp - prev_timestamp) :
		    (UINT32_MAX - prev_timestamp + timestamp);
	prev_timestamp = timestamp;
	rate_diff = prev_ble <= ble_recv ?
		    (ble_recv - prev_ble) :
		    (UINT32_MAX - prev_ble + ble_recv);
	prev_ble = ble_recv;
	ble_rate = ROUNDED_DIV(
		(rate_diff * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC),
		time_diff);

	str_len = snprintf(
		str,
		sizeof(str),
		"[%08" PRIu32 "]Rate:%04" PRIu32 "\n",
		timestamp,
		ble_rate);
	if (str_len <= 0 || str_len > sizeof(str)) {
		LOG_ERR("Encoding error");
		return;
	}
	send_uart_data(cdc_dev, (uint8_t *)str, str_len);
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t *subevent_code;
	sdc_hci_subevent_vs_qos_conn_event_report_t *evt;

	subevent_code = net_buf_simple_pull_mem(
		buf,
		sizeof(*subevent_code));

	switch (*subevent_code) {
	case SDC_HCI_SUBEVENT_VS_QOS_CONN_EVENT_REPORT:
		if (atomic_get(&processing)) {
			/* Cheaper to skip this update */
			/* instead of using locks */
			return true;
		}

		evt = (void *)buf->data;

		chmap_filter_crc_update(
			chmap_inst,
			evt->channel_index,
			evt->crc_ok_count,
			evt->crc_error_count);
		return true;
	default:
		return false;
	}
}

static void enable_qos_reporting(void)
{
	int err;
	struct net_buf *buf;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		LOG_ERR("Failed to register HCI VS callback");
		return;
	}

	sdc_hci_cmd_vs_qos_conn_event_report_enable_t *cmd_enable;

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_QOS_CONN_EVENT_REPORT_ENABLE,
				sizeof(*cmd_enable));
	if (!buf) {
		LOG_ERR("Failed to enable HCI VS QoS");
		return;
	}

	cmd_enable = net_buf_add(buf, sizeof(*cmd_enable));
	cmd_enable->enable = 1;

	err = bt_hci_cmd_send_sync(
		SDC_HCI_OPCODE_CMD_VS_QOS_CONN_EVENT_REPORT_ENABLE, buf, NULL);
	if (err) {
		LOG_ERR("Failed to enable HCI VS QoS");
		return;
	}
}

static void store_config(const uint8_t opt_id, const uint8_t *data,
			 const size_t data_size)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[MAX_KEY_LEN];

		int err = snprintk(key, sizeof(key), MODULE_NAME "/%s",
				   opt_descr[opt_id]);

		if ((err > 0) && (err < MAX_KEY_LEN)) {
			err = settings_save_one(key, data, data_size);
		}

		if (err) {
			LOG_ERR("Problem storing %s (err = %d)",
				opt_descr[opt_id], err);
		}
	}
}

static void update_config(const uint8_t opt_id, const uint8_t *data,
			  const size_t size)
{
	switch (opt_id) {
	case BLE_QOS_OPT_BLACKLIST:
		if (size != sizeof(struct params_blacklist)) {
			LOG_WRN("Invalid size");
		} else {
			update_blacklist(data);
			store_config(BLE_QOS_OPT_BLACKLIST, data, size);
		}
		break;

	case BLE_QOS_OPT_CHMAP:
		/* Avoid updating channel map directly to */
		/* reduce complexity in interaction with */
		/* chmap filter lib */
		LOG_WRN("Not supported");
		break;

	case BLE_QOS_OPT_PARAM_BLE:
		if (size != sizeof(struct params_ble)) {
			LOG_WRN("Invalid size");
		} else {
			update_parameters(data, NULL);
			store_config(BLE_QOS_OPT_PARAM_BLE, data, size);
		}
		break;

	case BLE_QOS_OPT_PARAM_WIFI:
		if (size != sizeof(struct params_wifi)) {
			LOG_WRN("Invalid size");
		} else {
			update_parameters(NULL, data);
			store_config(BLE_QOS_OPT_PARAM_WIFI, data, size);
		}
		break;

	default:
		LOG_WRN("Unknown opt %" PRIu8, opt_id);
		return;
	}
}

static void fill_qos_ble_params(uint8_t *data, size_t *size)
{
	struct chmap_filter_params chmap_params;
	size_t pos = 0;

	chmap_filter_params_get(chmap_inst, &chmap_params);

	sys_put_le16(chmap_params.maintenance_sample_count, &data[pos]);
	pos += sizeof(chmap_params.maintenance_sample_count);

	data[pos] = chmap_params.min_channel_count;
	pos += sizeof(chmap_params.min_channel_count);

	sys_put_le16(chmap_params.ble_weight_crc_ok, &data[pos]);
	pos += sizeof(chmap_params.ble_weight_crc_ok);

	sys_put_le16(chmap_params.ble_weight_crc_error, &data[pos]);
	pos += sizeof(chmap_params.ble_weight_crc_error);

	sys_put_le16(chmap_params.ble_block_threshold, &data[pos]);
	pos += sizeof(chmap_params.ble_block_threshold);

	data[pos] = chmap_params.eval_max_count;
	pos += sizeof(chmap_params.eval_max_count);

	sys_put_le16(chmap_params.eval_duration, &data[pos]);
	pos += sizeof(chmap_params.eval_duration);

	sys_put_le16(chmap_params.eval_keepout_duration, &data[pos]);
	pos += sizeof(chmap_params.eval_keepout_duration);

	sys_put_le16(chmap_params.eval_success_threshold, &data[pos]);
	pos += sizeof(chmap_params.eval_success_threshold);

	*size = pos;
}

static void fill_qos_wifi_params(uint8_t *data, size_t *size)
{
	struct chmap_filter_params chmap_params;
	size_t pos = 0;

	chmap_filter_params_get(chmap_inst, &chmap_params);

	sys_put_le16(chmap_params.wifi_rating_inc, &data[pos]);
	pos += sizeof(chmap_params.wifi_rating_inc);

	sys_put_le16(chmap_params.wifi_present_threshold, &data[pos]);
	pos += sizeof(chmap_params.wifi_present_threshold);

	sys_put_le16(chmap_params.wifi_active_threshold, &data[pos]);
	pos += sizeof(chmap_params.wifi_active_threshold);

	*size = pos;
}

static void fill_qos_channel_map(uint8_t *data, size_t *size)
{
	struct params_chmap qos_chmap;

	k_mutex_lock(&data_access_mutex, K_FOREVER);
	memcpy(data, current_chmap, sizeof(qos_chmap));
	k_mutex_unlock(&data_access_mutex);

	*size = sizeof(qos_chmap);
}

static void fill_qos_blacklist(uint8_t *data, size_t *size)
{
	sys_put_le16(chmap_filter_wifi_blacklist_get(), data);

	*size = sizeof(struct params_blacklist);
}

static void fetch_config(const uint8_t opt_id, uint8_t *data, size_t *size)
{
	switch (opt_id) {
	case BLE_QOS_OPT_BLACKLIST:
		fill_qos_blacklist(data, size);
		break;

	case BLE_QOS_OPT_CHMAP:
		fill_qos_channel_map(data, size);
		break;

	case BLE_QOS_OPT_PARAM_BLE:
		fill_qos_ble_params(data, size);
		break;

	case BLE_QOS_OPT_PARAM_WIFI:
		fill_qos_wifi_params(data, size);
		break;

	default:
		LOG_WRN("Unknown opt: %" PRIu8, opt_id);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE) &&
	    IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		static int32_t hid_pkt_recv_count;
		static uint32_t cdc_notify_count;

		/* Count number of HID packets received via BLE. */
		/* Send stats printout via CDC every 100 packets. */

		if (is_hid_report_event(aeh)) {
			const struct hid_report_event *event = cast_hid_report_event(aeh);

			/* Ignore HID output reports. */
			if (!event->subscriber) {
				return false;
			}

			hid_pkt_recv_count++;
			cdc_notify_count++;

			if (cdc_notify_count == 100) {
				hid_pkt_stats_print(hid_pkt_recv_count);
				cdc_notify_count = 0;
			}

			return false;
		}
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(
			    event,
			    MODULE_ID(main),
			    MODULE_STATE_READY)) {
			static bool initialized;
			int err;

			__ASSERT_NO_MSG(!initialized);

			initialized = true;

			chmap_filter_init();

			chmap_inst =
				(struct chmap_instance *) chmap_instance_buf;
			err = chmap_filter_instance_init(
				chmap_inst,
				sizeof(chmap_instance_buf));
			if (err) {
				LOG_ERR("Failed to initialize filter");
				module_set_state(MODULE_STATE_ERROR);
				return false;
			}

			LOG_DBG("Chmap lib version: %s",
				chmap_filter_version());

			chmap_filter_params_get(chmap_inst, &filter_params);

			k_mutex_init(&data_access_mutex);
			new_blacklist = INVALID_BLACKLIST;
			atomic_set(&params_updated, false);

			if (IS_ENABLED(CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE)) {
				__ASSERT_NO_MSG(device_is_ready(cdc_dev));
				/* CONFIG_UART_LINE_CTRL == 1: dynamic dtr */
				cdc_dtr = !IS_ENABLED(CONFIG_UART_LINE_CTRL);
			}

			k_thread_create(&thread, thread_stack,
					THREAD_STACK_SIZE,
					(k_thread_entry_t)ble_qos_thread_fn,
					NULL, NULL, NULL,
					THREAD_PRIORITY, 0, K_NO_WAIT);
			k_thread_name_set(&thread, MODULE_NAME "_thread");
		}

		if (check_state(
			    event,
			    MODULE_ID(ble_state),
			    MODULE_STATE_READY)) {
			enable_qos_reporting();
		}

		return false;
	}

	GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr, update_config,
				  fetch_config);

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

static void apply_new_params(void)
{
	int err;

	k_mutex_lock(&data_access_mutex, K_FOREVER);
	/* chmap_filter_params_set returns immediately */
	err = chmap_filter_params_set(chmap_inst, &filter_params);
	atomic_set(&params_updated, false);
	k_mutex_unlock(&data_access_mutex);

	if (err) {
		LOG_WRN("Param update failed");
	}
}

static void ble_qos_thread_fn(void)
{
	while (true) {
		bool update_channel_map;
		int err;

		k_sleep(K_MSEC(CONFIG_DESKTOP_BLE_QOS_INTERVAL));

		/* Check and apply new parameters received via config channel */
		if (atomic_get(&params_updated)) {
			apply_new_params();
		}

		/* Check and apply new blacklist received via config channel */
		uint16_t blacklist_update =
			(uint16_t) atomic_set(&new_blacklist, INVALID_BLACKLIST);

		if (blacklist_update != INVALID_BLACKLIST) {
			err = chmap_filter_blacklist_set(
				chmap_inst,
				blacklist_update);
			if (err) {
				LOG_WRN("Blacklist update failed");
			}
		}

		/* Run processing function. */
		/* Atomic variable is used as data busy flag */
		/* (this thread runs at the lowest priority) */
		atomic_set(&processing, true);
		update_channel_map = chmap_filter_process(chmap_inst);
		atomic_set(&processing, false);

		ble_chn_stats_print(update_channel_map);

		if (!update_channel_map) {
			continue;
		}

		uint8_t *chmap;

		chmap = chmap_filter_suggested_map_get(chmap_inst);

		struct ble_qos_event *event = new_ble_qos_event();
		BUILD_ASSERT(sizeof(event->chmap) == CHMAP_BLE_BITMASK_SIZE, "");
		memcpy(event->chmap, chmap, CHMAP_BLE_BITMASK_SIZE);
		APP_EVENT_SUBMIT(event);

		if (IS_ENABLED(CONFIG_DESKTOP_BT_CENTRAL)) {
			err = bt_le_set_chan_map(chmap);
			if (err) {
				LOG_WRN("bt_le_set_chan_map: %d", err);
			} else {
				LOG_DBG("Channel map update");
			}
		}

		chmap_filter_suggested_map_confirm(chmap_inst);
		k_mutex_lock(&data_access_mutex, K_FOREVER);
		memcpy(current_chmap, chmap, sizeof(current_chmap));
		k_mutex_unlock(&data_access_mutex);
	}
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
#endif
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
