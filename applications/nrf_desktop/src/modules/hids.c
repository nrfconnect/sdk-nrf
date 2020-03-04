/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <limits.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <bluetooth/services/hids.h>

#include "hids_event.h"
#include "hid_event.h"
#include "ble_event.h"
#include "config_event.h"

#include "hid_report_desc.h"
#include "config_channel.h"

#define MODULE hids
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HIDS_LOG_LEVEL);


#define BASE_USB_HID_SPEC_VERSION   0x0101


static size_t report_index[REPORT_ID_COUNT];

BT_GATT_HIDS_DEF(hids_obj,
		 REPORT_SIZE_MOUSE,
		 REPORT_SIZE_KEYBOARD_KEYS,
		 REPORT_SIZE_KEYBOARD_LEDS
#if CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT
		 , REPORT_SIZE_CTRL
#endif
#if CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT
		 , REPORT_SIZE_CTRL
#endif
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
		 , REPORT_SIZE_USER_CONFIG
#endif
);

static enum report_mode report_mode;
static bool report_enabled[IN_REPORT_COUNT][REPORT_MODE_COUNT];
static bool subscribed[IN_REPORT_COUNT];

static struct bt_conn *cur_conn;
static bool secured;

static struct config_channel_state cfg_chan;

static void broadcast_subscription_change(enum in_report tr, bool enabled)
{
	if (enabled == subscribed[tr]) {
		/* No change in subscription. */
		return;
	}

	if (!secured) {
		/* Ignore the change. */
		return;
	}

	subscribed[tr] = enabled;

	struct hid_report_subscription_event *event =
		new_hid_report_subscription_event();

	event->report_type = tr;
	event->enabled     = enabled;
	event->subscriber  = cur_conn;

	LOG_INF("Notifications %sabled", (event->enabled)?("en"):("dis"));

	EVENT_SUBMIT(event);
}

static void pm_evt_handler(enum bt_gatt_hids_pm_evt evt, struct bt_conn *conn)
{
	enum report_mode old_mode = report_mode;

	switch (evt) {
	case BT_GATT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		LOG_INF("Boot mode");
		report_mode = REPORT_MODE_BOOT;
		break;

	case BT_GATT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		LOG_INF("Report mode");
		report_mode = REPORT_MODE_PROTOCOL;
		break;

	default:
		break;
	}

	if (report_mode != old_mode) {
		for (size_t tr = 0; tr < IN_REPORT_COUNT; tr++) {
			bool enabled = report_enabled[tr][report_mode];
			broadcast_subscription_change(tr, enabled);
		}
	}
}

static void sync_notif_handler(const struct hid_notification_event *event)
{
	enum bt_gatt_hids_notif_evt evt = event->event;
	enum in_report tr = event->report_type;
	enum report_mode mode = event->report_mode;

	__ASSERT_NO_MSG((evt == BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED) ||
			(evt == BT_GATT_HIDS_CCCD_EVT_NOTIF_DISABLED));
	__ASSERT_NO_MSG(tr < IN_REPORT_COUNT);
	__ASSERT_NO_MSG(mode < REPORT_MODE_COUNT);

	if (!cur_conn) {
		LOG_WRN("Notification before connection");
		return;
	}

	bool enabled = (evt == BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED);

	report_enabled[tr][mode] = enabled;

	broadcast_subscription_change(tr, enabled);
}

static void async_notif_handler(enum bt_gatt_hids_notif_evt evt,
				enum in_report tr,
				enum report_mode mode)
{
	struct hid_notification_event *event =
		new_hid_notification_event();

	event->report_type = tr;
	event->report_mode = mode;
	event->event = evt;

	EVENT_SUBMIT(event);
}

static void mouse_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	async_notif_handler(evt, IN_REPORT_MOUSE, REPORT_MODE_PROTOCOL);
}

static void boot_mouse_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	async_notif_handler(evt, IN_REPORT_MOUSE, REPORT_MODE_BOOT);
}

static void keyboard_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	async_notif_handler(evt, IN_REPORT_KEYBOARD_KEYS, REPORT_MODE_PROTOCOL);
}

static void boot_keyboard_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	async_notif_handler(evt, IN_REPORT_KEYBOARD_KEYS, REPORT_MODE_BOOT);
}

