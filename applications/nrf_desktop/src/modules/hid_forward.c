/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/slist.h>

#include <bluetooth/services/hids_c.h>
#include <sys/byteorder.h>

#define MODULE hid_forward
#include "module_state_event.h"

#include "hid_report_desc.h"
#include "config_channel.h"

#include "hid_event.h"
#include "ble_event.h"
#include "usb_event.h"
#include "config_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_FORWARD_LOG_LEVEL);

#define MAX_ENQUEUED_ITEMS CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS

struct enqueued_report {
	sys_snode_t node;
	struct hid_report_event *report;
};

struct hids_subscriber {
	struct bt_gatt_hids_c hidc;
	uint16_t pid;
	uint8_t hwid[HWID_LEN];
};

static struct hids_subscriber subscribers[CONFIG_BT_MAX_CONN];
static const void *usb_id;
static bool usb_ready;
static bool usb_busy;
static bool forward_pending;
static void *channel_id;

static sys_slist_t enqueued_report_list;
static size_t enqueued_report_count;

static struct k_spinlock lock;


static void enqueue_hid_report(struct hid_report_event *report)
{
	struct enqueued_report *item;

	if (enqueued_report_count < MAX_ENQUEUED_ITEMS) {
		item = k_malloc(sizeof(*item));
		enqueued_report_count++;
	} else {
		LOG_WRN("Enqueue dropped the oldest report");
		item = CONTAINER_OF(sys_slist_get(&enqueued_report_list),
				    __typeof__(*item),
				    node);
		k_free(item->report);
	}

	if (!item) {
		LOG_ERR("OOM error");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		item->report = report;
		sys_slist_append(&enqueued_report_list, &item->node);
	}
}

static void forward_hid_report(uint8_t report_id, const uint8_t *data, size_t size)
{
	struct hid_report_event *report = new_hid_report_event(size + sizeof(report_id));

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!usb_ready) {
		k_spin_unlock(&lock, key);
		k_free(report);
		return;
	}

	report->subscriber = usb_id;

	/* Forward report as is adding report id on the front. */
	report->dyndata.data[0] = report_id;
	memcpy(&report->dyndata.data[1], data, size);

	if (!usb_busy) {
		EVENT_SUBMIT(report);
		usb_busy = true;
	} else {
		enqueue_hid_report(report);
	}

	k_spin_unlock(&lock, key);
}

static uint8_t hidc_read(struct bt_gatt_hids_c *hids_c,
		      struct bt_gatt_hids_c_rep_info *rep,
		      uint8_t err,
		      const uint8_t *data)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		return BT_GATT_ITER_CONTINUE;
	}

	uint8_t report_id = bt_gatt_hids_c_rep_id(rep);
	size_t size = bt_gatt_hids_c_rep_size(rep);

	__ASSERT_NO_MSG((report_id != REPORT_ID_RESERVED) &&
			(report_id < REPORT_ID_COUNT));

	forward_hid_report(report_id, data, size);

	return BT_GATT_ITER_CONTINUE;
}

static void hidc_ready(struct bt_gatt_hids_c *hids_c)
{
	struct bt_gatt_hids_c_rep_info *rep = NULL;

	while (NULL != (rep = bt_gatt_hids_c_rep_next(hids_c, rep))) {
		if (bt_gatt_hids_c_rep_type(rep) ==
		    BT_GATT_HIDS_REPORT_TYPE_INPUT) {
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

	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		bt_gatt_hids_c_init(&subscribers[i].hidc, &params);
	}
	sys_slist_init(&enqueued_report_list);
}

static int register_subscriber(struct bt_gatt_dm *dm, uint16_t pid,
			       const uint8_t *hwid, size_t hwid_len)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (!bt_gatt_hids_c_assign_check(&subscribers[i].hidc)) {
			break;
		}
	}
	__ASSERT_NO_MSG(i < ARRAY_SIZE(subscribers));

	subscribers[i].pid = pid;

	__ASSERT_NO_MSG(hwid_len == HWID_LEN);
	memcpy(subscribers[i].hwid, hwid, hwid_len);

	int err = bt_gatt_hids_c_handles_assign(dm, &subscribers[i].hidc);

	if (err) {
		LOG_ERR("Cannot assign handles (err:%d)", err);
	}

	return err;
}

static void notify_config_forwarded(enum config_status status)
{
	struct config_forwarded_event *event = new_config_forwarded_event();

	forward_pending = (status == CONFIG_STATUS_PENDING);

	event->status = status;
	EVENT_SUBMIT(event);
}

static void hidc_write_cb(struct bt_gatt_hids_c *hidc,
			  struct bt_gatt_hids_c_rep_info *rep,
			  uint8_t err)
{
	if (err) {
		LOG_WRN("Failed to write report: %d", err);
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
	} else {
		notify_config_forwarded(CONFIG_STATUS_SUCCESS);
	}
}

