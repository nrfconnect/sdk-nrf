/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbd_msg.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/class/usbd_hid.h>

#define MODULE usb_state
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
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

#define REPORT_ID_SIZE		1
#define REPORT_TYPE_INPUT	0x01
#define REPORT_TYPE_OUTPUT	0x02
#define REPORT_TYPE_FEATURE	0x03

#define USB_SUBSCRIBER_PRIORITY      CONFIG_DESKTOP_USB_SUBSCRIBER_REPORT_PRIORITY
#define USB_SUBSCRIBER_PIPELINE_SIZE (IS_ENABLED(CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF) ? 2 : 1)
#define USB_SUBSCRIBER_REPORT_MAX    USB_SUBSCRIBER_PIPELINE_SIZE

/* The definitions are available and used only for USB legacy stack.
 * Define them explicitly if needed to allow compiling code without USB legacy stack.
 */
#ifndef CONFIG_DESKTOP_USB_STACK_LEGACY
	#define CONFIG_USB_HID_DEVICE_NAME ""
	#define CONFIG_DESKTOP_USB_HID_PROTOCOL_CODE -1
#endif

enum {
	USB_HID_BUF_ALLOCATED		= BIT(0),
	USB_HID_BUF_SENDING		= BIT(1),
	USB_HID_BUF_BOOT_REPORT_FORMAT	= BIT(2),
};

BUILD_ASSERT(REPORT_BUFFER_SIZE_INPUT_REPORT <= UINT8_MAX);

/* Ensuring memory alignment for sent buffer is required by USB next stack. */
struct usb_hid_buf {
	/* An extra byte allows to store HID report ID next to HID report data. */
	uint8_t data[REPORT_ID_SIZE + REPORT_BUFFER_SIZE_INPUT_REPORT];
	uint8_t size;
	uint8_t status_bm;
} __aligned(sizeof(void *));

struct usb_hid_device {
	const struct device *dev;
	uint32_t report_bm;
	struct usb_hid_buf report_bufs[USB_SUBSCRIBER_REPORT_MAX];
	atomic_ptr_t report_sent_on_sof;
	uint32_t idle_duration[REPORT_ID_COUNT];
	uint8_t hid_protocol;
	bool report_enabled[REPORT_ID_COUNT];
	bool enabled;
};

static enum usb_state state;

BUILD_ASSERT(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT) ||
	     IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY),
	     "Unsupported USB stack");

#if CONFIG_DESKTOP_USB_STACK_NEXT
#define USB_NEXT_USB_HID_DEVICE_INIT(node_id)	\
{						\
	.dev = DEVICE_DT_GET(node_id),		\
	.hid_protocol = HID_PROTOCOL_REPORT,	\
},

static struct usb_hid_device usb_hid_device[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_hid_device, USB_NEXT_USB_HID_DEVICE_INIT)
};
#else
static struct usb_hid_device usb_hid_device[CONFIG_USB_HID_DEVICE_COUNT];
#endif

#if CONFIG_DESKTOP_USB_STACK_NEXT && DT_NODE_EXISTS(DT_NODELABEL(usbhs))
#define USB_DT_NODELABEL	DT_NODELABEL(usbhs)
#define NUM_IN_ENDPOINTS	DT_PROP(USB_DT_NODELABEL, num_in_eps)
#elif DT_NODE_EXISTS(DT_NODELABEL(usbd))
#define USB_DT_NODELABEL	DT_NODELABEL(usbd)
#define NUM_IN_ENDPOINTS	DT_PROP(USB_DT_NODELABEL, num_in_endpoints)
#else
#error USB device tree node is not defined.
#endif

BUILD_ASSERT(NUM_IN_ENDPOINTS > ARRAY_SIZE(usb_hid_device),
	     "Too few USB IN Endpoints enabled. Modify dts.overlay file.");

BUILD_ASSERT(!IS_ENABLED(CONFIG_DESKTOP_HID_STATE_ENABLE) ||
	     IS_ENABLED(CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION) ||
	     (ARRAY_SIZE(usb_hid_device) <= 1));

