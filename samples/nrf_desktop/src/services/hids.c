/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <limits.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <misc/byteorder.h>

#include <bluetooth/services/hids.h>

#include "hid_event.h"
#include "module_state_event.h"

#define MODULE		hids
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_HOG_LEVEL
#include <logging/sys_log.h>


#define BASE_USB_HID_SPEC_VERSION   0x0101

#define REPORT_SIZE_MOUSE_BUTTONS  1 /* bytes */
#define REPORT_SIZE_MOUSE_WP       2 /* bytes */
#define REPORT_SIZE_MOUSE_XY       3 /* bytes */
#define REPORT_SIZE_KEYBOARD       9 /* bytes */
#define REPORT_SIZE_MPLAYER        1 /* bytes */

enum {
	REPORT_ID_RESERVED,
	REPORT_ID_MOUSE_BUTTONS,
	REPORT_ID_MOUSE_WP,
	REPORT_ID_MOUSE_XY,
	REPORT_ID_KEYBOARD,
	REPORT_ID_MPLAYER,

	REPORT_ID_COUNT
};

static size_t report_index[REPORT_ID_COUNT];


HIDS_DEF(hids_obj);

static bool in_boot_mode;


static void pm_evt_handler(enum hids_pm_evt evt)
{
	switch (evt) {
	case HIDS_PM_EVT_BOOT_MODE_ENTERED:
		SYS_LOG_INF("Boot mode");
		in_boot_mode = true;
		break;

	case HIDS_PM_EVT_REPORT_MODE_ENTERED:
		SYS_LOG_INF("Report mode");
		in_boot_mode = false;
		break;

	default:
		break;
	}
}

