/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <assert.h>

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "hid_event.h"
#include "module_state_event.h"

#define MODULE		hog
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_HOG_LEVEL
#include <logging/sys_log.h>


/** Keyboard Input Report ID. */
#define KEYBOARD_IN_REP_ID              1
/** Size of a single field in the Keyboard Input Report [bits]. */
#define KEYBOARD_IN_REP_SIZE            8
/** Number of fields in the Keyboard Input Report. */
#define KEYBOARD_IN_REP_COUNT           3

/* Make sure that the Keyboard Input Report size is byte aligned. */
static_assert(((KEYBOARD_IN_REP_SIZE * KEYBOARD_IN_REP_COUNT) % 8) == 0, "");

/** Mouse Button Input Report ID. */
#define MOUSE_BTN_IN_REP_ID		2
/** Size of a single field in the Mouse Button Input Report [bits]. */
#define MOUSE_BTN_IN_REP_SIZE		1
/** Number of fields in the Mouse Button Input Report. */
#define MOUSE_BTN_IN_REP_COUNT		8

/* Make sure that the Mouse Button Input Report size is byte aligned. */
static_assert(((MOUSE_BTN_IN_REP_SIZE * MOUSE_BTN_IN_REP_COUNT) % 8) == 0, "");

/** Mouse X/Y Input Report ID. */
#define MOUSE_XY_IN_REP_ID		3
/** Size of a single field in the Mouse X/Y Input Report [bits]. */
#define MOUSE_XY_IN_REP_SIZE		8
/** Number of fields in the Mouse X/Y Input Report. */
#define MOUSE_XY_IN_REP_COUNT		2

/* Make sure that the Mouse X/Y Input Report size is byte aligned. */
static_assert(((MOUSE_XY_IN_REP_SIZE * MOUSE_XY_IN_REP_COUNT) % 8) == 0, "");

/** Mouse Wheel/Pan Input Report ID. */
#define MOUSE_WP_IN_REP_ID		4
/** Size of a single field in the Mouse Wheel/Pan Input Report [bits]. */
#define MOUSE_WP_IN_REP_SIZE		8
/** Number of fields in the Mouse Wheel/Pan Input Report. */
#define MOUSE_WP_IN_REP_COUNT		2

/* Make sure that the Mouse Wheel/Pan Input Report size is byte aligned. */
static_assert(((MOUSE_WP_IN_REP_SIZE * MOUSE_WP_IN_REP_COUNT) % 8) == 0, "");

enum {
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
	u16_t version; /* version number of base USB HID Specification */
	u8_t code; /* country HID Device hardware is localized for. */
	u8_t flags;
} __packed;

struct hids_report {
	u8_t id; /* report id */
	u8_t type; /* report type */
} __packed;