static struct usbd_context *usbd_ctx;
static bool usb_enabled;

static struct config_channel_transport cfg_chan_transport;

static struct k_thread usb_init_thread;
static K_THREAD_STACK_DEFINE(usb_init_thread_stack, CONFIG_DESKTOP_USB_INIT_THREAD_STACK_SIZE);

#define USB_REQ_TIMEOUT_MS	100

/* USB events */
enum {
	USB_REQ_ENABLE		= BIT(0),
	USB_REQ_DISABLE		= BIT(1),
	USB_RSP_SUCCESS		= BIT(2),
	USB_RSP_FAIL		= BIT(3),
};

static K_EVENT_DEFINE(usb_event);

static void report_sent(struct usb_hid_device *usb_hid, struct usb_hid_buf *buf, bool error);

static uint8_t usb_hid_buf_get_report_id(struct usb_hid_buf *buf)
{
	__ASSERT_NO_MSG(buf->status_bm & USB_HID_BUF_ALLOCATED);
	uint8_t report_id = REPORT_ID_COUNT;

	if (!(buf->status_bm & USB_HID_BUF_BOOT_REPORT_FORMAT)) {
		report_id = buf->data[0];
	} else {
		if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE)) {
			report_id = REPORT_ID_BOOT_MOUSE;
		} else if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD)) {
			report_id = REPORT_ID_BOOT_KEYBOARD;
		}
	}

	/* Should not happen. */
	__ASSERT_NO_MSG(report_id != REPORT_ID_COUNT);

	return report_id;
}

static struct usb_hid_buf *usb_hid_buf_alloc(struct usb_hid_device *usb_hid, const uint8_t *data,
					     size_t size)
{
	for (size_t i = 0; i < ARRAY_SIZE(usb_hid->report_bufs); i++) {
		struct usb_hid_buf *r = &usb_hid->report_bufs[i];

		if (r->status_bm == 0) {
			__ASSERT_NO_MSG(sizeof(r->data) >= size);

			memcpy(r->data, data, size);
			r->size = size;
			r->status_bm |= USB_HID_BUF_ALLOCATED;
			return r;
		}
	}

	return NULL;
}

static void usb_hid_buf_free(struct usb_hid_buf *report_buf)
{
	report_buf->status_bm = 0;
}

static struct usb_hid_buf *usb_hid_buf_find(struct usb_hid_device *usb_hid, uint8_t status_bm)
{
	for (size_t i = 0; i < ARRAY_SIZE(usb_hid->report_bufs); i++) {
		struct usb_hid_buf *r = &usb_hid->report_bufs[i];

		if ((status_bm & r->status_bm) == status_bm) {
			return r;
		}
	}

	return NULL;
}

static bool is_hid_boot_report(uint8_t report_id)
{
	return ((report_id == REPORT_ID_BOOT_MOUSE) || (report_id == REPORT_ID_BOOT_KEYBOARD));
}

static bool is_hid_boot_report_supported(uint8_t report_id)
{
	__ASSERT_NO_MSG(is_hid_boot_report(report_id));

	return ((IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) &&
		 (report_id == REPORT_ID_BOOT_MOUSE)) ||
		(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
		 (report_id == REPORT_ID_BOOT_KEYBOARD)));
}

static bool can_send_hid_report(struct usb_hid_device *usb_hid, uint8_t report_id)
{
	if (!usb_hid->enabled) {
		LOG_WRN("Cannot send report: USB not connected");
		return false;
	}

	if (usb_hid->hid_protocol == HID_PROTOCOL_BOOT) {
		if (!is_hid_boot_report(report_id)) {
			LOG_WRN("Cannot send report: incompatible boot/report mode");
			return false;
		}

		if (!is_hid_boot_report_supported(report_id)) {
			LOG_WRN("Cannot send report: unsupported boot report");
			return false;
		}
	} else {
		__ASSERT_NO_MSG(usb_hid->hid_protocol == HID_PROTOCOL_REPORT);

		if (is_hid_boot_report(report_id)) {
			LOG_WRN("Cannot send report: incompatible boot/report mode");
			return false;
		}
	}

	return true;
}

