/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(periodic_adv_rsp, LOG_LEVEL_INF);

/*
 * PAwR Throughput Demo - Advertiser
 *
 * The advertiser emits one subevent per periodic event. Each subevent carries
 * a small manufacturer-specific AD with control information:
 *
 *   [len][0xFF][company_id_le16][proto_ver][proto_flags]
 *   [open_len][open_bitmap...][ack_count][ack_entries...][rt_len][rt_bitmap...]
 *
 * Synchronizers listening to the subevent use the open-slot bitmap to pick a
 * free response slot, send a CLAIM in that slot, and are then acknowledged via
 * the ACK list in a later event. The advertiser tracks which assigned slots
 * actually responded and produces a retransmit hint bitmap.
 */

/* Sample configuration */
#define NUM_RSP_SLOTS CONFIG_APP_NUM_RSP_DEVICES
#define NUM_SUBEVENTS 1
#define MAX_BT_DATA_LENGTH CONFIG_BT_CTLR_DATA_LENGTH_MAX
/*
 * Per-response data size used by the sync (see periodic_sync_rsp/src/main.c).
 * Capped at 244 so a single HCI_LE_Periodic_Advertising_Response_Report event
 * fits in the 255-byte HCI event body: 255 - 1 (subevent code) - 4 (report hdr)
 * - 6 (per-response hdr) = 244. Larger payloads get flagged DATA_STATUS_PARTIAL
 * and dropped by the Zephyr host (zephyr/subsys/bluetooth/host/adv.c).
 */
#define RESPONSE_PAYLOAD_BYTES 244U
#define ADV_NAME "PAwR adv sample"

/* Wire protocol*/
#define NORDIC_COMPANY_ID 0x0059
#define PAWR_PROTO_VERSION 0x01U
#define PAWR_PROTO_FLAGS_NONE 0x00U
#define PAWR_CLAIM_MSG_ID 0xC1
#define CLAIM_MSG_LEN 5U /* id(1) + token(4) */
#define ACK_ENTRY_SIZE 5U /* token(4) + slot(1) */
#define OPEN_BITMAP_MAX_BYTES 32U
#define RT_BITMAP_MAX_BYTES 4U

/* Timing */
#define PAWR_UNIT_US 1250U /* 1.25 ms: PA interval, subevent interval, slot delay */
#define SLOT_SPACING_UNIT_US 125U /* 0.125 ms: response_slot_spacing unit per BT spec */
#define ADV_GUARD_US 1000U
#define SLOT_GUARD_US 250U
#define PHY_2M_BITS_PER_US 2U

/* Sample limits */
#define MAX_ACKS_PER_EVENT 32
#define SLOT_RECLAIM_MISSES 20
#define THROUGHPUT_PRINT_INTERVAL 1000U

static uint32_t slot_time_us(uint16_t payload_bytes)
{
	return (payload_bytes * 8U) / PHY_2M_BITS_PER_US + SLOT_GUARD_US;
}

static uint16_t us_to_pawr_units(uint32_t us)
{
	return (uint16_t)DIV_ROUND_UP(us, PAWR_UNIT_US);
}

static uint16_t us_to_slot_spacing_units(uint32_t us)
{
	return (uint16_t)DIV_ROUND_UP(us, SLOT_SPACING_UNIT_US);
}

static void log_pawr_config(const struct bt_le_per_adv_param *p, uint16_t payload_size,
		uint32_t slot_us, uint32_t subevent_us)
{
	const uint32_t adv_us = (uint32_t)p->interval_min * PAWR_UNIT_US;
	uint32_t theo_kbps = 0U;

	if (adv_us > 0U) {
		theo_kbps = (uint32_t)(((uint64_t)p->num_response_slots * payload_size *
					8ULL * 1000000ULL) / adv_us / 1000ULL);
	}

	LOG_INF("PAwR config (1 subevent):");
	LOG_INF("Devices: %u, Resp size: %u B, Slot: %u us",
		p->num_response_slots, payload_size, slot_us);
	LOG_INF("Delay: %u (%u us), SubEvt: %u (%u us), AdvInt: %u (%u us)",
		p->response_slot_delay, p->response_slot_delay * PAWR_UNIT_US,
		p->subevent_interval, subevent_us,
		p->interval_min, adv_us);
	LOG_INF("Est. uplink throughput (theoretical) ~%u kbps", theo_kbps);
}

