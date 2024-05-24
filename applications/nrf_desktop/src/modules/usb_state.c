/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <stdint.h>

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_hid.h>

#define MODULE usb_state
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_USB_STATE_LOG_LEVEL);

#include "hid_report_desc.h"
#include "config_channel_transport.h"

#include "hid_event.h"
#include "usb_event.h"
#include "config_event.h"
#include <caf/events/power_event.h>

#if CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION
  #include "usb_state_def.h"
#endif /* CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION */

#define REPORT_TYPE_INPUT	0x01
#define REPORT_TYPE_OUTPUT	0x02
#define REPORT_TYPE_FEATURE	0x03

#define USB_SUBSCRIBER_PRIORITY      CONFIG_DESKTOP_USB_SUBSCRIBER_REPORT_PRIORITY
#define USB_SUBSCRIBER_PIPELINE_SIZE 0x01
#define USB_SUBSCRIBER_REPORT_MAX    0x01

/* The definitions are available and used only for USB legacy stack.
 * Define them explicitly if needed to allow compiling code without USB legacy stack.
 */
#ifndef CONFIG_DESKTOP_USB_STACK_LEGACY
	#define CONFIG_USB_HID_DEVICE_NAME ""
	#define CONFIG_DESKTOP_USB_HID_PROTOCOL_CODE -1
#endif

struct usb_hid_device {
	const struct device *dev;
	uint32_t report_bm;
	uint8_t hid_protocol;
	uint8_t sent_report_id;
	bool report_enabled[REPORT_ID_COUNT];
	bool enabled;
};


static enum usb_state state;

BUILD_ASSERT(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT) ||
	     IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY),
	     "Unsupported USB stack");

#if CONFIG_DESKTOP_USB_STACK_NEXT
static struct usb_hid_device usb_hid_device[0];
#else
static struct usb_hid_device usb_hid_device[CONFIG_USB_HID_DEVICE_COUNT];
#endif

BUILD_ASSERT(DT_PROP(DT_NODELABEL(usbd), num_in_endpoints) > ARRAY_SIZE(usb_hid_device),
	     "Too few USB IN Endpoints enabled. Modify dts.overlay file.");

BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_HID_STATE_ENABLE) ||
	     IS_ENABLED(CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION) ||
	     (ARRAY_SIZE(usb_hid_device) <= 1));

static struct config_channel_transport cfg_chan_transport;


static struct usb_hid_device *dev_to_hid(const struct device *dev)
{
	struct usb_hid_device *usb_hid = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		if (usb_hid_device[i].dev == dev) {
			usb_hid = &usb_hid_device[i];
			break;
		}
	}

	if (usb_hid == NULL) {
		__ASSERT_NO_MSG(false);
	}

	return usb_hid;
}