static void usb_hid_buf_send(struct usb_hid_device *usb_hid, struct usb_hid_buf *buf)
{
	uint8_t report_id = usb_hid_buf_get_report_id(buf);

	buf->status_bm |= USB_HID_BUF_SENDING;

	if (!can_send_hid_report(usb_hid, report_id)) {
		report_sent(usb_hid, buf, true);
		return;
	}

	uint8_t *data = buf->data;
	size_t size = buf->size;

	if (is_hid_boot_report(report_id)) {
		/* Omit report ID for HID boot reports. Keep proper memory alignment. */
		for (size_t i = 1; i < size; i++) {
			data[i - 1] = data[i];
		}
		size--;
		buf->status_bm |= USB_HID_BUF_BOOT_REPORT_FORMAT;
	}

	int err;

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT)) {
		/* USB next stack expects buffer alignment. */
		__ASSERT_NO_MSG(IS_ALIGNED(data, sizeof(void *)));
		err = hid_device_submit_report(usb_hid->dev, size, data);
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY));
		err = hid_int_ep_write(usb_hid->dev, data, size, NULL);
	}

	if (err) {
		LOG_ERR("Failed to submit report to USB stack (%d)", err);
		report_sent(usb_hid, buf, true);
	}
}

static struct usb_hid_device *dev_to_usb_hid(const struct device *dev)
{
	struct usb_hid_device *usb_hid = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		if (usb_hid_device[i].dev == dev) {
			usb_hid = &usb_hid_device[i];
			break;
		}
	}

	__ASSERT_NO_MSG(usb_hid);
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
			struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);

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

static void report_sent_sof(struct usb_hid_device *usb_hid)
{
	/* Ensure that the function will not be preempted. Other functions that require
	 * synchronization with this function are assumed not to be called from an ISR context.
	 */
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hid_report_sent_event *event = atomic_ptr_set(&usb_hid->report_sent_on_sof, NULL);

	if (event) {
		APP_EVENT_SUBMIT(event);
	}
}

static void report_sent(struct usb_hid_device *usb_hid, struct usb_hid_buf *buf, bool error)
{
	/* Ensure that the function is executed in a cooperative thread context and no extra
	 * synchronization is required.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	__ASSERT_NO_MSG(buf);
	__ASSERT_NO_MSG(buf->status_bm & USB_HID_BUF_ALLOCATED);
	__ASSERT_NO_MSG(buf->status_bm & USB_HID_BUF_SENDING);

	uint8_t report_id = usb_hid_buf_get_report_id(buf);
	struct hid_report_sent_event *event = new_hid_report_sent_event();

	event->report_id = report_id;
	event->subscriber = usb_hid;
	event->error = error;

	if (!IS_ENABLED(CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF)) {
		APP_EVENT_SUBMIT(event);
	} else {
		if (error) {
			/* Synchronization to USB SOF is not used on send error. Instantly send
			 * enqueued report waiting for USB SOF to ensure proper HID report sent
			 * event order.
			 */
			report_sent_sof(usb_hid);
			APP_EVENT_SUBMIT(event);
		} else {
			if (!atomic_ptr_cas(&usb_hid->report_sent_on_sof, NULL, event)) {
				/* Instantly submit previous event to ensure proper HID report sent
				 * event order.
				 */
				LOG_WRN("Missing USB SOF between HID report sent callbacks");
				report_sent_sof(usb_hid);
				(void)atomic_ptr_set(&usb_hid->report_sent_on_sof, event);
			}
		}
	}

	usb_hid_buf_free(buf);

	/* Module uses very simple HID report buffering implementation that supports up to 2
	 * buffers. Configuring more buffers could break order of sent HID reports.
	 */
	BUILD_ASSERT(ARRAY_SIZE(usb_hid->report_bufs) <= 2);
	/* Make sure no report is currently being sent. */
	__ASSERT_NO_MSG(!usb_hid_buf_find(usb_hid, USB_HID_BUF_SENDING));

	/* Send subsequent HID report if queued. */
	struct usb_hid_buf *next_buf = usb_hid_buf_find(usb_hid, USB_HID_BUF_ALLOCATED);

	if (next_buf) {
		usb_hid_buf_send(usb_hid, next_buf);
	}
}