static void system_ctrl_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	async_notif_handler(evt, IN_REPORT_SYSTEM_CTRL, REPORT_MODE_PROTOCOL);
}

static void consumer_ctrl_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	async_notif_handler(evt, IN_REPORT_CONSUMER_CTRL, REPORT_MODE_PROTOCOL);
}

static void keyboard_leds_handler(struct bt_gatt_hids_rep *rep,
				  struct bt_conn *conn,
				  bool write)
{
	LOG_WRN("Keyboards LEDs report ignored");
}

static void feature_report_handler(struct bt_gatt_hids_rep *rep,
				   struct bt_conn *conn,
				   bool write)
{
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (!write) {
			int err = config_channel_report_get(&cfg_chan, rep->data,
							    rep->size, false,
							    CONFIG_BT_GATT_DIS_PNP_PID);

			if (err) {
				LOG_WRN("Failed to process report get");
			}
		} else {
			int err = config_channel_report_set(&cfg_chan, rep->data,
							    rep->size, false,
							    CONFIG_BT_GATT_DIS_PNP_PID);

			if (err) {
				LOG_WRN("Failed to process report set");
			}
		}
	}
}

static int module_init(void)
{
	/* HID service configuration */
	struct bt_gatt_hids_init_param hids_init_param = { 0 };
	static const u8_t mouse_mask[ceiling_fraction(REPORT_SIZE_MOUSE, 8)] = {0x01};

	hids_init_param.info.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
	hids_init_param.info.b_country_code = 0x00;
	hids_init_param.info.flags          = BT_GATT_HIDS_REMOTE_WAKE |
					      BT_GATT_HIDS_NORMALLY_CONNECTABLE;

	/* Attach report map */
	hids_init_param.rep_map.data = hid_report_desc;
	hids_init_param.rep_map.size = hid_report_desc_size;

	/* Declare HID reports */
	struct bt_gatt_hids_inp_rep *input_report =
		&hids_init_param.inp_rep_group_init.reports[0];
	struct bt_gatt_hids_outp_feat_rep *output_report =
		&hids_init_param.outp_rep_group_init.reports[0];
	struct bt_gatt_hids_outp_feat_rep *feature_report =
		&hids_init_param.feat_rep_group_init.reports[0];

	size_t ir_pos = 0;
	size_t or_pos = 0;
	size_t feat_pos = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		input_report[ir_pos].id       = REPORT_ID_MOUSE;
		input_report[ir_pos].size     = REPORT_SIZE_MOUSE;
		input_report[ir_pos].handler  = mouse_notif_handler;
		input_report[ir_pos].rep_mask = mouse_mask;

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		input_report[ir_pos].id      = REPORT_ID_KEYBOARD_KEYS;
		input_report[ir_pos].size    = REPORT_SIZE_KEYBOARD_KEYS;
		input_report[ir_pos].handler = keyboard_notif_handler;

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT)) {
		input_report[ir_pos].id      = REPORT_ID_SYSTEM_CTRL;
		input_report[ir_pos].size    = REPORT_SIZE_CTRL;
		input_report[ir_pos].handler = system_ctrl_notif_handler;

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) {
		input_report[ir_pos].id      = REPORT_ID_CONSUMER_CTRL;
		input_report[ir_pos].size    = REPORT_SIZE_CTRL;
		input_report[ir_pos].handler = consumer_ctrl_notif_handler;

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
		output_report[or_pos].handler = keyboard_leds_handler;

		report_index[output_report[or_pos].id] = or_pos;
		or_pos++;
	}

	hids_init_param.outp_rep_group_init.cnt = or_pos;

	/* Boot protocol setup */
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		hids_init_param.is_mouse = true;
		hids_init_param.boot_mouse_notif_handler =
			boot_mouse_notif_handler;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		hids_init_param.is_kb = true;
		hids_init_param.boot_kb_notif_handler =
			boot_keyboard_notif_handler;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		config_channel_init(&cfg_chan);
	}

	hids_init_param.pm_evt_handler = pm_evt_handler;

	return bt_gatt_hids_init(&hids_obj, &hids_init_param);
}

