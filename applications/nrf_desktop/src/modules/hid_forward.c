/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>
#include <zephyr/settings/settings.h>

#include <bluetooth/services/hogp.h>

#define MODULE hid_forward
#include <caf/events/module_state_event.h>

#include "hid_report_desc.h"
#include "config_channel_transport.h"

#include "hid_event.h"
#include <caf/events/ble_common_event.h>
#include "ble_event.h"
#include "config_event.h"
#include <caf/events/power_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_FORWARD_LOG_LEVEL);

#define MAX_ENQUEUED_ITEMS CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS
#define CFG_CHAN_RSP_READ_DELAY		15
#define CFG_CHAN_MAX_RSP_POLL_CNT	50
#define CFG_CHAN_UNUSED_PEER_ID		UINT8_MAX
#define CFG_CHAN_BASE_ID		(CFG_CHAN_RECIPIENT_LOCAL + 1)

#define PERIPHERAL_ADDRESSES_STORAGE_NAME "paddr"

#define OUTPUT_REPORT_DATA_MAX_LEN \
	(IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)?(REPORT_SIZE_KEYBOARD_LEDS):(0))

BUILD_ASSERT(CFG_CHAN_MAX_RSP_POLL_CNT <= UCHAR_MAX);

struct enqueued_report {
	sys_snode_t node;
	struct hid_report_event *report;
};

struct counted_list {
	sys_slist_t list;
	size_t count;
};

struct enqueued_reports {
	struct counted_list reports[ARRAY_SIZE(input_reports)];
	uint8_t last_idx;
};

struct report_data {
	uint8_t report_id;
	uint8_t data[OUTPUT_REPORT_DATA_MAX_LEN];
} __packed;

struct subscriber {
	const void *id;
	uint32_t enabled_reports_bm;
	struct enqueued_reports enqueued_reports;
	struct report_data out_reports[ARRAY_SIZE(output_reports)];
	uint32_t saved_out_reports_bm;

	bool busy;
	uint8_t last_peripheral_id;
};

struct hids_peripheral {
	struct bt_hogp hogp;
	struct enqueued_reports enqueued_reports;
	uint32_t enqueued_out_reports_bm;

	struct k_work_delayable read_rsp;
	struct config_event *cfg_chan_rsp;
	uint8_t cfg_chan_id;
	uint8_t hwid[HWID_LEN];
	uint8_t cur_poll_cnt;
	uint8_t sub_id;
};

static struct subscriber subscribers[CONFIG_USB_HID_DEVICE_COUNT];
static bt_addr_le_t peripheral_address[CONFIG_BT_MAX_PAIRED];
static struct hids_peripheral peripherals[CONFIG_BT_MAX_CONN];
static uint8_t peripheral_cache[CONFIG_BT_MAX_CONN];
static bool suspended;


static void hogp_out_rep_write_cb(struct bt_hogp *hogp, struct bt_hogp_rep_info *rep, uint8_t err);
static int send_hid_out_report(struct bt_hogp *hogp, const uint8_t *data, size_t size);
static void send_empty_hid_out_reports(struct hids_peripheral *per, struct subscriber *sub);

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

static size_t next_id(size_t id, size_t max)
{
	return (id + 1) % max;
}

static int get_input_report_idx(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(input_reports); i++) {
		if (input_reports[i] == report_id) {
			return i;
		}
	}

	return -ENOENT;
}

static size_t get_output_report_idx(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(output_reports); i++) {
		if (output_reports[i] == report_id) {
			return i;
		}
	}
	__ASSERT_NO_MSG(false);
	return 0;
}

static size_t get_output_report_size(uint8_t report_id)
{
	switch (report_id) {
	case REPORT_ID_KEYBOARD_LEDS:
		return sizeof(report_id) + REPORT_SIZE_KEYBOARD_LEDS;
	default:
		__ASSERT_NO_MSG(false);
		return 0;
	}
}

static bool is_report_enabled(const struct subscriber *sub, uint8_t report_id)
{
	__ASSERT_NO_MSG(report_id < __CHAR_BIT__ * sizeof(sub->enabled_reports_bm));
	return (sub->enabled_reports_bm & BIT(report_id)) != 0;
}

static bool is_report_enqueued(struct enqueued_reports *enqueued_reports,
			       size_t irep_idx)
{
	struct counted_list *reports = &enqueued_reports->reports[irep_idx];

	if (reports->count == 0) {
		__ASSERT_NO_MSG(sys_slist_is_empty(&reports->list));
		return false;
	}
	return true;
}

static bool is_any_report_enqueued(struct enqueued_reports *enqueued_reports)
{
	for (size_t irep_idx = 0; irep_idx < ARRAY_SIZE(enqueued_reports->reports); irep_idx++) {
		if (is_report_enqueued(enqueued_reports, irep_idx)) {
			return true;
		}
	}

	return false;
}