static int get_report(const struct device *dev, uint8_t report_type, uint8_t report_id,
		      uint8_t *buf, size_t size)
{
	int err = -ENOTSUP;

	switch (report_type) {
	case REPORT_TYPE_FEATURE:
		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
		    (report_id == REPORT_ID_USER_CONFIG) &&
		    (size == REPORT_SIZE_USER_CONFIG)) {
			if (dev == usb_hid_device[0].dev) {
				err = config_channel_transport_get(&cfg_chan_transport, buf, size);
			} else {
				err = config_channel_transport_get_disabled(buf, size);
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

	return err;
}

static int set_report(const struct device *dev, uint8_t report_type, uint8_t report_id,
		      const uint8_t *buf, size_t size)
{
	int err = -ENOTSUP;

	switch (report_type) {
	case REPORT_TYPE_FEATURE:
		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
		    (report_id == REPORT_ID_USER_CONFIG) &&
		    (size == REPORT_SIZE_USER_CONFIG)) {
			if (dev == usb_hid_device[0].dev) {
				err = config_channel_transport_set(&cfg_chan_transport, buf, size);
			} else {
				/* Ignore the request. Configuration channel does not use this USB
				 * HID instance.
				 */
				err = 0;
			}
		}
		break;

	case REPORT_TYPE_OUTPUT:
		if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
			struct usb_hid_device *usb_hid = dev_to_hid(dev);

			if ((report_id == REPORT_ID_KEYBOARD_LEDS) &&
			    (usb_hid->hid_protocol == HID_PROTOCOL_REPORT) &&
			    (size == REPORT_SIZE_KEYBOARD_LEDS)) {
				/* Handle HID keyboard LEDs report. */
			} else if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
				  (report_id == REPORT_ID_RESERVED) &&
				  (usb_hid->hid_protocol == HID_PROTOCOL_BOOT) &&
				  (size == REPORT_SIZE_KEYBOARD_LEDS)) {
				/* Handle HID boot keyboard LEDs report. */
			} else {
				/* Ignore invalid report. */
				break;
			}

			size_t dyndata_len = sizeof(uint8_t) + REPORT_SIZE_KEYBOARD_LEDS;
			struct hid_report_event *event = new_hid_report_event(dyndata_len);

			event->source = usb_hid;
			/* Subscriber is not specified for HID output report. */
			event->subscriber = NULL;

			uint8_t *evt_buf = event->dyndata.data;

			/* Explicitly add report ID. */
			evt_buf[0] = REPORT_ID_KEYBOARD_LEDS;
			evt_buf++;

			memcpy(evt_buf, buf, size);
			APP_EVENT_SUBMIT(event);

			err = 0;
		}
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}

	return err;
}

static void report_sent(const struct device *dev, bool error)
{
	struct usb_hid_device *usb_hid = dev_to_hid(dev);

	__ASSERT_NO_MSG(usb_hid->sent_report_id != REPORT_ID_COUNT);

	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_id = usb_hid->sent_report_id;
	event->subscriber = usb_hid;
	event->error = error;
	APP_EVENT_SUBMIT(event);

	/* Used to assert if previous report was sent before sending new one. */
	usb_hid->sent_report_id = REPORT_ID_COUNT;
}

static void report_sent_cb(const struct device *dev)
{
	report_sent(dev, false);
}

static void send_hid_report(const struct hid_report_event *event)
{
	struct usb_hid_device *usb_hid = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		if (event->subscriber == &usb_hid_device[i]) {
			usb_hid = &usb_hid_device[i];
			break;
		}
	}

	if (!usb_hid) {
		/* It's not us */
		return;
	}

	const uint8_t *report_buffer = event->dyndata.data;
	size_t report_size = event->dyndata.size;

	__ASSERT_NO_MSG(report_size > 0);

	if (!usb_hid->enabled) {
		/* USB not connected. */
		usb_hid->sent_report_id = event->dyndata.data[0];
		report_sent(usb_hid->dev, true);
		return;
	}

	__ASSERT_NO_MSG(usb_hid->sent_report_id == REPORT_ID_COUNT);

	usb_hid->sent_report_id = event->dyndata.data[0];
	if (usb_hid->hid_protocol != HID_PROTOCOL_REPORT) {
		if ((report_buffer[0] != REPORT_ID_BOOT_MOUSE) &&
		    (report_buffer[0] != REPORT_ID_BOOT_KEYBOARD)) {
			LOG_WRN("Cannot send report: incompatible mode");
			report_sent(usb_hid->dev, true);
			return;
		}
		if ((IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) &&
		     (report_buffer[0] == REPORT_ID_BOOT_MOUSE)) ||
		    (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
		     (report_buffer[0] == REPORT_ID_BOOT_KEYBOARD))) {
			/* For boot protocol omit the first byte. */
			report_buffer++;
			report_size--;
			__ASSERT_NO_MSG(report_size > 0);
		} else {
			/* Boot protocol is not supported. */
			report_sent(usb_hid->dev, true);
			return;
		}
	} else {
		if ((report_buffer[0] == REPORT_ID_BOOT_MOUSE) ||
		    (report_buffer[0] == REPORT_ID_BOOT_KEYBOARD)) {
			LOG_WRN("Cannot send report: incompatible mode");
			report_sent(usb_hid->dev, true);
			return;
		}
	}

	int err = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT)) {
		LOG_WRN("send_hid_report not integrated for USB next");
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY));
		err = hid_int_ep_write(usb_hid->dev, report_buffer, report_size, NULL);
	}

	if (err) {
		LOG_ERR("Cannot send report (%d)", err);
		report_sent(usb_hid->dev, true);
	}
}

