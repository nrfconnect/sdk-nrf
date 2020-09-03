/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/slist.h>
#include <settings/settings.h>

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
#include "power_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_FORWARD_LOG_LEVEL);

#define MAX_ENQUEUED_ITEMS CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS
#define CFG_CHAN_RSP_READ_DELAY		K_MSEC(15)
#define CFG_CHAN_MAX_RSP_POLL_CNT	50
#define CFG_CHAN_UNUSED_PEER_ID		UINT8_MAX
#define CFG_CHAN_BASE_ID		(CFG_CHAN_RECIPIENT_LOCAL + 1)

#define PERIPHERAL_ADDRESSES_STORAGE_NAME "paddr"

BUILD_ASSERT(CFG_CHAN_MAX_RSP_POLL_CNT <= UCHAR_MAX);

struct enqueued_report {
	sys_snode_t node;
	struct hid_report_event *report;
};

struct enqueued_reports {
	sys_slist_t list;
	size_t count;
};

struct subscriber {
	const void *id;
	uint32_t enabled_reports_bm;
	bool busy;
	uint8_t last_peripheral_id;
};

struct hids_peripheral {
	struct bt_gatt_hids_c hidc;
	struct enqueued_reports enqueued_report[REPORT_ID_COUNT];

	struct k_delayed_work read_rsp;
	struct config_event *cfg_chan_rsp;
	uint8_t cfg_chan_id;
	uint8_t hwid[HWID_LEN];
	uint8_t cur_poll_cnt;
	uint8_t sub_id;
	uint8_t last_report_id;
};

static struct subscriber subscribers[CONFIG_USB_HID_DEVICE_COUNT];
static bt_addr_le_t peripheral_address[CONFIG_BT_MAX_PAIRED];
static struct hids_peripheral peripherals[CONFIG_BT_MAX_CONN];
static bool suspended;


#if CONFIG_USB_HID_DEVICE_COUNT > 1
static void verify_data(const struct bt_bond_info *info, void *user_data)
{
	for (size_t i = 0; i < ARRAY_SIZE(peripheral_address); i++) {
		if (!bt_addr_le_cmp(&peripheral_address[i], &info->addr)) {
			return;
		}
	}

	LOG_WRN("Peer data inconsistency. Removing unknown peer.");
	int err = bt_unpair(BT_ID_DEFAULT, &info->addr);

	if (err) {
		LOG_ERR("Cannot unpair peer (err %d)", err);
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, PERIPHERAL_ADDRESSES_STORAGE_NAME)) {
		ssize_t len = read_cb(cb_arg, &peripheral_address,
				      sizeof(peripheral_address));

		if ((len != sizeof(peripheral_address)) || (len != len_rd)) {
			LOG_ERR("Can't read peripheral addresses from storage");
			module_set_state(MODULE_STATE_ERROR);
			return len;
		}
	}

	return 0;
}

static int verify_peripheral_address(void)
{
	/* On commit we should verify data to prevent inconsistency.
	 * Inconsistency could be caused e.g. by reset after secure,
	 * but before storing peer type in this module.
	 */
	bt_foreach_bond(BT_ID_DEFAULT, verify_data, NULL);

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(hid_forward, MODULE_NAME, NULL, settings_set,
			       verify_peripheral_address, NULL);
#endif /* CONFIG_USB_HID_DEVICE_COUNT > 1 */

static int store_peripheral_address(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" PERIPHERAL_ADDRESSES_STORAGE_NAME;
		int err = settings_save_one(key, peripheral_address,
					    sizeof(peripheral_address));

		if (err) {
			LOG_ERR("Problem storing peripheral addresses (err %d)",
				err);
			return err;
		}
	}

	return 0;
}

static void reset_peripheral_address(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(peripheral_address); i++) {
		bt_addr_le_copy(&peripheral_address[i], BT_ADDR_LE_NONE);
	}
}

static struct subscriber *get_subscriber(const struct hids_peripheral *per)
{
	__ASSERT_NO_MSG(per->sub_id < ARRAY_SIZE(subscribers));
	return &subscribers[per->sub_id];
}

static bool is_report_enabled(struct subscriber *sub, uint8_t report_id)
{
	__ASSERT_NO_MSG(report_id < __CHAR_BIT__ * sizeof(sub->enabled_reports_bm));
	return (sub->enabled_reports_bm & BIT(report_id)) != 0;
}

static void enqueue_hid_report(struct enqueued_reports *enqueued_report,
			       struct hid_report_event *report)
{
	struct enqueued_report *item;