static struct enqueued_report *get_enqueued_report(struct enqueued_reports *enqueued_reports,
						   size_t irep_idx)
{
	struct counted_list *reports = &enqueued_reports->reports[irep_idx];
	struct enqueued_report *item;

	item = CONTAINER_OF(sys_slist_get(&reports->list),
			    __typeof__(*item),
			    node);
	__ASSERT_NO_MSG(reports->count > 0);
	reports->count--;

	return item;
}

static void drop_enqueued_reports(struct enqueued_reports *enqueued_reports,
				  size_t irep_idx)
{
	__ASSERT_NO_MSG(irep_idx < ARRAY_SIZE(enqueued_reports->reports));

	while (is_report_enqueued(enqueued_reports, irep_idx)) {
		struct enqueued_report *item;

		item = get_enqueued_report(enqueued_reports, irep_idx);

		app_event_manager_free(item->report);
		k_free(item);
	}
}

static void init_enqueued_reports(struct enqueued_reports *enqueued_reports)
{
	for (size_t irep_idx = 0; irep_idx < ARRAY_SIZE(enqueued_reports->reports); irep_idx++) {
		struct counted_list *reports = &enqueued_reports->reports[irep_idx];

		sys_slist_init(&reports->list);
		reports->count = 0;
	}

	enqueued_reports->last_idx = 0;
}

static struct enqueued_report *get_next_enqueued_report(struct enqueued_reports *enqueued_reports)
{
	struct enqueued_report *item = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(enqueued_reports->reports); i++) {
		size_t irep_idx = next_id(enqueued_reports->last_idx + i,
					  ARRAY_SIZE(enqueued_reports->reports));

		if (is_report_enqueued(enqueued_reports, irep_idx)) {
			item = get_enqueued_report(enqueued_reports, irep_idx);

			enqueued_reports->last_idx = irep_idx;
			break;
		}
	}

	return item;
}

static void migrate_enqueued_reports(struct enqueued_reports *dst_reports,
				     struct enqueued_reports *src_reports)
{
	/* Migrate only up to MAX_ENQUEUED_ITEMS newest items.
	 * As per can hold up only up to MAX_ENQUEUED_ITEMS at a time,
	 * migration from per to sub will affect entire list.
	 * When migrating from sub to par we will never get more items
	 * then the defined limit.
	 * Leaving the oldest items at sub will allow them to be sent
	 * out first.
	 */
	for (size_t irep_idx = 0; irep_idx < ARRAY_SIZE(dst_reports->reports); irep_idx++) {
		if (is_report_enqueued(src_reports, irep_idx)) {
			struct counted_list *dst = &dst_reports->reports[irep_idx];
			struct counted_list *src = &src_reports->reports[irep_idx];

			sys_slist_t tmp_list;
			size_t count = 0;

			sys_slist_init(&tmp_list);

			/* Move excessive items to a temporary list. */
			while (src->count > MAX_ENQUEUED_ITEMS) {
				sys_snode_t *node = sys_slist_get(&src->list);
				src->count--;

				sys_slist_append(&tmp_list, node);
				count++;
			}

			/* Source hold MAX_ENQUEUED_ITEMS items max.
			 * Migrate the entire list. */
			sys_slist_merge_slist(&dst->list, &src->list);
			dst->count += src->count;

			/* Bring back excessive items to the source list. */
			src->list = tmp_list;
			src->count = count;
		}
	}
}

static void enqueue_hid_report(struct enqueued_reports *enqueued_reports,
			       size_t irep_idx,
			       struct hid_report_event *report)
{
	__ASSERT_NO_MSG(irep_idx < ARRAY_SIZE(enqueued_reports->reports));

	struct counted_list *reports = &enqueued_reports->reports[irep_idx];

	struct enqueued_report *item;

	if (reports->count < MAX_ENQUEUED_ITEMS) {
		item = k_malloc(sizeof(*item));
	} else {
		LOG_WRN("Enqueue dropped the oldest report");
		item = get_enqueued_report(enqueued_reports, irep_idx);
		app_event_manager_free(item->report);
	}

	if (!item) {
		LOG_ERR("Dropped HID report");
		/* Should never happen. */
		__ASSERT_NO_MSG(false);
	} else {
		item->report = report;
		sys_slist_append(&reports->list, &item->node);
		reports->count++;
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

	int irep_idx = get_input_report_idx(report_id);

	if (irep_idx < 0) {
		LOG_ERR("Unsupported report id %" PRIu8, report_id);
		return;
	}

	if (suspended) {
		/* If suspended, report should wake up the board. */
		struct wake_up_event *event = new_wake_up_event();
		APP_EVENT_SUBMIT(event);
	}

	if (!is_report_enabled(sub, report_id)) {
		/* Drop report if subscriber is not waiting for it. */
		return;
	}

	struct hid_report_event *report = new_hid_report_event(size + sizeof(report_id));

	report->source = per;
	report->subscriber = sub->id;

	/* Forward report as is adding report id on the front. */
	report->dyndata.data[0] = report_id;
	memcpy(&report->dyndata.data[1], data, size);

	if (!sub->busy) {
		__ASSERT_NO_MSG(!is_report_enqueued(&per->enqueued_reports, irep_idx));

		APP_EVENT_SUBMIT(report);
		per->enqueued_reports.last_idx = irep_idx;
		sub->busy = true;
	} else {
		enqueue_hid_report(&per->enqueued_reports, irep_idx, report);
	}
}

static uint8_t hogp_read(struct bt_hogp *hids_c,
			 struct bt_hogp_rep_info *rep,
			 uint8_t err,
			 const uint8_t *data)
{
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hids_peripheral *per = CONTAINER_OF(hids_c,
						   struct hids_peripheral,
						   hogp);
	__ASSERT_NO_MSG(per);

	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		return BT_GATT_ITER_CONTINUE;
	}

