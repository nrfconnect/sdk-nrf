/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(periodic_sync_rsp, LOG_LEVEL_INF);

#define TARGET_NAME "PAwR adv sample"
#define NORDIC_COMPANY_ID 0x0059

/* Manufacturer specific data header length */
#define MSD_HEADER_LEN 4

/* PAwR control header: proto version (1B) + proto flags (1B) */
#define PROTO_HEADER_LEN 2U

/* ACK entry on the wire: token (4B) + slot (1B) */
#define ACK_ENTRY_SIZE 5U
#define ACK_SLOT_OFFSET 4U

/* Minimum valid control payload */
/* MSD header (4) + proto header (2) + open_len (1) + ack_count (1) + rt_len (1) */
#define MIN_PAYLOAD_SIZE (MSD_HEADER_LEN + PROTO_HEADER_LEN + 3)

/*
 * Capped at 244 so a single HCI_LE_Periodic_Advertising_Response_Report event
 * fits in the 255-byte HCI event body: 255 - 1 (subevent code) - 4 (report hdr)
 * - 6 (per-response hdr) = 244. Larger payloads get flagged DATA_STATUS_PARTIAL
 * and dropped by the Zephyr host (zephyr/subsys/bluetooth/host/adv.c).
 */
#define MAX_INDIVIDUAL_RESPONSE_SIZE 244U
#define OPEN_BITMAP_MAX_BYTES 32U

/* Claim/ACK protocol wire format */
#define CLAIM_MSG_ID 0xC1

#define JOIN_BACKOFF_INITIAL 3U
#define JOIN_BACKOFF_MAX_INCREASE 10U
#define EVICT_DETECT_THRESHOLD 2U
#define SUBEVENT_SYNC_MAX_RETRIES 5U
#define SUBEVENT_SYNC_RETRY_MS 10U

static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);

/* Join/assignment state */
static int8_t assigned_slot = -1; /* -1 means unassigned */
static uint32_t my_token; /* 0 means no token yet */
static uint8_t join_backoff_mod = JOIN_BACKOFF_INITIAL; /* attempt every N events initially */
static uint8_t join_backoff_increase; /* linear backoff growth */
static uint8_t open_seen_consecutive; /* for eviction detection */

static struct bt_le_per_adv_sync *sync_handle;
static struct k_work_delayable retry_subevent_sync_work;
static uint8_t subevent_retry_tries;

/* Response scratch space */
static struct bt_le_per_adv_response_params rsp_params;
NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, 260);

static bool name_match_cb(struct bt_data *data, void *user_data)
{
	bool *matched = user_data;
	const char *target = TARGET_NAME;
	const size_t target_len = strlen(target);

	if (data->type != BT_DATA_NAME_COMPLETE &&
		data->type != BT_DATA_NAME_SHORTENED) {
		return true;
	}

	if (data->data_len == target_len &&
		memcmp(data->data, target, target_len) == 0) {
		*matched = true;
		return false;
	}

	return true;
}

static bool ad_has_target_name(struct net_buf_simple *ad)
{
	bool matched = false;

	bt_data_parse(ad, name_match_cb, &matched);
	return matched;
}

static void attempt_subevent_sync(void)
{
	/* This sample follows the single configured subevent (index 0). */
	uint8_t subevent_id = 0U;
	struct bt_le_per_adv_sync_subevent_params sync_param = {
		.properties = 0,
		.num_subevents = 1,
		.subevents = &subevent_id,
	};
	int err;

	if (!sync_handle) {
		subevent_retry_tries = 0;
		return;
	}

	err = bt_le_per_adv_sync_subevent(sync_handle, &sync_param);
	if (err) {
		subevent_retry_tries++;
		if (subevent_retry_tries <= SUBEVENT_SYNC_MAX_RETRIES) {
			(void)k_work_reschedule(&retry_subevent_sync_work,
						K_MSEC(SUBEVENT_SYNC_RETRY_MS));
		} else {
			LOG_ERR("[SYNC] Failed to set subevents after %u tries (err %d)",
				subevent_retry_tries, err);
		}
	} else {
		LOG_INF("[SYNC] Changed sync to subevent %u", subevent_id);
		subevent_retry_tries = 0;
		(void)k_work_cancel_delayable(&retry_subevent_sync_work);
	}
}