	if (enqueued_report->count < MAX_ENQUEUED_ITEMS) {
		item = k_malloc(sizeof(*item));
		enqueued_report->count++;
	} else {
		LOG_WRN("Enqueue dropped the oldest report");
		item = CONTAINER_OF(sys_slist_get(&enqueued_report->list),
				    __typeof__(*item),
				    node);
		k_free(item->report);
	}

	if (!item) {
		LOG_ERR("OOM error");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		item->report = report;
		sys_slist_append(&enqueued_report->list, &item->node);
	}
}

static void forward_hid_report(struct hids_peripheral *per, uint8_t report_id,
			       const uint8_t *data, size_t size)
{
	struct subscriber *sub = get_subscriber(per);

	if (report_id >= __CHAR_BIT__ * sizeof(sub->enabled_reports_bm)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	if (report_id > ARRAY_SIZE(per->enqueued_report)) {
		LOG_ERR("Unknown report id");
		return;
	}

	struct hid_report_event *report = new_hid_report_event(size + sizeof(report_id));

	if (!is_report_enabled(sub, report_id) && !suspended && !per->enqueued_report[report_id].count) {
		k_free(report);
		return;
	}

	report->subscriber = sub->id;

	/* Forward report as is adding report id on the front. */
	report->dyndata.data[0] = report_id;
	memcpy(&report->dyndata.data[1], data, size);

	if (!sub->busy && !suspended && !per->enqueued_report[report_id].count) {
		EVENT_SUBMIT(report);
		per->last_report_id = report_id;
		sub->busy = true;
	} else {
		enqueue_hid_report(&per->enqueued_report[report_id], report);
	}

	if (suspended) {
		struct wake_up_event *event = new_wake_up_event();
		EVENT_SUBMIT(event);
	}
}

static uint8_t hidc_read(struct bt_gatt_hids_c *hids_c,
		      struct bt_gatt_hids_c_rep_info *rep,
		      uint8_t err,
		      const uint8_t *data)
{
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hids_peripheral *per = CONTAINER_OF(hids_c,
						   struct hids_peripheral,
						   hidc);
	__ASSERT_NO_MSG(per);

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

	forward_hid_report(per, report_id, data, size);

	return BT_GATT_ITER_CONTINUE;
}

static bool is_peripheral_connected(struct hids_peripheral *per)
{
	return bt_gatt_hids_c_assign_check(&per->hidc);
}

static int register_peripheral(struct bt_gatt_dm *dm, const uint8_t *hwid,
			       size_t hwid_len)
{
	BUILD_ASSERT((ARRAY_SIZE(subscribers) == ARRAY_SIZE(peripheral_address)) ||
		     (ARRAY_SIZE(subscribers) == 1));

	/* Route peripheral to the right subscriber. */
	size_t sub_id;

	if (ARRAY_SIZE(subscribers) == 1) {
		/* If there is only one subscriber for all peripherals. */
		sub_id = 0;
	} else {
		/* If there is a dedicated subscriber for each bonded peer. */
		for (sub_id = 0; sub_id < ARRAY_SIZE(peripheral_address); sub_id++) {
			if (!bt_addr_le_cmp(&peripheral_address[sub_id],
					    bt_conn_get_dst(bt_gatt_dm_conn_get(dm)))) {
				LOG_INF("Subscriber id found (%zu)", sub_id);
				break;
			}
			if (!bt_addr_le_cmp(&peripheral_address[sub_id], BT_ADDR_LE_NONE)) {
				LOG_INF("New subscriber id attached (%zu)", sub_id);
				bt_addr_le_copy(&peripheral_address[sub_id],
						bt_conn_get_dst(bt_gatt_dm_conn_get(dm)));
				store_peripheral_address();
				break;
			}
		}
	}
	__ASSERT_NO_MSG(sub_id < ARRAY_SIZE(subscribers));

	size_t per_id;
	for (per_id = 0; per_id < ARRAY_SIZE(peripherals); per_id++) {
		if (!is_peripheral_connected(&peripherals[per_id])) {
			break;
		}
	}
	__ASSERT_NO_MSG(per_id < ARRAY_SIZE(peripherals));

	peripherals[per_id].sub_id = sub_id;

	__ASSERT_NO_MSG(hwid_len == HWID_LEN);
	memcpy(peripherals[per_id].hwid, hwid, hwid_len);

	int err = bt_gatt_hids_c_handles_assign(dm, &peripherals[per_id].hidc);

	if (err) {
		LOG_ERR("Cannot assign handles (err:%d)", err);
	}

	return err;
}

static void submit_forward_error_rsp(struct hids_peripheral *per,
				     enum config_status rsp_status)
{
	struct config_event *rsp = per->cfg_chan_rsp;

	rsp->status = rsp_status;
	/* Error response has no additional data. */
	rsp->dyndata.size = 0;
	EVENT_SUBMIT(rsp);

	per->cfg_chan_rsp = NULL;
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

	struct hids_peripheral *per = CONTAINER_OF(hidc,
						   struct hids_peripheral,
						   hidc);

	if (err) {
		LOG_WRN("Failed to read report: %d", err);
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_ERROR);
	} else {
		/* Recipient and event_id must be stored to send proper values
		 * on error.
		 */
		uint8_t recipient = per->cfg_chan_rsp->recipient;
		uint8_t event_id = per->cfg_chan_rsp->event_id;

		int pos = config_channel_report_parse(data,
						      REPORT_SIZE_USER_CONFIG,
						      per->cfg_chan_rsp);

		if ((per->cfg_chan_rsp->status != CONFIG_STATUS_PENDING) &&
		    (per->cfg_chan_rsp->status != CONFIG_STATUS_TIMEOUT) &&
		    (per->cfg_chan_rsp->recipient != CFG_CHAN_RECIPIENT_LOCAL)) {
			pos = -ENOTSUP;
		}

		per->cfg_chan_rsp->recipient = recipient;
		__ASSERT_NO_MSG(recipient == per->cfg_chan_id);

		if (pos < 0) {
			LOG_WRN("Failed to parse response: %d", pos);
			per->cfg_chan_rsp->event_id = event_id;
			submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_ERROR);
			return pos;
		}

		if (per->cfg_chan_rsp->status == CONFIG_STATUS_PENDING) {
			LOG_WRN("GATT read done, but fetch was not ready yet");
			per->cfg_chan_rsp->event_id = event_id;
			per->cur_poll_cnt++;

			if (per->cur_poll_cnt >= CFG_CHAN_MAX_RSP_POLL_CNT) {
				submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_ERROR);
			} else {
				k_delayed_work_submit(&per->read_rsp,
						      CFG_CHAN_RSP_READ_DELAY);
			}
		} else {
			EVENT_SUBMIT(per->cfg_chan_rsp);
			per->cfg_chan_rsp = NULL;
		}
	}

	return 0;
}