	uint8_t report_id = bt_hogp_rep_id(rep);
	size_t size = bt_hogp_rep_size(rep);

	/* HID boot protocol reports are received with report ID equal 0 (REPORT_ID_RESERVED).
	 * The report ID must be updated before HID report is submitted as an app_event_manager
	 * event.
	 */
	if (report_id == REPORT_ID_RESERVED) {
		if (bt_hogp_rep_boot_kbd_in(hids_c)) {
			report_id = REPORT_ID_BOOT_KEYBOARD;
		} else if (bt_hogp_rep_boot_mouse_in(hids_c)) {
			report_id = REPORT_ID_BOOT_MOUSE;
		}
	}

	__ASSERT_NO_MSG((report_id != REPORT_ID_RESERVED) &&
			(report_id < REPORT_ID_COUNT));

	forward_hid_report(per, report_id, data, size);

	return BT_GATT_ITER_CONTINUE;
}

static bool is_peripheral_connected(struct hids_peripheral *per)
{
	return bt_hogp_assign_check(&per->hogp);
}

static enum bt_hids_pm get_sub_protocol_mode(const struct subscriber *sub)
{
	if (is_report_enabled(sub, REPORT_ID_BOOT_MOUSE) ||
	    is_report_enabled(sub, REPORT_ID_BOOT_KEYBOARD)) {
		return BT_HIDS_PM_BOOT;
	} else {
		return BT_HIDS_PM_REPORT;
	}
}

static void set_peripheral_protocol_mode(struct hids_peripheral *per, enum bt_hids_pm pm)
{
	int err = bt_hogp_pm_write(&per->hogp, pm);

	if (err) {
		LOG_ERR("Cannot update Protocol Mode (err: %d)", err);
	}
}

static void update_sub_protocol_mode(const struct subscriber *sub, enum bt_hids_pm pm)
{
	size_t sub_id = sub - &subscribers[0];

	__ASSERT_NO_MSG(sub_id < ARRAY_SIZE(subscribers));

	for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
		struct hids_peripheral *per = &peripherals[i];

		if (is_peripheral_connected(per) && (per->sub_id == sub_id)) {
			set_peripheral_protocol_mode(per, pm);
		}
	}
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

	struct hids_peripheral *per = &peripherals[per_id];

	/* Handles can be assigned now as we are in cooperative thread
	 * and won't be preempted. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	int err = bt_hogp_handles_assign(dm, &per->hogp);

	if (err) {
		LOG_ERR("Cannot assign handles (err:%d)", err);
		return err;
	}

	per->sub_id = sub_id;

	/* Migrate part of the unsent reports to this peripheral.
	 * This is needed to make sure that at any time number of
	 * allocated reports is within configured bounds.
	 */
	__ASSERT_NO_MSG(!is_any_report_enqueued(&per->enqueued_reports));
	ARG_UNUSED(is_any_report_enqueued);
	migrate_enqueued_reports(&per->enqueued_reports,
				 &get_subscriber(per)->enqueued_reports);

	__ASSERT_NO_MSG(hwid_len == HWID_LEN);
	memcpy(per->hwid, hwid, hwid_len);

	peripheral_cache[per_id]++;
	/* An odd number is assigned to a connected peripheral. */
	__ASSERT_NO_MSG(peripheral_cache[per_id] & 0x01);

	LOG_INF("Peripheral %p registered and linked to %p", (void *)per,
		(void *)get_subscriber(per));

	return err;
}

static void submit_forward_error_rsp(struct hids_peripheral *per,
				     enum config_status rsp_status)
{
	struct config_event *rsp = per->cfg_chan_rsp;

	rsp->status = rsp_status;
	/* Error response has no additional data. */
	rsp->dyndata.size = 0;
	APP_EVENT_SUBMIT(rsp);

	per->cfg_chan_rsp = NULL;
}