static void set_pawr_params(struct bt_le_per_adv_param *params, uint8_t num_response_slots)
{
	const uint16_t payload_size = RESPONSE_PAYLOAD_BYTES;
	const uint8_t response_slot_delay_units = 3U; /* 3 * 1.25 ms = 3.75 ms */
	const uint32_t slot_us = slot_time_us(payload_size);
	const uint32_t subevent_us =
		(uint32_t)response_slot_delay_units * PAWR_UNIT_US +
		(uint32_t)num_response_slots * slot_us;
	const uint32_t event_us = subevent_us + ADV_GUARD_US;

	const uint16_t slot_spacing_units = us_to_slot_spacing_units(slot_us);
	const uint32_t subevent_raw_units = us_to_pawr_units(subevent_us);
	uint16_t subevent_interval_units;
	uint16_t periodic_interval_units;

	if (subevent_raw_units > 255U) {
		LOG_WRN("Subevent too long for one window; clamping.");
	}
	subevent_interval_units = (uint16_t)CLAMP(subevent_raw_units, 6U, 255U);
	periodic_interval_units = MAX(us_to_pawr_units(event_us), subevent_interval_units);

	params->interval_min = periodic_interval_units;
	params->interval_max = periodic_interval_units;
	params->options = 0;
	params->num_subevents = NUM_SUBEVENTS;
	params->subevent_interval = (uint8_t)subevent_interval_units;
	params->response_slot_delay = response_slot_delay_units;
	params->response_slot_spacing = (uint8_t)slot_spacing_units;
	params->num_response_slots = num_response_slots;

	log_pawr_config(params, payload_size, slot_us, subevent_us);
}

static struct bt_le_per_adv_param per_adv_params;

static struct bt_le_per_adv_subevent_data_params subevent_data_params[NUM_SUBEVENTS];
static struct net_buf_simple bufs[NUM_SUBEVENTS];
static uint8_t backing_store[NUM_SUBEVENTS][MAX_BT_DATA_LENGTH];

BUILD_ASSERT(ARRAY_SIZE(bufs) == ARRAY_SIZE(subevent_data_params));
BUILD_ASSERT(ARRAY_SIZE(backing_store) == ARRAY_SIZE(subevent_data_params));

#define BITMAP_BYTES_NEEDED(num_devices) (((num_devices) + 7) / 8)
#define RSP_BITMAP_BYTES BITMAP_BYTES_NEEDED(NUM_RSP_SLOTS)

static uint8_t response_bitmap[RSP_BITMAP_BYTES];
static uint8_t expected_responses[RSP_BITMAP_BYTES];

/* Slot assignment state and ACK queue for claim/ack protocol */
struct slot_assignment_state {
	bool assigned;
	uint32_t token; /* Token that owns this slot, 0 if unassigned */
};

static struct slot_assignment_state slot_state[NUM_RSP_SLOTS];

struct ack_entry {
	uint32_t token;
	uint8_t slot;
};

static struct ack_entry ack_queue[MAX_ACKS_PER_EVENT];
static uint8_t ack_queue_count;
static uint8_t noresp_counters[NUM_RSP_SLOTS];