static struct usb_hid_device *subscriber_to_usb_hid(const void *subscriber)
{
	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		if (subscriber == &usb_hid_device[i]) {
			return &usb_hid_device[i];
		}
	}

	return NULL;
}

static bool handle_hid_report_event(struct hid_report_event *event)
{
	/* Ensure that the function is executed in a cooperative thread context and no extra
	 * synchronization is required.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct usb_hid_device *usb_hid = subscriber_to_usb_hid(event->subscriber);

	if (!usb_hid) {
		/* Not us. */
		return false;
	}

	const uint8_t *data = event->dyndata.data;
	size_t size = event->dyndata.size;
	struct usb_hid_buf *sending_buf = usb_hid_buf_find(usb_hid, USB_HID_BUF_SENDING);
	struct usb_hid_buf *new_buf = usb_hid_buf_alloc(usb_hid, data, size);

	__ASSERT_NO_MSG(new_buf);

	/* Send HID report instantly only if there is no report that is currently being sent.
	 * Otherwise wait until the previous report is sent.
	 */
	if (!sending_buf) {
		usb_hid_buf_send(usb_hid, new_buf);
	} else {
		__ASSERT_NO_MSG(sending_buf->status_bm & USB_HID_BUF_ALLOCATED);
	}

	return false;
}

static void broadcast_usb_state(enum usb_state broadcast_state)
{
	struct usb_state_event *event = new_usb_state_event();

	event->state = broadcast_state;

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

static void usb_legacy_reset_pending_report(struct usb_hid_device *usb_hid)
{
	struct usb_hid_buf *buf = usb_hid_buf_find(usb_hid, USB_HID_BUF_SENDING);

	if (buf) {
		LOG_WRN("USB clear report notification waiting flag");
		report_sent(usb_hid, buf, true);
	}
}

static void update_usb_hid(struct usb_hid_device *usb_hid, bool enabled)
{
	/* Ensure that the function is executed in a cooperative thread context and no extra
	 * synchronization is required.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	if (usb_hid->enabled == enabled) {
		/* Already updated. */
		return;
	}

	usb_hid->enabled = enabled;

	/* Inform about the sent HID report if queued. */
	if (IS_ENABLED(CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF) && !enabled) {
		report_sent_sof(usb_hid);
	}

	/* USB legacy stack does not notify app about sent report when no longer configured.
	 * Pending report reset needs to be done after usb_hid->enabled field update to prevent
	 * module from submitting HID report to stack and before broadcast of new USB HID subscriber
	 * state to ensure proper Event Manager event order.
	 */
	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY) && !enabled) {
		usb_legacy_reset_pending_report(usb_hid);
	}

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
	/* Ensure that the function is executed in a cooperative thread context and no extra
	 * synchronization is required.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);

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
		if (usb_enabled) {
			__ASSERT_NO_MSG(usbd_ctx);
			err = usbd_wakeup_request(usbd_ctx);
		} else {
			/* Skip wakeup request. */
			return;
		}
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

static void report_sent_cb_legacy(const struct device *dev)
{
	struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);
	struct usb_hid_buf *buf = usb_hid_buf_find(usb_hid, USB_HID_BUF_SENDING);

	report_sent(usb_hid, buf, false);
}