static void broadcast_usb_state(void)
{
	struct usb_state_event *event = new_usb_state_event();

	event->state = state;

	APP_EVENT_SUBMIT(event);
}

static void broadcast_subscriber_change(struct usb_hid_device *usb_hid)
{
	struct hid_report_subscriber_event *event = new_hid_report_subscriber_event();

	event->subscriber = usb_hid;
	event->params.pipeline_size = USB_SUBSCRIBER_PIPELINE_SIZE;
	event->params.priority = USB_SUBSCRIBER_PRIORITY;
	event->params.report_max = USB_SUBSCRIBER_REPORT_MAX;
	event->connected = usb_hid->enabled;

	APP_EVENT_SUBMIT(event);
}

static void broadcast_subscription_change(struct usb_hid_device *usb_hid)
{
	bool new_rep_enabled = (usb_hid->enabled) && (usb_hid->hid_protocol == HID_PROTOCOL_REPORT);
	bool new_boot_enabled = (usb_hid->enabled) && (usb_hid->hid_protocol == HID_PROTOCOL_BOOT);

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT) &&
	    (new_rep_enabled != usb_hid->report_enabled[REPORT_ID_MOUSE]) &&
	    (usb_hid->report_bm & BIT(REPORT_ID_MOUSE))) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_MOUSE;
		event->enabled    = new_rep_enabled;
		event->subscriber = usb_hid;

		APP_EVENT_SUBMIT(event);

		usb_hid->report_enabled[REPORT_ID_MOUSE] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) &&
	    (new_rep_enabled != usb_hid->report_enabled[REPORT_ID_KEYBOARD_KEYS]) &&
	    (usb_hid->report_bm & BIT(REPORT_ID_KEYBOARD_KEYS))) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_KEYBOARD_KEYS;
		event->enabled    = new_rep_enabled;
		event->subscriber = usb_hid;

		APP_EVENT_SUBMIT(event);

		usb_hid->report_enabled[REPORT_ID_KEYBOARD_KEYS] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT) &&
	    (new_rep_enabled != usb_hid->report_enabled[REPORT_ID_SYSTEM_CTRL]) &&
	    (usb_hid->report_bm & BIT(REPORT_ID_SYSTEM_CTRL))) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_SYSTEM_CTRL;
		event->enabled    = new_rep_enabled;
		event->subscriber = usb_hid;

		APP_EVENT_SUBMIT(event);
		usb_hid->report_enabled[REPORT_ID_SYSTEM_CTRL] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT) &&
	    (new_rep_enabled != usb_hid->report_enabled[REPORT_ID_CONSUMER_CTRL]) &&
	    (usb_hid->report_bm & BIT(REPORT_ID_CONSUMER_CTRL))) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_CONSUMER_CTRL;
		event->enabled    = new_rep_enabled;
		event->subscriber = usb_hid;

		APP_EVENT_SUBMIT(event);
		usb_hid->report_enabled[REPORT_ID_CONSUMER_CTRL] = new_rep_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) &&
	    (new_boot_enabled != usb_hid->report_enabled[REPORT_ID_BOOT_MOUSE]) &&
	    (usb_hid->report_bm & BIT(REPORT_ID_BOOT_MOUSE))) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_BOOT_MOUSE;
		event->enabled    = new_boot_enabled;
		event->subscriber = usb_hid;

		APP_EVENT_SUBMIT(event);
		usb_hid->report_enabled[REPORT_ID_BOOT_MOUSE] = new_boot_enabled;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
	    (new_boot_enabled != usb_hid->report_enabled[REPORT_ID_BOOT_KEYBOARD]) &&
	    (usb_hid->report_bm & BIT(REPORT_ID_BOOT_KEYBOARD))) {
		struct hid_report_subscription_event *event =
			new_hid_report_subscription_event();

		event->report_id  = REPORT_ID_BOOT_KEYBOARD;
		event->enabled    = new_boot_enabled;
		event->subscriber = usb_hid;

		APP_EVENT_SUBMIT(event);
		usb_hid->report_enabled[REPORT_ID_BOOT_KEYBOARD] = new_boot_enabled;
	}

	LOG_INF("USB HID %p %sabled", (void *)usb_hid, (usb_hid->enabled) ? ("en"):("dis"));
	if (usb_hid->enabled) {
		LOG_INF("%s_PROTOCOL active", usb_hid->hid_protocol ? "REPORT" : "BOOT");
	}
}