static void set_subevent_retry(struct k_work *work)
{
	ARG_UNUSED(work);
	attempt_subevent_sync();
}

static void sync_cb(struct bt_le_per_adv_sync *sync,
		struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_INF("[SYNC] Synced to %s with %d subevents", le_addr, info->num_subevents);

	sync_handle = sync;
	subevent_retry_tries = 0;
	attempt_subevent_sync();
	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	ARG_UNUSED(sync);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_INF("[SYNC] Sync lost from %s (reason %u)", le_addr, info->reason);

	sync_handle = NULL;
	(void)k_work_cancel_delayable(&retry_subevent_sync_work);
	subevent_retry_tries = 0;
	k_sem_give(&sem_per_sync_lost);
}

static void build_claim_payload(uint32_t token)
{
	/* CLAIM payload: [CLAIM_MSG_ID (1B)][token (4B LE)] */
	net_buf_simple_reset(&rsp_buf);
	net_buf_simple_add_u8(&rsp_buf, CLAIM_MSG_ID);
	net_buf_simple_add_le32(&rsp_buf, token);
}

struct pawr_ctrl {
	uint8_t open_len;
	uint8_t ack_count;
	const uint8_t *open_bitmap; /* points into buf->data, not owned */
	const uint8_t *acks; /* points into buf->data, not owned */
};

static bool parse_control_frame(const struct net_buf_simple *buf, struct pawr_ctrl *out)
{
	const uint8_t *data;
	uint16_t idx;
	uint8_t rt_len;

	memset(out, 0, sizeof(*out));

	if (buf->len < MIN_PAYLOAD_SIZE) {
		return false;
	}
	if (buf->data[1] != BT_DATA_MANUFACTURER_DATA) {
		return false;
	}
	if (sys_get_le16(&buf->data[2]) != NORDIC_COMPANY_ID) {
		return false;
	}

	data = buf->data;
	idx = MSD_HEADER_LEN + PROTO_HEADER_LEN;

	if (buf->len < (uint16_t)(idx + 1U)) {
		return false;
	}
	out->open_len = data[idx++];
	if (out->open_len > OPEN_BITMAP_MAX_BYTES) {
		out->open_len = OPEN_BITMAP_MAX_BYTES;
	}
	if (buf->len < (uint16_t)(idx + out->open_len)) {
		return false;
	}
	out->open_bitmap = &data[idx];
	idx += out->open_len;

	if (buf->len < (uint16_t)(idx + 1U)) {
		return false;
	}
	out->ack_count = data[idx++];
	if (buf->len < (uint16_t)(idx + (uint16_t)out->ack_count * ACK_ENTRY_SIZE)) {
		return false;
	}
	out->acks = &data[idx];
	idx += (uint16_t)out->ack_count * ACK_ENTRY_SIZE;

	if (buf->len < (uint16_t)(idx + 1U)) {
		return false;
	}
	/*
	 * Retransmit bitmap: only validate the length field is well-formed.
	 * The sample does not implement retransmission (see README), so the
	 * bitmap contents themselves are ignored.
	 */
	rt_len = data[idx++];

	if (buf->len < (uint16_t)(idx + rt_len)) {
		return false;
	}

	return true;
}

static void check_evicted_from_open_slot(uint8_t open_len, const uint8_t *open_bitmap)
{
	uint8_t idx_b;
	uint8_t bit_b;
	bool is_open;

	if (assigned_slot < 0) {
		return;
	}

	idx_b = (uint8_t)(assigned_slot / 8);
	bit_b = (uint8_t)(assigned_slot % 8);
	is_open = (idx_b < open_len) && (open_bitmap[idx_b] & (uint8_t)(1U << bit_b));

	if (!is_open) {
		open_seen_consecutive = 0;
		return;
	}

	open_seen_consecutive++;
	if (open_seen_consecutive >= EVICT_DETECT_THRESHOLD) {
		LOG_INF("[JOIN] Evicted: slot %d marked open; rejoining", assigned_slot);
		assigned_slot = -1;
		open_seen_consecutive = 0;
		join_backoff_increase = 0;
	}
}

