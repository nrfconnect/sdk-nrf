/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <limits.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <misc/byteorder.h>

#include <bluetooth/services/hids.h>

#include "hid_event.h"
#include "ble_event.h"
#include "config_event.h"

#include "hid_report_desc.h"

#define MODULE hids
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HIDS_LOG_LEVEL);


#define BASE_USB_HID_SPEC_VERSION   0x0101


static size_t report_index[REPORT_ID_COUNT];

BT_GATT_HIDS_DEF(hids_obj,
		 REPORT_SIZE_MOUSE,
		 REPORT_SIZE_KEYBOARD,
		 REPORT_SIZE_MPLAYER
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
		 , REPORT_SIZE_USER_CONFIG
#endif
);

enum report_mode {
	REPORT_MODE_PROTOCOL,
	REPORT_MODE_BOOT,

	REPORT_MODE_COUNT
};

static enum report_mode report_mode;
static bool report_enabled[TARGET_REPORT_COUNT][REPORT_MODE_COUNT];

static const struct bt_conn *cur_conn;


static void broadcast_subscription_change(enum target_report tr,
					  enum report_mode old_mode,
					  enum report_mode new_mode)
{
	if ((old_mode != new_mode) &&
	    (report_enabled[tr][old_mode] == report_enabled[tr][new_mode])) {
		/* No change in report state. */
		return;
	}

	struct hid_report_subscription_event *event =
		new_hid_report_subscription_event();

	event->report_type = tr;
	event->enabled     = report_enabled[tr][new_mode];
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
		if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
			broadcast_subscription_change(TARGET_REPORT_MOUSE,
					old_mode, report_mode);
		}
		if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
			broadcast_subscription_change(TARGET_REPORT_MOUSE,
					old_mode, report_mode);
		}
		if (IS_ENABLED(CONFIG_DESKTOP_HID_MPLAYER)) {
			broadcast_subscription_change(TARGET_REPORT_MOUSE,
					old_mode, report_mode);
		}
	}
}

static void notif_handler(enum bt_gatt_hids_notif_evt evt,
			  enum target_report tr,
			  enum report_mode mode)
{
	__ASSERT_NO_MSG((evt == BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED) ||
			(evt == BT_GATT_HIDS_CCCD_EVT_NOTIF_DISABLED));

	bool enabled = (evt == BT_GATT_HIDS_CCCD_EVT_NOTIF_ENABLED);
	bool changed = (report_enabled[tr][mode] != enabled);

	report_enabled[tr][mode] = enabled;

	if ((report_mode == mode) && changed) {
		broadcast_subscription_change(tr, mode, mode);
	}
}

static void mouse_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	LOG_INF("");
	notif_handler(evt, TARGET_REPORT_MOUSE, REPORT_MODE_PROTOCOL);
}

static void boot_mouse_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	LOG_INF("");
	notif_handler(evt, TARGET_REPORT_MOUSE, REPORT_MODE_BOOT);
}

static void keyboard_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	LOG_INF("");
	notif_handler(evt, TARGET_REPORT_KEYBOARD, REPORT_MODE_PROTOCOL);
}

static void boot_keyboard_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	LOG_INF("");
	notif_handler(evt, TARGET_REPORT_KEYBOARD, REPORT_MODE_BOOT);
}

static void mplayer_notif_handler(enum bt_gatt_hids_notif_evt evt)
{
	LOG_INF("");
	notif_handler(evt, TARGET_REPORT_MPLAYER, REPORT_MODE_PROTOCOL);
}

static void feature_report_handler(struct bt_gatt_hids_rep const *rep,
				   struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (rep->size != REPORT_SIZE_USER_CONFIG) {
			LOG_WRN("Unsupported report size");
			return;
		}

		u16_t recipient = sys_get_le16(&rep->data[0]);
		if (recipient != CONFIG_BT_GATT_DIS_PNP_PID) {
			LOG_WRN("Drop event addressed to %x", recipient);
			return;
		}

		struct config_event *event = new_config_event();

		event->id = rep->data[sizeof(recipient)];
		memcpy(event->data,
		       &rep->data[sizeof(recipient) + sizeof(event->id)],
		       sizeof(event->data));
		event->store_needed = true;

		EVENT_SUBMIT(event);
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
	size_t ir_pos = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		input_report[ir_pos].id       = REPORT_ID_MOUSE;
		input_report[ir_pos].size     = REPORT_SIZE_MOUSE;
		input_report[ir_pos].handler  = mouse_notif_handler;
		input_report[ir_pos].rep_mask = mouse_mask;

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		input_report[ir_pos].id      = REPORT_ID_KEYBOARD;
		input_report[ir_pos].size    = REPORT_SIZE_KEYBOARD;
		input_report[ir_pos].handler = keyboard_notif_handler;

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_MPLAYER)) {
		input_report[ir_pos].id      = REPORT_ID_MPLAYER;
		input_report[ir_pos].size    = REPORT_SIZE_MPLAYER;
		input_report[ir_pos].handler = mplayer_notif_handler;

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		struct bt_gatt_hids_outp_feat_rep *feature_report =
			&hids_init_param.feat_rep_group_init.reports[0];
		size_t feat_pos = 0;

		feature_report[feat_pos].id      = REPORT_ID_USER_CONFIG;
		feature_report[feat_pos].size    = REPORT_SIZE_USER_CONFIG;
		feature_report[feat_pos].handler = feature_report_handler;

		feat_pos++;

		hids_init_param.feat_rep_group_init.cnt = feat_pos;
	}

	hids_init_param.inp_rep_group_init.cnt = ir_pos;

	/* Boot protocol setup */
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		hids_init_param.is_mouse = true;
		hids_init_param.boot_mouse_notif_handler =
			boot_mouse_notif_handler;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		hids_init_param.is_kb = true;
		hids_init_param.boot_kb_notif_handler =
			boot_keyboard_notif_handler;
	}

	hids_init_param.pm_evt_handler = pm_evt_handler;

	return bt_gatt_hids_init(&hids_obj, &hids_init_param);
}

