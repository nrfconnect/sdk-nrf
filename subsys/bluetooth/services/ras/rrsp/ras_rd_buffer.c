/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>
#include <bluetooth/services/ras.h>

#include "../ras_internal.h"

LOG_MODULE_DECLARE(ras_rrsp, CONFIG_BT_RAS_RRSP_LOG_LEVEL);

#define RD_POOL_SIZE (CONFIG_BT_RAS_RRSP_MAX_ACTIVE_CONN * CONFIG_BT_RAS_RRSP_RD_BUFFERS_PER_CONN)
#define DROP_PROCEDURE_COUNTER_EMPTY (-1)

BUILD_ASSERT(RD_POOL_SIZE <= UINT8_MAX);

static struct ras_rd_buffer rd_buffer_pool[RD_POOL_SIZE];
static int8_t tx_power_cache[CONFIG_BT_MAX_CONN];
static int32_t drop_procedure_counter[CONFIG_BT_MAX_CONN];
static sys_slist_t callback_list = SYS_SLIST_STATIC_INIT(&callback_list);

static void notify_new_rd_stored(struct bt_conn *conn, uint16_t ranging_counter)
{
	struct bt_ras_rd_buffer_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->new_ranging_data_received) {
			cb->new_ranging_data_received(conn, ranging_counter);
		}
	}
}

static void notify_rd_overwritten(struct bt_conn *conn, uint16_t ranging_counter)
{
	struct bt_ras_rd_buffer_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->ranging_data_overwritten) {
			cb->ranging_data_overwritten(conn, ranging_counter);
		}
	}
}

static struct ras_rd_buffer *rd_buffer_get(struct bt_conn *conn, uint16_t ranging_counter,
					   bool ready, bool busy)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(rd_buffer_pool); i++) {
		if (rd_buffer_pool[i].conn == conn &&
		    rd_buffer_pool[i].ranging_counter == ranging_counter &&
		    rd_buffer_pool[i].ready == ready && rd_buffer_pool[i].busy == busy) {
			return &rd_buffer_pool[i];
		}
	}

	return NULL;
}

static void rd_buffer_init(struct bt_conn *conn, struct ras_rd_buffer *buf,
			   uint16_t ranging_counter)
{
	buf->conn = bt_conn_ref(conn);
	buf->ranging_counter = ranging_counter;
	buf->ready = false;
	buf->busy = true;
	buf->acked = false;
	buf->subevent_cursor = 0;
	atomic_clear(&buf->refcount);
}

static void rd_buffer_free(struct ras_rd_buffer *buf)
{
	if (buf->conn) {
		bt_conn_unref(buf->conn);
	}

	buf->conn = NULL;
	buf->ready = false;
	buf->busy = false;
	buf->acked = false;
	buf->refcount = 0;
	buf->subevent_cursor = 0;
	atomic_clear(&buf->refcount);
}

static struct ras_rd_buffer *rd_buffer_alloc(struct bt_conn *conn, uint16_t ranging_counter)
{
	uint16_t conn_buffer_count = 0;
	uint16_t oldest_ranging_counter = UINT16_MAX;
	uint16_t oldest_ranging_counter_age = 0;
	struct ras_rd_buffer *available_free_buffer = NULL;
	struct ras_rd_buffer *available_oldest_buffer = NULL;

	for (uint8_t i = 0; i < ARRAY_SIZE(rd_buffer_pool); i++) {
		if (rd_buffer_pool[i].conn == conn) {
			conn_buffer_count++;

			const uint16_t ranging_counter_age = ranging_counter
				- rd_buffer_pool[i].ranging_counter;

			/* Only overwrite buffers that have ranging data stored
			 * and are not being read.
			 */
			if (rd_buffer_pool[i].ready && !rd_buffer_pool[i].busy &&
			    atomic_get(&rd_buffer_pool[i].refcount) == 0 &&
			    ranging_counter_age > oldest_ranging_counter_age) {
				oldest_ranging_counter = rd_buffer_pool[i].ranging_counter;
				oldest_ranging_counter_age = ranging_counter_age;
				available_oldest_buffer = &rd_buffer_pool[i];
			}
		}

		if (available_free_buffer == NULL && rd_buffer_pool[i].conn == NULL) {
			available_free_buffer = &rd_buffer_pool[i];
		}
	}