static void try_assign_from_acks(const uint8_t *acks_ptr, uint8_t ack_count)
{
	uint32_t tok;
	uint8_t slot;

	if (assigned_slot >= 0 || my_token == 0U || !acks_ptr || ack_count == 0U) {
		return;
	}

	for (uint8_t a = 0; a < ack_count; a++) {
		tok = sys_get_le32(&acks_ptr[a * ACK_ENTRY_SIZE]);
		slot = acks_ptr[a * ACK_ENTRY_SIZE + ACK_SLOT_OFFSET];

		if (tok == my_token) {
			assigned_slot = (int8_t)slot;
			LOG_INF("[JOIN] Acked: token 0x%08x assigned slot %d",
				my_token, assigned_slot);
			return;
		}
	}
}

static uint16_t bitmap_popcount(const uint8_t *bitmap, uint8_t len)
{
	uint16_t n = 0;

	for (uint8_t i = 0; i < len; i++) {
		n += (uint16_t)__builtin_popcount(bitmap[i]);
	}
	return n;
}

static int bitmap_nth_set_bit(const uint8_t *bitmap, uint8_t len, uint16_t n)
{
	uint16_t seen = 0;

	for (uint16_t s = 0; s < (uint16_t)len * 8U; s++) {
		if (bitmap[s / 8U] & (uint8_t)(1U << (s % 8U))) {
			if (seen == n) {
				return (int)s;
			}
			seen++;
		}
	}
	return -1;
}

static void try_claim_open_slot(struct bt_le_per_adv_sync *sync,
		const struct bt_le_per_adv_sync_recv_info *info,
		uint8_t open_len, const uint8_t *open_bitmap)
{
	uint16_t open_count;
	uint16_t pick;
	int slot;
	int err;

	if (open_len == 0U) {
		return;
	}

	open_count = bitmap_popcount(open_bitmap, open_len);
	if (open_count == 0U) {
		return;
	}

	pick = (uint16_t)(sys_rand32_get() % open_count);
	slot = bitmap_nth_set_bit(open_bitmap, open_len, pick);
	if (slot < 0) {
		return;
	}

	if (my_token == 0U) {
		my_token = sys_rand32_get();
		if (my_token == 0U) {
			my_token = 1U;
		}
	}

	build_claim_payload(my_token);
	rsp_params.request_event = info->periodic_event_counter;
	rsp_params.request_subevent = info->subevent;
	rsp_params.response_subevent = info->subevent;
	rsp_params.response_slot = (uint8_t)slot;

	err = bt_le_per_adv_set_response_data(sync, &rsp_params, &rsp_buf);
	if (err) {
		LOG_ERR("[JOIN] Failed to send claim (slot %d, err %d)", slot, err);
	} else {
		LOG_INF("[JOIN] Sent claim for slot %d with token 0x%08x", slot, my_token);
	}

	if (join_backoff_increase < JOIN_BACKOFF_MAX_INCREASE) {
		join_backoff_increase++;
	}
}