static int module_init(void)
{
	static const u8_t report_map[] = {
#if CONFIG_DESKTOP_HID_MOUSE
		/* Usage page */
		0x05, 0x01,     /* Usage Page (Generic Desktop) */
		0x09, 0x02,     /* Usage (Mouse) */

		0xA1, 0x01,     /* Collection (Application) */

		/* Report: Mouse buttons */
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x85, REPORT_ID_MOUSE_BUTTONS,
		0x75, 0x01,         /* Report Size (1) */
		0x95, 0x08,         /* Report Count (8) */
		0x05, 0x09,         /* Usage Page (Buttons) */
		0x19, 0x01,         /* Usage Minimum (1) */
		0x29, 0x08,         /* Usage Maximum (8) */
		0x15, 0x00,         /* Logical Minimum (0) */
		0x25, 0x01,         /* Logical Maximum (1) */
		0x81, 0x02,         /* Input (Data, Variable, Absolute) */
		0xC0,             /* End Collection (Physical) */

		/* Report: Mouse wheel/pan */
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x85, REPORT_ID_MOUSE_WP,
		0x75, 0x08,         /* Report Size (8) */
		0x95, 0x01,         /* Report Count (1) */
		0x05, 0x01,         /* Usage Page (Generic Desktop) */
		0x09, 0x38,         /* Usage (Wheel) */
		0x15, 0x81,         /* Logical Minimum (-127) */
		0x25, 0x7F,         /* Logical Maximum (127) */
		0x81, 0x06,         /* Input (Data, Variable, Relative) */
		0x05, 0x0C,         /* Usage Page (Consumer) */
		0x0A, 0x38, 0x02,   /* Usage (AC Pan) */
		0x75, 0x08,         /* Report Size (8) */
		0x95, 0x01,         /* Report Count (1) */
		0x15, 0x81,         /* Logical Minimum (-127) */
		0x25, 0x7F,         /* Logical Maximum (127) */
		0x81, 0x06,         /* Input (Data, Variable, Relative) */
		0xC0,             /* End Collection (Physical) */

		/* Report: Mouse motion */
		0x85, REPORT_ID_MOUSE_XY,
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x75, 0x0C,         /* Report Size (12) */
		0x95, 0x02,         /* Report Count (2) */
		0x05, 0x01,         /* Usage Page (Generic Desktop) */
		0x09, 0x30,         /* Usage (X) */
		0x09, 0x31,         /* Usage (Y) */
		0x16, 0x01, 0xF8,   /* Logical Maximum (2047) */
		0x26, 0xFF, 0x07,   /* Logical Minimum (-2047) */
		0x81, 0x06,         /* Input (Data, Variable, Relative) */
		0xC0,             /* End Collection (Physical) */
		0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_KEYBOARD
		/* Usage page - Keyboard */
		0x05, 0x01,     /* Usage Page (Generic Desktop) */
		0x09, 0x06,     /* Usage (Mouse) */

		0xA1, 0x01,     /* Collection (Application) */

		/* Report: Keyboard */
		0x85, REPORT_ID_KEYBOARD,

		/* Keyboard - Modifiers */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x08,       /* Report Count (8) */
		0x05, 0x07,       /* Usage Page (Key Codes) */
		0x19, 0xe0,       /* Usage Minimum (Left Ctrl) */
		0x29, 0xe7,       /* Usage Maximum (Right GUI) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */

		/* Keyboard - Reserved */
		0x75, 0x08,       /* Report Size (8) */
		0x95, 0x01,       /* Report Count (1) */
		0x81, 0x01,       /* Input (Constant) */

		/* Keyboard - Keys */
		0x75, 0x08,       /* Report Size (8) */
		0x95, 0x06,       /* Report Count (6) */
		0x05, 0x07,       /* Usage Page (Key codes) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x65,       /* Logical Maximum (101) */
		0x19, 0x00,       /* Usage Minimum (0) */
		0x29, 0x65,       /* Usage Maximum (101) */
		0x81, 0x00,       /* Input (Data, Array) */

		/* Keyboard - LEDs */
		0x95, 0x05,       /* Report Count (5) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x08,       /* Usage Page (LEDs) */
		0x19, 0x01,       /* Usage Minimum (1) */
		0x29, 0x05,       /* Usage Maximum (5) */
		0x91, 0x02,       /* Output (Data, Variable, Absolute) */

		/* Keyboard - LEDs padding */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) (padding) */
		0x91, 0x01,       /* Output (Data, Variable, Absolute) */

		0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_MPLAYER
		/* Usage page - Consume Control */
		0x05, 0x0C,     /* Usage Page (Consumer) */
		0x09, 0x01,     /* Usage (Consumer Control) */

		0xA1, 0x01,     /* Collection (Application) */

		0x85, REPORT_ID_MPLAYER,
		0x15, 0x00,       /* Logical minimum (0) */
		0x25, 0x01,       /* Logical maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x01,       /* Report Count (1) */

		0x09, 0xCD,       /* Usage (Play/Pause) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x83, 0x01, /* Usage (Consumer Control Configuration) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xB5,       /* Usage (Scan Next Track) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xB6,       /* Usage (Scan Previous Track) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */

		0x09, 0xEA,       /* Usage (Volume Down) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xE9,       /* Usage (Volume Up) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x25, 0x02, /* Usage (AC Forward) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x24, 0x02, /* Usage (AC Back) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0xC0            /* End Collection */
#endif
	};

	/* HID service configuration */
	struct hids_init hids_init_obj = { 0 };

	hids_init_obj.info.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
	hids_init_obj.info.b_country_code = 0x00;
	hids_init_obj.info.flags          = HIDS_REMOTE_WAKE |
			HIDS_NORMALLY_CONNECTABLE;

	/* Attach report map */
	hids_init_obj.rep_map.data = report_map;
	hids_init_obj.rep_map.size = sizeof(report_map);

	/* Declare HID reports */

	struct hids_inp_rep *input_report =
		&hids_init_obj.inp_rep_group_init.reports[0];
	size_t ir_pos = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		static u8_t mouse_buttons_buff[REPORT_ID_MOUSE_BUTTONS];

		input_report[ir_pos].id        = REPORT_ID_MOUSE_BUTTONS;
		input_report[ir_pos].buff.data = mouse_buttons_buff;
		input_report[ir_pos].buff.size = sizeof(mouse_buttons_buff);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;


		static u8_t mouse_wp_buff[REPORT_SIZE_MOUSE_WP];

		input_report[ir_pos].id        = REPORT_ID_MOUSE_WP;
		input_report[ir_pos].buff.data = mouse_wp_buff;
		input_report[ir_pos].buff.size = sizeof(mouse_wp_buff);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;


		static u8_t mouse_xy_buff[REPORT_SIZE_MOUSE_XY];

		input_report[ir_pos].id        = REPORT_ID_MOUSE_XY;
		input_report[ir_pos].buff.data = mouse_xy_buff;
		input_report[ir_pos].buff.size = sizeof(mouse_xy_buff);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		static u8_t keyboard_buff[REPORT_SIZE_KEYBOARD];

		input_report[ir_pos].id        = REPORT_ID_KEYBOARD;
		input_report[ir_pos].buff.data = keyboard_buff;
		input_report[ir_pos].buff.size = sizeof(keyboard_buff);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_MPLAYER)) {
		static u8_t mplayer_buff[REPORT_SIZE_MPLAYER];

		input_report[ir_pos].id        = REPORT_ID_MPLAYER;
		input_report[ir_pos].buff.data = mplayer_buff;
		input_report[ir_pos].buff.size = sizeof(mplayer_buff);

		report_index[input_report[ir_pos].id] = ir_pos;
		ir_pos++;
	}

	hids_init_obj.inp_rep_group_init.cnt = ir_pos;

	/* Boot protocol setup */
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		hids_init_obj.is_mouse = true;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		hids_init_obj.is_kb = true;
	}

	hids_init_obj.pm_evt_handler = pm_evt_handler;

	return hids_init(&hids_obj, &hids_init_obj);
}