static int usb_init_legacy_hid_device_init(struct usb_hid_device *usb_hid_dev,
					   const struct device *dev,
					   uint32_t report_bm)
{
	static const struct hid_ops hid_ops = {
		.get_report = get_report_legacy,
		.set_report = set_report_legacy,
		.int_in_ready = report_sent_cb_legacy,
		.protocol_change = protocol_change,
	};

	usb_hid_dev->dev = dev;
	usb_hid_dev->hid_protocol = HID_PROTOCOL_REPORT;
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

	case USB_DC_SOF:
		if (IS_ENABLED(CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF)) {
			for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
				struct usb_hid_device *usb_hid = &usb_hid_device[i];

				report_sent_sof(usb_hid);
			}
		}
		/* Fall-through. */

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
			update_usb_hid(&usb_hid_device[i], false);
		}
	}

	/* Update state. */
	state = new_state;
	broadcast_usb_state(state);

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

static bool is_usb_active_next(void)
{
	bool usb_active = false;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		if (usb_hid_device[i].enabled) {
			usb_active = true;
			break;
		}
	}

	return usb_active;
}

static void iface_ready_next(const struct device *dev, const bool ready)
{
	struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);
	bool was_usb_active = is_usb_active_next();

	update_usb_hid(usb_hid, ready);

	bool is_usb_active = is_usb_active_next();

	if (state == USB_STATE_SUSPENDED) {
		/* USB state update is delayed if USB is suspended. */
		return;
	}

	if (!was_usb_active && is_usb_active) {
		broadcast_usb_state(USB_STATE_ACTIVE);
	} else if (was_usb_active && !is_usb_active) {
		broadcast_usb_state(state);
	}
}

static int get_report_next(const struct device *dev, const uint8_t type, const uint8_t id,
			   const uint16_t len, uint8_t *const buf)
{
	/* Omit the first byte - HID report ID. */
	buf[0] = id;

	int err = get_report(dev, type, id, buf + 1, len - 1);

	return err ? err : len;
}

static int set_report_next(const struct device *dev, const uint8_t type, const uint8_t id,
			   const uint16_t len, const uint8_t *const buf)
{
	/* Omit the first byte - HID report ID. */
	return set_report(dev, type, id, buf + 1, len - 1);
}

static void set_idle_next(const struct device *dev, const uint8_t id, const uint32_t duration)
{
	struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);

	usb_hid->idle_duration[id] = duration;
}

static uint32_t get_idle_next(const struct device *dev, const uint8_t id)
{
	struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);

	return usb_hid->idle_duration[id];
}

static void report_sent_cb_next(const struct device *dev)
{
	struct usb_hid_device *usb_hid = dev_to_usb_hid(dev);
	struct usb_hid_buf *buf = usb_hid_buf_find(usb_hid, USB_HID_BUF_SENDING);
	/* USB next stack does not explicitly indicate failed transfers. */
	bool error = !usb_hid->enabled;

	report_sent(usb_hid, buf, error);
}

static void sof_next(const struct device *dev)
{
	report_sent_sof(dev_to_usb_hid(dev));
}

static int usb_init_next_hid_device_init(struct usb_hid_device *usb_hid_dev, uint32_t report_bm)
{
	static const struct hid_device_ops hid_ops = {
		.iface_ready = iface_ready_next,
		.get_report = get_report_next,
		.set_report = set_report_next,
		.set_idle = set_idle_next,
		.get_idle = get_idle_next,
		.set_protocol = protocol_change,
		.input_report_done = report_sent_cb_next,
		.sof = sof_next,
	};

	usb_hid_dev->report_bm = report_bm;

	int err = hid_device_register(usb_hid_dev->dev, hid_report_desc, hid_report_desc_size,
				      &hid_ops);

	if (err) {
		LOG_ERR("hid_device_register failed for %p (err: %d)",
			(void *)usb_hid_dev->dev, err);
	}

	return err;
}

static int usb_init_next_hids_init(void)
{
	int err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(usb_hid_device); i++) {
		err = usb_init_next_hid_device_init(&usb_hid_device[i], get_report_bm(i));
		if (err) {
			LOG_ERR("usb_init_next_hid_device_init failed (err: %d)", err);
			break;
		}
	}

	return err;
}

