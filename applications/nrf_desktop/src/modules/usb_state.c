/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_hid.h>


#define MODULE usb_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_USB_STATE_LOG_LEVEL);

#include "hid_report_desc.h"
#include "config_channel.h"

#include "hid_event.h"
#include "usb_event.h"
#include "config_event.h"

#define REPORT_TYPE_INPUT	0x01
#define REPORT_TYPE_OUTPUT	0x02
#define REPORT_TYPE_FEATURE	0x03

static enum usb_state state;
static u8_t hid_protocol = HID_PROTOCOL_REPORT;
static struct device *usb_dev;
static enum in_report sent_report_type = IN_REPORT_COUNT;

static struct config_channel_state cfg_chan;

static int get_report(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	u8_t request_value[2];

	sys_put_le16(setup->wValue, request_value);

	switch (request_value[1]) {
	case REPORT_TYPE_FEATURE:
		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			if (request_value[0] == REPORT_ID_USER_CONFIG) {
				size_t length = *len;
				u8_t *buffer = *data;

				int err = config_channel_report_get(&cfg_chan,
							buffer,
							length,
							true,
							CONFIG_USB_DEVICE_PID);
				if (err) {
					LOG_WRN("Failed to process report get");
				}
				return err;
			} else {
				LOG_WRN("Unsupported report ID");
				return -ENOTSUP;
			}
		}
		break;

	case REPORT_TYPE_INPUT:
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}

	LOG_WRN("Unsupported get report");
	LOG_WRN("bmRequestType: %02X bRequest: %02X wValue: %04X wIndex: %04X"
		" wLength: %04X", setup->bmRequestType, setup->bRequest,
		setup->wValue, setup->wIndex, setup->wLength);

	return 0;
}

static int set_report(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	u8_t request_value[2];

	sys_put_le16(setup->wValue, request_value);

	switch (request_value[1]) {
	case REPORT_TYPE_FEATURE:
		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			if (request_value[0] == REPORT_ID_USER_CONFIG) {
				size_t length = *len;
				u8_t *buffer = *data;

				int err = config_channel_report_set(&cfg_chan,
							buffer,
							length,
							true,
							CONFIG_USB_DEVICE_PID);

				if (err) {
					LOG_WRN("Failed to process report set");
				}
				return err;
			} else {
				LOG_WRN("Unsupported report ID");
				return -ENOTSUP;
			}
		}
		break;

	case REPORT_TYPE_OUTPUT:
		if (request_value[0] == REPORT_ID_KEYBOARD_LEDS) {
			LOG_INF("No action on keyboard LEDs report");
			return 0;
		}
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}

	LOG_WRN("Unsupported set report");
	LOG_WRN("bmRequestType: %02X bRequest: %02X wValue: %04X wIndex: %04X"
		" wLength: %04X", setup->bmRequestType, setup->bRequest,
		setup->wValue, setup->wIndex, setup->wLength);

	return 0;
}

static void report_sent(bool error)
{
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_type = sent_report_type;
	event->subscriber = &state;
	event->error = error;
	EVENT_SUBMIT(event);

	/* Used to assert if previous report was sent before sending new one. */
	sent_report_type = IN_REPORT_COUNT;
}

static void report_sent_cb(void)
{
	report_sent(false);
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

		__ASSERT(sizeof(buffer) == 6, "Invalid report size");

		/* Encode report. */
		buffer[0] = REPORT_ID_MOUSE;
		buffer[1] = event->button_bm;
		buffer[2] = wheel;
		buffer[3] = x_buff[0];
		buffer[4] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
		buffer[5] = (y_buff[1] << 4) | (y_buff[0] >> 4);

	} else {
		s8_t x = MAX(MIN(event->dx, MOUSE_REPORT_XY_MAX_BOOT),
				MOUSE_REPORT_XY_MIN_BOOT);
		s8_t y = MAX(MIN(event->dy, MOUSE_REPORT_XY_MAX_BOOT),
				MOUSE_REPORT_XY_MIN_BOOT);

		__ASSERT(sizeof(buffer) == 3, "Invalid boot report size");

		buffer[0] = event->button_bm;
		buffer[1] = x;
		buffer[2] = y;

	}

	__ASSERT_NO_MSG(sent_report_type == IN_REPORT_COUNT);
	sent_report_type = IN_REPORT_MOUSE;

	int err = hid_int_ep_write(usb_dev, buffer, sizeof(buffer), NULL);

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		report_sent(true);
	}
}

static void send_keyboard_report(const struct hid_keyboard_event *event)
{
	if (&state != event->subscriber) {
		/* It's not us */
		return;
	}

	if (state != USB_STATE_ACTIVE) {
		/* USB not connected. */
		return;
	}

	u8_t buffer[REPORT_SIZE_KEYBOARD_KEYS + sizeof(u8_t)];

	if (hid_protocol == HID_PROTOCOL_REPORT) {
		__ASSERT(sizeof(buffer) == 9, "Invalid report size");
		/* Encode report. */
		buffer[0] = REPORT_ID_KEYBOARD_KEYS;
		buffer[1] = event->modifier_bm;
		buffer[2] = 0;
		memcpy(&buffer[3], event->keys, ARRAY_SIZE(event->keys));
	} else {
		__ASSERT_NO_MSG(false);
	}

	__ASSERT_NO_MSG(sent_report_type == IN_REPORT_COUNT);
	sent_report_type = IN_REPORT_KEYBOARD_KEYS;

	int err = hid_int_ep_write(usb_dev, buffer, sizeof(buffer), NULL);

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		report_sent(true);
	}
}

