/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/uart.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#ifdef CONFIG_BT_LL_NRFXLIB
#include "ble_controller_hci_vs.h"
#else
#error NRFXLIB is required for channel map management
#endif

#include "chmap_filter.h"

#define MODULE ble_qos
#include "module_state_event.h"
#include "ble_event.h"
#include "config_event.h"
#include "hid_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_QOS_LOG_LEVEL);

K_SEM_DEFINE(sem_initialized, 0, 1);

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
	u8_t chmap[5];
} __packed;

struct params_blacklist {
	u16_t wifi_chn_bitmask;
} __packed;

static struct chmap_instance *mp_chmap_inst;
static u8_t m_current_chmap[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
static atomic_t m_processing;
static atomic_t m_new_blacklist;
static struct chmap_filter_params *m_new_params;
static struct k_mutex m_data_access_mutex;

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
# ifndef ROUNDED_DIV
# define ROUNDED_DIV(a, b)  (((a) + ((b) / 2)) / (b))
# endif /* ROUNDED_DIV */
# if DT_NORDIC_NRF_USBD_USBD_0_NUM_IN_ENDPOINTS < 4
# error Too few USB IN Endpoints enabled.\
Modify appropriate dts.overlay to increase num-in-endpoints to 4 or more
# endif
struct device *cdc_dev;
static u32_t cdc_dtr;

static void ble_chn_stats_print(bool update_channel_map)
{
	char str[64];
	int str_len, sent_len;
	s16_t chn_rating;
	u8_t chn_freq, chn_state;
	bool buf_loss = false;

# if CONFIG_UART_LINE_CTRL
	/* Repeated to monitor CDC state */
	uart_line_ctrl_get(cdc_dev, LINE_CTRL_DTR, &cdc_dtr);
# endif

	if (!cdc_dtr) {
		return;
	}

	if (update_channel_map) {
		str_len = snprintf(
			str,
			sizeof(str),
			"[%08u]Channel map update\n",
			(u32_t)k_cycle_get_32());
		sent_len = uart_fifo_fill(cdc_dev, str, str_len);
		buf_loss |= sent_len < str_len ? true : false;
	}

	/* Channel state information print format: */
	/* BT_INFO={idx0_freq:idx0_state:idx0_rating, ... } */

	str_len = snprintf(str, sizeof(str), "BT_INFO={");
	sent_len = uart_fifo_fill(cdc_dev, str, str_len);
	buf_loss |= sent_len < str_len ? true : false;

	for (int i = 0; i < 37; ++i) {
		chmap_filter_chn_info_get(
			mp_chmap_inst,
			i,
			&chn_state,
			&chn_rating,
			&chn_freq);
		str_len += snprintf(
						&str[str_len],
						sizeof(str)-str_len,
						"%u:%u:%d,",
						chn_freq,
						chn_state,
						chn_rating);
		if (str_len >= ((sizeof(str)*2)/3)) {
			sent_len = uart_fifo_fill(cdc_dev, str, str_len);
			buf_loss |= sent_len < str_len ? true : false;
			str_len = 0;
		}
	}
	str_len += snprintf(&str[str_len], sizeof(str)-str_len, "}\n");
	sent_len = uart_fifo_fill(cdc_dev, str, str_len);
	buf_loss |= sent_len < str_len ? true : false;

	if (buf_loss) {
		/* Increase USB_CDC_ACM_RINGBUF_SIZE to prevent buffer loss */
		LOG_WRN("CDC transmit buffer loss");
	}
}

static void hid_pkt_stats_print(u32_t ble_recv, u32_t usb_sent)
{
	static u32_t m_prev_ble;
	static u32_t m_prev_usb;
	static u32_t m_prev_timestamp;
	u32_t timestamp;
	u32_t time_diff;
	u32_t ble_rate;
	u32_t usb_rate;
	char str[64];
	int str_len, sent_len;

	if (!cdc_dtr) {
		return;
	}

	timestamp = k_cycle_get_32();
	time_diff = timestamp - m_prev_timestamp;
	m_prev_timestamp = timestamp;
	ble_rate = ROUNDED_DIV(
		((ble_recv - m_prev_ble) * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC),
		time_diff);
	usb_rate = ROUNDED_DIV(
		((usb_sent - m_prev_usb) * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC),
		time_diff);
	m_prev_ble = ble_recv;
	m_prev_usb = usb_sent;

	/* Printout format: */
	/* [Timestamp]Rate:ble_receive_rate;usb_transmit_rate [Hz]*/

	str_len = snprintf(str, sizeof(str), "[%08u]Rate:%04u;%04u\n",
		timestamp, ble_rate, usb_rate);
	sent_len = uart_fifo_fill(cdc_dev, str, str_len);

	if (sent_len != str_len) {
		/* Increase USB_CDC_ACM_RINGBUF_SIZE to prevent buffer loss */
		LOG_WRN("CDC transmit buffer loss");
	}
}
#endif

static bool is_my_config_id(u8_t config_id)
{
	return (GROUP_FIELD_GET(config_id) == EVENT_GROUP_SETUP) &&
	       (MOD_FIELD_GET(config_id) == SETUP_MODULE_QOS);
}

static struct config_fetch_event *fetch_event_qos_ble_params(void)
{
	struct chmap_filter_params chmap_params;
	struct config_fetch_event *fetch_event;
	struct params_ble *qos_ble_params;

	fetch_event = new_config_fetch_event(sizeof(struct params_ble));
	__ASSERT_NO_MSG(fetch_event != NULL);
	qos_ble_params = (struct params_ble *) fetch_event->dyndata.data;

	chmap_filter_params_get(mp_chmap_inst, &chmap_params);

	qos_ble_params->min_channel_count = chmap_params.min_channel_count;
	qos_ble_params->weight_crc_ok = chmap_params.ble_weight_crc_ok;
	qos_ble_params->weight_crc_error = chmap_params.ble_weight_crc_error;
	qos_ble_params->ble_block_threshold = chmap_params.ble_block_threshold;
	qos_ble_params->eval_max_count = chmap_params.eval_max_count;
	qos_ble_params->eval_duration = chmap_params.eval_duration;
	qos_ble_params->sample_count_min =
		chmap_params.maintenance_sample_count;
	qos_ble_params->eval_keepout_duration =
		chmap_params.eval_keepout_duration;
	qos_ble_params->eval_success_threshold =
		chmap_params.eval_success_threshold;

	return fetch_event;
}

static struct config_fetch_event *fetch_event_qos_wifi_params(void)
{
	struct chmap_filter_params chmap_params;
	struct config_fetch_event *fetch_event;
	struct params_wifi *qos_wifi_params;

	fetch_event = new_config_fetch_event(sizeof(struct params_wifi));
	__ASSERT_NO_MSG(fetch_event != NULL);
	qos_wifi_params = (struct params_wifi *) fetch_event->dyndata.data;

	chmap_filter_params_get(mp_chmap_inst, &chmap_params);

	qos_wifi_params->wifi_rating_inc = chmap_params.wifi_rating_inc;
	qos_wifi_params->wifi_active_threshold =
		chmap_params.wifi_active_threshold;
	qos_wifi_params->wifi_present_threshold =
		chmap_params.wifi_present_threshold;

	return fetch_event;
}

static struct config_fetch_event *fetch_event_qos_channel_map(void)
{
	struct config_fetch_event *fetch_event;
	struct params_chmap *qos_chmap;

	fetch_event = new_config_fetch_event(sizeof(struct params_chmap));
	__ASSERT_NO_MSG(fetch_event != NULL);
	qos_chmap = (struct params_chmap *) fetch_event->dyndata.data;

	k_mutex_lock(&m_data_access_mutex, K_FOREVER);
	memcpy(qos_chmap->chmap, m_current_chmap, sizeof(m_current_chmap));
	k_mutex_unlock(&m_data_access_mutex);

	return fetch_event;
}

static struct config_fetch_event *fetch_event_qos_blacklist(void)
{
	struct config_fetch_event *fetch_event;
	struct params_blacklist *qos_blacklist;

	fetch_event = new_config_fetch_event(sizeof(struct params_blacklist));
	__ASSERT_NO_MSG(fetch_event != NULL);
	qos_blacklist = (struct params_blacklist *) fetch_event->dyndata.data;
	memset(qos_blacklist, 0, sizeof(struct params_blacklist));

	qos_blacklist->wifi_chn_bitmask = chmap_filter_wifi_blacklist_get();

	return fetch_event;
}

static void update_parameters(
	struct params_ble *p_qos_ble_params,
	struct params_wifi *p_qos_wifi_params)
{
	struct chmap_filter_params *p_params;

	p_params = k_malloc(sizeof(struct chmap_filter_params));
	if (p_params == NULL) {
		LOG_ERR("OOM error");
		return;
	}

	chmap_filter_params_get(mp_chmap_inst, p_params);

	if (p_qos_ble_params != NULL) {
		p_params->min_channel_count =
			p_qos_ble_params->min_channel_count;
		p_params->ble_weight_crc_ok =
			p_qos_ble_params->weight_crc_ok;
		p_params->ble_weight_crc_error =
			p_qos_ble_params->weight_crc_error;
		p_params->ble_block_threshold =
			p_qos_ble_params->ble_block_threshold;
		p_params->eval_max_count =
			p_qos_ble_params->eval_max_count;
		p_params->eval_duration =
			p_qos_ble_params->eval_duration;
		p_params->maintenance_sample_count =
			p_qos_ble_params->sample_count_min;
		p_params->eval_keepout_duration =
			p_qos_ble_params->eval_keepout_duration;
		p_params->eval_success_threshold =
			p_qos_ble_params->eval_success_threshold;
	}
	if (p_qos_wifi_params != NULL) {
		p_params->wifi_rating_inc =
			p_qos_wifi_params->wifi_rating_inc;
		p_params->wifi_present_threshold =
			p_qos_wifi_params->wifi_present_threshold;
		p_params->wifi_active_threshold =
			p_qos_wifi_params->wifi_active_threshold;
	}

	k_mutex_lock(&m_data_access_mutex, K_FOREVER);
	if (m_new_params != NULL) {
		k_free(m_new_params);
	}
	m_new_params = p_params;
	k_mutex_unlock(&m_data_access_mutex);
}

static void update_blacklist(struct params_blacklist *p_blacklist)
{
	atomic_set(&m_new_blacklist, p_blacklist->wifi_chn_bitmask);
}

static int hci_chmap_cmd_send(const u8_t *p_chmap)
{
	struct bt_hci_cp_le_set_host_chan_classif *cp;
	struct net_buf *buf;

	__ASSERT_NO_MSG(p_chmap != NULL);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF,
				sizeof(*cp));
	if (!buf) {
		return -1;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	if (!cp) {
		return -1;
	}

	memcpy(cp->ch_map, p_chmap, sizeof(cp->ch_map));
	__ASSERT_NO_MSG(sizeof(cp->ch_map) == 5);

	return bt_hci_cmd_send_sync(
			BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF,
			buf,
			NULL);
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	u8_t *p_subevent_code;
	hci_vs_evt_qos_conn_event_report_t *p_evt;

	p_subevent_code = net_buf_simple_pull_mem(
						buf,
						sizeof(*p_subevent_code));

	switch (*p_subevent_code) {
	case HCI_VS_SUBEVENT_CODE_QOS_CONN_EVENT_REPORT:
	{
		if (atomic_get(&m_processing) == 1) {
			/* Cheaper to skip this update */
			/* instead of using locks */
			return true;
		}

		p_evt = (void *)buf->data;

		chmap_filter_crc_update(
			mp_chmap_inst,
			p_evt->channel_index,
			p_evt->crc_ok_count,
			p_evt->crc_error_count);
		return true;
	}
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

	hci_vs_cmd_qos_conn_event_report_enable_t *p_cmd_enable;

	buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_QOS_CONN_EVENT_REPORT_ENABLE,
							sizeof(*p_cmd_enable));
	if (!buf) {
		LOG_ERR("Failed to enable HCI VS QoS");
		return;
	}

	p_cmd_enable = net_buf_add(buf, sizeof(*p_cmd_enable));
	p_cmd_enable->enable = 1;

	err = bt_hci_cmd_send(
		HCI_VS_OPCODE_CMD_QOS_CONN_EVENT_REPORT_ENABLE,
		buf);
	if (err) {
		LOG_ERR("Failed to enable HCI VS QoS");
		return;
	}
}

static void handle_config_event(const struct config_event *event)
{
	switch (OPT_FIELD_GET(event->id)) {
	case QOS_OPT_BLACKLIST:
		update_blacklist(
			(struct params_blacklist *)event->dyndata.data);
		break;
	case QOS_OPT_CHMAP:
		/* Avoid updating channel map directly to */
		/* reduce complexity in interaction with */
		/* chmap filter lib */
		LOG_DBG("Not supported");
		break;
	case QOS_OPT_PARAM_BLE:
		__ASSERT_NO_MSG(
			event->dyndata.size == sizeof(struct params_ble));
		update_parameters(
			(struct params_ble *)event->dyndata.data,
			NULL);
		break;
	case QOS_OPT_PARAM_WIFI:
		__ASSERT_NO_MSG(
			event->dyndata.size == sizeof(struct params_wifi));
		update_parameters(
			NULL,
			(struct params_wifi *)event->dyndata.data);
		break;
	default:
		LOG_WRN("Unknown opt");
		return;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event;

		event = cast_module_state_event(eh);

		if (check_state(
				event,
				MODULE_ID(ble_state),
				MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);

			mp_chmap_inst = chmap_filter_init();
			__ASSERT_NO_MSG(mp_chmap_inst != NULL);

			k_mutex_init(&m_data_access_mutex);
			m_new_params = NULL;
			m_new_blacklist = -1;

			LOG_DBG("Chmap lib version: %s",
				chmap_filter_version());
			enable_qos_reporting();

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
			cdc_dev = device_get_binding(
				CONFIG_USB_CDC_ACM_DEVICE_NAME "_0");
			__ASSERT_NO_MSG(cdc_dev != NULL);
# if CONFIG_UART_LINE_CTRL
			cdc_dtr = 0;
# else
			cdc_dtr = 1;
# endif
#endif

			initialized = true;

			k_sem_give(&sem_initialized);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(eh)) {
			const struct config_event *event;

			event = cast_config_event(eh);

			if (!is_my_config_id(event->id)) {
				return false;
			}

			handle_config_event(event);

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

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		static s32_t m_hid_pkt_recv_count;
		static s32_t m_usb_pkt_sent_count;
		static u32_t m_cdc_notify_count;

		/* Count number of HID packets received via BLE */
		/* and number of HID packets sent via USB. */
		/* Send stats printout via CDC every 100 packets */

		if (is_hid_mouse_event(eh)) {
			++m_hid_pkt_recv_count;
			++m_cdc_notify_count;

			if (m_cdc_notify_count == 100) {
				hid_pkt_stats_print(
					m_hid_pkt_recv_count,
					m_usb_pkt_sent_count);
				m_cdc_notify_count = 0;
			}

			return false;
		}

		if (is_hid_report_sent_event(eh)) {
			++m_usb_pkt_sent_count;

			return false;
		}
	}
#endif

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

static void apply_new_params(void)
{
	int err;

	k_mutex_lock(&m_data_access_mutex, K_FOREVER);
	/* chmap_filter_params_set Returns immediately */
	err = chmap_filter_params_set(mp_chmap_inst, m_new_params);
	k_free(m_new_params);
	m_new_params = NULL;
	k_mutex_unlock(&m_data_access_mutex);

	if (err) {
		LOG_WRN("Param update failed");
	}
}

void ble_qos_thread_fn(void)
{
	int err;

	err = k_sem_take(&sem_initialized, K_FOREVER);
	if (err) {
		LOG_ERR("Init failed: %d", err);
		__ASSERT_NO_MSG(false);
	}

	for (;;) {
		bool update_channel_map;

		k_sleep(1000); /* Set time unit for chmap filter to ~1 second */

		/* Check and apply new parameters received via config channel */
		if (m_new_params != NULL) {
			apply_new_params();
		}

		/* Check and apply new blacklist received via config channel */
		atomic_t blacklist_update = atomic_set(&m_new_blacklist, -1);

		if (blacklist_update != -1) {
			err = chmap_filter_blacklist_set(
						mp_chmap_inst,
						(u16_t) blacklist_update);
			if (err) {
				LOG_WRN("Blacklist update failed");
			}
		}

		/* Run processing function. */
		/* Atomic variable is used as data busy flag */
		/* (this thread runs at the lowest priority) */
		atomic_set(&m_processing, 1);
		update_channel_map = chmap_filter_process(mp_chmap_inst);
		atomic_set(&m_processing, 0);

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
		ble_chn_stats_print(update_channel_map);
#endif

		if (!update_channel_map) {
			continue;
		}

		u8_t const *chmap;

		chmap = chmap_filter_suggested_map_get(mp_chmap_inst);
		err = hci_chmap_cmd_send(chmap);
		if (err) {
			LOG_WRN("hci_chmap_cmd_send: %d", err);
		} else {
			LOG_DBG("Channel map update");
			chmap_filter_suggested_map_confirm(mp_chmap_inst);

			k_mutex_lock(&m_data_access_mutex, K_FOREVER);
			memcpy(m_current_chmap, chmap, sizeof(m_current_chmap));
			k_mutex_unlock(&m_data_access_mutex);
		}
	}
}

#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
#define STACKSIZE 1024
#else
#define STACKSIZE 240
#endif
/* Lowest thread priority */
#define PRIORITY K_PRIO_PREEMPT((CONFIG_NUM_PREEMPT_PRIORITIES - 1))

K_THREAD_DEFINE(ble_qos_thread_id, STACKSIZE, ble_qos_thread_fn, NULL, NULL,
		NULL, PRIORITY, 0, K_NO_WAIT);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
#endif
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
#endif