/* Append [open_len][open_bitmap...] to buf. Caps bitmap at OPEN_BITMAP_MAX_BYTES. */
static void append_open_slots_bitmap(struct net_buf_simple *buf)
{
	const uint8_t open_len = MIN(RSP_BITMAP_BYTES, OPEN_BITMAP_MAX_BYTES);
	uint8_t *len_field;
	uint8_t *bitmap_field;

	len_field = net_buf_simple_add(buf, 1);
	bitmap_field = net_buf_simple_add(buf, open_len);

	*len_field = open_len;
	memset(bitmap_field, 0, open_len);

	for (uint8_t i = 0; i < NUM_RSP_SLOTS; i++) {
		if (!slot_state[i].assigned && (i / 8U) < open_len) {
			bitmap_field[i / 8U] |= (uint8_t)(1U << (i % 8U));
		}
	}
}

/* Linear search for slot by token owner */
static int find_slot_by_token(uint32_t token)
{
	if (token == 0) {
		return -1;
	}
	for (int i = 0; i < NUM_RSP_SLOTS; i++) {
		if (slot_state[i].assigned && slot_state[i].token == token) {
			return i;
		}
	}
	return -1;
}

/* Append an ACK (token, slot) if queue has room */
static void queue_ack(uint32_t token, uint8_t slot)
{
	if (ack_queue_count < MAX_ACKS_PER_EVENT) {
		ack_queue[ack_queue_count].token = token;
		ack_queue[ack_queue_count].slot = slot;
		ack_queue_count++;
	}
}

/* Set bit in a byte-addressed bitmap */
static void bitmap_set_bit(uint8_t *bitmap, int bit)
{
	bitmap[bit / 8] |= (1 << (bit % 8));
}

