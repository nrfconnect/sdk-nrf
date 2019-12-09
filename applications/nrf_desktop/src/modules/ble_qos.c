/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <stdio.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/uart.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>

#include "ble_controller_hci_vs.h"

#include "chmap_filter.h"

#define MODULE ble_qos
#include "module_state_event.h"
#include "ble_event.h"
#include "config_event.h"
#include "hid_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_QOS_LOG_LEVEL);

#define INVALID_BLACKLIST 0xFFFF

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
# if DT_NORDIC_NRF_USBD_USBD_0_NUM_IN_ENDPOINTS < 4
# error Too few USB IN Endpoints enabled. \
	Modify appropriate dts.overlay to increase num-in-endpoints to 4 or more
# endif
#endif

#ifdef CONFIG_USB_CDC_ACM_DEVICE_NAME
#define USB_SERIAL_DEVICE_NAME CONFIG_USB_CDC_ACM_DEVICE_NAME
#else
#define USB_SERIAL_DEVICE_NAME  ""
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

static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

struct params_ble {
	u16_t sample_count_min;
	u8_t min_channel_count;
	s16_t weight_crc_ok;
	s16_t weight_crc_error;
	u16_t ble_block_threshold;
	u8_t eval_max_count;
	u16_t eval_duration;
	u16_t eval_keepout_duration;
	u16_t eval_success_threshold;
} __packed;

struct params_wifi {
	s16_t wifi_rating_inc;
	s16_t wifi_present_threshold;
	s16_t wifi_active_threshold;
} __packed;

struct params_chmap {
	u8_t chmap[CHMAP_BLE_BITMASK_SIZE];
} __packed;

struct params_blacklist {
	u16_t wifi_chn_bitmask;
} __packed;

static u8_t chmap_instance_buf[CHMAP_FILTER_INST_SIZE];
static struct chmap_instance *chmap_inst;
static u8_t current_chmap[CHMAP_BLE_BITMASK_SIZE] = CHMAP_BLE_BITMASK_DEFAULT;
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

static struct device *cdc_dev;
static u32_t cdc_dtr;

static void ble_chn_stats_print(bool update_channel_map)
{
	char str[64];
	int str_len, part_len;
	s16_t chn_rating;
	u8_t chn_freq, chn_state;

	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE)) {
		return;
	}

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		int err;
		u32_t cdc_val;

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
			(u32_t)k_cycle_get_32());
		if (str_len <= 0 || str_len > sizeof(str)) {
			LOG_ERR("Encoding error");
			return;
		}
		uart_fifo_fill(cdc_dev, str, str_len);
	}

	/* Channel state information print format: */
	/* BT_INFO={idx0_freq:idx0_state:idx0_rating, ... } */

	str_len = snprintf(str, sizeof(str), "BT_INFO={");
	if (str_len <= 0 || str_len > sizeof(str)) {
		LOG_ERR("Encoding error");
		return;
	}
	uart_fifo_fill(cdc_dev, str, str_len);

	str_len = 0;
	for (u8_t i = 0; i < CHMAP_BLE_CHANNEL_COUNT; i++) {
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
			uart_fifo_fill(cdc_dev, str, str_len);
			str_len = 0;
		}
	}
	part_len = snprintf(&str[str_len], sizeof(str) - str_len, "}\n");
	str_len += part_len;
	if (part_len <= 0 || str_len > sizeof(str)) {
		LOG_ERR("Encoding error");
		return;
	}
	uart_fifo_fill(cdc_dev, str, str_len);
}

static void hid_pkt_stats_print(u32_t ble_recv)
{
	static u32_t prev_ble;
	static u32_t prev_timestamp;
	u32_t timestamp;
	u32_t time_diff, rate_diff;
	u32_t ble_rate;
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
	uart_fifo_fill(cdc_dev, str, str_len);
}

static bool is_my_config_id(u8_t config_id)
{
	return (GROUP_FIELD_GET(config_id) == EVENT_GROUP_SETUP) &&
	       (MOD_FIELD_GET(config_id) == SETUP_MODULE_QOS);
}

static struct config_fetch_event *fetch_event_qos_ble_params(void)
{
	struct chmap_filter_params chmap_params;
	struct params_ble qos_ble_params;
	struct config_fetch_event *fetch_event;
	size_t pos = 0;

	fetch_event = new_config_fetch_event(sizeof(qos_ble_params));

	chmap_filter_params_get(chmap_inst, &chmap_params);

	sys_put_le16(chmap_params.maintenance_sample_count,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.sample_count_min);

	fetch_event->dyndata.data[pos] = chmap_params.min_channel_count;
	pos += sizeof(qos_ble_params.min_channel_count);

