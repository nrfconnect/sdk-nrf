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
#include "config_channel_transport.h"

#include "hid_event.h"
#include "ble_event.h"
#include "usb_event.h"
#include "config_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_FORWARD_LOG_LEVEL);

#define MAX_ENQUEUED_ITEMS CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS
#define CFG_CHAN_RSP_READ_DELAY		K_MSEC(15)
#define CFG_CHAN_MAX_RSP_POLL_CNT	50

BUILD_ASSERT(CFG_CHAN_MAX_RSP_POLL_CNT <= UCHAR_MAX);

struct enqueued_report {
	sys_snode_t node;
	struct hid_report_event *report;
};

struct hids_subscriber {
	struct bt_gatt_hids_c hidc;
	struct k_delayed_work read_rsp;
	struct config_event *cfg_chan_rsp;
	uint16_t pid;
	uint8_t hwid[HWID_LEN];
	uint8_t cur_poll_cnt;
};

static struct hids_subscriber subscribers[CONFIG_BT_MAX_CONN];
static const void *usb_id;
static bool usb_ready;
static bool usb_busy;

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

static void submit_forward_error_rsp(struct hids_subscriber *sub,
				     enum config_status rsp_status)
{
	struct config_event *rsp = sub->cfg_chan_rsp;

	rsp->status = rsp_status;
	/* Error response has no additional data. */
	rsp->dyndata.size = 0;
	EVENT_SUBMIT(rsp);

	sub->cfg_chan_rsp = NULL;
}

static uint8_t hidc_read_cfg(struct bt_gatt_hids_c *hidc,
			  struct bt_gatt_hids_c_rep_info *rep,
			  uint8_t err, const uint8_t *data)
{
	/* Make sure that the handler will not preempt system workqueue
	 * and it will not be preempted by system workqueue.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hids_subscriber *sub = CONTAINER_OF(hidc,
						   struct hids_subscriber,
						   hidc);

	if (err) {
		LOG_WRN("Failed to read report: %d", err);
		submit_forward_error_rsp(sub, CONFIG_STATUS_WRITE_ERROR);
	} else {
		/* Recipient and event_id must be stored to send proper values
		 * on error.
		 */
		uint16_t recipient = sub->cfg_chan_rsp->recipient;
		uint8_t event_id = sub->cfg_chan_rsp->event_id;

		int pos = config_channel_report_parse(data,
						      REPORT_SIZE_USER_CONFIG,
						      sub->cfg_chan_rsp);

		if (pos < 0) {
			LOG_WRN("Failed to parse response: %d", pos);
			sub->cfg_chan_rsp->recipient = recipient;
			sub->cfg_chan_rsp->event_id = event_id;
			submit_forward_error_rsp(sub, CONFIG_STATUS_WRITE_ERROR);
			return pos;
		}

		if (sub->cfg_chan_rsp->status == CONFIG_STATUS_PENDING) {
			LOG_WRN("GATT read done, but fetch was not ready yet");
			sub->cfg_chan_rsp->recipient = recipient;
			sub->cfg_chan_rsp->event_id = event_id;
			sub->cur_poll_cnt++;

			if (sub->cur_poll_cnt >= CFG_CHAN_MAX_RSP_POLL_CNT) {
				submit_forward_error_rsp(sub, CONFIG_STATUS_WRITE_ERROR);
			} else {
				k_delayed_work_submit(&sub->read_rsp,
						      CFG_CHAN_RSP_READ_DELAY);
			}
		} else {
			EVENT_SUBMIT(sub->cfg_chan_rsp);
			sub->cfg_chan_rsp = NULL;
		}
	}

	return 0;
}

static void read_rsp_fn(struct k_work *work)
{
	struct hids_subscriber *sub = CONTAINER_OF(work,
						struct hids_subscriber,
						read_rsp);
	struct bt_gatt_hids_c_rep_info *config_rep =
		bt_gatt_hids_c_rep_find(&sub->hidc,
					BT_GATT_HIDS_REPORT_TYPE_FEATURE,
					REPORT_ID_USER_CONFIG);

	__ASSERT_NO_MSG(config_rep);
	int err = bt_gatt_hids_c_rep_read(&sub->hidc,
					  config_rep,
					  hidc_read_cfg);

	if (err) {
		LOG_WRN("Cannot read feature report (err: %d)", err);
		submit_forward_error_rsp(sub, CONFIG_STATUS_WRITE_ERROR);
	}
}

