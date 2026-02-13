/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(periodic_sync_rsp, LOG_LEVEL_INF);

#define TARGET_NAME "PAwR adv sample"
#define NORDIC_COMPANY_ID 0x0059

/* Manufacturer specific data header length */
#define MSD_HEADER_LEN 4

/* token (4B) + slot (1B) */
#define ACK_ENTRY_SIZE 5

/* offset of slot in ACK entry */
#define ACK_SLOT_OFFSET 4

/* MSD header (4B) + protocol version (1B) + protocl flags (1B), */
/* open_len (1B), ack_count(1B), rt_len(1B) */
#define MIN_PAYLOAD_SIZE (MSD_HEADER_LEN + 5)

#define MAX_INDIVIDUAL_RESPONSE_SIZE 247

/* Variables for token based claim/ack protocol */
#define CLAIM_MSG_ID 0xC1
#define CLAIM_TOKEN_LEN 4
#define CLAIM_TOKEN_OFFSET 1
#define CLAIM_MSG_LEN (CLAIM_TOKEN_LEN + CLAIM_TOKEN_OFFSET)

static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);

/* Join/assignment state */
static uint8_t target_subevent; /* single subevent flow */
static int8_t assigned_slot = -1; /* -1 means unassigned */
static uint32_t my_token; /* 0 means no token yet */
static uint8_t join_backoff_mod = 3; /* attempt every N events initially */
static uint8_t join_backoff_increase; /* linear backoff growth */
static uint8_t open_seen_consecutive; /* for eviction detection */


static struct bt_le_per_adv_sync *sync_handle;
static struct k_work_delayable retry_subevent_sync_work;
static uint8_t subevent_retry_tries;

static bool name_match_cb(struct bt_data *data, void *user_data)
{
	bool *matched = user_data;

	if (data->type == BT_DATA_NAME_COMPLETE ||
		data->type == BT_DATA_NAME_SHORTENED) {
		const char *target = TARGET_NAME;
		size_t target_len = strlen(target);
		/* data->data is not null-terminated; compare lengths and bytes */
		if (data->data_len == target_len &&
			memcmp(data->data, target, target_len) == 0) {
			*matched = true;
			return false; /* stop parsing */
		}
	}
	return true; /* continue parsing */
}

static bool ad_has_target_name(struct net_buf_simple *ad)
{
	bool matched = false;

	bt_data_parse(ad, name_match_cb, &matched);
	return matched;
}