	sys_put_le16(chmap_params.ble_weight_crc_ok,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.weight_crc_ok);

	sys_put_le16(chmap_params.ble_weight_crc_error,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.weight_crc_error);

	sys_put_le16(chmap_params.ble_block_threshold,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.ble_block_threshold);

	fetch_event->dyndata.data[pos] = chmap_params.eval_max_count;
	pos += sizeof(qos_ble_params.eval_max_count);

	sys_put_le16(chmap_params.eval_duration,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.eval_duration);

	sys_put_le16(chmap_params.eval_keepout_duration,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.eval_keepout_duration);

	sys_put_le16(chmap_params.eval_success_threshold,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_ble_params.eval_success_threshold);

	return fetch_event;
}

static struct config_fetch_event *fetch_event_qos_wifi_params(void)
{
	struct chmap_filter_params chmap_params;
	struct params_wifi qos_wifi_params;
	struct config_fetch_event *fetch_event;
	size_t pos = 0;

	fetch_event = new_config_fetch_event(sizeof(qos_wifi_params));

	chmap_filter_params_get(chmap_inst, &chmap_params);

	sys_put_le16(chmap_params.wifi_rating_inc,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_wifi_params.wifi_rating_inc);

	sys_put_le16(chmap_params.wifi_present_threshold,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_wifi_params.wifi_present_threshold);

	sys_put_le16(chmap_params.wifi_active_threshold,
		     &fetch_event->dyndata.data[pos]);
	pos += sizeof(qos_wifi_params.wifi_active_threshold);

	return fetch_event;
}

static struct config_fetch_event *fetch_event_qos_channel_map(void)
{
	struct config_fetch_event *fetch_event;
	struct params_chmap qos_chmap;

	fetch_event = new_config_fetch_event(sizeof(qos_chmap));

	k_mutex_lock(&data_access_mutex, K_FOREVER);
	memcpy(fetch_event->dyndata.data, current_chmap, sizeof(qos_chmap));
	k_mutex_unlock(&data_access_mutex);

	return fetch_event;
}

static struct config_fetch_event *fetch_event_qos_blacklist(void)
{
	struct config_fetch_event *fetch_event;

	fetch_event = new_config_fetch_event(sizeof(struct params_blacklist));

	sys_put_le16(chmap_filter_wifi_blacklist_get(),
		     fetch_event->dyndata.data);

	return fetch_event;
}

static void update_parameters(
	const u8_t *qos_ble_params,
	const u8_t *qos_wifi_params)
{
	struct params_ble ble_params;
	struct params_wifi wifi_params;
	size_t pos;

	k_mutex_lock(&data_access_mutex, K_FOREVER);

	if (qos_ble_params != NULL) {
		pos = 0;

		filter_params.maintenance_sample_count =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.sample_count_min);

		filter_params.min_channel_count = qos_ble_params[pos];
		pos += sizeof(ble_params.min_channel_count);

		filter_params.ble_weight_crc_ok =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.weight_crc_ok);

		filter_params.ble_weight_crc_error =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.weight_crc_error);

		filter_params.ble_block_threshold =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.ble_block_threshold);

		filter_params.eval_max_count = qos_ble_params[pos];
		pos += sizeof(ble_params.eval_max_count);

		filter_params.eval_duration =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.eval_duration);

		filter_params.eval_keepout_duration =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.eval_keepout_duration);

		filter_params.eval_success_threshold =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(ble_params.eval_success_threshold);
	}
	if (qos_wifi_params != NULL) {
		pos = 0;

		filter_params.wifi_rating_inc =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(wifi_params.wifi_rating_inc);

		filter_params.wifi_present_threshold =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(wifi_params.wifi_present_threshold);

		filter_params.wifi_active_threshold =
			sys_get_le16(&qos_ble_params[pos]);
		pos += sizeof(wifi_params.wifi_active_threshold);
	}

	atomic_set(&params_updated, true);
	k_mutex_unlock(&data_access_mutex);
}

