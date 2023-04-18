/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <limits.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/sys/util.h>

#include <bluetooth/services/hids.h>

#include "hids_event.h"
#include "hid_event.h"
#include <caf/events/ble_common_event.h>
#include "config_event.h"

#include "hid_report_desc.h"
#include "config_channel_transport.h"

#define MODULE hids
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HIDS_LOG_LEVEL);

#define BASE_USB_HID_SPEC_VERSION   0x0101

BT_HIDS_DEF(hids_obj,
	IF_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT,
		   (REPORT_SIZE_MOUSE,))
	IF_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT,
		   (REPORT_SIZE_KEYBOARD_KEYS,
		    REPORT_SIZE_KEYBOARD_LEDS,))
	IF_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT,
		   (REPORT_SIZE_SYSTEM_CTRL,))
	IF_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT,
		   (REPORT_SIZE_CONSUMER_CTRL,))
	IF_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE,
		   (REPORT_SIZE_USER_CONFIG, /* HID feature report. */
		    IF_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT,
			       /* HID output report. */
			       (REPORT_SIZE_USER_CONFIG,))))
		   0 /* Appease macro with a dummy zero */
);

static size_t report_index[REPORT_ID_COUNT];
static bool report_enabled[REPORT_ID_COUNT];
static bool subscribed[REPORT_ID_COUNT];

static struct bt_conn *cur_conn;
static bool secured;
static bool protocol_boot;

static struct config_channel_transport cfg_chan_transport;
static struct k_work_delayable notify_secured;

static void broadcast_subscription_change(uint8_t report_id, bool enabled)
{
	bool boot = (report_id == REPORT_ID_BOOT_MOUSE) ||
		    (report_id == REPORT_ID_BOOT_KEYBOARD);

	enabled = enabled && (protocol_boot == boot);

	if (enabled == subscribed[report_id]) {
		/* No change in subscription. */
		return;
	}

	if (!secured) {
		/* Ignore the change. */
		return;
	}

	subscribed[report_id] = enabled;

	struct hid_report_subscription_event *event =
		new_hid_report_subscription_event();

	event->report_id  = report_id;
	event->enabled    = enabled;
	event->subscriber = cur_conn;

	LOG_INF("Notifications for report 0x%x are %sabled", report_id,
		(event->enabled)?("en"):("dis"));

	APP_EVENT_SUBMIT(event);
}

static void pm_evt_handler(enum bt_hids_pm_evt evt, struct bt_conn *conn)
{
	switch (evt) {
	case BT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		LOG_INF("Boot mode");
		protocol_boot = true;
		break;

	case BT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		LOG_INF("Report mode");
		protocol_boot = false;
		break;

	default:
		break;
	}

	for (size_t r_id = 0; r_id < REPORT_ID_COUNT; r_id++) {
		bool enabled = report_enabled[r_id];
		broadcast_subscription_change(r_id, enabled);
	}
}

static void sync_notif_handler(const struct hid_notification_event *event)
{
	uint8_t report_id = event->report_id;
	bool enabled = event->enabled;

	__ASSERT_NO_MSG(report_id < ARRAY_SIZE(report_enabled));

	if (!cur_conn) {
		LOG_WRN("Notification before connection");
		return;
	}

	report_enabled[report_id] = enabled;

	broadcast_subscription_change(report_id, enabled);
}

static void async_notif_handler(uint8_t report_id, enum bt_hids_notify_evt evt)
{
	struct hid_notification_event *event = new_hid_notification_event();

	event->report_id = report_id;
	event->enabled = (evt == BT_HIDS_CCCD_EVT_NOTIFY_ENABLED);

	APP_EVENT_SUBMIT(event);
}

static void hid_report_sent(const struct bt_conn *conn, uint8_t report_id, bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_id = report_id;
	event->subscriber = conn;
	event->error = error;

	APP_EVENT_SUBMIT(event);
}

static void boot_mouse_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	hid_report_sent(conn, REPORT_ID_BOOT_MOUSE, false);
}
static void boot_mouse_notif_handler(enum bt_hids_notify_evt evt)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE));
	async_notif_handler(REPORT_ID_BOOT_MOUSE, evt);
}

static void boot_keyboard_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	hid_report_sent(conn, REPORT_ID_BOOT_KEYBOARD, false);
}
static void boot_keyboard_notif_handler(enum bt_hids_notify_evt evt)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD));
	async_notif_handler(REPORT_ID_BOOT_KEYBOARD, evt);
}

