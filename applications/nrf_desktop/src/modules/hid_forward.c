/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/slist.h>

#include <bluetooth/services/hids_c.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#ifdef CONFIG_BT_LL_NRFXLIB
#include "ble_controller_hci_vs.h"
#endif /* CONFIG_BT_LL_NRFXLIB */

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

#define CONN_INTERVAL_LLPM_US		1000   /* 1 ms */
#define CONN_INTERVAL_BLE		0x0006 /* 7.5 ms */
#define CONN_LATENCY_LOW		0
#define CONN_LATENCY_DEFAULT		99
#define CONN_SUPERVISION_TIMEOUT	400

#define LOW_LATENCY_TIMEOUT            K_SECONDS(CONFIG_DESKTOP_HID_FORWARD_LOW_LATENCY_TIMEOUT)
#define CONN_PARAMS_ERROR_TIMEOUT      K_MSEC(5)

#define MAX_ENQUEUED_ITEMS	5
#define INVALID_SUB_IDX		UCHAR_MAX

enum {
	CONN_LOW_LATENCY_ENABLED	= BIT(0),
	CONN_LOW_LATENCY_REQUIRED	= BIT(1),
	CONN_PARAMS_ERROR		= BIT(2)
};

struct enqueued_event_item {
	sys_snode_t node;
	struct hid_report_event *event;
};

struct hids_subscriber {
	struct bt_gatt_hids_c hidc;
	u16_t pid;
	u8_t conn_state;
	bool llpm_support;
	struct k_delayed_work timeout;
};

static struct hids_subscriber subscribers[CONFIG_BT_MAX_CONN];
static const void *usb_id;
static bool usb_ready;
static bool usb_busy;
static bool forward_pending;
static void *channel_id;

static sys_slist_t enqueued_event_list;
static size_t enqueued_event_count;
static u8_t rep_id_2_sub_idx[REPORT_ID_COUNT];

static struct k_spinlock lock;


static u8_t find_subscriber_idx_hidc(struct bt_gatt_hids_c *hidc)
{
	BUILD_ASSERT(ARRAY_SIZE(subscribers) < UCHAR_MAX);
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (&subscribers[i].hidc == hidc) {
			return (u8_t)i;
		}
	}
	return INVALID_SUB_IDX;
}

static struct hids_subscriber *find_subscriber_pid(u16_t pid)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].pid == pid) {
			return &subscribers[i];
		}
	}
	return NULL;
}

static int set_conn_params(struct bt_conn *conn, u16_t conn_latency,
			   bool peer_llpm_support)
{
	int err;

#ifdef CONFIG_BT_LL_NRFXLIB
	if (peer_llpm_support) {
		struct net_buf *buf;

		hci_vs_cmd_conn_update_t *cmd_conn_update;

		buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_CONN_UPDATE,
					sizeof(*cmd_conn_update));
		if (!buf) {
			LOG_ERR("Could not allocate command buffer");
			return -ENOBUFS;
		}

		u16_t conn_handle;

		err = bt_hci_get_conn_handle(conn, &conn_handle);
		if (err) {
			LOG_ERR("Failed obtaining conn_handle (err:%d)", err);
			return err;
		}

		cmd_conn_update = net_buf_add(buf, sizeof(*cmd_conn_update));
		cmd_conn_update->connection_handle   = conn_handle;
		cmd_conn_update->conn_interval_us    = CONN_INTERVAL_LLPM_US;
		cmd_conn_update->conn_latency        = conn_latency;
		cmd_conn_update->supervision_timeout = CONN_SUPERVISION_TIMEOUT;

		err = bt_hci_cmd_send_sync(HCI_VS_OPCODE_CMD_CONN_UPDATE, buf,
					   NULL);
	} else
#endif /* CONFIG_BT_LL_NRFXLIB */
	{
		struct bt_le_conn_param param = {
			.interval_min = CONN_INTERVAL_BLE,
			.interval_max = CONN_INTERVAL_BLE,
			.latency = conn_latency,
			.timeout = CONN_SUPERVISION_TIMEOUT,
		};

		err = bt_conn_le_param_update(conn, &param);

		if (err == -EALREADY) {
			/* Connection parameters are already set. */
			err = 0;
		}
	}

	return err;
}