	/* Allocate the buffer straight away if the connection has not reached
	 * the maximum number of buffers allocated.
	 */
	if (conn_buffer_count < CONFIG_BT_RAS_RRSP_RD_BUFFERS_PER_CONN) {
		rd_buffer_init(conn, available_free_buffer, ranging_counter);

		return available_free_buffer;
	}

	/* Overwrite the oldest stored ranging buffer that is not in use */
	if (available_oldest_buffer != NULL) {
		if (!available_oldest_buffer->acked) {
			/* Only notify if the peer has not read the buffer yet. */
			notify_rd_overwritten(conn, oldest_ranging_counter);
		}
		rd_buffer_free(available_oldest_buffer);

		rd_buffer_init(conn, available_oldest_buffer, ranging_counter);

		return available_oldest_buffer;
	}

	/* Could not allocate a buffer */
	return NULL;
}

static void cs_procedure_enabled(struct bt_conn *conn,
				 struct bt_conn_le_cs_procedure_enable_complete *params)
{
	uint8_t conn_index = bt_conn_index(conn);

	__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(tx_power_cache));

	tx_power_cache[conn_index] = params->selected_tx_power;
	drop_procedure_counter[conn_index] = DROP_PROCEDURE_COUNTER_EMPTY;
}

static bool process_step_data(struct bt_le_cs_subevent_step *step, void *user_data)
{
	struct ras_rd_buffer *buf = (struct ras_rd_buffer *)user_data;

	uint16_t buffer_len = buf->subevent_cursor + BT_RAS_STEP_MODE_LEN + step->data_len;
	uint16_t buffer_size = BT_RAS_PROCEDURE_MEM - BT_RAS_RANGING_HEADER_LEN;

	if (buffer_len > buffer_size) {
		uint8_t conn_index = bt_conn_index(buf->conn);

		__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(drop_procedure_counter));
		LOG_ERR("Out of buffer space: attempted to store %u bytes, buffer size: %u",
			buffer_len, buffer_size);
		drop_procedure_counter[conn_index] = buf->ranging_counter;

		return false;
	}

	buf->procedure.subevents[buf->subevent_cursor] = step->mode;
	buf->subevent_cursor += BT_RAS_STEP_MODE_LEN;

	memcpy(&buf->procedure.subevents[buf->subevent_cursor], step->data, step->data_len);
	buf->subevent_cursor += step->data_len;

	return true;
}

static void subevent_data_available(struct bt_conn *conn,
				    struct bt_conn_le_cs_subevent_result *result)
{
	LOG_DBG("Procedure %u", result->header.procedure_counter);
	struct ras_rd_buffer *buf =
		rd_buffer_get(conn, result->header.procedure_counter, false, true);

	uint8_t conn_index = bt_conn_index(conn);