static void attempt_subevent_sync(void)
{
	struct bt_le_per_adv_sync_subevent_params sync_param = {
		.properties = 0,
		.num_subevents = 1,
	};

	uint8_t desired_subevent_id = target_subevent;

	if (!sync_handle) {
		subevent_retry_tries = 0;
		return;
	}

	sync_param.subevents = &desired_subevent_id;

	int err = bt_le_per_adv_sync_subevent(sync_handle, &sync_param);

	if (err) {
		if (++subevent_retry_tries <= 5U) { /* retry up to 5 times */
			(void)k_work_reschedule(&retry_subevent_sync_work, K_MSEC(10));
		} else {
			LOG_ERR("[SYNC] Failed to set subevents after %u tries (err %d)",
					subevent_retry_tries, err);
		}
	} else {
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
	struct bt_le_per_adv_sync_subevent_params params;
	uint8_t subevents[1];
	char le_addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_INF("[SYNC] Synced to %s with %d subevents", le_addr, info->num_subevents);

	sync_handle = sync;
	params.properties = 0;
	params.num_subevents = 1;
	params.subevents = subevents;
	subevents[0] = target_subevent;

	err = bt_le_per_adv_sync_subevent(sync, &params);
	if (err || info->num_subevents == 0) {
		LOG_ERR("[SYNC] Failed to set subevents (err %d)", err);
		subevent_retry_tries = 0;
		(void)k_work_reschedule(&retry_subevent_sync_work, K_NO_WAIT);
	} else {
		LOG_INF("[SYNC] Changed sync to subevent %d", subevents[0]);
		subevent_retry_tries = 0;
		(void)k_work_cancel_delayable(&retry_subevent_sync_work);
	}
	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	sync_handle = NULL;
	(void)k_work_cancel_delayable(&retry_subevent_sync_work);
	subevent_retry_tries = 0;
	k_sem_give(&sem_per_sync_lost);
}


void build_claim_payload(struct net_buf_simple *rsp_buf, uint32_t my_token)
{
	/* Build CLAIM payload: [Claim ID][Claim token (4B) + Claim token offset (1B)] */
	net_buf_simple_reset(rsp_buf);
	net_buf_simple_add_u8(rsp_buf, CLAIM_MSG_ID);
	net_buf_simple_add_le32(rsp_buf, my_token);
}

static void parse_control_fields_after_header(struct net_buf_simple *buf,
	uint8_t *open_len, uint8_t *open_bitmap, uint8_t *ack_count,
	const uint8_t **acks_ptr, uint32_t *retransmit_bitmap32)
{
	uint8_t *data = buf->data;
	uint8_t idx = MSD_HEADER_LEN;
	uint8_t rt_len = 0;

	if (buf->len > idx + 1) {
		idx += 1; /* skip protocol version */
	}
	if (buf->len > idx + 1) {
		idx += 1; /* skip protocol flags */
	}
	if (buf->len > idx) {
		*open_len = data[idx++];
	}
	if (*open_len > 32) {
		*open_len = 32;
	}
	if (buf->len >= idx + *open_len) {
		memcpy(open_bitmap, &data[idx], *open_len);
		idx += *open_len;
	}
	if (buf->len > idx) {
		*ack_count = data[idx++];
	}
	if (buf->len >= idx + *ack_count * ACK_ENTRY_SIZE) {
		*acks_ptr = &data[idx];
		idx += *ack_count * ACK_ENTRY_SIZE;
	}
	if (buf->len > idx) {
		rt_len = data[idx++];
	}
	/* Extract up to 4 bytes of retransmit bitmap for quick checks */
	for (int i = 0; i < rt_len && i < 4; i++) {
		if (buf->len > idx + i) {
			*retransmit_bitmap32 |= ((uint32_t)data[idx + i]) << (8 * i);
		}
	}
}

static bool parse_pawr_control_payload(struct net_buf_simple *buf,
	uint8_t *open_len, uint8_t *open_bitmap, uint8_t *ack_count,
	const uint8_t **acks_ptr, uint32_t *retransmit_bitmap32)
{
	uint8_t *data = buf->data;

	if (data[1] != BT_DATA_MANUFACTURER_DATA || data[2] != NORDIC_COMPANY_ID ||
	    data[3] != 0x00) {
		return false;
	}
	parse_control_fields_after_header(buf, open_len, open_bitmap, ack_count,
					 acks_ptr, retransmit_bitmap32);
	return true;
}

static void check_evicted_from_open_slot(uint8_t open_len, const uint8_t *open_bitmap)
{
	if (assigned_slot >= 0) {
		uint8_t idx_b = (uint8_t)(assigned_slot / 8);
		uint8_t bit_b = (uint8_t)(assigned_slot % 8);
		bool is_open = (idx_b < open_len) && (open_bitmap[idx_b] & (uint8_t)(1U << bit_b));

		if (is_open) {
			if (++open_seen_consecutive >= 2) {
				LOG_INF("[JOIN] Evicted: slot %d marked open; rejoining", assigned_slot);
				assigned_slot = -1; /* drop claim */
				open_seen_consecutive = 0;
				join_backoff_increase = 0; /* optional: speed up re-join */
			}
		} else {
			open_seen_consecutive = 0;
		}
	}
}

static void try_assign_from_acks(const uint8_t *acks_ptr, uint8_t ack_count)
{
	if (assigned_slot < 0 && my_token != 0 && acks_ptr && ack_count > 0) {
		for (uint8_t a = 0; a < ack_count; a++) {
			uint32_t tok = sys_get_le32(&acks_ptr[a * ACK_ENTRY_SIZE]);
			uint8_t slot = acks_ptr[a * ACK_ENTRY_SIZE + ACK_SLOT_OFFSET];
			if (tok == my_token) {
				assigned_slot = slot;
				LOG_INF("[JOIN] Acked: token 0x%08x assigned slot %d",
					my_token, assigned_slot);
				break;
			}
		}
	}
}

static void try_claim_open_slot(struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_recv_info *info,
	uint8_t open_len, const uint8_t *open_bitmap,
	struct net_buf_simple *rsp_buf, struct bt_le_per_adv_response_params *rsp_params)
{
	if (open_len > 0) {
		/* Compute max slots */
		uint16_t max_slots = (uint16_t)(open_len * 8);
		uint16_t bitmap_slots_cap = 32 * 8; /* open_bitmap is 32 bytes */
		if (max_slots > bitmap_slots_cap) {
			max_slots = bitmap_slots_cap;
		}
		/* Collect open indices from bitmap */
		uint16_t open_indices[256];
		uint16_t open_count = 0;
		for (uint16_t s = 0; s < max_slots; s++) {
			uint8_t idx_b = s / 8;
			uint8_t bit_b = s % 8;
			if (open_bitmap[idx_b] & (1U << bit_b)) {
				open_indices[open_count++] = s;
			}
		}
		if (open_count > 0) {
			/* Pick a random open index */
			uint32_t r = sys_rand32_get();
			uint16_t pick = (uint16_t)(r % open_count);

			/* Ensure nonzero token */
			uint8_t try_slot = (uint8_t)open_indices[pick];
			if (my_token == 0) {
				my_token = sys_rand32_get();
				if (my_token == 0) {
					my_token = 1; /* avoid 0 */
				}
			}

			build_claim_payload(rsp_buf, my_token);
			rsp_params->request_event = info->periodic_event_counter;
			rsp_params->request_subevent = info->subevent;
			rsp_params->response_subevent = info->subevent;
			rsp_params->response_slot = try_slot;

			int err2 = bt_le_per_adv_set_response_data(sync, rsp_params, rsp_buf);
			if (err2) {
				LOG_ERR("[JOIN] Failed to send claim (slot %d, err %d)",
					try_slot, err2);
			} else {
				LOG_INF("[JOIN] Sent claim for slot %d with token 0x%08x",
					 try_slot, my_token);
			}

			/* Linear backoff increase to spread retries if not acked */
			if (join_backoff_increase < 10) {
				join_backoff_increase++;
			}
		}
	}
}

static void send_response_if_assigned(struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_recv_info *info,
	uint16_t payload_rsp_size,
	struct net_buf_simple *rsp_buf, struct bt_le_per_adv_response_params *rsp_params)
{
	if (assigned_slot < 0) {
		return;
	}
	net_buf_simple_reset(rsp_buf);
	for (int i = 0; i < payload_rsp_size; i++) {
		net_buf_simple_add_u8(rsp_buf, (uint8_t)(assigned_slot + i));
	}
	rsp_params->request_event = info->periodic_event_counter;
	rsp_params->request_subevent = info->subevent;
	rsp_params->response_subevent = info->subevent;
	rsp_params->response_slot = assigned_slot;
	int err3 = bt_le_per_adv_set_response_data(sync, rsp_params, rsp_buf);
	if (err3) {
		LOG_ERR("[SYNC] Failed to send response (err %d)", err3);
	}
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_recv_info *info,
	struct net_buf_simple *buf)
{
	static uint16_t payload_rsp_size;
	static struct bt_le_per_adv_response_params rsp_params;
	NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, 260);
	/* 1 byte for manufacturer data ID, 2 bytes for company ID */
	const int data_buf_size = 3;

	/* Initialize payload response size if not set */
	if (payload_rsp_size == 0) {
		payload_rsp_size = MAX_INDIVIDUAL_RESPONSE_SIZE - data_buf_size;
	}

	if (buf && buf->len) {
		uint8_t open_len = 0;
		uint8_t open_bitmap[32];
		memset(open_bitmap, 0, sizeof(open_bitmap));
		uint8_t ack_count = 0;
		const uint8_t *acks_ptr = NULL;
		uint32_t retransmit_bitmap32 = 0;

		/* Parsing incoming PAwR control AD payload in buf->data
		 * to extract open-slot bitmap, ACK list and retransmit bitmap */
		if (buf->len >= MIN_PAYLOAD_SIZE) {
			parse_pawr_control_payload(buf, &open_len, open_bitmap, &ack_count, &acks_ptr, &retransmit_bitmap32);
		}

		check_evicted_from_open_slot(open_len, open_bitmap);

		/* If we're unassigned, look for ACK with our token */
		try_assign_from_acks(acks_ptr, ack_count);

		/* If still unassigned, probabilistically attempt a claim to reduce collisions */
		if (assigned_slot < 0) {
			uint32_t ev = info->periodic_event_counter;
			uint8_t mod = join_backoff_mod + join_backoff_increase;
			if (mod == 0) {
				mod = 1; }
			if ((ev % mod) == 0) {
				try_claim_open_slot(sync, info, open_len, open_bitmap, &rsp_buf, &rsp_params);
			}
		}

		send_response_if_assigned(sync, info, payload_rsp_size, &rsp_buf, &rsp_params);
	} else {
		/* No data to process for this subevent */
	}
}