static int usb_thread_req_send(bool enable)
{
	uint32_t events;
	int err;

	__ASSERT_NO_MSG(!k_event_test(&usb_event, USB_REQ_ENABLE | USB_REQ_DISABLE));

	LOG_DBG("%sabling USB", enable ? "En" : "Dis");
	(void) k_event_set(&usb_event, enable ? USB_REQ_ENABLE : USB_REQ_DISABLE);
	events = k_event_wait(&usb_event,
			      USB_RSP_SUCCESS | USB_RSP_FAIL,
			      false,
			      K_MSEC(USB_REQ_TIMEOUT_MS));
	(void) k_event_clear(&usb_event, events);

	if (events & USB_RSP_SUCCESS) {
		LOG_DBG("USB %sabled", enable ? "en" : "dis");
		usb_enabled = enable;
		err = 0;
	} else if (events & USB_RSP_FAIL) {
		LOG_ERR("Failed to %sable USB", enable ? "en" : "dis");
		module_set_state(MODULE_STATE_ERROR);
		err = -EIO;
	} else {
		LOG_ERR("Fatal error - USB %sable timeout. Rebooting...", enable ? "en" : "dis");
		LOG_PANIC();
		sys_reboot(SYS_REBOOT_WARM);
		err = -EIO;
	}

	return err;
}

static int handle_usbd_state_on_status_change(enum usbd_msg_type type)
{
	int err = 0;

	switch (type) {
	case USBD_MSG_VBUS_READY:
		if (!usb_enabled) {
			err = usb_thread_req_send(true);
		}
		break;

	case USBD_MSG_VBUS_REMOVED:
		if (usb_enabled) {
			err = usb_thread_req_send(false);
		}
		break;

	default:
		/* Ignore */
		break;
	}

	return err;
}

static void usb_init_next_status_cb(struct usbd_context *const usbd,
				    const struct usbd_msg *const msg)
{
	static enum usb_state before_suspend;
	enum usb_state new_state = state;

	LOG_DBG("USBD msg: %s", usbd_msg_type_string(msg->type));

	__ASSERT_NO_MSG(usbd == usbd_ctx);

	switch (msg->type) {
	case USBD_MSG_VBUS_READY:
		if (state != USB_STATE_DISCONNECTED) {
			LOG_WRN("USBD_MSG_VBUS_READY while USB is not disconnected");
		}
		new_state = USB_STATE_POWERED;
		break;

	case USBD_MSG_VBUS_REMOVED:
		new_state = USB_STATE_DISCONNECTED;
		break;

	case USBD_MSG_RESUME:
		if (state == USB_STATE_SUSPENDED) {
			new_state = before_suspend;
		}
		break;

	case USBD_MSG_SUSPEND:
		before_suspend = state;
		new_state = USB_STATE_SUSPENDED;
		break;

	case USBD_MSG_RESET:
		if (state != USB_STATE_DISCONNECTED) {
			new_state = USB_STATE_POWERED;
		} else {
			LOG_WRN("Reset while disconnected");
		}
		break;

	case USBD_MSG_UDC_ERROR:
		LOG_ERR("Non-correctable UDC error message");
		module_set_state(MODULE_STATE_ERROR);
		break;

	case USBD_MSG_STACK_ERROR:
		LOG_ERR("Unrecoverable USB device stack error");
		module_set_state(MODULE_STATE_ERROR);
		break;

	case USBD_MSG_CDC_ACM_LINE_CODING:
	case USBD_MSG_CDC_ACM_CONTROL_LINE_STATE:
		/* Ignore */
		break;

	default:
		LOG_ERR("Unexpected USB device message type: %d", msg->type);
		break;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT_DISABLE_ON_VBUS_REMOVAL)) {
		int err;

		err = handle_usbd_state_on_status_change(msg->type);
		if (err) {
			LOG_ERR("handle_usbd_state_on_status_change failed (err: %d)", err);
			return;
		}
	}

	if (new_state != state) {
		if ((state == USB_STATE_SUSPENDED) && is_usb_active_next()) {
			broadcast_usb_state(USB_STATE_ACTIVE);
		} else {
			broadcast_usb_state(new_state);
		}
		state = new_state;
	}
}