static uint8_t hogp_read_cfg(struct bt_hogp *hogp,
			     struct bt_hogp_rep_info *rep,
			     uint8_t err, const uint8_t *data)
{
	/* Ensure k_work_reschedule won't fail on canceling. */
	BUILD_ASSERT(CFG_CHAN_RSP_READ_DELAY > 0, "");

	/* Make sure that the handler will not preempt system workqueue
	 * and it will not be preempted by system workqueue.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hids_peripheral *per = CONTAINER_OF(hogp,
						   struct hids_peripheral,
						   hogp);

	if (err) {
		LOG_WRN("Failed to read report: %d", err);
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_FAIL);
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
			submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_FAIL);
			return pos;
		}

		if (per->cfg_chan_rsp->status == CONFIG_STATUS_PENDING) {
			LOG_WRN("GATT read done, but fetch was not ready yet");
			per->cfg_chan_rsp->event_id = event_id;
			per->cur_poll_cnt++;

			if (per->cur_poll_cnt >= CFG_CHAN_MAX_RSP_POLL_CNT) {
				submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_FAIL);
			} else {
				/* Reset response size. */
				per->cfg_chan_rsp->dyndata.size = CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE;
				k_work_reschedule(&per->read_rsp, K_MSEC(CFG_CHAN_RSP_READ_DELAY));
			}
		} else {
			APP_EVENT_SUBMIT(per->cfg_chan_rsp);
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
	struct bt_hogp_rep_info *config_rep =
		bt_hogp_rep_find(&per->hogp,
				 BT_HIDS_REPORT_TYPE_FEATURE,
				 REPORT_ID_USER_CONFIG);

	__ASSERT_NO_MSG(config_rep);
	int err = bt_hogp_rep_read(&per->hogp, config_rep, hogp_read_cfg);

	if (err) {
		LOG_WRN("Cannot read feature report (err: %d)", err);
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_FAIL);
	}
}

