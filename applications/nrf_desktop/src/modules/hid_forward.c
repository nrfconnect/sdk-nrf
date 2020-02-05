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

#include "ble_controller_hci_vs.h"

#define MODULE hid_forward
#include "module_state_event.h"

#include "hid_report_desc.h"
#include "config_channel.h"

#include "hid_event.h"
#include "ble_event.h"
#include "usb_event.h"
#include "config_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);

#define LATENCY_LOW		0
#define LATENCY_DEFAULT		99

#define FORWARD_TIMEOUT		K_SECONDS(5)
#define FORWARD_WORK_DELAY	K_SECONDS(1)

struct keyboard_event_item {
	sys_snode_t node;
	struct hid_keyboard_event *evt;
};

struct consumer_ctrl_event_item {
	sys_snode_t node;
	struct hid_consumer_ctrl_event *evt;
};

struct hids_subscriber {
	struct bt_gatt_hids_c hidc;
	u16_t pid;
	u32_t timestamp;
};

static struct k_delayed_work config_fwd_timeout;

static struct hids_subscriber subscribers[CONFIG_BT_MAX_CONN];
static const void *usb_id;
static atomic_t usb_ready;
static bool usb_busy;
static bool forward_pending;
static void *channel_id;

static struct hid_mouse_event *next_mouse_event;
static sys_slist_t keyboard_event_list;
static sys_slist_t consumer_ctrl_event_list;

static struct k_spinlock lock;


static int nrfxlib_vs_conn_latency_update(struct bt_conn *conn, u16_t latency)
{
	struct net_buf *buf;
	hci_vs_cmd_conn_update_t *cmd_conn_update;

	buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_CONN_UPDATE,
				sizeof(*cmd_conn_update));
	if (!buf) {
		LOG_ERR("Could not allocate command buffer");
		return -ENOBUFS;
	}

	u16_t conn_handle;

	int err = bt_hci_get_conn_handle(conn, &conn_handle);

	if (err) {
		LOG_ERR("Failed obtaining conn_handle (err %d)", err);
		return err;
	}

	cmd_conn_update = net_buf_add(buf, sizeof(*cmd_conn_update));

	cmd_conn_update->connection_handle   = conn_handle;
	cmd_conn_update->conn_interval_us    = 1000;
	cmd_conn_update->conn_latency        = latency;
	cmd_conn_update->supervision_timeout = 400;

	err = bt_hci_cmd_send_sync(HCI_VS_OPCODE_CMD_CONN_UPDATE, buf, NULL);

	return err;
}

static int change_connection_latency(struct hids_subscriber *subscriber, u16_t latency)
{
	__ASSERT_NO_MSG(subscriber != NULL);

	struct bt_conn *conn = bt_gatt_hids_c_conn(&subscriber->hidc);

	if (!conn) {
		LOG_WRN("There is no connection for a HIDS subscriber: %"
			PRIx16, subscriber->pid);
		return -ENXIO;
	}

	struct bt_conn_info info;
	int err = bt_conn_get_info(conn, &info);

	if (err) {
		LOG_WRN("Cannot get conn info (%d)", err);
		return err;
	}

	__ASSERT_NO_MSG(info.role == BT_CONN_ROLE_MASTER);

	const struct bt_le_conn_param param = {
		.interval_min = info.le.interval,
		.interval_max = info.le.interval,
		.latency = latency,
		.timeout = info.le.timeout
	};

	err = bt_conn_le_param_update(conn, &param);
	if (err == -EALREADY) {
		LOG_INF("Slave latency already changed");
		err = 0;
	} else if (err == -EINVAL) {
		/* LLPM connection interval is not supported by
		 * bt_conn_le_param_update function.
		 */
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_BT_LL_NRFXLIB));
		LOG_INF("Use vendor specific HCI command");
		err = nrfxlib_vs_conn_latency_update(conn, latency);
	}

	if (err) {
		LOG_WRN("Cannot update parameters (%d)", err);
	} else {
		LOG_INF("BLE latency changed to: %"PRIu16, latency);
	}

	return err;
}

static void forward_config_timeout(struct k_work *work_desc)
{
	__ASSERT_NO_MSG(work_desc != NULL);

	u32_t cur_time = k_uptime_get_32();
	bool forward_is_pending = false;
	int err = 0;

	for (size_t idx = 0; idx < ARRAY_SIZE(subscribers); idx++) {
		if ((subscribers[idx].timestamp != 0) &&
		    (bt_gatt_hids_c_assign_check(&subscribers[idx].hidc))) {
			if ((cur_time - subscribers[idx].timestamp) > FORWARD_TIMEOUT) {
				LOG_WRN("Configuration forward timed out");

				err = change_connection_latency(&subscribers[idx], LATENCY_DEFAULT);
				if (err) {
					LOG_WRN("Can't set latency to default");
					continue;
				}

				subscribers[idx].timestamp = 0;
			} else {
				forward_is_pending = true;
			}
		}
	}

	if (forward_is_pending) {
		k_delayed_work_submit(&config_fwd_timeout, FORWARD_WORK_DELAY);
	}
}