	__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(tx_power_cache));
	__ASSERT_NO_MSG(conn_index < ARRAY_SIZE(drop_procedure_counter));

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_ABORTED) {
		LOG_DBG("Procedure was aborted.");
		drop_procedure_counter[conn_index] = result->header.procedure_counter;
	}

	if (drop_procedure_counter[conn_index] == result->header.procedure_counter) {
		/* This procedure will not be sent to the peer, so ignore all data. */
		LOG_DBG("Dropping subevent data for procedure %u",
			result->header.procedure_counter);

		if (buf) {
			rd_buffer_free(buf);
		}

		return;
	}

	drop_procedure_counter[conn_index] = DROP_PROCEDURE_COUNTER_EMPTY;

	if (!buf) {
		/* First subevent - allocate a buffer */
		buf = rd_buffer_alloc(conn, result->header.procedure_counter);

		if (!buf) {
			LOG_ERR("Failed to allocate buffer for procedure %u",
				result->header.procedure_counter);
			drop_procedure_counter[conn_index] = result->header.procedure_counter;

			return;
		}

		buf->procedure.ranging_header.ranging_counter = result->header.procedure_counter;
		buf->procedure.ranging_header.config_id = result->header.config_id;
		buf->procedure.ranging_header.selected_tx_power = tx_power_cache[conn_index];
		buf->procedure.ranging_header.antenna_paths_mask =
			BIT_MASK(result->header.num_antenna_paths);
	}

	struct ras_subevent_header *hdr =
		(struct ras_subevent_header *)&buf->procedure.subevents[buf->subevent_cursor];
	uint16_t buffer_size = BT_RAS_PROCEDURE_MEM - BT_RAS_RANGING_HEADER_LEN;

	buf->subevent_cursor += sizeof(struct ras_subevent_header);

	if (buf->subevent_cursor > buffer_size) {
		LOG_ERR("Out of buffer space: attempted to store %u bytes, buffer size: %u",
			buf->subevent_cursor, buffer_size);
		drop_procedure_counter[conn_index] = buf->ranging_counter;

		rd_buffer_free(buf);

		return;
	}

	hdr->start_acl_conn_event = result->header.start_acl_conn_event;
	hdr->freq_compensation = result->header.frequency_compensation;
	hdr->ranging_done_status = result->header.procedure_done_status;
	hdr->subevent_done_status = result->header.subevent_done_status;
	hdr->ranging_abort_reason = result->header.procedure_abort_reason;
	hdr->subevent_abort_reason = result->header.subevent_abort_reason;
	hdr->ref_power_level = result->header.reference_power_level;

	if (result->header.subevent_done_status == BT_CONN_LE_CS_SUBEVENT_ABORTED) {
		hdr->num_steps_reported = 0;
		LOG_DBG("Discarding %u steps in aborted subevent",
			result->header.num_steps_reported);
	} else {
		hdr->num_steps_reported = result->header.num_steps_reported;

		if (result->step_data_buf) {
			bt_le_cs_step_data_parse(result->step_data_buf, process_step_data, buf);
		}
	}

	/* process_step_data might have requested dropping this procedure. */
	bool drop = (drop_procedure_counter[conn_index] == result->header.procedure_counter);

	if (drop) {
		rd_buffer_free(buf);
		return;
	}

	if (hdr->ranging_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE ||
	    hdr->ranging_done_status == BT_CONN_LE_CS_PROCEDURE_ABORTED) {
		buf->ready = true;
		buf->busy = false;
		notify_new_rd_stored(conn, result->header.procedure_counter);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	for (uint8_t i = 0; i < ARRAY_SIZE(rd_buffer_pool); i++) {
		if (rd_buffer_pool[i].conn == conn) {
			rd_buffer_free(&rd_buffer_pool[i]);
		}
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.le_cs_procedure_enabled = cs_procedure_enabled,
	.le_cs_subevent_data_available = subevent_data_available,
	.disconnected = disconnected,
};

void bt_ras_rd_buffer_cb_register(struct bt_ras_rd_buffer_cb *cb)
{
	sys_slist_append(&callback_list, &cb->node);
}

bool bt_ras_rd_buffer_ready_check(struct bt_conn *conn, uint16_t ranging_counter)
{
	struct ras_rd_buffer *buf = rd_buffer_get(conn, ranging_counter, true, false);

	return buf != NULL;
}

struct ras_rd_buffer *bt_ras_rd_buffer_claim(struct bt_conn *conn, uint16_t ranging_counter)
{
	struct ras_rd_buffer *buf = rd_buffer_get(conn, ranging_counter, true, false);

	if (buf) {
		atomic_inc(&buf->refcount);
		return buf;
	}

	return NULL;
}

int bt_ras_rd_buffer_release(struct ras_rd_buffer *buf)
{
	if (!buf || atomic_get(&buf->refcount) == 0) {
		return -EINVAL;
	}

	atomic_dec(&buf->refcount);

	/* Not freeing the buffer as it may be requested again by the app.
	 * It will get overwritten when new subevent data is available.
	 */

	return 0;
}

int bt_ras_rd_buffer_bytes_pull(struct ras_rd_buffer *buf, uint8_t *out_buf, uint16_t max_data_len,
				uint16_t *read_cursor, bool *empty)
{
	if (!buf->ready) {
		return 0;
	}

	uint16_t buf_len = sizeof(struct ras_ranging_header) + buf->subevent_cursor;
	uint16_t remaining = buf_len - (*read_cursor);
	uint16_t pull_bytes = MIN(max_data_len, remaining);

	__ASSERT_NO_MSG(*read_cursor <= buf_len);

	memcpy(out_buf, &buf->procedure.buf[*read_cursor], pull_bytes);
	*read_cursor += pull_bytes;
	*empty = (remaining == pull_bytes);

	return pull_bytes;
}
