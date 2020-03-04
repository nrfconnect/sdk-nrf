/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <sys/util.h>

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
static u8_t sent_report_id = REPORT_ID_COUNT;

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

	event->report_id = sent_report_id;
	event->subscriber = &state;
	event->error = error;
	EVENT_SUBMIT(event);

	/* Used to assert if previous report was sent before sending new one. */
	sent_report_id = REPORT_ID_COUNT;
}

static void report_sent_cb(void)
{
	report_sent(false);
}

static void send_hid_report(const struct hid_report_event *event)
{
	if (&state != event->subscriber) {
		/* It's not us */
		return;
	}

	if (state != USB_STATE_ACTIVE) {
		/* USB not connected. */
		return;
	}

	if (hid_protocol != HID_PROTOCOL_REPORT) {
		/* Not supported */
		return;
	}

	__ASSERT_NO_MSG(sent_report_id == REPORT_ID_COUNT);
	__ASSERT_NO_MSG(event->dyndata.size > 0);

	sent_report_id = event->dyndata.data[0];

	int err = hid_int_ep_write(usb_dev, event->dyndata.data,
				   event->dyndata.size, NULL);

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
	if (sent_report_id != REPORT_ID_COUNT) {
		LOG_WRN("USB clear report notification waiting flag");
		sent_report_id = REPORT_ID_COUNT;
	}
}

static void broadcast_subscription_change(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_MOUSE;
		event->enabled    = (state == USB_STATE_ACTIVE);
		event->subscriber = &state;

		EVENT_SUBMIT(event);
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_KEYBOARD_KEYS;
		event->enabled    = (state == USB_STATE_ACTIVE);
		event->subscriber = &state;

		EVENT_SUBMIT(event);
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT)) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_SYSTEM_CTRL;
		event->enabled    = (state == USB_STATE_ACTIVE);
		event->subscriber = &state;

		EVENT_SUBMIT(event);
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_CONSUMER_CTRL;
		event->enabled    = (state == USB_STATE_ACTIVE);
		event->subscriber = &state;

		EVENT_SUBMIT(event);
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
	if (is_hid_report_event(eh)) {
		send_hid_report(cast_hid_report_event(eh));

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
EVENT_SUBSCRIBE(MODULE, hid_report_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_forwarded_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_event);
EVENT_SUBSCRIBE(MODULE, config_event);
#endif