static void update_usb_hid(struct usb_hid_device *usb_hid, bool enabled)
{
	if (usb_hid->enabled == enabled) {
		/* Already updated. */
		return;
	}

	usb_hid->enabled = enabled;

	if (usb_hid->enabled) {
		usb_hid->hid_protocol = HID_PROTOCOL_REPORT;
		broadcast_subscriber_change(usb_hid);
	}

	broadcast_subscription_change(usb_hid);

	if (!usb_hid->enabled) {
		broadcast_subscriber_change(usb_hid);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    (usb_hid == &usb_hid_device[0]) && !usb_hid->enabled) {
		config_channel_transport_disconnect(&cfg_chan_transport);
	}
}

static void protocol_change(const struct device *dev, uint8_t protocol)
{
	struct usb_hid_device *usb_hid = dev_to_hid(dev);

	BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY) ||
		     (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_DISABLED) ==
		      !IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL)),
		     "Boot protocol setup inconsistency");
	BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY) ||
		     (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) ==
		      ((IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL) &&
		       (CONFIG_DESKTOP_USB_HID_PROTOCOL_CODE == 1)))),
		    "Boot protocol code does not reflect selected interface");
	BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY) ||
		     (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) ==
		      ((IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL) &&
		       (CONFIG_DESKTOP_USB_HID_PROTOCOL_CODE == 2)))),
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

	if (usb_hid->hid_protocol == protocol) {
		/* Already updated. */
		return;
	}

	usb_hid->hid_protocol = protocol;

	if (usb_hid->enabled) {
		broadcast_subscription_change(usb_hid);
	}
}

static void usb_wakeup(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT)) {
		LOG_WRN("usb_wakeup not integrated for USB next");
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY));
		err = usb_wakeup_request();
	}

	if (!err) {
		LOG_INF("USB wakeup requested");
	} else if (err == -EAGAIN) {
		/* Already woken up - waiting for host */
		LOG_WRN("USB wakeup pending");
	} else if (err == -EACCES) {
		LOG_INF("USB wakeup was not enabled by the host");
	} else {
		LOG_ERR("USB wakeup request failed (err:%d)", err);
	}
}

static uint32_t get_report_bm(size_t hid_id)
{
	BUILD_ASSERT(REPORT_ID_COUNT <=
		     sizeof(usb_hid_device[0].report_bm) * CHAR_BIT);
#if CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION
	BUILD_ASSERT(ARRAY_SIZE(usb_hid_report_bm) ==
		     ARRAY_SIZE(usb_hid_device));
	return usb_hid_report_bm[hid_id];
#else
	return UINT32_MAX;
#endif
}

static void verify_report_bm(void)
{
#if defined(CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION) && defined(CONFIG_ASSERT)
	/* Make sure that selected reports bitmasks are proper. */
	uint32_t common_bitmask = 0;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		/* USB HID instance that does not subscribe to any report
		 * should be removed. On nRF Desktop peripheral device, given
		 * HID report can be handled only by one USB HID instance.
		 */
		__ASSERT_NO_MSG(usb_hid_report_bm[i] != 0);
		__ASSERT_NO_MSG((common_bitmask & usb_hid_report_bm[i]) == 0);
		common_bitmask |= usb_hid_report_bm[i];
	}