static int usb_init_next_register_fs_classes(struct usbd_context *usbd)
{
	int err = 0;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		err = usbd_register_class(usbd, c_nd->c_data->name, USBD_SPEED_FS, 1);
		if (err) {
			LOG_ERR("usbd_register_class failed for %s (err: %d)",
				c_nd->c_data->name, err);
			return err;
		}
	}

	return err;
}

static int usb_init_next_register_hs_classes(struct usbd_context *usbd)
{
	int err = 0;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
		err = usbd_register_class(usbd, c_nd->c_data->name, USBD_SPEED_HS, 1);
		if (err) {
			LOG_ERR("usbd_register_class failed for %s (err: %d)",
				c_nd->c_data->name, err);
			return err;
		}
	}

	return err;
}

static int usb_init_next_add_configuration(struct usbd_context *usbd,
					   const enum usbd_speed speed,
					   struct usbd_config_node *config)
{
	int err;

	err = usbd_add_configuration(usbd, speed, config);
	if (err) {
		LOG_ERR("usbd_add_configuration failed (err: %d)", err);
		return err;
	}

	if (speed == USBD_SPEED_FS) {
		err = usb_init_next_register_fs_classes(usbd);
	} else if (speed == USBD_SPEED_HS) {
		err = usb_init_next_register_hs_classes(usbd);
	}

	if (err) {
		return err;
	}

	/* Always use class code information from Interface Descriptors */
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS)) {
		BUILD_ASSERT(!IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS));
		BUILD_ASSERT(!IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS));
		/*
		 * Class with multiple interfaces have an Interface
		 * Association Descriptor available, use an appropriate triple
		 * to indicate it.
		 */
		usbd_device_set_code_triple(usbd, speed, USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		usbd_device_set_code_triple(usbd, speed, 0, 0, 0);
	}

	return 0;
}

static struct usbd_context *usb_init_next_usbd_init(void)
{
	int err;

	USBD_DEVICE_DEFINE(usbd, DEVICE_DT_GET(USB_DT_NODELABEL),
			   CONFIG_DESKTOP_DEVICE_VID, CONFIG_DESKTOP_DEVICE_PID);

	USBD_DESC_LANG_DEFINE(lang);
	USBD_DESC_MANUFACTURER_DEFINE(manufacturer, CONFIG_DESKTOP_DEVICE_MANUFACTURER);
	USBD_DESC_PRODUCT_DEFINE(product, CONFIG_DESKTOP_DEVICE_PRODUCT);
	USBD_DESC_SERIAL_NUMBER_DEFINE(serial_number);

	/* Maximum power consumption of a device: 250 * 2 mA = 500 mA. */
	static const uint8_t max_power = 250;
	static const uint8_t attributes = IS_ENABLED(CONFIG_DESKTOP_USB_REMOTE_WAKEUP) ?
					  (USB_SCD_REMOTE_WAKEUP) : (0);

	USBD_CONFIGURATION_DEFINE(fs_config, attributes, max_power, NULL);
	USBD_CONFIGURATION_DEFINE(hs_config, attributes, max_power, NULL);

	if (!usbd_can_detect_vbus(&usbd)) {
		LOG_ERR("USBD controller cannot detect VBUS state change");
		return NULL;
	}