static int update_subscriber_conn_params(struct hids_subscriber *sub)
{
	__ASSERT_NO_MSG(sub != NULL);
	struct bt_conn *conn = bt_gatt_hids_c_conn(&sub->hidc);

	__ASSERT_NO_MSG(conn);
	u16_t latency = (sub->conn_state & CONN_LOW_LATENCY_REQUIRED) ?
			CONN_LATENCY_LOW : CONN_LATENCY_DEFAULT;
	int err = set_conn_params(conn, latency, sub->llpm_support);

	if (err) {
		LOG_WRN("Cannot update suscriber %" PRIx16 " conn parameters"
			"(err:%d)", sub->pid, err);
		sub->conn_state |= CONN_PARAMS_ERROR;
		/* Retry to update the connection parameters after an error. */
		k_delayed_work_submit(&sub->timeout, CONN_PARAMS_ERROR_TIMEOUT);
	} else {
		LOG_INF("Conn parms for %"PRIx16 " set: %s, latency: %"PRIu16,
			sub->pid, sub->llpm_support ? "LLPM" : "BLE", latency);
		sub->conn_state &= ~CONN_PARAMS_ERROR;

		if (latency == CONN_LATENCY_LOW) {
			sub->conn_state |= CONN_LOW_LATENCY_ENABLED;
			sub->conn_state &= ~CONN_LOW_LATENCY_REQUIRED;
			k_delayed_work_submit(&sub->timeout,
					      LOW_LATENCY_TIMEOUT);
		} else {
			sub->conn_state &= ~CONN_LOW_LATENCY_ENABLED;
			k_delayed_work_cancel(&sub->timeout);
		}
	}

	return err;
}

static void peripheral_timeout_fn(struct k_work *work_desc)
{
	__ASSERT_NO_MSG(work_desc != NULL);

	struct hids_subscriber *sub = CONTAINER_OF(work_desc,
						   struct hids_subscriber,
						   timeout);

	if (!(sub->conn_state & CONN_PARAMS_ERROR) &&
	    (sub->conn_state & CONN_LOW_LATENCY_REQUIRED)) {
		__ASSERT_NO_MSG(sub->conn_state & CONN_LOW_LATENCY_ENABLED);
		sub->conn_state &= ~CONN_LOW_LATENCY_REQUIRED;
		k_delayed_work_submit(&sub->timeout, LOW_LATENCY_TIMEOUT);
	} else {
		update_subscriber_conn_params(sub);
	}
}

static int switch_to_low_latency(struct hids_subscriber *sub)
{
	int err = 0;

	sub->conn_state |= CONN_LOW_LATENCY_REQUIRED;
	if (unlikely(!(sub->conn_state & CONN_LOW_LATENCY_ENABLED))) {
		err = update_subscriber_conn_params(sub);
	}

	return err;
}

static void enqueue_hid_event(struct hid_report_event *event)
{
	struct enqueued_event_item *item;

	if (enqueued_event_count < MAX_ENQUEUED_ITEMS) {
		item = k_malloc(sizeof(*item));
		enqueued_event_count++;
	} else {
		LOG_WRN("Enqueue dropped the oldest event");
		item = CONTAINER_OF(sys_slist_get(&enqueued_event_list),
				    __typeof__(*item),
				    node);
		k_free(item->event);
	}

	if (!item) {
		LOG_ERR("OOM error");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		item->event = event;
		sys_slist_append(&enqueued_event_list, &item->node);
	}
}

static void forward_hid_report(u8_t report_id, const u8_t *data, size_t size)
{
	struct hid_report_event *event = new_hid_report_event(size + sizeof(report_id));

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!usb_ready) {
		k_spin_unlock(&lock, key);
		k_free(event);
		return;
	}

	event->subscriber = usb_id;

	/* Forward report as is adding report id on the front. */
	event->dyndata.data[0] = report_id;
	memcpy(&event->dyndata.data[1], data, size);

	if (!usb_busy) {
		EVENT_SUBMIT(event);
		usb_busy = true;
	} else {
		enqueue_hid_event(event);
	}

	k_spin_unlock(&lock, key);
}