static void hogp_write_cb(struct bt_hogp *hogp,
			  struct bt_hogp_rep_info *rep,
			  uint8_t err)
{
	/* Make sure that the handler will not preempt system workqueue
	 * and it will not be preempted by system workqueue.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	struct hids_peripheral *per = CONTAINER_OF(hogp,
						   struct hids_peripheral,
						   hogp);

	if (err) {
		LOG_WRN("Failed to write report: %d", err);
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_FAIL);
		return;
	}

	/* HID forward does not fetch configuration channel response for
	 * set operation. This is done to prevent HID report rate drops
	 * (for LLPM connection, the peripheral may send either HID report
	 * or configuration channel response in given connection interval).
	 */
	if (per->cfg_chan_rsp->status != CONFIG_STATUS_SET) {
		per->cur_poll_cnt = 0;
		k_work_reschedule(&per->read_rsp, K_MSEC(CFG_CHAN_RSP_READ_DELAY));
	} else {
		__ASSERT_NO_MSG(per->cfg_chan_rsp->dyndata.size == 0);
		per->cfg_chan_rsp->status = CONFIG_STATUS_SUCCESS;
		APP_EVENT_SUBMIT(per->cfg_chan_rsp);
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
	APP_EVENT_SUBMIT(rsp);
}

static void handle_config_channel_peers_req(const struct config_event *event)
{
	BUILD_ASSERT(ARRAY_SIZE(peripherals) + CFG_CHAN_BASE_ID <=
		     CFG_CHAN_UNUSED_PEER_ID);

	static uint8_t cur_per_idx;

	switch (event->status) {
	case CONFIG_STATUS_INDEX_PEERS:
		for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
			if (is_peripheral_connected(&peripherals[i])) {
				peripherals[i].cfg_chan_id =
					CFG_CHAN_BASE_ID + i;
			}
		}

		cur_per_idx = 0;
		send_nodata_response(event, CONFIG_STATUS_SUCCESS);
		break;

	case CONFIG_STATUS_GET_PEER:
	{
		struct hids_peripheral *per = NULL;

		BUILD_ASSERT(sizeof(per->hwid) == HWID_LEN);

		while (cur_per_idx < ARRAY_SIZE(peripherals)) {
			struct hids_peripheral *cur_per = &peripherals[cur_per_idx++];

			if (cur_per->cfg_chan_id != CFG_CHAN_UNUSED_PEER_ID) {
				per = cur_per;
				break;
			}
		}

		struct config_event *rsp = generate_response(event,
						    HWID_LEN + sizeof(uint8_t));
		size_t pos = 0;

		if (per) {
			memcpy(&rsp->dyndata.data[pos],
			       per->hwid, sizeof(per->hwid));
			pos += sizeof(per->hwid);

			rsp->dyndata.data[pos] = per->cfg_chan_id;
		} else {
			pos += HWID_LEN;

			rsp->dyndata.data[pos] = CFG_CHAN_UNUSED_PEER_ID;
		}

		rsp->status = CONFIG_STATUS_SUCCESS;
		APP_EVENT_SUBMIT(rsp);
		break;
	}

	case CONFIG_STATUS_GET_PEERS_CACHE:
	{
		BUILD_ASSERT(ARRAY_SIZE(peripherals) <= CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE);
		BUILD_ASSERT(ARRAY_SIZE(peripherals) == ARRAY_SIZE(peripheral_cache));

		size_t pos = 0;
		struct config_event *rsp = generate_response(event,
							     CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE);

		memcpy(&rsp->dyndata.data[pos], peripheral_cache, sizeof(peripheral_cache));
		pos += sizeof(peripheral_cache);

		memset(&rsp->dyndata.data[pos], 0x00, CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE - pos);
		pos = CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE;

		rsp->status = CONFIG_STATUS_SUCCESS;
		APP_EVENT_SUBMIT(rsp);
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
	/* Make sure the function will not be preempted by hogp callbacks. */
	BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0);

	/* Ignore response event. */
	if (!event->is_request) {
		return false;
	}

	if (event->recipient == CFG_CHAN_RECIPIENT_LOCAL) {
		if ((event->status == CONFIG_STATUS_INDEX_PEERS) ||
		    (event->status == CONFIG_STATUS_GET_PEER) ||
		    (event->status == CONFIG_STATUS_GET_PEERS_CACHE)) {
			handle_config_channel_peers_req(event);
			return true;
		}

		/* Do not try to forward requests for local recipient. */
		return false;
	}

	struct hids_peripheral *per = find_peripheral(event->recipient);

	if (!per) {
		LOG_INF("Recipient %02" PRIx8 " not found", event->recipient);
		return false;
	}

	if (per->cfg_chan_rsp) {
		send_nodata_response(event, CONFIG_STATUS_REJECT);
		LOG_WRN("Transaction already in progress");
		return true;
	}

	struct bt_hogp *recipient_hogp = &per->hogp;

	__ASSERT_NO_MSG(recipient_hogp != NULL);

	if (!bt_hogp_ready_check(recipient_hogp)) {
		send_nodata_response(event, CONFIG_STATUS_REJECT);
		LOG_WRN("Cannot forward, peer disconnected");
		return true;
	}

	bool has_out_report = false;
	struct bt_hogp_rep_info *config_rep;

	config_rep = bt_hogp_rep_find(recipient_hogp,
				      BT_HIDS_REPORT_TYPE_OUTPUT,
				      REPORT_ID_USER_CONFIG_OUT);

	if (!config_rep) {
		config_rep = bt_hogp_rep_find(recipient_hogp,
					      BT_HIDS_REPORT_TYPE_FEATURE,
					      REPORT_ID_USER_CONFIG);
	} else {
		has_out_report = true;
	}

	if (!config_rep) {
		send_nodata_response(event, CONFIG_STATUS_WRITE_FAIL);
		LOG_ERR("Feature report not found");
		return true;
	}

	if (event->dyndata.size > UCHAR_MAX) {
		send_nodata_response(event, CONFIG_STATUS_WRITE_FAIL);
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
		send_nodata_response(event, CONFIG_STATUS_WRITE_FAIL);
		LOG_WRN("Invalid event data");
		return true;
	}

	int err;

	if (has_out_report) {
		err = bt_hogp_rep_write_wo_rsp(recipient_hogp, config_rep,
					       report, sizeof(report),
					       hogp_write_cb);
	} else {
		err = bt_hogp_rep_write(recipient_hogp, config_rep,
					hogp_write_cb, report, sizeof(report));
	}

	if (err) {
		send_nodata_response(event, CONFIG_STATUS_WRITE_FAIL);
		LOG_ERR("Writing report failed, err:%d", err);
	} else {
		size_t dyndata_size;

		/* Set opetaion provides response without additional data. */
		if (event->status == CONFIG_STATUS_SET) {
			dyndata_size = 0;
		} else {
			dyndata_size = CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE;
		}
		/* Response will be handled by hogp_write_cb. */
		__ASSERT_NO_MSG(per->cfg_chan_rsp == NULL);
		per->cfg_chan_rsp = generate_response(event, dyndata_size);
	}

	return true;
}

static void forward_empty_hid_report(struct hids_peripheral *per,
				     const struct bt_hogp_rep_info *rep,
				     uint8_t report_id)
{
	if (bt_hogp_rep_type(rep) != BT_HIDS_REPORT_TYPE_INPUT) {
		return;
	}
	size_t size = bt_hogp_rep_size(rep);

	/* Release all pressed keys. */
	uint8_t empty_data[size];

	memset(empty_data, 0, sizeof(empty_data));

	forward_hid_report(per, report_id, empty_data, size);
}

static void disconnect_peripheral(struct hids_peripheral *per)
{
	LOG_INF("Peripheral %p disconnected", (void *)per);

	struct bt_hogp_rep_info *rep;
	uint8_t report_id;

	if (bt_hogp_pm_get(&per->hogp) == BT_HIDS_PM_BOOT) {
		rep = bt_hogp_rep_boot_kbd_in(&per->hogp);
		if (rep != NULL) {
			report_id = REPORT_ID_BOOT_KEYBOARD;
			forward_empty_hid_report(per, rep, report_id);
		}
		rep = bt_hogp_rep_boot_mouse_in(&per->hogp);
		if (rep != NULL) {
			report_id = REPORT_ID_BOOT_MOUSE;
			forward_empty_hid_report(per, rep, report_id);
		}
	} else {
		rep = NULL;
		while (NULL != (rep = bt_hogp_rep_next(&per->hogp, rep))) {
			report_id = bt_hogp_rep_id(rep);
			if ((report_id == REPORT_ID_RESERVED) || (report_id >= REPORT_ID_COUNT)) {
				continue;
			}
			forward_empty_hid_report(per, rep, report_id);
		}
	}

	migrate_enqueued_reports(&get_subscriber(per)->enqueued_reports,
				 &per->enqueued_reports);
	__ASSERT_NO_MSG(!is_any_report_enqueued(&per->enqueued_reports));

	per->enqueued_out_reports_bm = 0;

	bt_hogp_release(&per->hogp);

	/* Cancel cannot fail if executed from another work's context. */
	(void)k_work_cancel_delayable(&per->read_rsp);
	memset(per->hwid, 0, sizeof(per->hwid));
	per->cur_poll_cnt = 0;
	per->cfg_chan_id = CFG_CHAN_UNUSED_PEER_ID;
	if (per->cfg_chan_rsp) {
		submit_forward_error_rsp(per, CONFIG_STATUS_WRITE_FAIL);
	}

	size_t per_id = per - &peripherals[0];

	peripheral_cache[per_id]++;
	/* An even number is assigned to a disconnected peripheral. */
	__ASSERT_NO_MSG(!(peripheral_cache[per_id] & 0x01));
}

static void enable_subscription(struct subscriber *sub, uint8_t report_id)
{
	int irep_idx = get_input_report_idx(report_id);

	if (irep_idx < 0) {
		LOG_ERR("Cannot enable subscription for report %" PRIu8,
			report_id);
		return;
	}

	WRITE_BIT(sub->enabled_reports_bm, report_id, 1);
}

static void disable_subscription(struct subscriber *sub, uint8_t report_id)
{
	int irep_idx = get_input_report_idx(report_id);

	if (irep_idx < 0) {
		LOG_ERR("Cannot disable subscription for report %" PRIu8,
			report_id);
		return;
	}

	WRITE_BIT(sub->enabled_reports_bm, report_id, 0);
	bool sub_disconnected = (sub->enabled_reports_bm == 0);

	/* For all peripherals connected to this subscriber. */
	for (size_t per_id = 0; per_id < ARRAY_SIZE(peripherals); per_id++) {
		struct hids_peripheral *per = &peripherals[per_id];

		if ((sub != get_subscriber(per)) ||
		    (!is_peripheral_connected(per))) {
			continue;
		}

		if (sub_disconnected) {
			send_empty_hid_out_reports(per, sub);
		}

		drop_enqueued_reports(&per->enqueued_reports, irep_idx);
	}

	/* And also this subscriber. */
	drop_enqueued_reports(&sub->enqueued_reports, irep_idx);

	if (sub_disconnected) {
		sub->saved_out_reports_bm = 0;
	}
}

static void hogp_ready(struct bt_hogp *hids_c)
{
	struct bt_hogp_rep_info *rep = NULL;

	while (NULL != (rep = bt_hogp_rep_next(hids_c, rep))) {
		if (bt_hogp_rep_type(rep) == BT_HIDS_REPORT_TYPE_INPUT) {
			int err = bt_hogp_rep_subscribe(hids_c, rep, hogp_read);

			if (err) {
				LOG_ERR("Cannot subscribe to report (err:%d)", err);
			} else {
				LOG_INF("Subscriber to rep id:%d", bt_hogp_rep_id(rep));
			}
		}
	}

	rep = bt_hogp_rep_boot_kbd_in(hids_c);

	if (rep) {
		int err = bt_hogp_rep_subscribe(hids_c, rep, hogp_read);

		if (err) {
			LOG_ERR("Cannot subscribe to boot keyboard report (err:%d)", err);
		} else {
			LOG_INF("Subscriber to boot keyboard report");
		}
	}

	rep = bt_hogp_rep_boot_mouse_in(hids_c);

	if (rep) {
		int err = bt_hogp_rep_subscribe(hids_c, rep, hogp_read);

		if (err) {
			LOG_ERR("Cannot subscribe to boot mouse report (err:%d)", err);
		} else {
			LOG_INF("Subscriber to boot mouse report");
		}
	}

	struct hids_peripheral *per = CONTAINER_OF(hids_c,
						   struct hids_peripheral,
						   hogp);

	enum bt_hids_pm per_pm = get_sub_protocol_mode(get_subscriber(per));

	if (per_pm == BT_HIDS_PM_BOOT) {
		set_peripheral_protocol_mode(per, per_pm);
	}

	/* Send collected HID output reports dedicated to this peripheral. */
	struct subscriber *sub = get_subscriber(per);

	for (size_t orep_idx = 0; orep_idx < ARRAY_SIZE(sub->out_reports); orep_idx++) {
		if (sub->saved_out_reports_bm & BIT(orep_idx)) {
			size_t size = get_output_report_size(sub->out_reports[orep_idx].report_id);

			int err = send_hid_out_report(&per->hogp,
						      (uint8_t *)&sub->out_reports[orep_idx],
						      size);

			if (err) {
				LOG_ERR("Failed to forward output report (err: %d)", err);
			}
		}
	}
}

static void hogp_prep_error(struct bt_hogp *hids_c, int err)
{
	if (err) {
		LOG_ERR("err:%d", err);
	}
}

static void hogp_pm_update(struct bt_hogp *hids_c)
{
	LOG_INF("Protocol mode updated");
}

static void init(void)
{
	static const struct bt_hogp_init_params params = {
		.ready_cb = hogp_ready,
		.prep_error_cb = hogp_prep_error,
		.pm_update_cb = hogp_pm_update,
	};

	for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
		struct hids_peripheral *per = &peripherals[i];

		bt_hogp_init(&per->hogp, &params);
		k_work_init_delayable(&per->read_rsp, read_rsp_fn);
		per->cfg_chan_id = CFG_CHAN_UNUSED_PEER_ID;

		per->enqueued_out_reports_bm = 0;
		init_enqueued_reports(&per->enqueued_reports);
	}

	reset_peripheral_address();

	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		struct subscriber *sub = &subscribers[i];

		__ASSERT_NO_MSG(ARRAY_SIZE(sub->out_reports) <=
				__CHAR_BIT__ * sizeof(sub->saved_out_reports_bm));
		init_enqueued_reports(&sub->enqueued_reports);
		sub->saved_out_reports_bm = 0;
	}
}