static void process_mouse_report(const u8_t *data)
{
	u8_t button_bm = data[0];
	s16_t wheel = (s8_t)data[1];

	u8_t x_buff[2] = {data[2], data[3] & 0x0F};
	u8_t y_buff[2] = {(data[3] >> 4) | ((data[4] << 4) & 0xF0), data[4] >> 4};

	u16_t x = sys_get_le16(x_buff);
	u16_t y = sys_get_le16(y_buff);

	if (x > MOUSE_REPORT_XY_MAX) {
		x |= 0xF000;
	}
	if (y > MOUSE_REPORT_XY_MAX) {
		y |= 0xF000;
	}

	struct hid_mouse_event *event;

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!atomic_get(&usb_ready)) {
		/* Do not process report if USB is disconnected. */
		k_spin_unlock(&lock, key);
		return;
	}

	if (next_mouse_event) {
		__ASSERT_NO_MSG(usb_busy);

		LOG_WRN("Event override");
		event = next_mouse_event;
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
		next_mouse_event = event;
	}

	k_spin_unlock(&lock, key);
}

static void process_keyboard_report(const u8_t *data)
{
	struct hid_keyboard_event *event = new_hid_keyboard_event();

	event->subscriber = usb_id;
	event->modifier_bm = data[0];
	memcpy(event->keys, &data[2], ARRAY_SIZE(event->keys));

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!atomic_get(&usb_ready)) {
		/* Do not process report if USB is disconnected. */
		k_spin_unlock(&lock, key);
		k_free(event);
		return;
	}

	if (!usb_busy) {
		EVENT_SUBMIT(event);
		usb_busy = true;
	} else {
		struct keyboard_event_item *evt_item = k_malloc(sizeof(*evt_item));

		if (!evt_item) {
			LOG_ERR("OOM error");
			k_panic();
		}

		evt_item->evt = event;
		sys_slist_append(&keyboard_event_list, &evt_item->node);
	}
	k_spin_unlock(&lock, key);
}

static void process_consumer_ctrl_report(const u8_t *data)
{
	struct hid_consumer_ctrl_event *event = new_hid_consumer_ctrl_event();

	event->subscriber = usb_id;
	event->usage = sys_get_le16(data);

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!atomic_get(&usb_ready)) {
		/* Do not process report if USB is disconnected. */
		k_spin_unlock(&lock, key);
		k_free(event);
		return;
	}

	if (!usb_busy) {
		EVENT_SUBMIT(event);
		usb_busy = true;
	} else {
		struct consumer_ctrl_event_item *evt_item =
			k_malloc(sizeof(*evt_item));

		if (!evt_item) {
			LOG_ERR("OOM error");
			k_panic();
		}

		evt_item->evt = event;
		sys_slist_append(&consumer_ctrl_event_list, &evt_item->node);
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

	if (err || !atomic_get(&usb_ready)) {
		return BT_GATT_ITER_CONTINUE;
	}

	switch (bt_gatt_hids_c_rep_id(rep)) {
	case REPORT_ID_MOUSE:
		process_mouse_report(data);
		break;

	case REPORT_ID_KEYBOARD_KEYS:
		process_keyboard_report(data);
		break;

	case REPORT_ID_CONSUMER_CTRL:
		process_consumer_ctrl_report(data);
		break;

	default:
		__ASSERT_NO_MSG(false);
	}

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
	sys_slist_init(&keyboard_event_list);

	k_delayed_work_init(&config_fwd_timeout, forward_config_timeout);
}

static int register_subscriber(struct bt_gatt_dm *dm, u16_t pid)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (!bt_gatt_hids_c_assign_check(&subscribers[i].hidc)) {
			break;
		}
	}
	__ASSERT_NO_MSG(i < ARRAY_SIZE(subscribers));

	subscribers[i].pid = pid;
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

static struct hids_subscriber *find_subscriber_hidc(u16_t pid)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].pid == pid) {
			return &subscribers[i];
		}
	}
	return NULL;
}

static int switch_to_low_latency(struct hids_subscriber *subscriber)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_BT_LL_NRFXLIB)) {
		if (subscriber->timestamp == 0) {
			LOG_INF("DFU next forward");
			err = change_connection_latency(subscriber, LATENCY_LOW);
			if (err) {
				LOG_WRN("Error while change of connection parameters");
				return err;
			}
		}
		subscriber->timestamp = k_uptime_get_32();

		if (!k_delayed_work_remaining_get(&config_fwd_timeout)) {
			k_delayed_work_submit(&config_fwd_timeout,
					      FORWARD_WORK_DELAY);
		}
	}

	return err;
}