static void mouse_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	hid_report_sent(conn, REPORT_ID_MOUSE, false);
}
static void mouse_notif_handler(enum bt_hids_notify_evt evt)
{
	async_notif_handler(REPORT_ID_MOUSE, evt);
}

static void keyboard_keys_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	hid_report_sent(conn, REPORT_ID_KEYBOARD_KEYS, false);
}
static void keyboard_keys_notif_handler(enum bt_hids_notify_evt evt)
{
	async_notif_handler(REPORT_ID_KEYBOARD_KEYS, evt);
}

static void system_ctrl_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	hid_report_sent(conn, REPORT_ID_SYSTEM_CTRL, false);
}
static void system_ctrl_notif_handler(enum bt_hids_notify_evt evt)
{
	async_notif_handler(REPORT_ID_SYSTEM_CTRL, evt);
}

static void consumer_ctrl_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	hid_report_sent(conn, REPORT_ID_CONSUMER_CTRL, false);
}
static void consumer_ctrl_notif_handler(enum bt_hids_notify_evt evt)
{
	async_notif_handler(REPORT_ID_CONSUMER_CTRL, evt);
}

static void broadcast_kbd_leds_report(struct bt_hids_rep *rep, struct bt_conn *conn, bool write)
{
	/* Ignore HID keyboard LEDs report read. */
	if (!write) {
		return;
	}

	struct hid_report_event *event = new_hid_report_event(rep->size + 1);

	event->source = conn;
	/* Subscriber is not specified for HID output report. */
	event->subscriber = NULL;
	event->dyndata.data[0] = REPORT_ID_KEYBOARD_LEDS;
	memcpy(&event->dyndata.data[1], rep->data, rep->size);

	APP_EVENT_SUBMIT(event);
}

static void feature_report_handler(struct bt_hids_rep *rep,
				   struct bt_conn *conn,
				   bool write)
{
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (!write) {
			int err = config_channel_transport_get(&cfg_chan_transport,
							       rep->data,
							       rep->size);

			if (err) {
				LOG_WRN("Failed to process report get");
			}
		} else {
			int err = config_channel_transport_set(&cfg_chan_transport,
							       rep->data,
							       rep->size);

			if (err) {
				LOG_WRN("Failed to process report set");
			}
		}
	}
}