static void send_response_if_assigned(struct bt_le_per_adv_sync *sync,
		const struct bt_le_per_adv_sync_recv_info *info,
		uint16_t payload_rsp_size)
{
	int err;

	if (assigned_slot < 0) {
		return;
	}

	net_buf_simple_reset(&rsp_buf);

	if (payload_rsp_size > net_buf_simple_tailroom(&rsp_buf)) {
		payload_rsp_size = (uint16_t)net_buf_simple_tailroom(&rsp_buf);
	}

	for (uint16_t i = 0; i < payload_rsp_size; i++) {
		net_buf_simple_add_u8(&rsp_buf, (uint8_t)(assigned_slot + i));
	}

	rsp_params.request_event = info->periodic_event_counter;
	rsp_params.request_subevent = info->subevent;
	rsp_params.response_subevent = info->subevent;
	rsp_params.response_slot = (uint8_t)assigned_slot;

	err = bt_le_per_adv_set_response_data(sync, &rsp_params, &rsp_buf);
	if (err) {
		LOG_ERR("[SYNC] Failed to send response (err %d)", err);
	}
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		const struct bt_le_per_adv_sync_recv_info *info,
		struct net_buf_simple *buf)
{
	struct pawr_ctrl ctrl;
	uint8_t mod;

	if (!buf || buf->len == 0U) {
		return;
	}

	if (!parse_control_frame(buf, &ctrl)) {
		return;
	}

	check_evicted_from_open_slot(ctrl.open_len, ctrl.open_bitmap);
	try_assign_from_acks(ctrl.acks, ctrl.ack_count);

	if (assigned_slot < 0) {
		mod = join_backoff_mod + join_backoff_increase;
		if (mod == 0U) {
			mod = 1U;
		}
		if ((info->periodic_event_counter % mod) == 0U) {
			try_claim_open_slot(sync, info, ctrl.open_len, ctrl.open_bitmap);
		}
	}

	send_response_if_assigned(sync, info, MAX_INDIVIDUAL_RESPONSE_SIZE);
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	struct bt_le_per_adv_sync_param param = {
		.sid = info->sid,
		.skip = 0,
		.timeout = 1000, /* 10 s, in units of 10 ms */
	};
	int err;

	if (sync_handle) {
		return;
	}
	if (!(info->adv_props & BT_GAP_ADV_PROP_EXT_ADV)) {
		return;
	}
	if (info->sid == BT_GAP_SID_INVALID) {
		return;
	}
	if (!ad_has_target_name(ad)) {
		return;
	}

	bt_addr_le_copy(&param.addr, info->addr);

	err = bt_le_per_adv_sync_create(&param, &sync_handle);
	if (err) {
		LOG_ERR("[SCAN] bt_le_per_adv_sync_create failed (err %d)", err);
		return;
	}

	LOG_INF("[SCAN] Creating periodic sync to SID %u", info->sid);

	err = bt_le_scan_stop();
	if (err) {
		LOG_WRN("[SCAN] bt_le_scan_stop failed (err %d)", err);
	}
}

static void reset_join_state(void)
{
	assigned_slot = -1;
	my_token = 0U;
	join_backoff_mod = JOIN_BACKOFF_INITIAL;
	join_backoff_increase = 0U;
	open_seen_consecutive = 0U;
	subevent_retry_tries = 0U;
}

int main(void)
{
	int err;
	static struct bt_le_scan_cb scan_cb = { .recv = scan_recv_cb };
	static struct bt_le_per_adv_sync_cb sync_callbacks = {
		.synced = sync_cb,
		.term = term_cb,
		.recv = recv_cb
	};

	LOG_INF("Starting PAwR Synchronizer");

	k_work_init_delayable(&retry_subevent_sync_work, set_subevent_retry);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("[INIT] Bluetooth init failed (err %d)", err);
		return err;
	}

	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	bt_le_scan_cb_register(&scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
	if (err) {
		LOG_ERR("[SCAN] start failed (err %d)", err);
		return err;
	}
	LOG_INF("[SCAN] Waiting for periodic advertiser...");

	(void)k_sem_take(&sem_per_sync, K_FOREVER);

	while (true) {
		(void)k_sem_take(&sem_per_sync_lost, K_FOREVER);

		LOG_INF("[SYNC] Sync lost; restarting scan");
		reset_join_state();
		sync_handle = NULL;

		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
		if (err && err != -EALREADY) {
			LOG_ERR("[SCAN] restart failed (err %d)", err);
			k_sleep(K_MSEC(100));
		}
	}

	return 0;
}