static void mouse_report_sent(const struct bt_conn *conn, bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = IN_REPORT_MOUSE;
	event->subscriber  = conn;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void mouse_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	mouse_report_sent(conn, false);
}

static void send_mouse_report(const struct hid_mouse_event *event)
{
	if (cur_conn != event->subscriber) {
		/* It's not us */
		return;
	}
	__ASSERT_NO_MSG(cur_conn);

	if (!report_enabled[IN_REPORT_MOUSE][report_mode]) {
		/* Notification disabled */
		return;
	}

	int err;

	if (report_mode == REPORT_MODE_BOOT) {
		s8_t x = MAX(MIN(event->dx, SCHAR_MAX), SCHAR_MIN);
		s8_t y = MAX(MIN(event->dy, SCHAR_MAX), SCHAR_MIN);

		err = bt_gatt_hids_boot_mouse_inp_rep_send(&hids_obj, cur_conn,
							   &event->button_bm,
							   x, y,
							   mouse_report_sent_cb);
	} else {
		s16_t wheel = MAX(MIN(event->wheel, MOUSE_REPORT_WHEEL_MAX),
				  MOUSE_REPORT_WHEEL_MIN);
		s16_t x = MAX(MIN(event->dx, MOUSE_REPORT_XY_MAX),
			      MOUSE_REPORT_XY_MIN);
		s16_t y = MAX(MIN(event->dy, MOUSE_REPORT_XY_MAX),
			      MOUSE_REPORT_XY_MIN);

		/* Convert to little-endian. */
		u8_t x_buff[2];
		u8_t y_buff[2];

		sys_put_le16(x, x_buff);
		sys_put_le16(y, y_buff);

		/* Encode report. */
		u8_t buffer[REPORT_SIZE_MOUSE];

		BUILD_ASSERT_MSG(sizeof(buffer) == 5, "Invalid report size");

		buffer[0] = event->button_bm;
		buffer[1] = wheel;
		buffer[2] = x_buff[0];
		buffer[3] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
		buffer[4] = (y_buff[1] << 4) | (y_buff[0] >> 4);

		err = bt_gatt_hids_inp_rep_send(&hids_obj, cur_conn,
						report_index[REPORT_ID_MOUSE],
						buffer, sizeof(buffer),
						mouse_report_sent_cb);
	}

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		mouse_report_sent(cur_conn, true);
	}
}

static void keyboard_report_sent(const struct bt_conn *conn, bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = IN_REPORT_KEYBOARD_KEYS;
	event->subscriber  = conn;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void keyboard_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	keyboard_report_sent(conn, false);
}

static void send_keyboard_report(const struct hid_keyboard_event *event)
{
	if (cur_conn != event->subscriber) {
		/* It's not us */
		return;
	}
	__ASSERT_NO_MSG(cur_conn);

	if (!report_enabled[IN_REPORT_KEYBOARD_KEYS][report_mode]) {
		/* Notification disabled */
		return;
	}

	u8_t report[REPORT_SIZE_KEYBOARD_KEYS];

	BUILD_ASSERT_MSG(ARRAY_SIZE(report) == ARRAY_SIZE(event->keys) + 2,
			 "Incorrect number of keys in event");

	/* Modifiers */
	report[0] = event->modifier_bm;

	/* Reserved */
	report[1] = 0;

	/* Pressed keys */
	memcpy(&report[2], &event->keys[0], sizeof(event->keys));

	int err;

	if (report_mode == REPORT_MODE_BOOT) {
		err = bt_gatt_hids_boot_kb_inp_rep_send(&hids_obj, cur_conn, report,
							sizeof(report) - sizeof(report[8]),
							keyboard_report_sent_cb);
	} else {
		err = bt_gatt_hids_inp_rep_send(&hids_obj, cur_conn,
						report_index[REPORT_ID_KEYBOARD_KEYS],
						report, sizeof(report),
						keyboard_report_sent_cb);
	}

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		keyboard_report_sent(cur_conn, true);
	}
}

static void system_ctrl_report_sent(const struct bt_conn *conn, bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = IN_REPORT_SYSTEM_CTRL;
	event->subscriber  = conn;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void system_ctrl_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	system_ctrl_report_sent(conn, false);
}