static struct hids_info info = {
	.version = 0x0000,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

enum {
	HIDS_INPUT = 0x01,
	HIDS_OUTPUT = 0x02,
	HIDS_FEATURE = 0x03,
};

static struct hids_report input_kbd = {
	.id = KEYBOARD_IN_REP_ID,
	.type = HIDS_INPUT,
};

static struct hids_report input_mouse_btn = {
	.id = MOUSE_BTN_IN_REP_ID,
	.type = HIDS_INPUT,
};

static struct hids_report input_mouse_motion = {
	.id = MOUSE_XY_IN_REP_ID,
	.type = HIDS_INPUT,
};

static struct hids_report input_mouse_wheel = {
	.id = MOUSE_WP_IN_REP_ID,
	.type = HIDS_INPUT,
};

static struct bt_gatt_ccc_cfg input_ccc_cfg[BT_GATT_CCC_MAX] = {};
static bool active;
static u8_t ctrl_point;

static u8_t report_map[] = {
	/* Mouse */
	0x05, 0x01,                   /* Usage Page (Generic Desktop) */
	0x09, 0x02,                   /* Usage (Mouse) */
	0xA1, 0x01,                   /* Collection (Application) */
	0x09, 0x01,                   /*  Usage (Pointer) */
	0xA1, 0x00,                   /*  Collection (Physical) */
	0x85, MOUSE_BTN_IN_REP_ID,    /*   Report ID */
	0x75, MOUSE_BTN_IN_REP_SIZE,  /*   Report Size */
	0x95, MOUSE_BTN_IN_REP_COUNT, /*   Report Count */
	0x15, 0x00,                   /*   Logical Minimum (0) */
	0x25, 0x01,                   /*   Logical Maximum (1) */
	0x05, 0x09,                   /*   Usage Page (Button) */
	0x19, 0x01,                   /*   Usage Minimum (Button 1) */
	0x29, 0x08,                   /*   Usage Maximum (Button 8) */
	0x81, 0x02,                   /*   Input (Data, Var, Abs) */
	0xC0,                         /*  End Collection */
	0xA1, 0x00,                   /*  Collection (Physical) */
	0x85, MOUSE_XY_IN_REP_ID,     /*   Report ID */
	0x75, MOUSE_XY_IN_REP_SIZE,   /*   Report Size */
	0x95, MOUSE_XY_IN_REP_COUNT,  /*   Report Count */
	0x16, 0x01, 0xF8,             /*   Logical Minimum (-2047) */
	0x26, 0xFF, 0x07,             /*   Logical Maximum (2047) */
	0x05, 0x01,                   /*   Usage Page (Generic Desktop) */
	0x09, 0x30,                   /*   Usage (X) */
	0x09, 0x31,                   /*   Usage (Y) */
	0x81, 0x06,                   /*   Input (Data, Var, Rel) */
	0xC0,                         /*  End Collection */
	0xA1, 0x00,                   /*  Collection (Physical) */
	0x85, MOUSE_WP_IN_REP_ID,     /*   Report ID */
	0x75, MOUSE_WP_IN_REP_SIZE,   /*   Report Size */
	0x95, 0x01,                   /*   Report Count (1) */
	0x15, 0x81,                   /*   Logical Minimum (-127) */
	0x25, 0x7F,                   /*   Logical Maximum (127) */
	0x05, 0x01,                   /*   Usage Page (Generic Desktop) */
	0x09, 0x38,                   /*   Usage (Wheel) */
	0x81, 0x06,                   /*   Input (Data, Var, Rel) */
	0x75, MOUSE_WP_IN_REP_SIZE,   /*   Report Size */
	0x95, 0x01,                   /*   Report Count (1) */
	0x15, 0x81,                   /*   Logical Minimum (-127) */
	0x25, 0x7F,                   /*   Logical Maximum (127) */
	0x05, 0x0C,                   /*   Usage Page (Consumer Devices) */
	0x0A, 0x38, 0x02,             /*   Usage (AC Pan) */
	0x81, 0x06,                   /*   Input (Data, Var, Rel) */
	0xC0,                         /*  End Collection */
	0xC0,                         /* End Collection */

	/* Keyboard */
	0x05, 0x01,                   /* Usage Page (Generic Desktop) */
	0x09, 0x06,                   /* Usage (Keyboard) */
	0xA1, 0x01,                   /* Collection (Application) */
	0x85, KEYBOARD_IN_REP_ID,     /*  Report ID */
	0x75, KEYBOARD_IN_REP_SIZE,   /*  Report Size */
	0x95, KEYBOARD_IN_REP_COUNT,  /*  Report Count */
	0x15, 0x00,                   /*  Logical Minimum (0) */
	0x25, 0xFF,                   /*  Logical Maximum (255) */
	0x05, 0x07,                   /*  Usage Page (Keyboard) */
	0x19, 0x00,                   /*  Usage Minimum (0) */
	0x29, 0xFF,                   /*  Usage Maximum (255) */
	0x81, 0x00,                   /*  Input (Data, Ary, Abs) */
	0xC0,                         /* End Collection */
};

static ssize_t read_info(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_map,
				 sizeof(report_map));
}

static ssize_t read_report(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	active = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t read_input_report(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_point)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/* HID Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_info, NULL, &info),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_report_map, NULL, NULL),

	/* Keyboard */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_AUTHEN, read_input_report,
			       NULL, NULL),
	BT_GATT_CCC(input_ccc_cfg, input_ccc_changed),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report, NULL, &input_kbd),

	/* Mouse buttons: TODO */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_AUTHEN, read_input_report,
			       NULL, NULL),
	BT_GATT_CCC(input_ccc_cfg, input_ccc_changed),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report, NULL, &input_mouse_btn),

	/* Mouse axis motion */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_AUTHEN, read_input_report,
			       NULL, NULL),
	BT_GATT_CCC(input_ccc_cfg, input_ccc_changed),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report, NULL, &input_mouse_motion),

	/* Mouse wheel: TODO */
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_AUTHEN, read_input_report,
			       NULL, NULL),
	BT_GATT_CCC(input_ccc_cfg, input_ccc_changed),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report, NULL, &input_mouse_wheel),

	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, write_ctrl_point,
			       &ctrl_point),
};

static int hog_init(void)
{
	static struct bt_gatt_service hog_svc = BT_GATT_SERVICE(attrs);

	return bt_gatt_service_register(&hog_svc);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_axis_event(eh) && active) {
		const struct hid_axis_event *event = cast_hid_axis_event(eh);

		u8_t report[MOUSE_XY_IN_REP_COUNT] = {
			event->x,
			event->y
		};

		if (bt_gatt_notify(NULL, &attrs[9], &report, sizeof(report))) {
			SYS_LOG_WRN("axis notify failed");
		}

		return false;
	}

	if (is_hid_keys_event(eh) && active) {
		const struct hid_keys_event *event = cast_hid_keys_event(eh);

		u8_t report[KEYBOARD_IN_REP_COUNT];

		__ASSERT_NO_MSG(ARRAY_SIZE(report) == ARRAY_SIZE(event->keys));

		for (size_t i = 0; i < ARRAY_SIZE(report); i++) {
			report[i] = event->keys[i];
		}

		if (bt_gatt_notify(NULL, &attrs[3], &report, sizeof(report))) {
			SYS_LOG_WRN("button notify failed");
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, "ble_state", "ready")) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (hog_init()) {
				SYS_LOG_ERR("service init failed");

				return false;
			}
			SYS_LOG_INF("service initialized");
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_keys_event);
EVENT_SUBSCRIBE(MODULE, hid_axis_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