static bool bitmap_test_bit(const uint8_t *bitmap, int bit)
{
	return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

static void bitmap_clear_bit(uint8_t *bitmap, int bit)
{
	bitmap[bit / 8] &= (uint8_t)~(1U << (bit % 8));
}

static void reclaim_idle_slots(void)
{
	for (int slot = 0; slot < NUM_RSP_SLOTS; slot++) {
		bool was_expected = bitmap_test_bit(expected_responses, slot);
		bool got_response = bitmap_test_bit(response_bitmap, slot);

		if (was_expected && !got_response) {
			if (noresp_counters[slot] < UINT8_MAX) {
				noresp_counters[slot]++;
			}
			if (noresp_counters[slot] >= SLOT_RECLAIM_MISSES) {
				/* Consider this slot idle; free it for new claimers */
				slot_state[slot].assigned = false;
				slot_state[slot].token = 0;
				bitmap_clear_bit(expected_responses, slot);
				noresp_counters[slot] = 0;
				LOG_INF("[CLAIM] Reclaimed idle slot %d", slot);
			}
		} else if (got_response) {
			noresp_counters[slot] = 0;
		} else {
			/* not expected and no response: ignore */
		}
	}
}

static void append_acks(struct net_buf_simple *buf, uint8_t *ack_count_ptr)
{
	const size_t reserved_for_rt = 1U + MIN(RSP_BITMAP_BYTES, RT_BITMAP_MAX_BYTES);
	const size_t tail = net_buf_simple_tailroom(buf);
	const size_t ack_space = (tail > reserved_for_rt) ? (tail - reserved_for_rt) : 0;
	uint8_t ack_capacity = (uint8_t)(ack_space / ACK_ENTRY_SIZE);
	uint8_t acks_to_send;

	if (ack_capacity > MAX_ACKS_PER_EVENT) {
		ack_capacity = MAX_ACKS_PER_EVENT;
	}
	acks_to_send = MIN(ack_queue_count, ack_capacity);

	for (uint8_t a = 0; a < acks_to_send; a++) {
		net_buf_simple_add_le32(buf, ack_queue[a].token);
		net_buf_simple_add_u8(buf, ack_queue[a].slot);
	}
	*ack_count_ptr = acks_to_send;
}

static void append_retransmit_bitmap(struct net_buf_simple *buf, const uint8_t *retransmit_bitmap)
{
	const uint8_t rt_bytes_to_send = MIN(RSP_BITMAP_BYTES, RT_BITMAP_MAX_BYTES);

	net_buf_simple_add_u8(buf, rt_bytes_to_send);
	net_buf_simple_add_mem(buf, retransmit_bitmap, rt_bytes_to_send);
}

static void build_control_frame(struct net_buf_simple *buf, const uint8_t *retransmit_bitmap)
{
	uint8_t *length_field;
	uint8_t *ack_count_ptr;

	net_buf_simple_reset(buf);

	length_field = net_buf_simple_add(buf, 1);
	net_buf_simple_add_u8(buf, BT_DATA_MANUFACTURER_DATA);
	net_buf_simple_add_le16(buf, NORDIC_COMPANY_ID);
	net_buf_simple_add_u8(buf, PAWR_PROTO_VERSION);
	net_buf_simple_add_u8(buf, PAWR_PROTO_FLAGS_NONE);

	append_open_slots_bitmap(buf);

	ack_count_ptr = net_buf_simple_add(buf, 1);
	append_acks(buf, ack_count_ptr);

	append_retransmit_bitmap(buf, retransmit_bitmap);

	*length_field = (uint8_t)(buf->len - 1U);
}

static void request_cb(struct bt_le_ext_adv *adv, const struct bt_le_per_adv_data_request *request)
{
	uint8_t retransmit_bitmap[RSP_BITMAP_BYTES];
	uint8_t to_send;
	int err;

	if (!request) {
		LOG_ERR("NULL request received");
		return;
	}

	to_send = (uint8_t)MIN(request->count, ARRAY_SIZE(subevent_data_params));

	for (uint8_t i = 0; i < RSP_BITMAP_BYTES; i++) {
		retransmit_bitmap[i] = expected_responses[i] & (uint8_t)~response_bitmap[i];
	}

	for (uint8_t s = 0; s < NUM_RSP_SLOTS; s++) {
		if (retransmit_bitmap[s / 8U] & (uint8_t)(1U << (s % 8U))) {
			LOG_DBG("[LOSS] slot %u missed response", s);
		}
	}

	reclaim_idle_slots();
	memset(response_bitmap, 0, sizeof(response_bitmap));

	for (uint8_t i = 0; i < to_send; i++) {
		build_control_frame(&bufs[i], retransmit_bitmap);
		subevent_data_params[i].subevent = request->start + i;
		subevent_data_params[i].response_slot_start = 0;
		subevent_data_params[i].response_slot_count = NUM_RSP_SLOTS;
		subevent_data_params[i].data = &bufs[i];
	}

	ack_queue_count = 0;

	err = bt_le_per_adv_set_subevent_data(adv, to_send, subevent_data_params);
	if (err) {
		LOG_ERR("[ADV] Failed to set subevent data (err %d)", err);
	}
}

static void handle_claim_message(const struct net_buf_simple *buf,
		const struct bt_le_per_adv_response_info *info)
{
	/* CLAIM message: [0xC1][token(4B LE)] */
	uint32_t token = sys_get_le32(&buf->data[1]);

	if (token == 0U) {
		return;
	}

	if (info->response_slot < NUM_RSP_SLOTS) {
		int already = find_slot_by_token(token);

		if (already >= 0) {
			/* This token already owns a slot: only ACK its true slot */
			if (already == info->response_slot) {
				queue_ack(token, (uint8_t)already);
			}
		} else if (!slot_state[info->response_slot].assigned) {
			/* Assign the slot to this token and queue ACK */
			slot_state[info->response_slot].assigned = true;
			slot_state[info->response_slot].token = token;
			bitmap_set_bit(expected_responses, info->response_slot);
			queue_ack(token, info->response_slot);
			LOG_INF("[CLAIM] accepted: slot %d token 0x%08x",
					info->response_slot, token);
		} else {
			/* Slot already assigned to a different token: ignore */
		}
	}
}

static void print_throughput(int64_t stamp, uint32_t total_bytes)
{
	const uint32_t adv_us = (uint32_t)per_adv_params.interval_min * PAWR_UNIT_US;
	const uint16_t num_rsp_slots = per_adv_params.num_response_slots;
	const uint16_t payload_bytes = RESPONSE_PAYLOAD_BYTES;
	const int64_t delta = k_uptime_delta(&stamp);
	const uint64_t measured_kbps =
		(delta > 0) ? ((uint64_t)total_bytes * 8ULL) / (uint64_t)delta : 0ULL;
	uint32_t theo_kbps = 0U;
	uint32_t efficiency_pct = 0U;

	if (adv_us > 0U && num_rsp_slots > 0U) {
		theo_kbps = (uint32_t)(((uint64_t)num_rsp_slots * payload_bytes * 8ULL *
			1000ULL) / adv_us);
	}
	if (theo_kbps > 0U) {
		efficiency_pct = (uint32_t)((measured_kbps * 100ULL) / theo_kbps);
	}

	LOG_INF("[ADV] received %u bytes (%u KB) in %lld ms at %llu kbps",
		total_bytes, total_bytes / 1024, delta, measured_kbps);
	LOG_INF("[PAwR] theoretical ~%u kbps; efficiency ~%u%% (N=%u, payload=%u, AdvInt=%u us)",
		theo_kbps, efficiency_pct, num_rsp_slots, payload_bytes, adv_us);
}

static void response_cb(struct bt_le_ext_adv *adv,
		struct bt_le_per_adv_response_info *info,
		struct net_buf_simple *buf)
{
	static uint32_t total_bytes;
	static int64_t stamp;

	if (buf) {
		/* Initialize timestamp on first response */
		if (total_bytes == 0) {
			stamp = k_uptime_get();
		}

		/* Handle claim messages or data responses */
		if (buf->len >= CLAIM_MSG_LEN && buf->data[0] == PAWR_CLAIM_MSG_ID) {
			handle_claim_message(buf, info);
		} else {
			/* Count payload bytes as throughput */
			total_bytes += buf->len;
		}

		if (info->response_slot < NUM_RSP_SLOTS) {
			bitmap_set_bit(response_bitmap, info->response_slot);
		}

		if (k_uptime_get() - stamp > THROUGHPUT_PRINT_INTERVAL) {
			print_throughput(stamp, total_bytes);
			total_bytes = 0;
		}
	}
}

static void init_bufs(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(backing_store); i++) {
		/* Initialize buffer with backing storage */
		net_buf_simple_init_with_data(&bufs[i],
				&backing_store[i], ARRAY_SIZE(backing_store[i]));
		net_buf_simple_reset(&bufs[i]);
	}
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, ADV_NAME, sizeof(ADV_NAME) - 1),
};

static const struct bt_le_ext_adv_cb adv_cb = {
	.pawr_data_request = request_cb,
	.pawr_response = response_cb,
};

int main(void)
{
	int err;
	struct bt_le_ext_adv *pawr_adv;

	init_bufs();

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}
	LOG_INF("Bluetooth initialized");

	set_pawr_params(&per_adv_params, NUM_RSP_SLOTS);

	LOG_INF("Starting PAwR Advertiser");
	LOG_INF("===========================================");

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &pawr_adv);
	if (err) {
		LOG_ERR("Failed to create advertising set (err %d)", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(pawr_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set ext adv data (err %d)", err);
		return err;
	}

	err = bt_le_per_adv_set_param(pawr_adv, &per_adv_params);
	if (err) {
		LOG_ERR("Failed to set periodic advertising parameters (err %d)", err);
		return err;
	}

	LOG_INF("Start Extended Advertising");
	err = bt_le_ext_adv_start(pawr_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_ERR("Failed to start extended advertising (err %d)", err);
		return err;
	}

	LOG_INF("Start Periodic Advertising");
	err = bt_le_per_adv_start(pawr_adv);
	if (err) {
		LOG_ERR("Failed to enable periodic advertising (err %d)", err);
		return err;
	}

	return 0;
}