static void handle_config_forward(const struct config_forward_event *event)
{
	struct hids_subscriber *subscriber = find_subscriber_hidc(event->recipient);

	if (!subscriber) {
		LOG_INF("Recipent %02" PRIx16 "not found", event->recipient);
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
		return;
	}

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
		find_subscriber_hidc(event->recipient);

	if (!subscriber) {
		LOG_INF("Recipent %02" PRIx16 "not found", event->recipient);
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
		LOG_DBG("GATT read already pending");
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

	/* Release all pressed keys. */
	u8_t empty_data[MAX(REPORT_SIZE_CONSUMER_CTRL,
			MAX(REPORT_SIZE_MOUSE,
				REPORT_SIZE_KEYBOARD_KEYS))] = {0};
	struct bt_gatt_hids_c_rep_info *rep = NULL;

	while (NULL != (rep = bt_gatt_hids_c_rep_next(&subscriber->hidc, rep))) {
		if (bt_gatt_hids_c_rep_type(rep) ==
				BT_GATT_HIDS_REPORT_TYPE_INPUT) {

			switch (bt_gatt_hids_c_rep_id(rep)) {
			case REPORT_ID_MOUSE:
				process_mouse_report(empty_data);
				break;

			case REPORT_ID_KEYBOARD_KEYS:
				process_keyboard_report(empty_data);
				break;

			case REPORT_ID_CONSUMER_CTRL:
				process_consumer_ctrl_report(empty_data);
				break;

			default:
				/* Unsupported report ID. */
				__ASSERT_NO_MSG(false);
				break;
			}
		}
	}

	bt_gatt_hids_c_release(&subscriber->hidc);
	subscriber->pid = 0;
	subscriber->timestamp = 0;
}

static void clear_state(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	atomic_set(&usb_ready, false);
	usb_id = NULL;
	usb_busy = false;

	/* Clear all the reports. */
	while (!sys_slist_is_empty(&keyboard_event_list)) {
		sys_snode_t *kbd_event_node =
			sys_slist_get(&keyboard_event_list);
		struct keyboard_event_item *next_item_kbd;

		next_item_kbd = CONTAINER_OF(kbd_event_node,
					     struct keyboard_event_item,
					     node);

		k_spin_unlock(&lock, key);
		k_free(next_item_kbd->evt);
		k_free(next_item_kbd);
		key = k_spin_lock(&lock);
	}

	while (!sys_slist_is_empty(&consumer_ctrl_event_list)) {
		sys_snode_t *c_ctrl_event_node =
			sys_slist_get(&consumer_ctrl_event_list);
		struct consumer_ctrl_event_item *next_item_c_ctrl;

		next_item_c_ctrl = CONTAINER_OF(c_ctrl_event_node,
						struct consumer_ctrl_event_item,
						node);

		k_spin_unlock(&lock, key);
		k_free(next_item_c_ctrl->evt);
		k_free(next_item_c_ctrl);
		key = k_spin_lock(&lock);
	}

	struct hid_mouse_event *mouse_evt = next_mouse_event;

	next_mouse_event = NULL;
	k_spin_unlock(&lock, key);

	k_free(mouse_evt);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		__ASSERT_NO_MSG(atomic_get(&usb_ready));

		struct keyboard_event_item *next_item_kbd = NULL;
		struct consumer_ctrl_event_item *next_item_c_ctrl = NULL;
		k_spinlock_key_t key = k_spin_lock(&lock);

		if (!sys_slist_is_empty(&keyboard_event_list)) {
			sys_snode_t *kbd_event_node =
				sys_slist_get(&keyboard_event_list);

			next_item_kbd = CONTAINER_OF(kbd_event_node,
						     struct keyboard_event_item,
						     node);

			EVENT_SUBMIT(next_item_kbd->evt);
		} else if (!sys_slist_is_empty(&consumer_ctrl_event_list)) {
			sys_snode_t *c_ctrl_event_node =
				sys_slist_get(&consumer_ctrl_event_list);

			next_item_c_ctrl = CONTAINER_OF(c_ctrl_event_node,
						struct consumer_ctrl_event_item,
						node);

			EVENT_SUBMIT(next_item_c_ctrl->evt);
		} else if (next_mouse_event) {
			EVENT_SUBMIT(next_mouse_event);
			next_mouse_event = NULL;
		} else {
			usb_busy = false;
		}
		k_spin_unlock(&lock, key);
		k_free(next_item_kbd);
		k_free(next_item_c_ctrl);

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

		register_subscriber(event->dm, event->pid);

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