static void send_enqueued_report(struct subscriber *sub)
{
	if (sub->busy) {
		return;
	}

	struct enqueued_report *item;

	/* First try to send report left at subscriber. */
	item = get_next_enqueued_report(&sub->enqueued_reports);

	if (!item) {
		/* Look for any report to sent at linked peripherals. */
		for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
			size_t per_id = next_id(sub->last_peripheral_id + i,
						ARRAY_SIZE(peripherals));
			struct hids_peripheral *per = &peripherals[per_id];

			/* Check if peripheral is linked with the subscriber. */
			if (sub != get_subscriber(per)) {
				continue;
			}

			item = get_next_enqueued_report(&per->enqueued_reports);

			if (item) {
				sub->last_peripheral_id = per_id;
				break;
			}
		}
	}

	if (item) {
		APP_EVENT_SUBMIT(item->report);

		k_free(item);

		sub->busy = true;
	}
}

static int send_hid_out_report(struct bt_hogp *hogp, const uint8_t *data, size_t size)
{
	uint8_t report_id = data[0];
	struct bt_hogp_rep_info *out_rep = bt_hogp_rep_find(hogp,
							    BT_HIDS_REPORT_TYPE_OUTPUT,
							    report_id);

	if (!out_rep) {
		/* HID output report with given ID is not supported by the peripheral. */
		return 0;
	}

	int err = bt_hogp_rep_write_wo_rsp(hogp, out_rep,
					   &data[1], size - 1,
					   hogp_out_rep_write_cb);

	if (err == -ENOTCONN) {
		LOG_INF("Cannot forward, peer disconnected");
		err = 0;
	}

	return err;
}