static void mouse_report_sent(const struct bt_conn *conn, bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = TARGET_REPORT_MOUSE;
	event->subscriber  = cur_conn;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void mouse_report_sent_cb(struct bt_conn *conn)
{
	mouse_report_sent(conn, false);
}

static void send_mouse_report(const struct hid_mouse_event *event)
{
	if (cur_conn != event->subscriber) {
		/* It's not us */
		return;
	}

	if (!report_enabled[TARGET_REPORT_MOUSE][report_mode]) {
		/* Notification disabled */
		return;
	}

	int err;

	if (report_mode == REPORT_MODE_BOOT) {
		s8_t x = MAX(MIN(event->dx, SCHAR_MAX), SCHAR_MIN);
		s8_t y = MAX(MIN(event->dy, SCHAR_MAX), SCHAR_MIN);

		err = bt_gatt_hids_boot_mouse_inp_rep_send(&hids_obj, NULL,
							   &event->button_bm,
							   x, y,
							   mouse_report_sent_cb);
	} else {
		s16_t wheel = MAX(MIN(event->wheel, REPORT_MOUSE_WHEEL_MAX),
				  REPORT_MOUSE_WHEEL_MIN);
		s16_t x = MAX(MIN(event->dx, REPORT_MOUSE_XY_MAX),
			      REPORT_MOUSE_XY_MIN);
		s16_t y = MAX(MIN(event->dy, REPORT_MOUSE_XY_MAX),
			      REPORT_MOUSE_XY_MIN);

		/* Convert to little-endian. */
		u8_t x_buff[2];
		u8_t y_buff[2];

		sys_put_le16(x, x_buff);
		sys_put_le16(y, y_buff);

		/* Encode report. */
		u8_t buffer[REPORT_SIZE_MOUSE];

		static_assert(sizeof(buffer) == 5, "Invalid report size");

		buffer[0] = event->button_bm;
		buffer[1] = wheel;
		buffer[2] = x_buff[0];
		buffer[3] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
		buffer[4] = (y_buff[1] << 4) | (y_buff[0] >> 4);

		err = bt_gatt_hids_inp_rep_send(&hids_obj, NULL,
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

	event->report_type = TARGET_REPORT_KEYBOARD;
	event->subscriber  = cur_conn;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void keyboard_report_sent_cb(struct bt_conn *conn)
{
	keyboard_report_sent(conn, false);
}

static void send_keyboard_report(const struct hid_keyboard_event *event)
{
	if (cur_conn != event->subscriber) {
		/* It's not us */
		return;
	}

	if (!report_enabled[TARGET_REPORT_KEYBOARD][report_mode]) {
		/* Notification disabled */
		return;
	}

	u8_t report[REPORT_SIZE_KEYBOARD];

	static_assert(ARRAY_SIZE(report) == ARRAY_SIZE(event->keys) + 3,
			"Incorrect number of keys in event");

	/* Modifiers */
	report[0] = event->modifier_bm;

	/* Reserved */
	report[1] = 0;

	/* Pressed keys */
	memcpy(&report[2], &event->keys[0], sizeof(event->keys));

	/* Led */
	report[8] = 0;

	int err;

	if (report_mode == REPORT_MODE_BOOT) {
		err = bt_gatt_hids_boot_kb_inp_rep_send(&hids_obj, NULL, report,
							sizeof(report) - sizeof(report[8]),
							keyboard_report_sent_cb);
	} else {
		err = bt_gatt_hids_inp_rep_send(&hids_obj, NULL,
						report_index[REPORT_ID_KEYBOARD],
						report, sizeof(report),
						keyboard_report_sent_cb);
	}

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		keyboard_report_sent(cur_conn, true);
	}
}

static void notify_hids(const struct ble_peer_event *event)
{
	int err = 0;

	switch (event->state) {
	case PEER_STATE_CONNECTED:
		__ASSERT_NO_MSG(cur_conn == NULL);
		err = bt_gatt_hids_notify_connected(&hids_obj, event->id);
		if (!err) {
			cur_conn = event->id;
		}
		break;

	case PEER_STATE_DISCONNECTED:
		__ASSERT_NO_MSG(cur_conn == event->id);
		err = bt_gatt_hids_notify_disconnected(&hids_obj, event->id);
		cur_conn = NULL;
		break;

	case PEER_STATE_SECURED:
		/* No action */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	if (err) {
		LOG_ERR("Failed to notify the HID service about the connection");
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		if (is_hid_mouse_event(eh)) {
			send_mouse_report(cast_hid_mouse_event(eh));

			return false;
		}
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		if (is_hid_keyboard_event(eh)) {
			send_keyboard_report(cast_hid_keyboard_event(eh));

			return false;
		}
	}

	if (is_ble_peer_event(eh)) {
		notify_hids(cast_ble_peer_event(eh));

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

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_event);