static void hidc_write_cb(struct bt_gatt_hids_c *hidc,
			  struct bt_gatt_hids_c_rep_info *rep,
			  uint8_t err)
{
	/* Make sure that the handler will not preempt system workqueue
	 * and it will not be preempted by system workqueue.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hids_subscriber *sub = CONTAINER_OF(hidc,
						   struct hids_subscriber,
						   hidc);

	if (err) {
		LOG_WRN("Failed to write report: %d", err);
		submit_forward_error_rsp(sub, CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	/* HID forward does not fetch configuration channel response for
	 * set operation. This is done to prevent HID report rate drops
	 * (for LLPM connection, the peripheral may send either HID report
	 * or configuration channel response in given connection interval).
	 */
	if (sub->cfg_chan_rsp->status != CONFIG_STATUS_SET) {
		sub->cur_poll_cnt = 0;
		k_delayed_work_submit(&sub->read_rsp,
				      CFG_CHAN_RSP_READ_DELAY);
	} else {
		__ASSERT_NO_MSG(sub->cfg_chan_rsp->dyndata.size == 0);
		sub->cfg_chan_rsp->status = CONFIG_STATUS_SUCCESS;
		EVENT_SUBMIT(sub->cfg_chan_rsp);
		sub->cfg_chan_rsp = NULL;
	}
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


static struct config_event *generate_response(const struct config_event *event,
					      size_t dyndata_size)
{
	__ASSERT_NO_MSG(event->is_request);

	struct config_event *rsp =
		new_config_event(dyndata_size);

	rsp->recipient = event->recipient;
	rsp->event_id = event->event_id;
	rsp->transport_id = event->transport_id;
	rsp->is_request = false;

	return rsp;
}

static void send_nodata_response(const struct config_event *event,
				 enum config_status rsp_status)
{
	struct config_event *rsp = generate_response(event, 0);

	rsp->status = rsp_status;
	EVENT_SUBMIT(rsp);
}

static bool handle_config_event(const struct config_event *event)
{
	/* Make sure the function will not be preempted by hidc callbacks. */
	BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0);

	/* Ignore response event. */
	if (!event->is_request) {
		return false;
	}

	struct hids_subscriber *subscriber = find_subscriber_hidc(event->recipient);

	if (!subscriber) {
		LOG_INF("Recipent %02" PRIx16 "not found", event->recipient);
		return false;
	}

	if (subscriber->cfg_chan_rsp) {
		send_nodata_response(event, CONFIG_STATUS_REJECT);
		LOG_WRN("Transaction already in progress");
		return true;
	}

	struct bt_gatt_hids_c *recipient_hidc =	&subscriber->hidc;

	__ASSERT_NO_MSG(recipient_hidc != NULL);

	if (!bt_gatt_hids_c_ready_check(recipient_hidc)) {
		send_nodata_response(event, CONFIG_STATUS_REJECT);
		LOG_WRN("Cannot forward, peer disconnected");
		return true;
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
		send_nodata_response(event, CONFIG_STATUS_WRITE_ERROR);
		LOG_ERR("Feature report not found");
		return true;
	}

	if (event->dyndata.size > UCHAR_MAX) {
		send_nodata_response(event, CONFIG_STATUS_WRITE_ERROR);
		LOG_WRN("Event data too big");
		return true;
	}

	uint8_t report[REPORT_SIZE_USER_CONFIG];
	int pos = config_channel_report_fill(report, REPORT_SIZE_USER_CONFIG,
					     event);

	if (pos < 0) {
		send_nodata_response(event, CONFIG_STATUS_WRITE_ERROR);
		LOG_WRN("Invalid event data");
		return true;
	}

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
		send_nodata_response(event, CONFIG_STATUS_WRITE_ERROR);
		LOG_ERR("Writing report failed, err:%d", err);
	} else {
		size_t dyndata_size;

		/* Set opetaion provides response without additional data. */
		if (event->status == CONFIG_STATUS_SET) {
			dyndata_size = 0;
		} else {
			dyndata_size = CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE;
		}
		/* Response will be handled by hidc_write_cb. */
		__ASSERT_NO_MSG(subscriber->cfg_chan_rsp == NULL);
		subscriber->cfg_chan_rsp = generate_response(event,
							     dyndata_size);
	}

	return true;
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
	k_delayed_work_cancel(&subscriber->read_rsp);
	memset(subscriber->hwid, 0, sizeof(subscriber->hwid));
	subscriber->cur_poll_cnt = 0;
	if (subscriber->cfg_chan_rsp) {
		submit_forward_error_rsp(subscriber, CONFIG_STATUS_WRITE_ERROR);
	}
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

static void hidc_prep_error(struct bt_gatt_hids_c *hids_c, int err)
{
	if (err) {
		LOG_ERR("err:%d", err);
	}
}

static void hidc_pm_update(struct bt_gatt_hids_c *hids_c)
{
	LOG_INF("Protocol mode updated");
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
		k_delayed_work_init(&subscribers[i].read_rsp, read_rsp_fn);
	}
	sys_slist_init(&enqueued_report_list);
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
		if (is_config_event(eh)) {
			return handle_config_event(cast_config_event(eh));
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
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