static void read_rsp_fn(struct k_work *work)
{
	struct hids_peripheral *per = CONTAINER_OF(work,
						struct hids_peripheral,
						read_rsp);
	struct bt_gatt_hids_c_rep_info *config_rep =
		bt_gatt_hids_c_rep_find(&per->hidc,
					BT_GATT_HIDS_REPORT_TYPE_FEATURE,
					REPORT_ID_USER_CONFIG);

	__ASSERT_NO_MSG(config_rep);
	int err = bt_gatt_hids_c_rep_read(&per->hidc,
					  config_rep,
					  hidc_read_cfg);

	if (err) {
		LOG_WRN("Cannot read feature report (err: %d)", err);
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_ERROR);
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

	struct hids_peripheral *per = CONTAINER_OF(hidc,
						   struct hids_peripheral,
						   hidc);

	if (err) {
		LOG_WRN("Failed to write report: %d", err);
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_ERROR);
		return;
	}

	/* HID forward does not fetch configuration channel response for
	 * set operation. This is done to prevent HID report rate drops
	 * (for LLPM connection, the peripheral may send either HID report
	 * or configuration channel response in given connection interval).
	 */
	if (per->cfg_chan_rsp->status != CONFIG_STATUS_SET) {
		per->cur_poll_cnt = 0;
		k_delayed_work_submit(&per->read_rsp,
				      CFG_CHAN_RSP_READ_DELAY);
	} else {
		__ASSERT_NO_MSG(per->cfg_chan_rsp->dyndata.size == 0);
		per->cfg_chan_rsp->status = CONFIG_STATUS_SUCCESS;
		EVENT_SUBMIT(per->cfg_chan_rsp);
		per->cfg_chan_rsp = NULL;
	}
}

