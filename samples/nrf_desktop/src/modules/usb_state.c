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
static u8_t hid_protocol = HID_PROTOCOL_REPORT;
static struct device *usb_dev;

static atomic_t forward_status;

static int get_report(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		u8_t request_value[2];
		sys_put_le16(setup->wValue, request_value);

		if (request_value[1] == 0x03) {
			/* Request for feature report */
			if (request_value[0] == REPORT_ID_USER_CONFIG) {
				static u8_t status;

				status = atomic_get(&forward_status);
				*len = sizeof(status);
				*data = (u8_t *)(&status);

				return 0;
			} else {
				LOG_WRN("Unsupported report ID");
				return -ENOTSUP;
			}
		}
	}

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

			u16_t recipient = sys_get_le16(&buffer[1]);

			if (recipient == CONFIG_USB_DEVICE_PID) {
				/* Event intended for this device */
				struct config_event *event = new_config_event();

				static_assert(REPORT_SIZE_USER_CONFIG >=
					(sizeof(event->id) +
					sizeof(event->data)),
					"Incorrect report size");

				event->id = buffer[1 + sizeof(recipient)];
				memcpy(event->data,
				       &buffer[1 + sizeof(recipient) + sizeof(event->id)],
				       sizeof(event->data));
				event->store_needed = true;

				atomic_set(&forward_status, FORWARD_STATUS_SUCCESS);

				EVENT_SUBMIT(event);
			} else {
				/* Forward event */
				struct config_forward_event *event = new_config_forward_event();

				static_assert(REPORT_SIZE_USER_CONFIG >=
					(sizeof(event->recipient) +
					sizeof(event->id) +
					sizeof(event->data)),
					"Incorrect report size");

				event->recipient = recipient;
				event->id = buffer[1 + sizeof(event->recipient)];
				memcpy(event->data,
				       &buffer[1 + sizeof(event->recipient) + sizeof(event->id)],
				       sizeof(event->data));

				atomic_set(&forward_status, FORWARD_STATUS_PENDING);

				EVENT_SUBMIT(event);
			}
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

	u8_t buffer[(hid_protocol) ?
		    REPORT_SIZE_MOUSE + sizeof(u8_t) :
		    REPORT_SIZE_MOUSE_BOOT];

	if (hid_protocol == HID_PROTOCOL_REPORT) {
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

		__ASSERT(sizeof(buffer) == 6, "Invalid report size");

		/* Encode report. */
		buffer[0] = REPORT_ID_MOUSE;
		buffer[1] = event->button_bm;
		buffer[2] = wheel;
		buffer[3] = x_buff[0];
		buffer[4] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
		buffer[5] = (y_buff[1] << 4) | (y_buff[0] >> 4);

	} else {
		s8_t x = MAX(MIN(event->dx, REPORT_MOUSE_XY_MAX_BOOT),
				REPORT_MOUSE_XY_MIN_BOOT);
		s8_t y = MAX(MIN(event->dy, REPORT_MOUSE_XY_MAX_BOOT),
				REPORT_MOUSE_XY_MIN_BOOT);

		__ASSERT(sizeof(buffer) == 3, "Invalid boot report size");

		buffer[0] = event->button_bm;
		buffer[1] = x;
		buffer[2] = y;

	}
	int err = hid_int_ep_write(usb_dev, buffer, sizeof(buffer), NULL);
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
	static enum usb_state before_suspend;
	enum usb_state new_state = state;

	switch (cb_status) {
	case USB_DC_CONNECTED:
		if (state != USB_STATE_DISCONNECTED) {
			LOG_WRN("USB_DC_CONNECTED when USB is not disconnected");
		}
		new_state = USB_STATE_POWERED;
		break;

	case USB_DC_DISCONNECTED:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_DISCONNECTED;
		break;

	case USB_DC_CONFIGURED:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_ACTIVE;
		break;

	case USB_DC_RESET:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		if (state == USB_STATE_SUSPENDED) {
			LOG_WRN("USB reset in suspended state, ignoring");
		} else {
			new_state = USB_STATE_POWERED;
		}
		break;

	case USB_DC_SUSPEND:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		before_suspend = state;
		new_state = USB_STATE_SUSPENDED;
		LOG_WRN("USB suspend");
		break;

	case USB_DC_RESUME:
		__ASSERT_NO_MSG(state == USB_STATE_SUSPENDED);
		new_state = before_suspend;
		LOG_WRN("USB resume");
		break;

	case USB_DC_SET_HALT:
	case USB_DC_CLEAR_HALT:
		/* Ignore */
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

static void protocol_change(u8_t protocol)
{
	hid_protocol = protocol;
	LOG_INF("%s_PROTOCOL selected", protocol ? "REPORT" : "BOOT");
}

static int usb_init(void)
{
	usb_dev = device_get_binding(CONFIG_USB_HID_DEVICE_NAME_0);
	if (usb_dev == NULL) {
		return -ENXIO;
	}

	static const struct hid_ops ops = {
		.get_report   		= get_report,
		.set_report   		= set_report,
		.int_in_ready 		= mouse_report_sent_cb,
		.status_cb    		= device_status,
		.protocol_change 	= protocol_change,
	};

	usb_hid_register_device(usb_dev, hid_report_desc,
				hid_report_desc_size, &ops);

	int err = usb_hid_init(usb_dev);
	if (err) {
		LOG_ERR("Cannot initialize HID class");
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
				LOG_ERR("Cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_forwarded_event(eh)) {
			struct config_forwarded_event *event =
				cast_config_forwarded_event(eh);

			atomic_set(&forward_status, event->status);

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_forwarded_event);
#endif
