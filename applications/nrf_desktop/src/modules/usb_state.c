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
#include "config_channel_transport.h"

#include "hid_event.h"
#include "usb_event.h"
#include "config_event.h"

#define REPORT_TYPE_INPUT	0x01
#define REPORT_TYPE_OUTPUT	0x02
#define REPORT_TYPE_FEATURE	0x03


#ifndef CONFIG_USB_HID_PROTOCOL_CODE
#define CONFIG_USB_HID_PROTOCOL_CODE -1
#endif

static enum usb_state state;
static uint8_t hid_protocol = HID_PROTOCOL_REPORT;
static struct device *usb_dev;
static uint8_t sent_report_id = REPORT_ID_COUNT;

static struct config_channel_transport cfg_chan_transport;

static int get_report(struct usb_setup_packet *setup, int32_t *len, uint8_t **data)
{
	uint8_t request_value[2];

	sys_put_le16(setup->wValue, request_value);

	switch (request_value[1]) {
	case REPORT_TYPE_FEATURE:
		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			if (request_value[0] == REPORT_ID_USER_CONFIG) {
				size_t length = *len;
				uint8_t *buffer = *data;

				/* HID Feature report ID is specific to USB.
				 * Config channel does not use it.
				 */
				buffer[0] = REPORT_ID_USER_CONFIG;
				int err = config_channel_transport_get(&cfg_chan_transport,
								&buffer[1],
								length - 1);

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

static int set_report(struct usb_setup_packet *setup, int32_t *len, uint8_t **data)
{
	uint8_t request_value[2];

	sys_put_le16(setup->wValue, request_value);

	switch (request_value[1]) {
	case REPORT_TYPE_FEATURE:
		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
			if (request_value[0] == REPORT_ID_USER_CONFIG) {
				size_t length = *len;
				uint8_t *buffer = *data;

				/* HID Feature report ID is specific to USB.
				 * Config channel does not use it.
				 */
				int err = config_channel_transport_set(&cfg_chan_transport,
								&buffer[1],
								length - 1);

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
	__ASSERT_NO_MSG(sent_report_id != REPORT_ID_COUNT);

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
		sent_report_id = event->dyndata.data[0];
		report_sent(true);
		return;
	}

	const uint8_t *report_buffer = event->dyndata.data;
	size_t report_size = event->dyndata.size;

	__ASSERT_NO_MSG(report_size > 0);
	__ASSERT_NO_MSG(sent_report_id == REPORT_ID_COUNT);

	if (hid_protocol != HID_PROTOCOL_REPORT) {
		if ((IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) &&
		     (report_buffer[0] == REPORT_ID_BOOT_MOUSE)) ||
		    (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
		     (report_buffer[0] == REPORT_ID_BOOT_KEYBOARD))) {
			sent_report_id = event->dyndata.data[0];
			/* For boot protocol omit the first byte. */
			report_buffer++;
			report_size--;
			__ASSERT_NO_MSG(report_size > 0);
		} else {
			/* Boot protocol is not supported or this is not a
			 * boot report.
			 */
			sent_report_id = event->dyndata.data[0];
			report_sent(true);
			return;
		}
	} else {
		sent_report_id = event->dyndata.data[0];
	}

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
		report_sent(true);
	}
}

static void broadcast_subscription_change(void)
{
	static bool report_enabled[REPORT_ID_COUNT];

	bool new_rep_enabled = (state == USB_STATE_ACTIVE) &&
			       (hid_protocol == HID_PROTOCOL_REPORT);
	bool new_boot_enabled = (state == USB_STATE_ACTIVE) &&
				(hid_protocol == HID_PROTOCOL_BOOT);

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT) &&
	    (new_rep_enabled != report_enabled[REPORT_ID_MOUSE])) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_MOUSE;
		event->enabled    = new_rep_enabled;
		event->subscriber = &state;

		EVENT_SUBMIT(event);

		report_enabled[REPORT_ID_MOUSE] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) &&
	    (new_rep_enabled != report_enabled[REPORT_ID_KEYBOARD_KEYS])) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_KEYBOARD_KEYS;
		event->enabled    = new_rep_enabled;
		event->subscriber = &state;

		EVENT_SUBMIT(event);

		report_enabled[REPORT_ID_KEYBOARD_KEYS] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT) &&
	    (new_rep_enabled != report_enabled[REPORT_ID_SYSTEM_CTRL])) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_SYSTEM_CTRL;
		event->enabled    = new_rep_enabled;
		event->subscriber = &state;

		EVENT_SUBMIT(event);
		report_enabled[REPORT_ID_SYSTEM_CTRL] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT) &&
	    (new_rep_enabled != report_enabled[REPORT_ID_CONSUMER_CTRL])) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_CONSUMER_CTRL;
		event->enabled    = new_rep_enabled;
		event->subscriber = &state;

		EVENT_SUBMIT(event);
		report_enabled[REPORT_ID_CONSUMER_CTRL] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) &&
	    (new_boot_enabled != report_enabled[REPORT_ID_BOOT_MOUSE])) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_BOOT_MOUSE;
		event->enabled    = new_boot_enabled;
		event->subscriber = &state;

		EVENT_SUBMIT(event);
		report_enabled[REPORT_ID_BOOT_MOUSE] = new_boot_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
	    (new_boot_enabled != report_enabled[REPORT_ID_BOOT_KEYBOARD])) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_BOOT_KEYBOARD;
		event->enabled    = new_boot_enabled;
		event->subscriber = &state;

		EVENT_SUBMIT(event);
		report_enabled[REPORT_ID_BOOT_KEYBOARD] = new_boot_enabled;
	}

	LOG_INF("USB HID %sabled", (state == USB_STATE_ACTIVE) ? ("en"):("dis"));
	if (state == USB_STATE_ACTIVE) {
		LOG_INF("%s_PROTOCOL active", hid_protocol ? "REPORT" : "BOOT");
	}
}

static void device_status(enum usb_dc_status_code cb_status, const uint8_t *param)
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

	case USB_DC_INTERFACE:
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
			hid_protocol = HID_PROTOCOL_REPORT;
			broadcast_subscription_change();
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
		    new_state != USB_STATE_ACTIVE) {
			config_channel_transport_disconnect(&cfg_chan_transport);
		}
	}
}

static void protocol_change(uint8_t protocol)
{
	BUILD_ASSERT(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_DISABLED) ==
			 !IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL),
			 "Boot protocol setup inconsistency");
	BUILD_ASSERT(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) ==
			 (IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL) && (CONFIG_USB_HID_PROTOCOL_CODE == 1)),
			 "Boot protocol code does not reflect selected interface");
	BUILD_ASSERT(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) ==
			 (IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL) && (CONFIG_USB_HID_PROTOCOL_CODE == 2)),
			 "Boot protocol code does not reflect selected interface");

	if ((protocol != HID_PROTOCOL_BOOT) &&
	    (protocol != HID_PROTOCOL_REPORT)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_DISABLED) &&
	    (protocol == HID_PROTOCOL_BOOT)) {
		LOG_WRN("BOOT protocol is not supported");
		return;
	}

	hid_protocol = protocol;

	if (state == USB_STATE_ACTIVE) {
		broadcast_subscription_change();
	}
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
		config_channel_transport_init(&cfg_chan_transport);
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
	    is_config_event(eh)) {
		config_channel_transport_rsp_receive(&cfg_chan_transport,
						     cast_config_event(eh));

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
EVENT_SUBSCRIBE(MODULE, config_event);
#endif