static int module_init(void)
{
	/* HID service configuration */
	struct bt_hids_init_param hids_init_param = { 0 };

	hids_init_param.info.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
	hids_init_param.info.b_country_code = 0x00;
	hids_init_param.info.flags          = BT_HIDS_REMOTE_WAKE |
					      BT_HIDS_NORMALLY_CONNECTABLE;

	/* Attach report map */
	hids_init_param.rep_map.data = hid_report_desc;
	hids_init_param.rep_map.size = hid_report_desc_size;

	/* Declare HID reports */
	struct bt_hids_inp_rep *input_report =
		&hids_init_param.inp_rep_group_init.reports[0];
	struct bt_hids_outp_feat_rep *output_report =
		&hids_init_param.outp_rep_group_init.reports[0];
	struct bt_hids_outp_feat_rep *feature_report =
		&hids_init_param.feat_rep_group_init.reports[0];

	size_t ir_pos = 0;
	size_t or_pos = 0;
	size_t feat_pos = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		static const uint8_t mask[] = REPORT_MASK_MOUSE;
		BUILD_ASSERT((sizeof(mask) == 0) ||
			     (sizeof(mask) == DIV_ROUND_UP(REPORT_SIZE_MOUSE, 8)));
		BUILD_ASSERT(REPORT_ID_MOUSE < ARRAY_SIZE(report_index));

		input_report[ir_pos].id       = REPORT_ID_MOUSE;
		input_report[ir_pos].size     = REPORT_SIZE_MOUSE;
		input_report[ir_pos].handler  = mouse_notif_handler;
		input_report[ir_pos].rep_mask = (sizeof(mask) == 0)?(NULL):(mask);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		static const uint8_t mask[] = REPORT_MASK_KEYBOARD_KEYS;
		BUILD_ASSERT((sizeof(mask) == 0) ||
			     (sizeof(mask) == DIV_ROUND_UP(REPORT_SIZE_KEYBOARD_KEYS, 8)));
		BUILD_ASSERT(REPORT_ID_KEYBOARD_KEYS < ARRAY_SIZE(report_index));

		input_report[ir_pos].id       = REPORT_ID_KEYBOARD_KEYS;
		input_report[ir_pos].size     = REPORT_SIZE_KEYBOARD_KEYS;
		input_report[ir_pos].handler  = keyboard_keys_notif_handler;
		input_report[ir_pos].rep_mask = (sizeof(mask) == 0)?(NULL):(mask);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT)) {
		static const uint8_t mask[] = REPORT_MASK_SYSTEM_CTRL;
		BUILD_ASSERT((sizeof(mask) == 0) ||
			     (sizeof(mask) == DIV_ROUND_UP(REPORT_SIZE_SYSTEM_CTRL, 8)));
		BUILD_ASSERT(REPORT_ID_SYSTEM_CTRL < ARRAY_SIZE(report_index));

		input_report[ir_pos].id       = REPORT_ID_SYSTEM_CTRL;
		input_report[ir_pos].size     = REPORT_SIZE_SYSTEM_CTRL;
		input_report[ir_pos].handler  = system_ctrl_notif_handler;
		input_report[ir_pos].rep_mask = (sizeof(mask) == 0)?(NULL):(mask);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) {
		static const uint8_t mask[] = REPORT_MASK_CONSUMER_CTRL;
		BUILD_ASSERT((sizeof(mask) == 0) ||
			     (sizeof(mask) == DIV_ROUND_UP(REPORT_SIZE_CONSUMER_CTRL, 8)));
		BUILD_ASSERT(REPORT_ID_CONSUMER_CTRL < ARRAY_SIZE(report_index));

		input_report[ir_pos].id       = REPORT_ID_CONSUMER_CTRL;
		input_report[ir_pos].size     = REPORT_SIZE_CONSUMER_CTRL;
		input_report[ir_pos].handler  = consumer_ctrl_notif_handler;
		input_report[ir_pos].rep_mask = (sizeof(mask) == 0)?(NULL):(mask);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	hids_init_param.inp_rep_group_init.cnt = ir_pos;

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		feature_report[feat_pos].id      = REPORT_ID_USER_CONFIG;
		feature_report[feat_pos].size    = REPORT_SIZE_USER_CONFIG;
		feature_report[feat_pos].handler = feature_report_handler;

		report_index[feature_report[feat_pos].id] = feat_pos;
		feat_pos++;
	}

	hids_init_param.feat_rep_group_init.cnt = feat_pos;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		output_report[or_pos].id      = REPORT_ID_KEYBOARD_LEDS;
		output_report[or_pos].size    = REPORT_SIZE_KEYBOARD_LEDS;
		output_report[or_pos].handler = broadcast_kbd_leds_report;

		report_index[output_report[or_pos].id] = or_pos;
		or_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT)) {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE));
		output_report[or_pos].id      = REPORT_ID_USER_CONFIG_OUT;
		output_report[or_pos].size    = REPORT_SIZE_USER_CONFIG;
		output_report[or_pos].handler = feature_report_handler;

		report_index[output_report[or_pos].id] = or_pos;
		or_pos++;
	}

	hids_init_param.outp_rep_group_init.cnt = or_pos;

	/* Boot protocol setup */
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE)) {
		hids_init_param.is_mouse = true;
		hids_init_param.boot_mouse_notif_handler =
			boot_mouse_notif_handler;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD)) {
		hids_init_param.is_kb = true;
		hids_init_param.boot_kb_notif_handler = boot_keyboard_notif_handler;
		hids_init_param.boot_kb_outp_rep_handler = broadcast_kbd_leds_report;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		config_channel_transport_init(&cfg_chan_transport);
	}

	hids_init_param.pm_evt_handler = pm_evt_handler;

	return bt_hids_init(&hids_obj, &hids_init_param);
}

