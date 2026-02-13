/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(periodic_adv_rsp, LOG_LEVEL_INF);

/*
 * PAwR Throughput Demo
 * Advertiser sends subevent data with control information.
 * Devices claim response slots using PAwR responses.
 * Retransmissions are bitmap-controlled.
 */


#define NUM_RSP_SLOTS CONFIG_APP_NUM_RSP_DEVICES
#define NUM_SUBEVENTS 1
#define MAX_BT_DATA_LENGTH CONFIG_BT_CTLR_DATA_LENGTH_MAX
#define ADV_NAME "PAwR adv sample"
#define NORDIC_COMPANY_ID 0x0059

#define MAX_INDIVIDUAL_RESPONSE_SIZE CONFIG_BT_CTLR_SDC_PERIODIC_ADV_RSP_TX_MAX_DATA_SIZE
#define THROUGHPUT_PRINT_INTERVAL 1000
#define PAWR_CLAIM_MSG_ID 0xC1
/* Claim token length (4B) + Claim token offset (1B) */
#define CLAIM_MSG_LEN 5
#define PAWR_INTERVAL_UNIT_MS_X100 125U
#define PHY_2M 2U


void set_pawr_params(struct bt_le_per_adv_param *params, uint8_t num_response_slots)
{
	const uint16_t min_pawr_interval_ms = 1;
	const uint16_t payload_size = MAX_INDIVIDUAL_RESPONSE_SIZE;
	const uint32_t advertiser_guard_ms_x100 = 100U; /* 1.00 ms -> 100 */
	const uint32_t quarter_ms_x100 = 25U; /* 0.25 ms -> 25 */
	const uint8_t response_slot_delay_units = 3U; /* 3 * 1.25 ms = 3.75 ms */
	const uint32_t response_slot_delay_ms_x100 =
		(uint32_t)response_slot_delay_units * PAWR_INTERVAL_UNIT_MS_X100;

	/* Approximate on-air time + radio turnarounds at 2M PHY */
	/* payload_size * 8 bits = total bits to transmit */
	const uint32_t payload_bits = (uint32_t)payload_size * 8U;
	const uint32_t tx_denom_ms = (uint32_t)PHY_2M * 10U; /* 2M_PHY * 10 = bits per ms */
	const uint32_t tx_time_ms_x100 = (payload_bits + tx_denom_ms - 1U) / tx_denom_ms;
	const uint32_t slot_ms_x100 = tx_time_ms_x100 + quarter_ms_x100;
	const uint32_t slot_units = DIV_ROUND_UP(slot_ms_x100 * 10U, PAWR_INTERVAL_UNIT_MS_X100);

	const uint16_t slot_spacing_units = (uint16_t)slot_units;

	const uint32_t subevent_ms_x100 =
		response_slot_delay_ms_x100 + ((uint32_t)num_response_slots * slot_ms_x100);

	uint32_t subevent_interval_units_tmp =
		DIV_ROUND_UP(subevent_ms_x100, PAWR_INTERVAL_UNIT_MS_X100);
	uint16_t subevent_interval_units = (uint16_t)(subevent_interval_units_tmp);

	if (subevent_interval_units < 6U) {
		subevent_interval_units = 6U; /* Minimum subevent interval (6*1.25 = 7.5 ms) */
	} else if (subevent_interval_units > 255U) { /* remaining cases ignored */
		subevent_interval_units = 255U; /* Maximum subevent interval (255*1.25 ms) */
		LOG_WRN("Warning: subevent too long for one window.");
	} else {
		/* in range: no change */
	}

	const uint32_t total_event_ms_x100 = subevent_ms_x100 + advertiser_guard_ms_x100;
	const uint32_t min_interval_units =
		(uint32_t)(((uint32_t)min_pawr_interval_ms * 100U +
			PAWR_INTERVAL_UNIT_MS_X100 - 1U) / PAWR_INTERVAL_UNIT_MS_X100);

	const uint32_t required_interval_units =
		(total_event_ms_x100 + PAWR_INTERVAL_UNIT_MS_X100 - 1U) / 
			PAWR_INTERVAL_UNIT_MS_X100;

	uint16_t advertising_event_interval_units =
		(required_interval_units > min_interval_units) ?
		(uint16_t)required_interval_units : (uint16_t)min_interval_units;

	if (advertising_event_interval_units < subevent_interval_units) {
		advertising_event_interval_units = subevent_interval_units;
	}

	/* fill params */
	params->interval_min = advertising_event_interval_units; /* Minimum advertising interval */
	params->interval_max = advertising_event_interval_units;
	params->options = 0; /* No options */
	params->num_subevents = 1; /* One subevent */
	params->subevent_interval = (uint8_t)subevent_interval_units;
	params->response_slot_delay = (uint8_t)response_slot_delay_units;
	params->response_slot_spacing = (uint8_t)slot_spacing_units;
	params->num_response_slots = num_response_slots;

	/* Diagnostics: */
	const uint32_t adv_event_ms_x100 = (uint32_t)advertising_event_interval_units * 125U;
	const uint32_t est_throughput_kbps = (adv_event_ms_x100 > 0U) ?
											(uint32_t)(((uint64_t)num_response_slots *
											payload_size * 8ULL * 100000ULL) /
											(uint64_t)adv_event_ms_x100) / 1000U : 0U;

	LOG_INF("PAwR config (1 subevent):");
	LOG_INF("Devices: %u, Resp size: %u B, Slot: %u.%02u ms",
			num_response_slots, payload_size,
			slot_ms_x100 / 100U, slot_ms_x100 % 100U);
	LOG_INF("Delay: %u (%u.%02u ms), SubEvt: %u (%u.%02u ms), AdvInt: %u (%u.%02u ms)",
			response_slot_delay_units,
			response_slot_delay_ms_x100 / 100U,
			response_slot_delay_ms_x100 % 100U,
			subevent_interval_units,
			subevent_ms_x100 / 100U, subevent_ms_x100 % 100U,
			advertising_event_interval_units,
			adv_event_ms_x100 / 100U, adv_event_ms_x100 % 100U);
	LOG_INF("Est. uplink throughput (theoretical) ~%u kbps",
			(unsigned int)est_throughput_kbps);
}