static void hogp_out_rep_write_cb(struct bt_hogp *hogp, struct bt_hogp_rep_info *rep, uint8_t err)
{
	/* Make sure that the handler will not preempt system workqueue
	 * and it will not be preempted by system workqueue.
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	if (err) {
		LOG_WRN("Failed to forward HID out report (err %" PRIu8 ")", err);
	}

	struct hids_peripheral *per = CONTAINER_OF(hogp, struct hids_peripheral, hogp);

	/* Send update if it was queued. */
	uint8_t sent_report_id = bt_hogp_rep_id(rep);
	size_t orep_idx = get_output_report_idx(sent_report_id);
	struct subscriber *sub = get_subscriber(per);

	if (!(per->enqueued_out_reports_bm & BIT(orep_idx))) {
		return;
	}

	__ASSERT_NO_MSG(sub->saved_out_reports_bm & BIT(orep_idx));
	__ASSERT_NO_MSG(sent_report_id == sub->out_reports[orep_idx].report_id);
	size_t size = get_output_report_size(sub->out_reports[orep_idx].report_id);

	int send_err = send_hid_out_report(hogp, (uint8_t *)&sub->out_reports[orep_idx], size);

	if (send_err) {
		LOG_ERR("Cannot forward HID report (err: %d)", send_err);
	}
	WRITE_BIT(per->enqueued_out_reports_bm, orep_idx, 0);
}