#endif
}

static void usb_legacy_reset_pending_report(struct usb_hid_device *usb_hid)
{
	if (usb_hid->sent_report_id != REPORT_ID_COUNT) {
		LOG_WRN("USB clear report notification waiting flag");
		report_sent(usb_hid->dev, true);
	}
}

static void report_legacy_wValue_parse(uint16_t wValue, uint8_t *report_type, uint8_t *report_id)
{
	uint8_t request_value[2];

	sys_put_le16(wValue, request_value);
	*report_type = request_value[1];
	*report_id = request_value[0];
}

static int get_report_legacy(const struct device *dev, struct usb_setup_packet *setup,
			     int32_t *len_to_set, uint8_t **data)
{
	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(setup);
	__ASSERT_NO_MSG(len_to_set);
	__ASSERT_NO_MSG(data);

	uint8_t report_type;
	uint8_t report_id;
	uint8_t *buf = *data;
	size_t size = setup->wLength;

	report_legacy_wValue_parse(setup->wValue, &report_type, &report_id);

	if (report_id != REPORT_ID_RESERVED) {
		buf[0] = report_id;
		buf++;
		size--;
	}

	int err = get_report(dev, report_type, report_id, buf, size);

	if (err) {
		LOG_WRN("get_report_legacy failed (err: %d)", err);
		LOG_WRN("bmRequestType: %02X bRequest: %02X wValue: %04X wIndex: %04X"
			" wLength: %04X", setup->bmRequestType, setup->bRequest,
			setup->wValue, setup->wIndex, setup->wLength);
		LOG_HEXDUMP_WRN(buf, size, "Get report");
	} else {
		*len_to_set = setup->wLength;
	}

	return err;
}

static int set_report_legacy(const struct device *dev, struct usb_setup_packet *setup,
			     int32_t *len, uint8_t **data)
{
	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(setup);
	__ASSERT_NO_MSG(len);
	__ASSERT_NO_MSG(data);

	uint8_t report_type;
	uint8_t report_id;
	uint8_t *buf = *data;
	size_t size = setup->wLength;

	report_legacy_wValue_parse(setup->wValue, &report_type, &report_id);

	int err = 0;

	if (report_id != REPORT_ID_RESERVED) {
		/* The first byte of buf is report ID. Validate it and omit. */
		if (report_id == buf[0]) {
			buf++;
			size--;
		} else {
			LOG_WRN("HID report ID mismatch");
			err = -EINVAL;
		}
	}

	if (!err) {
		err = set_report(dev, report_type, report_id, buf, size);
	}

	if (err) {
		LOG_WRN("set_report_legacy failed (err: %d)", err);
		LOG_WRN("bmRequestType: %02X bRequest: %02X wValue: %04X wIndex: %04X"
			" wLength: %04X", setup->bmRequestType, setup->bRequest,
			setup->wValue, setup->wIndex, setup->wLength);
		LOG_HEXDUMP_WRN(buf, size, "Set report");
	}

	return err;
}

static int usb_init_legacy_hid_device_init(struct usb_hid_device *usb_hid_dev,
					   const struct device *dev,
					   uint32_t report_bm)
{
	static const struct hid_ops hid_ops = {
		.get_report = get_report_legacy,
		.set_report = set_report_legacy,
		.int_in_ready = report_sent_cb,
		.protocol_change = protocol_change,
	};

	usb_hid_dev->dev = dev;
	usb_hid_dev->hid_protocol = HID_PROTOCOL_REPORT;
	usb_hid_dev->sent_report_id = REPORT_ID_COUNT;
	usb_hid_dev->report_bm = report_bm;

	usb_hid_register_device(dev, hid_report_desc, hid_report_desc_size, &hid_ops);

	int err = 0;

	/* Legacy way of setting HID boot protocol requires an additional API call. */
	if (IS_ENABLED(CONFIG_USB_HID_BOOT_PROTOCOL)) {
		err = usb_hid_set_proto_code(dev, CONFIG_DESKTOP_USB_HID_PROTOCOL_CODE);
		if (err) {
			LOG_ERR("usb_hid_set_proto_code failed for dev: %p (err: %d)",
				(void *)dev, err);
			return err;
		}
	}

	err = usb_hid_init(dev);
	if (err) {
		LOG_ERR("usb_hid_init failed for dev: %p (err: %d)", (void *)dev, err);
		return err;
	}

	return 0;
}