static void update_blacklist(const u8_t *blacklist)
{
	atomic_set(&new_blacklist, sys_get_le16(blacklist));
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	u8_t *subevent_code;
	hci_vs_evt_qos_conn_event_report_t *evt;

	subevent_code = net_buf_simple_pull_mem(
		buf,
		sizeof(*subevent_code));

	switch (*subevent_code) {
	case HCI_VS_SUBEVENT_CODE_QOS_CONN_EVENT_REPORT:
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

	hci_vs_cmd_qos_conn_event_report_enable_t *cmd_enable;

	buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_QOS_CONN_EVENT_REPORT_ENABLE,
				sizeof(*cmd_enable));
	if (!buf) {
		LOG_ERR("Failed to enable HCI VS QoS");
		return;
	}

	cmd_enable = net_buf_add(buf, sizeof(*cmd_enable));
	cmd_enable->enable = 1;

	err = bt_hci_cmd_send_sync(
		HCI_VS_OPCODE_CMD_QOS_CONN_EVENT_REPORT_ENABLE, buf, NULL);
	if (err) {
		LOG_ERR("Failed to enable HCI VS QoS");
		return;
	}
}

static void handle_config_event(const struct config_event *event)
{
	switch (OPT_FIELD_GET(event->id)) {
	case QOS_OPT_BLACKLIST:
		if (event->dyndata.size != sizeof(struct params_blacklist)) {
			LOG_WRN("Invalid size");
		} else {
			update_blacklist(event->dyndata.data);
		}
		break;
	case QOS_OPT_CHMAP:
		/* Avoid updating channel map directly to */
		/* reduce complexity in interaction with */
		/* chmap filter lib */
		LOG_WRN("Not supported");
		break;
	case QOS_OPT_PARAM_BLE:
		if (event->dyndata.size != sizeof(struct params_ble)) {
			LOG_WRN("Invalid size");
		} else {
			update_parameters(event->dyndata.data, NULL);
		}
		break;
	case QOS_OPT_PARAM_WIFI:
		if (event->dyndata.size != sizeof(struct params_wifi)) {
			LOG_WRN("Invalid size");
		} else {
			update_parameters(NULL, event->dyndata.data);
		}
		break;
	default:
		LOG_WRN("Unknown opt");
		return;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE) &&
	    IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		static s32_t hid_pkt_recv_count;
		static u32_t cdc_notify_count;

		/* Count number of HID packets received via BLE. */
		/* Send stats printout via CDC every 100 packets. */

		if (is_hid_mouse_event(eh)) {
			hid_pkt_recv_count++;
			cdc_notify_count++;

			if (cdc_notify_count == 100) {
				hid_pkt_stats_print(
					hid_pkt_recv_count);
				cdc_notify_count = 0;
			}

			return false;
		}
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

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
				cdc_dev = device_get_binding(
					USB_SERIAL_DEVICE_NAME "_0");
				__ASSERT_NO_MSG(cdc_dev != NULL);
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

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(eh)) {
			const struct config_event *event =
				cast_config_event(eh);

			if (is_my_config_id(event->id)) {
				handle_config_event(event);
			}

			return false;
		}

		if (is_config_fetch_request_event(eh)) {
			const struct config_fetch_request_event *event =
				cast_config_fetch_request_event(eh);

			if (is_my_config_id(event->id)) {
				struct config_fetch_event *fetch_event;

				switch (OPT_FIELD_GET(event->id)) {
				case QOS_OPT_BLACKLIST:
					fetch_event =
						fetch_event_qos_blacklist();
					break;
				case QOS_OPT_CHMAP:
					fetch_event =
						fetch_event_qos_channel_map();
					break;
				case QOS_OPT_PARAM_BLE:
					fetch_event =
						fetch_event_qos_ble_params();
					break;
				case QOS_OPT_PARAM_WIFI:
					fetch_event =
						fetch_event_qos_wifi_params();
					break;
				default:
					LOG_WRN("Unknown opt");
					return false;
				}

				fetch_event->id = event->id;
				fetch_event->recipient = event->recipient;
				fetch_event->channel_id = event->channel_id;

				EVENT_SUBMIT(fetch_event);
			}

			return false;
		}
	}

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

		k_sleep(CONFIG_DESKTOP_BLE_QOS_INTERVAL);

		/* Check and apply new parameters received via config channel */
		if (atomic_get(&params_updated)) {
			apply_new_params();
		}

		/* Check and apply new blacklist received via config channel */
		u16_t blacklist_update =
			(u16_t) atomic_set(&new_blacklist, INVALID_BLACKLIST);

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

		u8_t *chmap;

		chmap = chmap_filter_suggested_map_get(chmap_inst);
		err = bt_le_set_chan_map(chmap);
		if (err) {
			LOG_WRN("bt_le_set_chan_map: %d", err);
		} else {
			LOG_DBG("Channel map update");
			chmap_filter_suggested_map_confirm(chmap_inst);

			k_mutex_lock(&data_access_mutex, K_FOREVER);
			memcpy(current_chmap, chmap, sizeof(current_chmap));
			k_mutex_unlock(&data_access_mutex);
		}
	}
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
#endif
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
#endif