static uint8_t hidc_read_cfg(struct bt_gatt_hids_c *hidc,
			  struct bt_gatt_hids_c_rep_info *rep,
			  uint8_t err, const uint8_t *data)
{
	if (err) {
		LOG_WRN("Failed to write report: %d", err);
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
	} else {
		struct config_channel_frame frame;

		int pos = config_channel_report_parse(data, REPORT_SIZE_USER_CONFIG,
						      &frame, false);
		if (pos < 0) {
			LOG_WRN("Could not set report");
			return pos;
		}

		if (frame.status != CONFIG_STATUS_SUCCESS) {
			LOG_INF("GATT read done, but fetch was not ready yet");

			/* Do not notify requester, host will schedule next read. */
			forward_pending = false;
			return 0;
		}

		struct config_fetch_event *event =
			new_config_fetch_event(frame.event_data_len);

		event->id = frame.event_id;
		event->recipient = frame.recipient;
		event->channel_id = channel_id;

		memcpy(event->dyndata.data, &data[pos], frame.event_data_len);

		EVENT_SUBMIT(event);
	}

	return 0;
}

static struct hids_subscriber *find_subscriber_hidc(uint16_t pid)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].pid == pid) {
			return &subscribers[i];
		}
	}
	return NULL;
}

static void handle_config_forward(const struct config_forward_event *event)
{
	struct hids_subscriber *subscriber = find_subscriber_hidc(event->recipient);

	if (!subscriber) {
		LOG_INF("Recipent %02" PRIx16 "not found", event->recipient);
		notify_config_forwarded(CONFIG_STATUS_DISCONNECTED_ERROR);
		return;
	}

	struct bt_gatt_hids_c *recipient_hidc =	&subscriber->hidc;

	__ASSERT_NO_MSG(recipient_hidc != NULL);

	if (!bt_gatt_hids_c_ready_check(recipient_hidc)) {
		LOG_WRN("Cannot forward, peer disconnected");

		notify_config_forwarded(CONFIG_STATUS_DISCONNECTED_ERROR);
		return;
	}

	bool has_out_report = false;
	struct bt_gatt_hids_c_rep_info *config_rep;

	config_rep = bt_gatt_hids_c_rep_find(recipient_hidc,
					     BT_GATT_HIDS_REPORT_TYPE_OUTPUT,
					     REPORT_ID_USER_CONFIG_OUT);

	if (!config_rep) {
		config_rep = bt_gatt_hids_c_rep_find(recipient_hidc,
					     BT_GATT_HIDS_REPORT_TYPE_FEATURE,
					     REPORT_ID_USER_CONFIG);
	} else {
		has_out_report = true;
	}

	if (!config_rep) {
		LOG_ERR("Feature report not found");
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	if (event->dyndata.size > UCHAR_MAX) {
		LOG_WRN("Event data too big");
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	struct config_channel_frame frame;
	uint8_t report[REPORT_SIZE_USER_CONFIG];

	if (event->status == CONFIG_STATUS_FETCH) {
		LOG_INF("Forwarding fetch request");
		frame.status = CONFIG_STATUS_FETCH;
	} else {
		frame.status = CONFIG_STATUS_PENDING;
	}

	frame.recipient = event->recipient;
	frame.event_id = event->id;
	frame.event_data_len = event->dyndata.size;
	frame.event_data = (uint8_t *)event->dyndata.data;

	int pos = config_channel_report_fill(report, sizeof(report), &frame,
					     false);

	if (pos < 0) {
		LOG_WRN("Could not set report");
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	notify_config_forwarded(CONFIG_STATUS_PENDING);

	int err;

	if (has_out_report) {
		err = bt_gatt_hids_c_rep_write_wo_rsp(recipient_hidc,
						      config_rep,
						      report,
						      sizeof(report),
						      hidc_write_cb);
	} else {
		err = bt_gatt_hids_c_rep_write(recipient_hidc,
					       config_rep,
					       hidc_write_cb,
					       report,
					       sizeof(report));
	}

	if (err) {
		LOG_ERR("Writing report failed, err:%d", err);
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
	}

	return;
}

static void handle_config_forward_get(const struct config_forward_get_event *event)
{
	struct hids_subscriber *subscriber =
		find_subscriber_hidc(event->recipient);

	if (!subscriber) {
		LOG_INF("Recipent %02" PRIx16 "not found", event->recipient);
		notify_config_forwarded(CONFIG_STATUS_DISCONNECTED_ERROR);
		return;
	}

	struct bt_gatt_hids_c *recipient_hidc =	&subscriber->hidc;

	__ASSERT_NO_MSG(recipient_hidc != NULL);

	if (forward_pending) {
		LOG_DBG("GATT operation already pending");
		return;
	}

	if (!bt_gatt_hids_c_ready_check(recipient_hidc)) {
		LOG_WRN("Cannot forward, peer disconnected");

		notify_config_forwarded(CONFIG_STATUS_DISCONNECTED_ERROR);
		return;
	}

	struct bt_gatt_hids_c_rep_info *config_rep =
		bt_gatt_hids_c_rep_find(recipient_hidc,
					BT_GATT_HIDS_REPORT_TYPE_FEATURE,
					REPORT_ID_USER_CONFIG);

	if (!config_rep) {
		LOG_ERR("Feature report not found");
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	notify_config_forwarded(CONFIG_STATUS_PENDING);

	channel_id = event->channel_id;

	int err = bt_gatt_hids_c_rep_read(recipient_hidc,
					  config_rep,
					  hidc_read_cfg);

	if (err) {
		LOG_ERR("Reading report failed, err:%d", err);
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
	}
}

static void disconnect_subscriber(struct hids_subscriber *subscriber)
{
	LOG_INF("HID device disconnected");

	struct bt_gatt_hids_c_rep_info *rep = NULL;

	while (NULL != (rep = bt_gatt_hids_c_rep_next(&subscriber->hidc, rep))) {
		if (bt_gatt_hids_c_rep_type(rep) == BT_GATT_HIDS_REPORT_TYPE_INPUT) {
			uint8_t report_id = bt_gatt_hids_c_rep_id(rep);
			size_t size = bt_gatt_hids_c_rep_size(rep);

			__ASSERT_NO_MSG((report_id != REPORT_ID_RESERVED) &&
					(report_id < REPORT_ID_COUNT));

			/* Release all pressed keys. */
			uint8_t empty_data[size];

			memset(empty_data, 0, sizeof(empty_data));

			forward_hid_report(report_id, empty_data, size);
		}
	}

	bt_gatt_hids_c_release(&subscriber->hidc);
	subscriber->pid = 0;
	memset(subscriber->hwid, 0, sizeof(subscriber->hwid));
}

static void clear_state(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	usb_ready = false;
	usb_id = NULL;
	usb_busy = false;

	/* Clear all the reports. */
	while (!sys_slist_is_empty(&enqueued_report_list)) {
		struct enqueued_report *item;;

		item = CONTAINER_OF(sys_slist_get(&enqueued_report_list),
				     __typeof__(*item),
				    node);
		enqueued_report_count--;

		k_spin_unlock(&lock, key);

		k_free(item->report);
		k_free(item);

		key = k_spin_lock(&lock);
	}

	__ASSERT_NO_MSG(enqueued_report_count == 0);

	k_spin_unlock(&lock, key);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		k_spinlock_key_t key = k_spin_lock(&lock);

		__ASSERT_NO_MSG(usb_ready);

		struct enqueued_report *item = NULL;

		if (!sys_slist_is_empty(&enqueued_report_list)) {
			item = CONTAINER_OF(sys_slist_get(&enqueued_report_list),
					    __typeof__(*item),
					    node);
			enqueued_report_count--;

			EVENT_SUBMIT(item->report);
		} else {
			usb_busy = false;
		}

		k_spin_unlock(&lock, key);

		k_free(item);

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

		register_subscriber(event->dm, event->pid,
				    event->hwid, sizeof(event->hwid));

		return false;
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->subscriber == usb_id) {
			if (event->enabled) {
				k_spinlock_key_t key = k_spin_lock(&lock);
				usb_ready = true;
				k_spin_unlock(&lock, key);
			} else {
				clear_state();
			}
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		if (event->state == PEER_STATE_DISCONNECTED) {
			for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
				if ((bt_gatt_hids_c_assign_check(&subscribers[i].hidc)) &&
				    (bt_gatt_hids_c_conn(&subscribers[i].hidc) == event->id)) {
					disconnect_subscriber(&subscribers[i]);
				}
			}
		}

		return false;
	}

	if (is_usb_state_event(eh)) {
		const struct usb_state_event *event =
			cast_usb_state_event(eh);

		switch (event->state) {
		case USB_STATE_ACTIVE:
			usb_id = event->id;
			break;
		case USB_STATE_DISCONNECTED:
			clear_state();
			break;
		default:
			/* Ignore */
			break;
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_forward_event(eh)) {
			handle_config_forward(cast_config_forward_event(eh));

			return false;
		}

		if (is_config_forward_get_event(eh)) {
			handle_config_forward_get(cast_config_forward_get_event(eh));

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, ble_discovery_complete_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, usb_state_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_forward_event);
EVENT_SUBSCRIBE(MODULE, config_forward_get_event);
#endif