static void consumer_ctrl_report_sent(const struct bt_conn *conn, bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = IN_REPORT_CONSUMER_CTRL;
	event->subscriber  = conn;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void consumer_ctrl_report_sent_cb(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);
	consumer_ctrl_report_sent(conn, false);
}

static void send_ctrl_report(const struct hid_ctrl_event *event)
{
	bt_gatt_complete_func_t sent_cb = NULL;
	enum report_id report_id = REPORT_ID_COUNT;
	void (*sent_fn)(const struct bt_conn*, bool) = NULL;
	enum in_report report_type = event->report_type;

	switch (report_type) {
	case IN_REPORT_SYSTEM_CTRL:
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT));
		report_id = REPORT_ID_SYSTEM_CTRL;
		sent_cb = system_ctrl_report_sent_cb;
		sent_fn = system_ctrl_report_sent;
	break;

	case IN_REPORT_CONSUMER_CTRL:
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT));
		report_id = REPORT_ID_CONSUMER_CTRL;
		sent_cb = consumer_ctrl_report_sent_cb;
		sent_fn = consumer_ctrl_report_sent;
	break;

	default:
		__ASSERT_NO_MSG(false);
	}

	if (cur_conn != event->subscriber) {
		/* It's not us */
		return;
	}
	__ASSERT_NO_MSG(cur_conn);

	if (!report_enabled[report_type][report_mode]) {
		/* Notification disabled */
		return;
	}

	u8_t report[REPORT_SIZE_CTRL];

	BUILD_ASSERT_MSG(ARRAY_SIZE(report) == sizeof(event->usage),
			 "Incorrect data size in event");

	/* Set selected usage */
	sys_put_le16(event->usage, report);
	int err = bt_gatt_hids_inp_rep_send(&hids_obj, cur_conn,
					report_index[report_id],
					report, sizeof(report),
					sent_cb);
	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		(*sent_fn)(cur_conn, true);
	}
}

static void notify_hids(const struct ble_peer_event *event)
{
	int err = 0;

	switch (event->state) {
	case PEER_STATE_CONNECTED:
		__ASSERT_NO_MSG(cur_conn == NULL);
		cur_conn = event->id;
		break;

	case PEER_STATE_DISCONNECTED:
		__ASSERT_NO_MSG(cur_conn == event->id);
		err = bt_gatt_hids_notify_disconnected(&hids_obj, event->id);

		if (err) {
			LOG_WRN("Connection was not secured");
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			config_channel_disconnect(&cfg_chan);
		}

		cur_conn = NULL;
		secured = false;
		break;

	case PEER_STATE_SECURED:
		__ASSERT_NO_MSG(cur_conn == event->id);
		secured = true;
		err = bt_gatt_hids_notify_connected(&hids_obj, event->id);
		if (!err) {
			for (size_t tr = 0; tr < IN_REPORT_COUNT; tr++) {
				bool enabled = report_enabled[tr][report_mode];
				broadcast_subscription_change(tr, enabled);
			}
		} else {
			LOG_ERR("Failed to notify the HID service about the "
				"connection");
		}

		break;

	case PEER_STATE_CONN_FAILED:
		/* No action */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT) &&
	    is_hid_mouse_event(eh)) {
		send_mouse_report(cast_hid_mouse_event(eh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) &&
	    is_hid_keyboard_event(eh)) {
		send_keyboard_report(cast_hid_keyboard_event(eh));

		return false;
	}

	if ((IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT) ||
	     IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) &&
	    is_hid_ctrl_event(eh)) {
		send_ctrl_report(cast_hid_ctrl_event(eh));

		return false;
	}

	if (is_ble_peer_event(eh)) {
		notify_hids(cast_ble_peer_event(eh));

		return false;
	}

	if (is_hid_notification_event(eh)) {
		sync_notif_handler(cast_hid_notification_event(eh));

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

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
	    is_config_fetch_event(eh)) {
		config_channel_fetch_receive(&cfg_chan,
					     cast_config_fetch_event(eh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    is_config_event(eh)) {
		config_channel_event_done(&cfg_chan, cast_config_event(eh));

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, hid_ctrl_event);
EVENT_SUBSCRIBE(MODULE, hid_notification_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_fetch_event);
EVENT_SUBSCRIBE(MODULE, config_event);
#endif
EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_event);