static u8_t hidc_read(struct bt_gatt_hids_c *hids_c,
		      struct bt_gatt_hids_c_rep_info *rep,
		      u8_t err,
		      const u8_t *data)
{
	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		return BT_GATT_ITER_CONTINUE;
	}

	u8_t report_id = bt_gatt_hids_c_rep_id(rep);
	size_t size = bt_gatt_hids_c_rep_size(rep);

	__ASSERT_NO_MSG((report_id != REPORT_ID_RESERVED) &&
			(report_id < REPORT_ID_COUNT));

	forward_hid_report(report_id, data, size);

	return BT_GATT_ITER_CONTINUE;
}

static void hidc_ready(struct bt_gatt_hids_c *hids_c)
{
	struct bt_gatt_hids_c_rep_info *rep = NULL;
	u8_t sub_idx = find_subscriber_idx_hidc(hids_c);

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
				u8_t report_id = bt_gatt_hids_c_rep_id(rep);

				LOG_INF("Subscriber to rep id:%d", report_id);
				if (IS_ENABLED(CONFIG_DESKTOP_HID_FORWARD_LOW_LATENCY_ON_HID_INPUT)) {
					/* Application assumes that only one
					 * subscriber provides HID input report
					 * with given ID.
					 */
					__ASSERT_NO_MSG(rep_id_2_sub_idx[report_id] ==
							INVALID_SUB_IDX);
					rep_id_2_sub_idx[report_id] = sub_idx;
				}
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

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	LOG_INF("Reject connection parameter update request");

	return false;
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
		k_delayed_work_init(&subscribers[i].timeout,
				    peripheral_timeout_fn);
	}
	sys_slist_init(&enqueued_event_list);

	static struct bt_conn_cb conn_callbacks = {
		.le_param_req = le_param_req,
	};

	bt_conn_cb_register(&conn_callbacks);

	if (IS_ENABLED(CONFIG_DESKTOP_HID_FORWARD_LOW_LATENCY_ON_HID_INPUT)) {
		for (size_t i = 0; i < ARRAY_SIZE(rep_id_2_sub_idx); i++) {
			rep_id_2_sub_idx[i] = INVALID_SUB_IDX;
		}
	}
}

static int register_subscriber(struct bt_gatt_dm *dm, u16_t pid,
			       bool peer_llpm_support)
{
	struct hids_subscriber *new_sub = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (!bt_gatt_hids_c_assign_check(&subscribers[i].hidc)) {
			new_sub = &subscribers[i];
			break;
		}
	}
	__ASSERT_NO_MSG(new_sub != NULL);

	new_sub->pid = pid;
	new_sub->llpm_support = peer_llpm_support;
	int err = bt_gatt_hids_c_handles_assign(dm, &new_sub->hidc);

	if (err) {
		LOG_ERR("Cannot assign handles (err:%d)", err);
	}

	new_sub->conn_state |= CONN_LOW_LATENCY_REQUIRED;
	update_subscriber_conn_params(new_sub);

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
			  u8_t err)
{
	if (err) {
		LOG_WRN("Failed to write report: %d", err);
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
	} else {
		notify_config_forwarded(CONFIG_STATUS_SUCCESS);
	}
}

static u8_t hidc_read_cfg(struct bt_gatt_hids_c *hidc,
			  struct bt_gatt_hids_c_rep_info *rep,
			  u8_t err, const u8_t *data)
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