static void send_mouse_xy(const struct hid_mouse_xy_event *event)
{
	if (in_boot_mode) {
		s8_t x = max(min(event->dx, SCHAR_MAX), SCHAR_MIN);
		s8_t y = max(min(event->dy, SCHAR_MAX), SCHAR_MIN);

		hids_boot_mouse_inp_rep_send(&hids_obj, 0x00, x, y);
	} else {
		s16_t x = max(min(event->dx, 0x07ff), -0x07ff);
		s16_t y = max(min(event->dy, 0x07ff), -0x07ff);

		/* Convert to little-endian. */
		u8_t x_buff[2];
		u8_t y_buff[2];

		sys_put_le16(x, x_buff);
		sys_put_le16(y, y_buff);

		/* Encode report. */
		u8_t buffer[REPORT_SIZE_MOUSE_XY];

		static_assert(sizeof(buffer) == 3,
				"Only 2 axis, 12-bit each, are supported");

		buffer[0] = x_buff[0];
		buffer[1] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
		buffer[2] = y_buff[1];

		hids_inp_rep_send(&hids_obj, report_index[REPORT_ID_MOUSE_XY],
				buffer, sizeof(buffer));
	}
}

static void send_mouse_wp(const struct hid_mouse_wp_event *event)
{
	/* Not yet implemented. */
}

static void send_mouse_buttons(const struct hid_mouse_button_event *event)
{
	if (in_boot_mode) {
		hids_boot_mouse_inp_rep_send(&hids_obj, event->button_bm, 0, 0);
	} else {
		u8_t report[REPORT_SIZE_MOUSE_BUTTONS];

		report[0] = event->button_bm;

		hids_inp_rep_send(&hids_obj, report_index[REPORT_ID_MOUSE_BUTTONS],
				report, sizeof(report));
	}
}

static void send_keyboard(const struct hid_keyboard_event *event)
{
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

	if (in_boot_mode) {
		hids_boot_kb_inp_rep_send(&hids_obj, report,
				sizeof(report) - sizeof(report[8]));
	} else {
		hids_inp_rep_send(&hids_obj, report_index[REPORT_ID_KEYBOARD],
				report, sizeof(report));
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		if (is_hid_mouse_xy_event(eh)) {
			send_mouse_xy(cast_hid_mouse_xy_event(eh));

			return false;
		}

		if (is_hid_mouse_wp_event(eh)) {
			send_mouse_wp(cast_hid_mouse_wp_event(eh));

			return false;
		}

		if (is_hid_mouse_button_event(eh)) {
			send_mouse_buttons(cast_hid_mouse_button_event(eh));

			return false;
		}
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		if (is_hid_keyboard_event(eh)) {
			send_keyboard(cast_hid_keyboard_event(eh));

			return false;
		}
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, "ble_state", "ready")) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (module_init()) {
				SYS_LOG_ERR("service init failed");

				return false;
			}
			SYS_LOG_INF("service initialized");

			module_set_state("ready");
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_xy_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_button_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