static void send_ctrl_report(const struct hid_ctrl_event *event)
{
	u8_t report_id = 0;
	size_t report_size = 0;

	switch (event->report_type) {
	case IN_REPORT_SYSTEM_CTRL:
		report_id = REPORT_ID_SYSTEM_CTRL;
		report_size = REPORT_SIZE_SYSTEM_CTRL;
		break;
	case IN_REPORT_CONSUMER_CTRL:
		report_id = REPORT_ID_CONSUMER_CTRL;
		report_size = REPORT_SIZE_CONSUMER_CTRL;
		break;
	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	if (&state != event->subscriber) {
		/* It's not us */
		return;
	}

	if (state != USB_STATE_ACTIVE) {
		/* USB not connected. */
		return;
	}

	u8_t buffer[report_size + sizeof(u8_t)];

	if (hid_protocol == HID_PROTOCOL_REPORT) {
		__ASSERT(sizeof(buffer) == 3, "Invalid report size");
		/* Encode report. */
		buffer[0] = report_id;
		sys_put_le16(event->usage, &buffer[1]);
	} else {
		/* Do not send when in boot mode. */
		sent_report_type = event->report_type;
		report_sent(false);
		return;
	}

	__ASSERT_NO_MSG(sent_report_type == IN_REPORT_COUNT);
	sent_report_type = event->report_type;

	int err = hid_int_ep_write(usb_dev, buffer, sizeof(buffer), NULL);

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		report_sent(true);
	}
}

static void broadcast_usb_state(void)
{
	struct usb_state_event *event = new_usb_state_event();

	event->state = state;
	event->id = &state;

	EVENT_SUBMIT(event);
}

static void reset_pending_report(void)
{
	if (sent_report_type != IN_REPORT_COUNT) {
		LOG_WRN("USB clear report notification waiting flag");
		sent_report_type = IN_REPORT_COUNT;
	}
}

static void broadcast_subscription_change(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		struct hid_report_subscription_event *event_mouse =
			new_hid_report_subscription_event();

		event_mouse->report_type = IN_REPORT_MOUSE;
		event_mouse->enabled     = (state == USB_STATE_ACTIVE);
		event_mouse->subscriber  = &state;

		EVENT_SUBMIT(event_mouse);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		struct hid_report_subscription_event *event_kbd =
			new_hid_report_subscription_event();

		event_kbd->report_type = IN_REPORT_KEYBOARD_KEYS;
		event_kbd->enabled     = (state == USB_STATE_ACTIVE);
		event_kbd->subscriber  = &state;

		EVENT_SUBMIT(event_kbd);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT)) {
		struct hid_report_subscription_event *event_system_ctrl =
			new_hid_report_subscription_event();

		event_system_ctrl->report_type = IN_REPORT_SYSTEM_CTRL;
		event_system_ctrl->enabled     = (state == USB_STATE_ACTIVE);
		event_system_ctrl->subscriber  = &state;

		EVENT_SUBMIT(event_system_ctrl);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) {
		struct hid_report_subscription_event *event_consumer_ctrl =
			new_hid_report_subscription_event();

		event_consumer_ctrl->report_type = IN_REPORT_CONSUMER_CTRL;
		event_consumer_ctrl->enabled     = (state == USB_STATE_ACTIVE);
		event_consumer_ctrl->subscriber  = &state;

		EVENT_SUBMIT(event_consumer_ctrl);
	}

	LOG_INF("USB HID %sabled", (state == USB_STATE_ACTIVE) ? ("en"):("dis"));
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
		if (state == USB_STATE_DISCONNECTED) {
			/* Due to the way USB driver and stack are written
			 * some events may be issued before application
			 * connect its callback.
			 * We assume that device was powered.
			 */
			state = USB_STATE_POWERED;
			LOG_WRN("USB suspended while disconnected");
		}
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
			reset_pending_report();
		}

		broadcast_usb_state();

		if (new_state == USB_STATE_ACTIVE) {
			broadcast_subscription_change();
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
		    new_state != USB_STATE_ACTIVE) {
			config_channel_disconnect(&cfg_chan);
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
	usb_dev = device_get_binding(CONFIG_USB_HID_DEVICE_NAME "_0");
	if (usb_dev == NULL) {
		return -ENXIO;
	}

	static const struct hid_ops ops = {
		.get_report   		= get_report,
		.set_report   		= set_report,
		.int_in_ready		= report_sent_cb,
		.protocol_change 	= protocol_change,
	};

	usb_hid_register_device(usb_dev, hid_report_desc,
				hid_report_desc_size, &ops);

	int err = usb_hid_init(usb_dev);
	if (err) {
		LOG_ERR("Cannot initialize HID class");
		return err;
	}

	err = usb_enable(device_status);
	if (err) {
		LOG_ERR("Cannot enable USB");
		return err;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		config_channel_init(&cfg_chan);
	}

	return err;
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
	     IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPORT)) &&
	    is_hid_ctrl_event(eh)) {
		send_ctrl_report(cast_hid_ctrl_event(eh));

		return false;
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

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    is_config_forwarded_event(eh)) {
		config_channel_forwarded_receive(&cfg_chan,
						 cast_config_forwarded_event(eh));

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
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE(MODULE, hid_ctrl_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_forwarded_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_event);
EVENT_SUBSCRIBE(MODULE, config_event);
#endif