static void handle_config_forward(const struct config_forward_event *event)
{
	struct hids_subscriber *subscriber = find_subscriber_pid(event->recipient);

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

	struct bt_gatt_hids_c_rep_info *config_rep =
		bt_gatt_hids_c_rep_find(recipient_hidc,
					BT_GATT_HIDS_REPORT_TYPE_FEATURE,
					REPORT_ID_USER_CONFIG);

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
	u8_t report[REPORT_SIZE_USER_CONFIG];

	if (event->status == CONFIG_STATUS_FETCH) {
		LOG_INF("Forwarding fetch request");
		frame.status = CONFIG_STATUS_FETCH;
	} else {
		frame.status = CONFIG_STATUS_PENDING;
	}

	frame.recipient = event->recipient;
	frame.event_id = event->id;
	frame.event_data_len = event->dyndata.size;
	frame.event_data = (u8_t *)event->dyndata.data;

	int err = switch_to_low_latency(subscriber);

	if (err) {
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	int pos = config_channel_report_fill(report, sizeof(report), &frame,
					     false);

	if (pos < 0) {
		LOG_WRN("Could not set report");
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	notify_config_forwarded(CONFIG_STATUS_PENDING);

	err = bt_gatt_hids_c_rep_write(recipient_hidc,
				       config_rep,
				       hidc_write_cb,
				       report,
				       sizeof(report));
	if (err) {
		LOG_ERR("Writing report failed, err:%d", err);
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
	}

	return;
}

static void handle_config_forward_get(const struct config_forward_get_event *event)
{
	struct hids_subscriber *subscriber =
		find_subscriber_pid(event->recipient);

	if (!subscriber) {
		LOG_INF("Recipent %02" PRIx16 "not found", event->recipient);
		notify_config_forwarded(CONFIG_STATUS_DISCONNECTED_ERROR);
		return;
	}

	struct bt_gatt_hids_c *recipient_hidc =	&subscriber->hidc;

	__ASSERT_NO_MSG(recipient_hidc != NULL);

	int err = switch_to_low_latency(subscriber);

	if (err) {
		notify_config_forwarded(CONFIG_STATUS_WRITE_ERROR);
		return;
	}

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

	err = bt_gatt_hids_c_rep_read(recipient_hidc,
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
			u8_t report_id = bt_gatt_hids_c_rep_id(rep);
			size_t size = bt_gatt_hids_c_rep_size(rep);

			__ASSERT_NO_MSG((report_id != REPORT_ID_RESERVED) &&
					(report_id < REPORT_ID_COUNT));

			/* Release all pressed keys. */
			u8_t empty_data[size];

			memset(empty_data, 0, sizeof(empty_data));

			forward_hid_report(report_id, empty_data, size);
			if (IS_ENABLED(CONFIG_DESKTOP_HID_FORWARD_LOW_LATENCY_ON_HID_INPUT)) {
				rep_id_2_sub_idx[report_id] = INVALID_SUB_IDX;
			}
		}
	}

	bt_gatt_hids_c_release(&subscriber->hidc);
	subscriber->pid = 0;
	subscriber->conn_state = 0;
	subscriber->llpm_support = false;
	k_delayed_work_cancel(&subscriber->timeout);
}

static void clear_state(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	usb_ready = false;
	usb_id = NULL;
	usb_busy = false;

	/* Clear all the reports. */
	while (!sys_slist_is_empty(&enqueued_event_list)) {
		struct enqueued_event_item *item;;

		item = CONTAINER_OF(sys_slist_get(&enqueued_event_list),
				     __typeof__(*item),
				    node);
		enqueued_event_count--;

		k_spin_unlock(&lock, key);

		k_free(item->event);
		k_free(item);

		key = k_spin_lock(&lock);
	}

	__ASSERT_NO_MSG(enqueued_event_count == 0);

	k_spin_unlock(&lock, key);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		k_spinlock_key_t key = k_spin_lock(&lock);

		__ASSERT_NO_MSG(usb_ready);

		struct enqueued_event_item *item = NULL;

		if (!sys_slist_is_empty(&enqueued_event_list)) {
			item = CONTAINER_OF(sys_slist_get(&enqueued_event_list),
					    __typeof__(*item),
					    node);
			enqueued_event_count--;

			EVENT_SUBMIT(item->event);
		} else {
			usb_busy = false;
		}

		k_spin_unlock(&lock, key);

		k_free(item);

		if (IS_ENABLED(CONFIG_DESKTOP_HID_FORWARD_LOW_LATENCY_ON_HID_INPUT)) {
			const struct hid_report_sent_event *event =
				cast_hid_report_sent_event(eh);
			u8_t sub_idx = rep_id_2_sub_idx[event->report_id];

			if (sub_idx != INVALID_SUB_IDX &&
			    subscribers[sub_idx].llpm_support) {
				switch_to_low_latency(&subscribers[sub_idx]);
			}
		}

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
				    event->peer_llpm_support);

		return false;
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->subscriber == usb_id) {
			k_spinlock_key_t key = k_spin_lock(&lock);
			usb_ready = event->enabled;
			k_spin_unlock(&lock, key);
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
		case USB_STATE_POWERED:
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