static void usb_init_legacy_status_cb(enum usb_dc_status_code cb_status, const uint8_t *param)
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
			LOG_WRN("USB resume after reset");
		}
		new_state = USB_STATE_POWERED;
		break;

	case USB_DC_SUSPEND:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		before_suspend = state;
		new_state = USB_STATE_SUSPENDED;
		LOG_WRN("USB suspend");
		break;

	case USB_DC_RESUME:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		if (state == USB_STATE_SUSPENDED) {
			new_state = before_suspend;
			LOG_WRN("USB resume");
		}
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

	if (new_state == state) {
		/* Update not needed. */
		return;
	}

	/* HID unsubscribe if leaving active state. */
	if (state == USB_STATE_ACTIVE) {
		for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
			/* USB legacy stack does not notify application about sent report when no
			 * longer configured.
			 */
			usb_legacy_reset_pending_report(&usb_hid_device[i]);
			update_usb_hid(&usb_hid_device[i], false);
		}
	}

	/* Update state. */
	state = new_state;
	broadcast_usb_state();

	/* HID subscribe when entering active state. */
	if (state == USB_STATE_ACTIVE) {
		for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
			update_usb_hid(&usb_hid_device[i], true);
		}
	}
}

static int usb_init_legacy_hids_init(void)
{
	int err = 0;
	char name[32];
	const struct device *dev;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		err = snprintf(name, sizeof(name), CONFIG_USB_HID_DEVICE_NAME "_%zu", i);
		if ((err < 0) || (err >= sizeof(name))) {
			LOG_ERR("HID dev name snprintf failed (err: %d)", err);
			return err;
		}

		dev = device_get_binding(name);
		if (!dev) {
			LOG_ERR("No device %s", name);
			return -ENXIO;
		}

		err = usb_init_legacy_hid_device_init(&usb_hid_device[i], dev, get_report_bm(i));
		if (err) {
			LOG_ERR("usb_init_legacy_hid_device_init failed for %zu (err: %d)", i, err);
			return err;
		}
	}

	return err;
}

static int usb_init_legacy(void)
{
	int err = usb_init_legacy_hids_init();

	if (err) {
		LOG_ERR("usb_init_legacy_hids_init failed (err: %d)", err);
		return err;
	}

	err = usb_enable(usb_init_legacy_status_cb);
	if (err) {
		LOG_ERR("usb_enable failed (err: %d)", err);
		return err;
	}

	return 0;
}

static int usb_init(void)
{
	int err = 0;

	verify_report_bm();

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		config_channel_transport_init(&cfg_chan_transport);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT)) {
		LOG_WRN("USB next stack APIs are not yet in use");
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY));
		err = usb_init_legacy();
	}

	if (err) {
		LOG_ERR("Cannot enable USB (err: %d)", err);
	}

	return err;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_event(aeh)) {
		send_hid_report(cast_hid_report_event(aeh));

		return false;
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (usb_init()) {
				LOG_ERR("USB stack initialization failed");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    is_config_event(aeh)) {
		config_channel_transport_rsp_receive(&cfg_chan_transport,
						     cast_config_event(aeh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_REMOTE_WAKEUP) &&
	    is_wake_up_event(aeh)) {
		usb_wakeup();
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, config_event);
#endif
#if CONFIG_DESKTOP_USB_REMOTE_WAKEUP
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