static struct hids_peripheral *find_peripheral(uint8_t cfg_chan_id)
{
	if (cfg_chan_id == CFG_CHAN_UNUSED_PEER_ID) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
		if (peripherals[i].cfg_chan_id == cfg_chan_id) {
			return &peripherals[i];
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
	rsp->status = event->status;
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

static void handle_config_channel_peers_req(const struct config_event *event)
{
	BUILD_ASSERT(ARRAY_SIZE(peripherals) + CFG_CHAN_BASE_ID <=
		     CFG_CHAN_UNUSED_PEER_ID);

	static uint8_t cur_per;

	switch (event->status) {
	case CONFIG_STATUS_INDEX_PEERS:
		for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
			if (is_peripheral_connected(&peripherals[i])) {
				peripherals[i].cfg_chan_id =
					CFG_CHAN_BASE_ID + i;
			}
		}

		cur_per = 0;
		send_nodata_response(event, CONFIG_STATUS_SUCCESS);
		break;

	case CONFIG_STATUS_GET_PEER:
	{
		struct hids_peripheral *per;

		if (cur_per < ARRAY_SIZE(peripherals)) {
			per = &peripherals[cur_per];
		} else {
			per = NULL;
		}

		while ((cur_per < ARRAY_SIZE(peripherals)) &&
		       (per->cfg_chan_id == CFG_CHAN_UNUSED_PEER_ID)) {
			cur_per++;
			if (cur_per < ARRAY_SIZE(peripherals)) {
				per = &peripherals[cur_per];
			}
		}

		BUILD_ASSERT(sizeof(per->hwid) == HWID_LEN);

		struct config_event *rsp = generate_response(event,
						    HWID_LEN + sizeof(uint8_t));
		size_t pos = 0;

		if (cur_per < ARRAY_SIZE(peripherals)) {
			memcpy(&rsp->dyndata.data[pos],
			       per->hwid, sizeof(per->hwid));
			pos += sizeof(per->hwid);

			rsp->dyndata.data[pos] = per->cfg_chan_id;
			cur_per++;
		} else {
			pos += HWID_LEN;

			rsp->dyndata.data[pos] = CFG_CHAN_UNUSED_PEER_ID;
		}

		rsp->status = CONFIG_STATUS_SUCCESS;
		EVENT_SUBMIT(rsp);
		break;
	}

	default:
		/* Not supported by this function. */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static bool handle_config_event(struct config_event *event)
{
	/* Make sure the function will not be preempted by hidc callbacks. */
	BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0);

	/* Ignore response event. */
	if (!event->is_request) {
		return false;
	}

	if ((event->recipient == CFG_CHAN_RECIPIENT_LOCAL) &&
	    ((event->status == CONFIG_STATUS_INDEX_PEERS) ||
	     (event->status == CONFIG_STATUS_GET_PEER))) {
		handle_config_channel_peers_req(event);
		return true;
	}

	struct hids_peripheral *per = find_peripheral(event->recipient);

	if (!per) {
		LOG_INF("Recipent %02" PRIx8 "not found", event->recipient);
		return false;
	}

	if (per->cfg_chan_rsp) {
		send_nodata_response(event, CONFIG_STATUS_REJECT);
		LOG_WRN("Transaction already in progress");
		return true;
	}

	struct bt_gatt_hids_c *recipient_hidc =	&per->hidc;

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

	uint8_t recipient = event->recipient;
	uint8_t report[REPORT_SIZE_USER_CONFIG];

	/* Update recipient of forwarded data. */
	event->recipient = CFG_CHAN_RECIPIENT_LOCAL;

	int pos = config_channel_report_fill(report, REPORT_SIZE_USER_CONFIG,
					     event);

	event->recipient = recipient;

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
		__ASSERT_NO_MSG(per->cfg_chan_rsp == NULL);
		per->cfg_chan_rsp = generate_response(event, dyndata_size);
	}

	return true;
}

static void disconnect_peripheral(struct hids_peripheral *per)
{
	LOG_INF("HID device disconnected");

	struct bt_gatt_hids_c_rep_info *rep = NULL;

	while (NULL != (rep = bt_gatt_hids_c_rep_next(&per->hidc, rep))) {
		if (bt_gatt_hids_c_rep_type(rep) == BT_GATT_HIDS_REPORT_TYPE_INPUT) {
			uint8_t report_id = bt_gatt_hids_c_rep_id(rep);
			size_t size = bt_gatt_hids_c_rep_size(rep);

			__ASSERT_NO_MSG((report_id != REPORT_ID_RESERVED) &&
					(report_id < REPORT_ID_COUNT));

			/* Release all pressed keys. */
			uint8_t empty_data[size];

			memset(empty_data, 0, sizeof(empty_data));

			forward_hid_report(per, report_id, empty_data, size);
		}
	}

	bt_gatt_hids_c_release(&per->hidc);
	k_delayed_work_cancel(&per->read_rsp);
	memset(per->hwid, 0, sizeof(per->hwid));
	per->cur_poll_cnt = 0;
	per->cfg_chan_id = CFG_CHAN_UNUSED_PEER_ID;
	if (per->cfg_chan_rsp) {
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_ERROR);
	}
}