static struct bt_le_per_adv_param per_adv_params;

void init_adv_params(void)
{
	set_pawr_params(&per_adv_params, NUM_RSP_SLOTS);
}

static struct bt_le_per_adv_subevent_data_params subevent_data_params[NUM_SUBEVENTS];
static struct net_buf_simple bufs[NUM_SUBEVENTS];
static uint8_t backing_store[NUM_SUBEVENTS][MAX_BT_DATA_LENGTH];

BUILD_ASSERT(ARRAY_SIZE(bufs) == ARRAY_SIZE(subevent_data_params));
BUILD_ASSERT(ARRAY_SIZE(backing_store) == ARRAY_SIZE(subevent_data_params));

#define BITMAP_BYTES_NEEDED(num_devices) (((num_devices) + 7) / 8)
static uint8_t response_bitmap[BITMAP_BYTES_NEEDED(NUM_RSP_SLOTS)];
static uint8_t expected_responses[BITMAP_BYTES_NEEDED(NUM_RSP_SLOTS)];

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

#define MAX_ACKS_PER_EVENT 32
static struct ack_entry ack_queue[MAX_ACKS_PER_EVENT];
static uint8_t ack_queue_count;

/* Build a bitmap of unassigned (open) slots; Bit i set => slot i is open */
static void build_open_slots_bitmap(uint8_t *out, uint8_t bytes)
{
	memset(out, 0, bytes);
	for (int i = 0; i < NUM_RSP_SLOTS; i++) {
		if (!slot_state[i].assigned) {
			uint8_t idx = (uint8_t)(i / 8);
			uint8_t bit = (uint8_t)(i % 8);

			if (idx < bytes) {
				out[idx] |= (uint8_t)(1U << bit);
			}
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
	/* Test bit in a byte-addressed bitmap */
	bitmap[bit / 8] |= (1 << (bit % 8));
}

/* Clear bitmap to zero for the first num_bits bits */
static bool bitmap_test_bit(const uint8_t *bitmap, int bit)
{
	return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

/* Build PAwR control payload and program controller */
static void bitmap_clear(uint8_t *bitmap, int num_bits)
{
	memset(bitmap, 0, BITMAP_BYTES_NEEDED(num_bits));
}

static void bitmap_clear_bit(uint8_t *bitmap, int bit)
{
	bitmap[bit / 8] &= (uint8_t)~(1U << (bit % 8));
}

static void reclaim_idle_slots(void)
{
	static uint8_t noresp_counters[NUM_RSP_SLOTS];

	for (int slot = 0; slot < NUM_RSP_SLOTS; slot++) {
		bool was_expected = bitmap_test_bit(expected_responses, slot);
		bool got_response = bitmap_test_bit(response_bitmap, slot);

		if (was_expected && !got_response) {
			if (noresp_counters[slot] < 255) {
				noresp_counters[slot]++;
			}
			if (noresp_counters[slot] >= 3) {
				/* Consider this slot idle; free it for new claimers */
				slot_state[slot].assigned = false;
				slot_state[slot].token = 0;
				bitmap_clear_bit(expected_responses, slot);
				noresp_counters[slot] = 0;
				LOG_INF("[CLAIM] Reclaimed idle slot %d", slot);
			}
		} else if (got_response) { /* remaining cases ignored */
			noresp_counters[slot] = 0;
		} else {
			/* not expected and no response: ignore */
		}
	}
}

static void open_slots_bitmap(struct net_buf_simple *buf)
{
	uint8_t open_len_full = BITMAP_BYTES_NEEDED(NUM_RSP_SLOTS);
	uint8_t open_len = open_len_full > 32 ? 32 : open_len_full;
	uint8_t tmp_open[32];

	build_open_slots_bitmap(tmp_open, open_len);
	net_buf_simple_add_u8(buf, open_len);
	for (uint8_t j = 0; j < open_len; j++) {
		net_buf_simple_add_u8(buf, tmp_open[j]);
	}
}

static void append_acks(struct net_buf_simple *buf, uint8_t bitmap_bytes, uint8_t *ack_count_ptr)
{
	*ack_count_ptr = 0;
	size_t reserved_for_rt = 1 + bitmap_bytes; /* rt len + rt bytes */
	size_t tail = net_buf_simple_tailroom(buf);
	size_t ack_space = (tail > reserved_for_rt) ? (tail - reserved_for_rt) : 0;
	uint8_t ack_capacity = (uint8_t)(ack_space / CLAIM_MSG_LEN);

	if (ack_capacity > MAX_ACKS_PER_EVENT) {
		ack_capacity = MAX_ACKS_PER_EVENT;
	}
	uint8_t acks_to_send = MIN(ack_queue_count, ack_capacity);

	for (uint8_t a = 0; a < acks_to_send; a++) {
		net_buf_simple_add_le32(buf, ack_queue[a].token);
		net_buf_simple_add_u8(buf, ack_queue[a].slot);
	}
	*ack_count_ptr = acks_to_send;
}

static void append_retransmit_bitmap(struct net_buf_simple *buf,
	uint8_t bitmap_bytes,
	uint8_t *retransmit_bitmap)
{
	uint8_t rt_bytes_to_send = bitmap_bytes > 4 ? 4 : bitmap_bytes;
		net_buf_simple_add_u8(buf, rt_bytes_to_send);
		for (int j = 0; j < rt_bytes_to_send; j++) {
			net_buf_simple_add_u8(buf, retransmit_bitmap[j]);
		}
}

static void request_cb(struct bt_le_ext_adv *adv, const struct bt_le_per_adv_data_request *request)
{
	int err;
	uint8_t to_send;
	struct net_buf_simple *buf;

	if (!request) {
		LOG_ERR("Error: NULL request received");
		return;
	}

	to_send = MIN(request->count, ARRAY_SIZE(subevent_data_params));

	/* Calculate retransmission bitmap (bits set for slots that didn't respond) */
	uint8_t bitmap_bytes = BITMAP_BYTES_NEEDED(NUM_RSP_SLOTS);
	uint8_t retransmit_bitmap[bitmap_bytes];

	for (int i = 0; i < bitmap_bytes; i++) {
		retransmit_bitmap[i] = expected_responses[i] & (~response_bitmap[i]);
	}

	/* Reporting which slots missed a response */
	for (int s = 0; s < NUM_RSP_SLOTS; s++) {
		uint8_t idx = (uint8_t)(s / 8);
		uint8_t bit = (uint8_t)(s % 8);

		if (idx < bitmap_bytes && (retransmit_bitmap[idx] & (uint8_t)(1U << bit))) {
			LOG_WRN("[LOSS] slot %d missed response; should retransmit", s);
		}
	}

	reclaim_idle_slots();

	/* Reset response bitmap */
	bitmap_clear(response_bitmap, NUM_RSP_SLOTS);

	/* Process each subevent */
	for (size_t i = 0; i < to_send; i++) {
		buf = &bufs[i];
		net_buf_simple_reset(buf);
		uint8_t *length_field = net_buf_simple_add(buf, 1);

		net_buf_simple_add_u8(buf, BT_DATA_MANUFACTURER_DATA);
		net_buf_simple_add_le16(buf, NORDIC_COMPANY_ID);
		net_buf_simple_add_u8(buf, 0x01); /* protocol version */
		net_buf_simple_add_u8(buf, 0x00); /* protocol flags (none) */

		/* Open slots bitmap */
		open_slots_bitmap(buf);

		/* ACK list: pack as many as tailroom allows, but cap to MAX_ACKS_PER_EVENT */
		uint8_t *ack_count_ptr = net_buf_simple_add(buf, 1);

		append_acks(buf, bitmap_bytes, ack_count_ptr);

		/* Retransmit bitmap: clamp to 4 bytes to keep control small during join */
		append_retransmit_bitmap(buf, bitmap_bytes, retransmit_bitmap);

		*length_field = buf->len - 1;

		subevent_data_params[i].subevent = request->start + i;
		subevent_data_params[i].response_slot_start = 0;
		subevent_data_params[i].response_slot_count = NUM_RSP_SLOTS;
		subevent_data_params[i].data = buf;
	}

	/* We just consumed pending ACKs; clear the queue for next event */
	ack_queue_count = 0;

	err = bt_le_per_adv_set_subevent_data(adv, to_send, subevent_data_params);
	if (err) {
		LOG_ERR("[ADV] Failed to set subevent data (err %d)", err);
	}
}

void handle_claim_message(struct net_buf_simple *buf, struct bt_le_per_adv_response_info *info)
{
	/* CLAIM message: [0xC1][token(4B LE)] */
	uint32_t token = sys_get_le32(&buf->data[1]);

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

static void print_throughput(uint64_t stamp, uint32_t total_bytes)
{
	int64_t delta = k_uptime_delta(&stamp);
	uint64_t measured_kbps =
		(delta > 0) ? (((uint64_t)total_bytes * 8ULL) / (uint64_t)delta) : 0ULL;
	LOG_INF("[ADV] received %u bytes (%u KB) in %lld ms at %llu kbps",
			total_bytes, total_bytes / 1024, delta, measured_kbps);

	uint16_t interval_units = per_adv_params.interval_min;
	uint32_t adv_ms_x100 = (uint32_t)interval_units * 125U;
	uint16_t num_rsp_slots = per_adv_params.num_response_slots;
	uint16_t payload_bytes = MAX_INDIVIDUAL_RESPONSE_SIZE;

	if (adv_ms_x100 > 0U && num_rsp_slots > 0U) {
		uint32_t theo_kbps =
			(uint32_t)(((uint64_t)num_rsp_slots * payload_bytes * 8ULL * 100000ULL) /
						(uint64_t)adv_ms_x100) / 1000U;

	LOG_INF("[PAwR] theoretical throughput: ~%u kbps",
		(unsigned int)theo_kbps);
	LOG_INF("[PAwR] efficiency: ~%u%%",
		(unsigned int)((theo_kbps > 0U) ?
		(uint32_t)((measured_kbps * 100ULL)
		/ theo_kbps) : 0U));
	LOG_INF("[PAwR] N: %u, payload: %u, AdvInt: %u.%02u ms",
		(unsigned int)num_rsp_slots,
		(unsigned int)payload_bytes,
		(unsigned int)(adv_ms_x100 / 100U),
		(unsigned int)(adv_ms_x100 % 100U));
	}
}

static void response_cb(struct bt_le_ext_adv *adv,
		 struct bt_le_per_adv_response_info *info,
		 struct net_buf_simple *buf)
{
	static uint32_t total_bytes;
	static uint64_t stamp;

	if (buf) {
		/* Initialize timestamp on first response */
		if (total_bytes == 0) {
			stamp = k_uptime_get_32();
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

		if (k_uptime_get_32() - stamp > THROUGHPUT_PRINT_INTERVAL) {
			print_throughput(stamp, total_bytes);
			total_bytes = 0;
		}
	}
}


void init_bufs(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(backing_store); i++) {
		/* Initialize buffer with backing storage */
		net_buf_simple_init_with_data(&bufs[i],
				&backing_store[i], ARRAY_SIZE(backing_store[i]));
		net_buf_simple_reset(&bufs[i]);

		/* Add manufacturer specific data */
		net_buf_simple_add_u8(&bufs[i], 3); /* Length of manufacturer data */
		net_buf_simple_add_u8(&bufs[i], BT_DATA_MANUFACTURER_DATA);
		net_buf_simple_add_le16(&bufs[i], NORDIC_COMPANY_ID);

		LOG_INF("[ADV] Buffer %zu initialized with len %u", i, bufs[i].len);
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

	/* Initialize buffers */
	init_bufs();

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}
	LOG_INF("Bluetooth initialized");

	/* Initialize parameters */
	init_adv_params();

	LOG_INF("Starting PAwR Advertiser");
	LOG_INF("===========================================");

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &pawr_adv);
	if (err) {
		LOG_ERR("Failed to create advertising set (err %d)", err);
		return 0;
	}

	/* Set extended advertising data */
	err = bt_le_ext_adv_set_data(pawr_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set ext adv data (err %d)", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(pawr_adv, &per_adv_params);
	if (err) {
		LOG_ERR("Failed to set periodic advertising parameters (err %d)", err);
		return 0;
	}

	/* Enable Extended Advertising */
	LOG_INF("Start Extended Advertising");
	err = bt_le_ext_adv_start(pawr_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_ERR("Failed to start extended advertising (err %d)", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	LOG_INF("Start Periodic Advertising");
	err = bt_le_per_adv_start(pawr_adv);
	if (err) {
		LOG_ERR("Failed to enable periodic advertising (err %d)", err);
		return 0;
	}

	return 0;
}
