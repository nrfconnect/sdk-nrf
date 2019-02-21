/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <bluetooth/services/hids_c.h>
#include <misc/byteorder.h>

#define MODULE hid_forward
#include "module_state_event.h"

#include "hid_report_desc.h"

#include "hid_event.h"
#include "ble_event.h"
#include "usb_event.h"
#include "config_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);


static struct bt_gatt_hids_c hidc;
static const void *usb_id;
static atomic_t usb_ready;
static struct hid_mouse_event *next_event;
static bool usb_busy;


static u8_t hidc_read(struct bt_gatt_hids_c *hids_c,
		      struct bt_gatt_hids_c_rep_info *rep,
		      u8_t err,
		      const u8_t *data)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (err || !atomic_get(&usb_ready)) {
		return BT_GATT_ITER_CONTINUE;
	}

	u8_t button_bm = data[0];
	s16_t wheel = (s8_t)data[1];

	u8_t x_buff[2] = {data[2], data[3] & 0x0F};
	u8_t y_buff[2] = {(data[3] >> 4) | ((data[4] << 4) & 0xF0), data[4] >> 4};

	u16_t x = sys_get_le16(x_buff);
	u16_t y = sys_get_le16(y_buff);

	if (x > REPORT_MOUSE_XY_MAX) {
		x |= 0xF000;
	}
	if (y > REPORT_MOUSE_XY_MAX) {
		y |= 0xF000;
	}

	struct hid_mouse_event *event;

	u32_t key = irq_lock();

	if (next_event) {
		__ASSERT_NO_MSG(usb_busy);

		LOG_WRN("Event override");
		event = next_event;
	} else {
		event = new_hid_mouse_event();
	}

	event->subscriber = usb_id;

	event->button_bm = button_bm;
	event->wheel     = wheel;

	event->dx = x;
	event->dy = y;

	if (!usb_busy) {
		EVENT_SUBMIT(event);
		usb_busy = true;
	} else {
		next_event = event;
	}

	irq_unlock(key);

	return BT_GATT_ITER_CONTINUE;
}

static void hidc_ready(struct bt_gatt_hids_c *hids_c)
{
	struct bt_gatt_hids_c_rep_info *rep = NULL;

	while (NULL != (rep = bt_gatt_hids_c_rep_next(hids_c, rep))) {
		if (bt_gatt_hids_c_rep_type(rep) ==
		    BT_GATT_HIDS_C_REPORT_TYPE_INPUT) {
			int err = bt_gatt_hids_c_rep_subscribe(hids_c,
							       rep,
							       hidc_read);

			if (err) {
				LOG_ERR("Cannot subscribe to report (err:%d)",
					err);
			} else {
				LOG_INF("Subscriber to rep id:%d",
					bt_gatt_hids_c_rep_id(rep));
			}
			break;
		}
	}
}

static void hidc_pm_update(struct bt_gatt_hids_c *hids_c)
{
	LOG_INF("Protocol mode updated");
}

static void hidc_prep_error(struct bt_gatt_hids_c *hids_c, int err)
{
	if (err) {
		LOG_ERR("err:%d", err);
	}
}

static void init(void)
{
	static const struct bt_gatt_hids_c_init_params params = {
		.ready_cb = hidc_ready,
		.prep_error_cb = hidc_prep_error,
		.pm_update_cb = hidc_pm_update,
	};

	bt_gatt_hids_c_init(&hidc, &params);
}

static int assign_handles(struct bt_gatt_dm *dm)
{
	int err = bt_gatt_hids_c_handles_assign(dm, &hidc);

	if (err) {
		LOG_ERR("Cannot assign handles (err:%d)", err);
	}

	return err;
}

void hidc_write_cb(struct bt_gatt_hids_c *hidc,
		      struct bt_gatt_hids_c_rep_info *rep,
		      u8_t err)
{
	if (err) {
		LOG_WRN("Failed to write report via HIDS: %d", err);
	} else {
		struct config_sent_event *event = new_config_sent_event();

		EVENT_SUBMIT(event);
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		u32_t key = irq_lock();
		if (next_event) {
			EVENT_SUBMIT(next_event);
			next_event = NULL;
		} else {
			usb_busy = false;
		}
		irq_unlock(key);

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state),
				MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			init();
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_ble_discovery_complete_event(eh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(eh);

		assign_handles(event->dm);

		return false;
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->subscriber == usb_id) {
			atomic_set(&usb_ready, event->enabled);
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		if ((event->state == PEER_STATE_DISCONNECTED) &&
		    bt_gatt_hids_c_assign_check(&hidc)) {
			LOG_INF("HID device disconnected");
			bt_gatt_hids_c_release(&hidc);
		}

		return false;
	}

	if (is_usb_state_event(eh)) {
		const struct usb_state_event *event =
			cast_usb_state_event(eh);

		switch (event->state) {
		case USB_STATE_POWERED:
			usb_id = event->id;
			break;
		case USB_STATE_DISCONNECTED:
			usb_id = NULL;
			atomic_set(&usb_ready, false);
			break;
		default:
			/* Ignore */
			break;
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(eh)) {
			struct config_event *event = cast_config_event(eh);

			/* Do not forward events addressed to current device */
			if (event->recipient == CONFIG_USB_DEVICE_PID) {
				return false;
			}

			if (!bt_gatt_hids_c_ready_check(&hidc)) {
				LOG_WRN("Cannot forward, peer disconnected");
				return false;
			}

			struct bt_gatt_hids_c_rep_info *config_rep =
				bt_gatt_hids_c_rep_find(&hidc,
					BT_GATT_HIDS_C_REPORT_TYPE_FEATURE,
					REPORT_ID_USER_CONFIG);
			if (config_rep == NULL) {
				LOG_ERR("Feature report not found");
				return false;
			}

			u8_t data[REPORT_SIZE_USER_CONFIG];

			memcpy(&(data[0]), &event->recipient, sizeof(event->recipient));
			data[2] = event->id;
			memcpy(&(data[3]), event->data, sizeof(event->data));

			int err = bt_gatt_hids_c_rep_write(&hidc, config_rep,
					hidc_write_cb, data, sizeof(data));
			if (err) {
				LOG_ERR("Writing report failed, err:%d", err);
				return false;
			}

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_discovery_complete_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, usb_state_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_event);
#endif
