/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <misc/byteorder.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_hid.h>


#define MODULE usb_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_USB_STATE_LOG_LEVEL);

#include "hid_report_desc.h"

#include "hid_event.h"
#include "usb_event.h"
#include "config_event.h"

static enum usb_state state;


static int get_report(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	*len  = hid_report_desc_size;
	*data = (u8_t *)hid_report_desc;

	return 0;
}

static int set_report(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		size_t length = *len;
		const u8_t *buffer = *data;

		if (length > 0) {
			if (buffer[0] != REPORT_ID_USER_CONFIG) {
				LOG_WRN("Unsupported report ID");
				return -ENOTSUP;
			}

			if (length != (REPORT_SIZE_USER_CONFIG + 1)) {
				LOG_WRN("Unsupported report size");
				return -ENOTSUP;
			}

			struct config_event *event = new_config_event();

			event->id = buffer[1];
			memcpy(event->data, &(buffer[2]), sizeof(event->data));

			EVENT_SUBMIT(event);
		}
	}

	return 0;
}

static void mouse_report_sent(bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = TARGET_REPORT_MOUSE;
	event->subscriber = &state;
	event->error = error;
	EVENT_SUBMIT(event);
}

static void mouse_report_sent_cb(void)
{
	mouse_report_sent(false);
}

static void send_mouse_report(const struct hid_mouse_event *event)
{
	if (&state != event->subscriber) {
		/* It's not us */
		return;
	}

	if (state != USB_STATE_ACTIVE) {
		/* USB not connected. */
		return;
	}

	s16_t wheel = max(min(event->wheel, REPORT_MOUSE_WHEEL_MAX),
			  REPORT_MOUSE_WHEEL_MIN);
	s16_t x = max(min(event->dx, REPORT_MOUSE_XY_MAX),
		      REPORT_MOUSE_XY_MIN);
	s16_t y = max(min(event->dy, REPORT_MOUSE_XY_MAX),
		      REPORT_MOUSE_XY_MIN);

	/* Convert to little-endian. */
	u8_t x_buff[2];
	u8_t y_buff[2];

	sys_put_le16(x, x_buff);
	sys_put_le16(y, y_buff);

	/* Encode report. */
	u8_t buffer[REPORT_SIZE_MOUSE + sizeof(u8_t)];

	static_assert(sizeof(buffer) == 6, "Invalid report size");

	buffer[0] = REPORT_ID_MOUSE;
	buffer[1] = event->button_bm;
	buffer[2] = wheel;
	buffer[3] = x_buff[0];
	buffer[4] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
	buffer[5] = (y_buff[1] << 4) | (y_buff[0] >> 4);

	int err = hid_int_ep_write(buffer, sizeof(buffer), NULL);
	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		mouse_report_sent(true);
	}
}

static void broadcast_usb_state(void)
{
	struct usb_state_event *event = new_usb_state_event();

	event->state = state;
	event->id = &state;

	EVENT_SUBMIT(event);
}

static void broadcast_subscription_change(void)
{
	struct hid_report_subscription_event *event =
		new_hid_report_subscription_event();

	event->report_type = TARGET_REPORT_MOUSE;
	event->enabled     = state == USB_STATE_ACTIVE;
	event->subscriber  = &state;

	LOG_INF("USB HID %sabled", (event->enabled)?("en"):("dis"));

	EVENT_SUBMIT(event);
}

static void device_status(enum usb_dc_status_code cb_status, const u8_t *param)
{
	enum usb_state new_state = state;

	switch (cb_status) {
	case USB_DC_CONNECTED:
		__ASSERT_NO_MSG(state == USB_STATE_DISCONNECTED);
		new_state = USB_STATE_POWERED;
		break;

	case USB_DC_DISCONNECTED:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_DISCONNECTED;
		break;

	case USB_DC_CONFIGURED:
		__ASSERT_NO_MSG(state == USB_STATE_POWERED);
		new_state = USB_STATE_ACTIVE;
		break;

	case USB_DC_RESET:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_POWERED;
		break;

	case USB_DC_SUSPEND:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_SUSPENDED;
		/* Not supported yet */
		__ASSERT_NO_MSG(false);
		break;

	case USB_DC_RESUME:
		__ASSERT_NO_MSG(state == USB_STATE_SUSPENDED);
		new_state = USB_STATE_POWERED;
		/* Not supported yet */
		__ASSERT_NO_MSG(false);
		break;

	case USB_DC_SET_HALT:
		/* fall through */
	case USB_DC_CLEAR_HALT:
		/* Not supported */
		__ASSERT_NO_MSG(false);
		break;

	case USB_DC_ERROR:
		module_set_state(MODULE_STATE_ERROR);
		break;

	default:
		/* Should not happen */
		__ASSERT_NO_MSG(false);
		break;
	}

	if (new_state != state) {
		enum usb_state old_state = state;

		state = new_state;

		if (old_state == USB_STATE_ACTIVE) {
			broadcast_subscription_change();
		}

		broadcast_usb_state();

		if (new_state == USB_STATE_ACTIVE) {
			broadcast_subscription_change();
		}
	}
}

static int usb_init(void)
{
	static const struct hid_ops ops = {
		.get_report   = get_report,
		.set_report   = set_report,
		.int_in_ready = mouse_report_sent_cb,
		.status_cb    = device_status,
	};
	usb_hid_register_device(hid_report_desc, hid_report_desc_size, &ops);

	int err = usb_hid_init();
	if (err) {
		LOG_ERR("cannot initialize HID class");
	}

	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		if (is_hid_mouse_event(eh)) {
			send_mouse_report(cast_hid_mouse_event(eh));

			return false;
		}
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (usb_init()) {
				LOG_ERR("cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