static void send_empty_hid_out_reports(struct hids_peripheral *per, struct subscriber *sub)
{
	struct bt_hogp *per_hogp = &per->hogp;

	for (size_t orep_idx = 0; orep_idx < ARRAY_SIZE(sub->out_reports); orep_idx++) {
		if (sub->saved_out_reports_bm & BIT(orep_idx)) {
			uint8_t report_id = sub->out_reports[orep_idx].report_id;
			size_t size = get_output_report_size(report_id);
			uint8_t empty_data[size];

			memset(empty_data, 0, sizeof(empty_data));
			empty_data[0] = report_id;
			int err = send_hid_out_report(per_hogp, empty_data, size);

			if (err) {
				LOG_ERR("Failed to forward output report (err: %d)", err);
			}
		}
	}
}

static void save_hid_out_report(struct subscriber *sub, const uint8_t *data, size_t size)
{
	uint8_t report_id = data[0];

	__ASSERT_NO_MSG(size == get_output_report_size(report_id));
	size_t orep_idx = get_output_report_idx(report_id);

	memcpy(&sub->out_reports[orep_idx], data, size);
	WRITE_BIT(sub->saved_out_reports_bm, orep_idx, 1);
}

static void schedule_hid_out_report(uint32_t *hid_out_reports_bm, uint8_t report_id)
{
	/* Set bit informing that report was updated and it should be sent on next opportunity. */
	size_t orep_idx = get_output_report_idx(report_id);

	__ASSERT_NO_MSG(orep_idx < __CHAR_BIT__ * sizeof(*hid_out_reports_bm));
	WRITE_BIT(*hid_out_reports_bm, orep_idx, 1);
}

static void forward_hid_out_report(const struct hid_report_event *event,
				   struct hids_peripheral *per)
{
	struct bt_hogp *per_hogp = &per->hogp;

	__ASSERT_NO_MSG(per_hogp);

	const uint8_t *data = event->dyndata.data;
	size_t size = event->dyndata.size;

	int err = send_hid_out_report(per_hogp, data, size);

	if (err == -EBUSY) {
		schedule_hid_out_report(&per->enqueued_out_reports_bm, data[0]);
	} else if (err) {
		LOG_ERR("Failed to forward output report (err: %d)", err);
	} else {
		/* Report was sent successfully. */
	}
}

static bool handle_hid_report_event(const struct hid_report_event *event)
{
	/* Ignore HID input events. */
	if (event->subscriber) {
		return false;
	}

	size_t sub_idx;

	for (sub_idx = 0; sub_idx < ARRAY_SIZE(subscribers); sub_idx++) {
		if (subscribers[sub_idx].id == event->source) {
			break;
		}
	}

	if (sub_idx == ARRAY_SIZE(subscribers)) {
		LOG_WRN("No subscriber with ID: %p", event->source);
		return false;
	}

	save_hid_out_report(&subscribers[sub_idx],
			    event->dyndata.data,
			    event->dyndata.size);

	for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
		struct hids_peripheral *per = &peripherals[i];

		if ((per->sub_id == sub_idx) && is_peripheral_connected(per)) {
			forward_hid_out_report(event, per);
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_sent_event(aeh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(aeh);
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

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

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

	if (is_ble_discovery_complete_event(aeh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(aeh);

		register_peripheral(event->dm, event->hwid,
				    sizeof(event->hwid));

		return false;
	}

	if (is_hid_report_subscription_event(aeh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(aeh);

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

		enum bt_hids_pm prev_pm = get_sub_protocol_mode(sub);

		if (event->enabled) {
			enable_subscription(sub, event->report_id);
			send_enqueued_report(sub);
		} else {
			disable_subscription(sub, event->report_id);
		}

		if (prev_pm != get_sub_protocol_mode(sub)) {
			update_sub_protocol_mode(sub, get_sub_protocol_mode(sub));
		}

		return false;
	}

	if (is_hid_report_event(aeh)) {
		return handle_hid_report_event(cast_hid_report_event(aeh));
	}

	if (is_ble_peer_event(aeh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(aeh);

		if (event->state == PEER_STATE_DISCONNECTED) {
			for (size_t i = 0; i < ARRAY_SIZE(peripherals); i++) {
				struct bt_hogp *hogp = &peripherals[i].hogp;

				if (bt_hogp_assign_check(hogp) &&
				    bt_hogp_conn(hogp) == event->id) {
					disconnect_peripheral(&peripherals[i]);
				}
			}
		}

		return false;
	}

	if (is_ble_peer_operation_event(aeh)) {
		const struct ble_peer_operation_event *event =
			cast_ble_peer_operation_event(aeh);

		if (event->op == PEER_OPERATION_ERASED) {
			reset_peripheral_address();
			store_peripheral_address();
		}

		return false;
	}

	if (is_wake_up_event(aeh)) {
		suspended = false;

		return false;
	}

	if (is_power_down_event(aeh)) {
		suspended = true;

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(aeh)) {
			return handle_config_event(cast_config_event(aeh));
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_discovery_complete_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