static void send_hid_report(const struct hid_report_event *event)
{
	static void (*const report_sent_cb[REPORT_ID_COUNT])(struct bt_conn *conn, void *user_data) = {
		[REPORT_ID_MOUSE]         = mouse_report_sent_cb,
		[REPORT_ID_KEYBOARD_KEYS] = keyboard_keys_report_sent_cb,
		[REPORT_ID_SYSTEM_CTRL]   = system_ctrl_report_sent_cb,
		[REPORT_ID_CONSUMER_CTRL] = consumer_ctrl_report_sent_cb,
		[REPORT_ID_BOOT_MOUSE] = boot_mouse_report_sent_cb,
		[REPORT_ID_BOOT_KEYBOARD] = boot_keyboard_report_sent_cb,
	};

	if (!cur_conn || (cur_conn != event->subscriber)) {
		/* It's not us */
		return;
	}

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	uint8_t report_id = event->dyndata.data[0];

	__ASSERT_NO_MSG(report_id < ARRAY_SIZE(report_index));
	__ASSERT_NO_MSG(report_sent_cb[report_id]);

	if (!subscribed[report_id]) {
		/* Notification disabled */
		LOG_WRN("Notification disabled");
		hid_report_sent(cur_conn, report_id, true);
		return;
	}

	const uint8_t *buffer = &event->dyndata.data[sizeof(report_id)];
	size_t size = event->dyndata.size - sizeof(report_id);
	int err;

	switch (report_id) {
	case REPORT_ID_BOOT_MOUSE:
		if (!protocol_boot) {
			err = -EBADF;
		} else {
			err = bt_hids_boot_mouse_inp_rep_send(&hids_obj, cur_conn,
							      &buffer[0], buffer[1],
							      buffer[2],
							      report_sent_cb[report_id]);
		}
		break;
	case REPORT_ID_BOOT_KEYBOARD:
		if (!protocol_boot) {
			err = -EBADF;
		} else {
			err = bt_hids_boot_kb_inp_rep_send(&hids_obj, cur_conn,
							   buffer, size,
							   report_sent_cb[report_id]);
		}
		break;
	default:
		if (protocol_boot) {
			err = -EBADF;
		} else {
			err = bt_hids_inp_rep_send(&hids_obj, cur_conn,
						   report_index[report_id],
						   buffer, size,
						   report_sent_cb[report_id]);
		}
		break;
	}

	if (err) {
		if (err == -ENOTCONN) {
			LOG_WRN("Cannot send report: device disconnected");
		} else if (err == -EBADF) {
			LOG_WRN("Cannot send report: incompatible mode");
		} else {
			LOG_ERR("Cannot send report (%d)", err);
		}
		hid_report_sent(cur_conn, report_id, true);
	}
}

static void notify_secured_fn(struct k_work *work)
{
	secured = true;

	for (size_t r_id = 0; r_id < REPORT_ID_COUNT; r_id++) {
		bool enabled = report_enabled[r_id];
		broadcast_subscription_change(r_id, enabled);
	}
}

static void notify_hids(const struct ble_peer_event *event)
{
	int err = 0;

	switch (event->state) {
	case PEER_STATE_CONNECTED:
		__ASSERT_NO_MSG(cur_conn == NULL);
		cur_conn = event->id;
		err = bt_hids_connected(&hids_obj, event->id);

		if (err) {
			LOG_ERR("Failed to notify the HID Service about the"
				" connection");
		}
		break;

	case PEER_STATE_DISCONNECTED:
		__ASSERT_NO_MSG(cur_conn == event->id);
		err = bt_hids_disconnected(&hids_obj, event->id);

		if (err) {
			LOG_ERR("Connection context was not allocated");
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			config_channel_transport_disconnect(
				&cfg_chan_transport);
		}

		cur_conn = NULL;
		secured = false;
		protocol_boot = false;
		if (CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY > 0) {
			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&notify_secured);
		}
		break;

	case PEER_STATE_SECURED:
		__ASSERT_NO_MSG(cur_conn == event->id);

		if (CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY > 0) {
			k_work_reschedule(&notify_secured,
				K_MSEC(CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY));
		} else {
			notify_secured_fn(NULL);
		}

		break;

	case PEER_STATE_DISCONNECTING:
	case PEER_STATE_CONN_FAILED:
		/* No action */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_event(aeh)) {
		send_hid_report(cast_hid_report_event(aeh));

		return false;
	}

	if (is_ble_peer_event(aeh)) {
		notify_hids(cast_ble_peer_event(aeh));

		return false;
	}

	if (is_hid_notification_event(aeh)) {
		sync_notif_handler(cast_hid_notification_event(aeh));

		return false;
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (CONFIG_DESKTOP_HIDS_FIRST_REPORT_DELAY > 0) {
				k_work_init_delayable(&notify_secured,
						    notify_secured_fn);
			}

			if (module_init()) {
				LOG_ERR("Service init failed");

				return false;
			}
			LOG_INF("Service initialized");

			module_set_state(MODULE_STATE_READY);
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    is_config_event(aeh)) {
		config_channel_transport_rsp_receive(&cfg_chan_transport,
					cast_config_event(aeh));

		return false;
	}


	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_notification_event);
/* The module is initialized before CAF BLE state module to make sure that the GATT HIDS is
 * registered before Bluetooth is enabled. This is done to avoid submitting works related to Service
 * Changed indication and GATT database hash calculation before system settings are loaded from
 * non-volatile memory.
 */
APP_EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, config_event);
#endif
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_event);