/* Scan callback to detect periodic advertiser and create sync */
static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (sync_handle) {
		return;
	}
	if (!(info->adv_props & BT_GAP_ADV_PROP_EXT_ADV)) {
		return;
	}
	if (info->sid == 0xFF) {
		return;
	}
	if (!ad_has_target_name(ad)) {
		return;
	}

	struct bt_le_per_adv_sync_param param = {0};
	bt_addr_le_copy(&param.addr, info->addr);
	param.sid = info->sid;
	param.skip = 0;
	param.timeout = 1000; /* 10s */

	int err = bt_le_per_adv_sync_create(&param, &sync_handle);
	if (!err) {
		LOG_INF("[SCAN] Creating periodic sync to SID %u", info->sid);
		(void)bt_le_scan_stop();
	}
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
		LOG_ERR("[INIT] Error: Bluetooth init failed (err %d)", err);
		return 0;
	}

	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	bt_le_scan_cb_register(&scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
	if (err) {
		LOG_ERR("[SCAN] Error: start failed (err %d)", err);
		return 0;
	}
	LOG_INF("[SCAN] Waiting for periodic advertiser...");

	/* Wait for sync */
	err = k_sem_take(&sem_per_sync, K_FOREVER);
	if (err) {
		return 0;
	}

	while (true) {
		/* If sync is ever lost, restart scanning */
		(void)k_sem_take(&sem_per_sync_lost, K_FOREVER);
		assigned_slot = -1;
		my_token = 0;
		join_backoff_mod = 3;
		join_backoff_increase = 0;
		open_seen_consecutive = 0;
		sync_handle = NULL;
		(void)bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
	}

	return 0;
}