static void clear_state(void)
{
	for (size_t per_id = 0; per_id < ARRAY_SIZE(peripherals); per_id++) {
		struct hids_peripheral *per = &peripherals[per_id];

		for (size_t report_id = 0; report_id < ARRAY_SIZE(per->enqueued_report); report_id++) {
			struct enqueued_reports *enqueued_report = &per->enqueued_report[report_id];

			/* Clear all the reports. */
			while (!sys_slist_is_empty(&enqueued_report->list)) {
				struct enqueued_report *item;;

				item = CONTAINER_OF(sys_slist_get(&enqueued_report->list),
						     __typeof__(*item),
						    node);
				enqueued_report->count--;

				k_free(item->report);
				k_free(item);
			}

			__ASSERT_NO_MSG(enqueued_report->count == 0);
		}
	}
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

	for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
		struct hids_peripheral *per = &peripherals[i];

		bt_gatt_hids_c_init(&per->hidc, &params);
		k_delayed_work_init(&per->read_rsp, read_rsp_fn);
		per->cfg_chan_id = CFG_CHAN_UNUSED_PEER_ID;

		for (size_t report_id = 0; report_id < ARRAY_SIZE(per->enqueued_report); report_id++) {
			struct enqueued_reports *enqueued_report = &per->enqueued_report[report_id];

			sys_slist_init(&enqueued_report->list);
			enqueued_report->count = 0;
		}
	}

	reset_peripheral_address();
}

static void send_enqueued_report(struct subscriber *sub)
{
	if (sub->busy) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
		size_t per_id = (sub->last_peripheral_id + i + 1) % ARRAY_SIZE(peripherals);
		struct hids_peripheral *per = &peripherals[per_id];

		/* Check if peripheral is linked with the subscriber. */
		if (sub != get_subscriber(per)) {
			continue;
		}

		/* Check if peripheral is connected. */
		if (!is_peripheral_connected(per)) {
			continue;
		}

		for (size_t j = 0; j < ARRAY_SIZE(per->enqueued_report); j++) {
			size_t report_id = (per->last_report_id + j + 1) % ARRAY_SIZE(per->enqueued_report);
			struct enqueued_reports *enqueued_report = &per->enqueued_report[report_id];

			if (!is_report_enabled(sub, report_id)) {
				/* Not subscribed yet. */
				continue;
			}

			if (sys_slist_is_empty(&enqueued_report->list)) {
				/* Nothing to send. */
				continue;
			}

			struct enqueued_report *item;

			item = CONTAINER_OF(sys_slist_get(&enqueued_report->list),
					    __typeof__(*item),
					    node);
			enqueued_report->count--;

			EVENT_SUBMIT(item->report);

			k_free(item);

			per->last_report_id = report_id;
			sub->busy = true;
			sub->last_peripheral_id = per_id;

			return;
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(eh);
		struct subscriber *sub = NULL;

		for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
			if (subscribers[i].id == event->subscriber) {
				sub = &subscribers[i];
				break;
			}
		}
		__ASSERT_NO_MSG(sub);

		sub->busy = false;
		send_enqueued_report(sub);

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

		register_peripheral(event->dm, event->hwid,
				    sizeof(event->hwid));

		return false;
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		struct subscriber *sub = NULL;

		for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
			if ((subscribers[i].id == event->subscriber) ||
			    (subscribers[i].id == NULL)) {
				sub = &subscribers[i];
			}

			if (subscribers[i].id == event->subscriber) {
				break;
			}
		}
		__ASSERT_NO_MSG(sub);

		if (!sub->id) {
			sub->id = event->subscriber;
		}

		__ASSERT_NO_MSG(event->report_id < __CHAR_BIT__ * sizeof(sub->enabled_reports_bm));
		if (event->enabled) {
			sub->enabled_reports_bm |= BIT(event->report_id);
			send_enqueued_report(sub);
		} else {
			sub->enabled_reports_bm &= ~BIT(event->report_id);
			clear_state();
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		if (event->state == PEER_STATE_DISCONNECTED) {
			for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
				if ((bt_gatt_hids_c_assign_check(&peripherals[i].hidc)) &&
				    (bt_gatt_hids_c_conn(&peripherals[i].hidc) == event->id)) {
					disconnect_peripheral(&peripherals[i]);
				}
			}
		}

		return false;
	}

	if (is_ble_peer_operation_event(eh)) {
		const struct ble_peer_operation_event *event =
			cast_ble_peer_operation_event(eh);

		if (event->op == PEER_OPERATION_ERASED) {
			reset_peripheral_address();
			store_peripheral_address();
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		suspended = false;

		return false;
	}

	if (is_power_down_event(eh)) {
		suspended = true;

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
EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