	err = usbd_msg_register_cb(&usbd, usb_init_next_status_cb);
	if (err) {
		LOG_ERR("usbd_msg_register_cb failed (err: %d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&usbd, &lang);
	if (err) {
		LOG_ERR("usbd_add_descriptor(lang) failed (err: %d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&usbd, &manufacturer);
	if (err) {
		LOG_ERR("usbd_add_descriptor(manufacturer) failed (err: %d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&usbd, &product);
	if (err) {
		LOG_ERR("usbd_add_descriptor(product) failed (err: %d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&usbd, &serial_number);
	if (err) {
		LOG_ERR("usbd_add_descriptor(serial_number) failed (err: %d)", err);
		return NULL;
	}

	if (usbd_caps_speed(&usbd) == USBD_SPEED_HS) {
		err = usb_init_next_add_configuration(&usbd, USBD_SPEED_HS, &hs_config);
		if (err) {
			LOG_ERR("usb_init_next_add_configuration failed (err: %d)", err);
			return NULL;
		}
	}

	err = usb_init_next_add_configuration(&usbd, USBD_SPEED_FS, &fs_config);
	if (err) {
		LOG_ERR("usb_init_next_add_configuration failed (err: %d)", err);
		return NULL;
	}

	err = usbd_init(&usbd);
	if (err) {
		LOG_ERR("usbd_init (err: %d)", err);
		return NULL;
	}

	return &usbd;
}

static int usb_init_next(void)
{
	int err = usb_init_next_hids_init();

	if (err) {
		LOG_ERR("usb_init_next_hids_init failed (err: %d)", err);
		return err;
	}

	struct usbd_context *usbd = usb_init_next_usbd_init();

	if (!usbd) {
		LOG_ERR("usb_init_next_usbd_init failed");
		return -ENXIO;
	}

	usbd_ctx = usbd;

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT_DISABLE_ON_VBUS_REMOVAL)) {
		/* Do not enable USBD now. It will be enabled when USB cable will be connected. */
		return 0;
	}

	err = usbd_enable(usbd);
	if (err) {
		LOG_ERR("usbd_enable failed (err: %d)", err);
		return err;
	}

	usb_enabled = true;

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
		err = usb_init_next();
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_USB_STACK_LEGACY));
		err = usb_init_legacy();
	}

	if (err) {
		LOG_ERR("Cannot enable USB (err: %d)", err);
	} else {
		LOG_DBG("USB enabled");
	}

	return err;
}

static void wait_for_usb_requests(void)
{
	while (true) {
		uint32_t events;
		int err;

		events = k_event_wait(&usb_event,
				      USB_REQ_ENABLE | USB_REQ_DISABLE,
				      false,
				      K_FOREVER);
		(void) k_event_clear(&usb_event, events);
		__ASSERT_NO_MSG(!(events & USB_REQ_ENABLE) || !(events & USB_REQ_DISABLE));

		__ASSERT_NO_MSG(usbd_ctx);
		if (events & USB_REQ_ENABLE) {
			err = usbd_enable(usbd_ctx);
			if (err) {
				LOG_ERR("usbd_enable failed (err: %d)", err);
			}
		} else if (events & USB_REQ_DISABLE) {
			err = usbd_disable(usbd_ctx);
			if (err) {
				LOG_ERR("usbd_disable failed (err: %d)", err);
			}
		} else {
			__ASSERT_NO_MSG(false);
			err = -EINVAL;
		}

		__ASSERT_NO_MSG(!k_event_test(&usb_event, USB_RSP_FAIL | USB_RSP_SUCCESS));
		(void) k_event_post(&usb_event, err ? USB_RSP_FAIL : USB_RSP_SUCCESS);
	}
}

static void usb_init_thread_fn(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	if (usb_init()) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_STACK_NEXT_DISABLE_ON_VBUS_REMOVAL)) {
		wait_for_usb_requests();
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_event(aeh)) {
		return handle_hid_report_event(cast_hid_report_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (IS_ENABLED(CONFIG_DESKTOP_USB_INIT_THREAD)) {
				(void) k_thread_create(
					&usb_init_thread,
					usb_init_thread_stack,
					K_THREAD_STACK_SIZEOF(usb_init_thread_stack),
					usb_init_thread_fn,
					NULL, NULL, NULL,
					CONFIG_DESKTOP_USB_INIT_THREAD_PRIORITY, 0,
					K_MSEC(CONFIG_DESKTOP_USB_INIT_THREAD_DELAY_MS));
			} else {
				usb_init_thread_fn(NULL, NULL, NULL);
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
