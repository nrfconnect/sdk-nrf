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
	for (size_t i = 0; i < hids_c->rep_cnt; i++) {
		struct bt_gatt_hids_c_rep_info *rep = hids_c->rep_info[i];

		if (rep->ref.type == BT_GATT_HIDS_C_REPORT_TYPE_INPUT) {
			int err = bt_gatt_hids_c_rep_subscribe(&hidc,
							       hidc.rep_info[i],
							       hidc_read);

			if (err) {
				LOG_ERR("Cannot subscribe to report (err:%d)",
					err);
			} else {
				LOG_INF("Subscriber to rep id:%d", rep->ref.id);
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
