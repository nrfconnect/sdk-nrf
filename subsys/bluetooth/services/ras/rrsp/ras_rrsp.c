/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/net_buf.h>
#include <bluetooth/services/ras.h>

#include "../ras_internal.h"

LOG_MODULE_REGISTER(ras_rrsp, CONFIG_BT_RAS_RRSP_LOG_LEVEL);

#define RRSP_WQ_STACK_SIZE 1024
#define RRSP_WQ_PRIORITY   K_PRIO_PREEMPT(K_LOWEST_APPLICATION_THREAD_PRIO)
K_THREAD_STACK_DEFINE(rrsp_wq_stack_area, RRSP_WQ_STACK_SIZE);

NET_BUF_SIMPLE_DEFINE_STATIC(segment_buf, CONFIG_BT_L2CAP_TX_MTU);

static struct bt_ras_rrsp {
	struct bt_conn *conn;

	struct ras_rd_buffer *active_buf;
	struct k_work send_data_work;
	struct k_work rascp_work;
	struct k_work status_work;
	struct k_timer rascp_timeout;

	struct bt_gatt_indicate_params ranging_data_ind_params;
	struct bt_gatt_indicate_params rascp_ind_params;
	struct bt_gatt_indicate_params rd_status_params;

	uint8_t rascp_cmd_buf[RASCP_WRITE_MAX_LEN];
	uint8_t rascp_cmd_len;

	uint16_t ready_ranging_counter;
	uint16_t overwritten_ranging_counter;
	uint16_t segment_counter;
	uint16_t active_buf_read_cursor;

	bool streaming;
	bool notify_ready;
	bool notify_overwritten;
	bool handle_rascp_timeout;
} rrsp_pool[CONFIG_BT_RAS_RRSP_MAX_ACTIVE_CONN];

static struct k_work_q rrsp_wq;

static uint32_t ras_optional_features = RAS_FEAT_REALTIME_RD;

static void send_data_work_handler(struct k_work *work);
static void rascp_work_handler(struct k_work *work);
static void status_work_handler(struct k_work *work);
static void rascp_timeout_handler(struct k_timer *timer);

static int ranging_data_notify_or_indicate(struct bt_conn *conn, struct net_buf_simple *buf);
static int rd_status_notify_or_indicate(struct bt_conn *conn, const struct bt_uuid *uuid,
					uint16_t ranging_counter);

static void rascp_send_complete_rd_rsp(struct bt_conn *conn, uint16_t ranging_counter);
static void rascp_cmd_handle(struct bt_ras_rrsp *rrsp);

static ssize_t ondemand_rd_ccc_cfg_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					    uint16_t value);
static ssize_t realtime_rd_ccc_cfg_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					    uint16_t value);

static struct bt_ras_rrsp *rrsp_find(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(rrsp_pool); i++) {
		struct bt_ras_rrsp *rrsp = &rrsp_pool[i];

		if (rrsp->conn == conn) {
			return rrsp;
		}
	}

	return NULL;
}

int bt_ras_rrsp_alloc(struct bt_conn *conn)
{
	struct bt_ras_rrsp *rrsp = NULL;

	if (rrsp_find(conn) != NULL) {
		return -EALREADY;
	}

	for (size_t i = 0; i < ARRAY_SIZE(rrsp_pool); i++) {
		if (rrsp_pool[i].conn == NULL) {
			rrsp = &rrsp_pool[i];
			break;
		}
	}

	if (rrsp == NULL) {
		return -ENOMEM;
	}

	LOG_DBG("conn %p new rrsp %p", (void *)conn, (void *)rrsp);

	memset(rrsp, 0, sizeof(struct bt_ras_rrsp));
	rrsp->conn = bt_conn_ref(conn);

	k_work_init(&rrsp->send_data_work, send_data_work_handler);
	k_work_init(&rrsp->rascp_work, rascp_work_handler);
	k_work_init(&rrsp->status_work, status_work_handler);
	k_timer_init(&rrsp->rascp_timeout, rascp_timeout_handler, NULL);

	return 0;
}

void bt_ras_rrsp_free(struct bt_conn *conn)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (rrsp) {
		LOG_DBG("conn %p rrsp %p", (void *)rrsp->conn, (void *)rrsp);

		(void)k_work_cancel(&rrsp->send_data_work);
		(void)k_work_cancel(&rrsp->rascp_work);
		(void)k_work_cancel(&rrsp->status_work);
		k_timer_stop(&rrsp->rascp_timeout);

		k_work_queue_drain(&rrsp_wq, false);

		bt_conn_unref(rrsp->conn);
		rrsp->conn = NULL;
	}
}

static ssize_t ras_features_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ras_optional_features,
				 sizeof(ras_optional_features));
}

static ssize_t rd_ready_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (!rrsp) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &rrsp->ready_ranging_counter,
				 sizeof(uint16_t));
}

static ssize_t rd_overwritten_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (!rrsp) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &rrsp->overwritten_ranging_counter,
				 sizeof(uint16_t));
}

static ssize_t ras_cp_write(struct bt_conn *conn, struct bt_gatt_attr const *attr, void const *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_HEXDUMP_DBG(buf, len, "Write request");

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE)) {
		LOG_DBG("Not subscribed");
		return BT_GATT_ERR(RAS_ATT_ERROR_CCC_CONFIG);
	}

	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (!rrsp || k_work_is_pending(&rrsp->rascp_work) || len > RASCP_WRITE_MAX_LEN) {
		LOG_DBG("Write rejected");
		return BT_GATT_ERR(RAS_ATT_ERROR_WRITE_REQ_REJECTED);
	}

	memcpy(rrsp->rascp_cmd_buf, buf, len);
	rrsp->rascp_cmd_len = (uint8_t)len;

	k_work_submit_to_queue(&rrsp_wq, &rrsp->rascp_work);

	return len;
}

static void ras_cp_ccc_cfg_changed(struct bt_gatt_attr const *attr, uint16_t value)
{
	LOG_DBG("RAS-CP CCCD changed: %u", value);
}

static void rd_ready_ccc_cfg_changed(struct bt_gatt_attr const *attr, uint16_t value)
{
	LOG_DBG("Ranging Data Ready CCCD changed: %u", value);
}

static void rd_overwritten_ccc_cfg_changed(struct bt_gatt_attr const *attr, uint16_t value)
{
	LOG_DBG("Ranging Data Overwritten CCCD changed: %u", value);
}

BT_GATT_SERVICE_DEFINE(
	rrsp_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_RANGING_SERVICE),
	/* RAS Features */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_FEATURES, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       ras_features_read, NULL, NULL),
	/* On-demand Ranging Data */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_ONDEMAND_RD, BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC_WITH_WRITE_CB(NULL, ondemand_rd_ccc_cfg_write_cb,
				  BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	/* Real-time Ranging Data */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_REALTIME_RD, BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC_WITH_WRITE_CB(NULL, realtime_rd_ccc_cfg_write_cb,
				  BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	/* RAS-CP */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_CP,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE_ENCRYPT, NULL, ras_cp_write, NULL),
	BT_GATT_CCC(ras_cp_ccc_cfg_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	/* Ranging Data Ready */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_RD_READY,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT, rd_ready_read, NULL, NULL),
	BT_GATT_CCC(rd_ready_ccc_cfg_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	/* Ranging Data Overwritten */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_RD_OVERWRITTEN,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT, rd_overwritten_read, NULL, NULL),
	BT_GATT_CCC(rd_overwritten_ccc_cfg_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
);

static ssize_t ondemand_rd_ccc_cfg_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					    uint16_t value)
{
	if (!value) {
		LOG_DBG("Disabled On-demand Ranging Data mode.");
	} else {
		struct bt_gatt_attr *realtime_rd_attr =
			bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_REALTIME_RD);

		if (bt_gatt_is_subscribed(conn, realtime_rd_attr,
					  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
			LOG_DBG("On-demand Ranging Data CCCD write rejected because Real-time "
				"Ranging Data is currently enabled.");
			return BT_GATT_ERR(RAS_ATT_ERROR_CCC_CONFIG);
		}

		LOG_DBG("Enabled On-demand Ranging Data mode.");
	}

	return sizeof(value);
}

static ssize_t realtime_rd_ccc_cfg_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					    uint16_t value)
{
	if (!value) {
		LOG_DBG("Disabled Real-time Ranging Data mode.");
	} else {
		struct bt_gatt_attr *ondemand_rd_attr =
			bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_ONDEMAND_RD);

		if (bt_gatt_is_subscribed(conn, ondemand_rd_attr,
					  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
			LOG_DBG("Real-time Ranging Data CCCD write rejected because On-demand "
				"Ranging Data is currently enabled.");
			return BT_GATT_ERR(RAS_ATT_ERROR_CCC_CONFIG);
		}

		LOG_DBG("Enabled Real-time Ranging Data mode.");
	}

	return sizeof(value);
}

#if defined(CONFIG_BT_RAS_RRSP_AUTO_ALLOC_INSTANCE)
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("Allocating RRSP for %s\n", addr);

	bt_ras_rrsp_alloc(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("Freeing RRSP for %s (reason 0x%02x)", addr, reason);

	bt_ras_rrsp_free(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};
#endif

static int rd_segment_send(struct bt_ras_rrsp *rrsp)
{
	int err;

	__ASSERT_NO_MSG(rrsp->conn);

	/* By spec, up to (ATT_MTU-4) octets of the data to be transported
	 * shall be used to fill the characteristic message.
	 * An extra byte is reserved for the segment header
	 */
	uint16_t max_data_len = bt_gatt_get_mtu(rrsp->conn) - (4 + sizeof(struct ras_seg_header));

	/* segment_buf is only accessed by the RRSP WQ, so is safe to reuse. */
	net_buf_simple_reset(&segment_buf);

	struct ras_segment *ras_segment =
		net_buf_simple_add(&segment_buf, sizeof(struct ras_segment) + max_data_len);
	if (!ras_segment) {
		LOG_ERR("Cannot allocate segment buffer");
		return -ENOMEM;
	}

	bool first_seg = (rrsp->active_buf_read_cursor == 0);
	bool last_seg;
	uint16_t actual_data_len =
		bt_ras_rd_buffer_bytes_pull(rrsp->active_buf, ras_segment->data, max_data_len,
					    &rrsp->active_buf_read_cursor, &last_seg);

	LOG_DBG("Got %u bytes (max: %u)", actual_data_len, max_data_len);

	if (actual_data_len) {
		ras_segment->header.first_seg = first_seg;
		ras_segment->header.last_seg = last_seg;
		ras_segment->header.seg_counter = rrsp->segment_counter & BIT_MASK(6);

		(void)net_buf_simple_remove_mem(&segment_buf, (max_data_len - actual_data_len));

		err = ranging_data_notify_or_indicate(rrsp->conn, &segment_buf);
		if (err) {
			LOG_WRN("ranging_data_notify_or_indicate failed err %d", err);

			/* Keep retrying */
			rrsp->active_buf_read_cursor -= actual_data_len;

			return err;
		}

		rrsp->segment_counter++;

		LOG_DBG("Segment with RSC %d sent", rrsp->segment_counter);
	}

	if (!last_seg) {
		k_work_submit_to_queue(&rrsp_wq, &rrsp->send_data_work);
	} else {
		LOG_DBG("All segments sent");

		rrsp->streaming = false;
		k_work_cancel(&rrsp->send_data_work);

		struct bt_gatt_attr *ondemand_rd_attr =
			bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_ONDEMAND_RD);

		if (bt_gatt_is_subscribed(rrsp->conn, ondemand_rd_attr,
					  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
			rascp_send_complete_rd_rsp(rrsp->conn, rrsp->active_buf->ranging_counter);
			k_timer_start(&rrsp->rascp_timeout, RASCP_ACK_DATA_TIMEOUT, K_NO_WAIT);
		} else {
			struct bt_gatt_attr *realtime_rd_attr =
				bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_REALTIME_RD);

			if (bt_gatt_is_subscribed(rrsp->conn, realtime_rd_attr,
						  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
				bt_ras_rd_buffer_release(rrsp->active_buf);
				rrsp->active_buf = NULL;
				rrsp->active_buf_read_cursor = 0;
			}
		}
	}

	return 0;
}

static void send_data_work_handler(struct k_work *work)
{
	struct bt_ras_rrsp *rrsp = CONTAINER_OF(work, struct bt_ras_rrsp, send_data_work);

	if (!rrsp->streaming || !rrsp->active_buf) {
		return;
	}

	int err = rd_segment_send(rrsp);

	if (err) {
		/* Will keep retrying. */
		LOG_WRN("Failed to send segment: %d", err);
	}
}

static void rascp_work_handler(struct k_work *work)
{
	struct bt_ras_rrsp *rrsp = CONTAINER_OF(work, struct bt_ras_rrsp, rascp_work);

	LOG_DBG("rrsp %p", rrsp);

	rascp_cmd_handle(rrsp);
}

static void status_work_handler(struct k_work *work)
{
	struct bt_ras_rrsp *rrsp = CONTAINER_OF(work, struct bt_ras_rrsp, status_work);

	LOG_DBG("rrsp %p", rrsp);

	if (rrsp->handle_rascp_timeout) {
		/* The procedure is considered to have failed if the peer does not ACK
		 * the data within 5 seconds of rascp_send_complete_rd_rsp being called.
		 */
		(void)bt_ras_rd_buffer_release(rrsp->active_buf);
		rrsp->active_buf = NULL;
		rrsp->active_buf_read_cursor = 0;
		rrsp->handle_rascp_timeout = false;
	}

	if (rrsp->notify_overwritten) {
		int err = rd_status_notify_or_indicate(rrsp->conn, BT_UUID_RAS_RD_OVERWRITTEN,
						       rrsp->overwritten_ranging_counter);
		if (err) {
			LOG_WRN("Notify overwritten failed: %d", err);
		}

		rrsp->notify_overwritten = false;
	}

	if (rrsp->notify_ready) {
		int err = rd_status_notify_or_indicate(rrsp->conn, BT_UUID_RAS_RD_READY,
						       rrsp->ready_ranging_counter);
		if (err) {
			LOG_WRN("Notify ready failed: %d", err);
		}

		rrsp->notify_ready = false;
	}
}

static void rascp_timeout_handler(struct k_timer *timer)
{
	struct bt_ras_rrsp *rrsp = CONTAINER_OF(timer, struct bt_ras_rrsp, rascp_timeout);

	LOG_DBG("rrsp %p", rrsp);

	LOG_WRN("RAS-CP timeout");

	rrsp->handle_rascp_timeout = true;
	k_work_submit_to_queue(&rrsp_wq, &rrsp->status_work);
}

static void new_rd_handle(struct bt_conn *conn, uint16_t ranging_counter)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (rrsp) {
		struct bt_gatt_attr *ondemand_rd_attr =
			bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_ONDEMAND_RD);

		if (bt_gatt_is_subscribed(conn, ondemand_rd_attr,
					  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
			rrsp->ready_ranging_counter = ranging_counter;
			rrsp->notify_ready = true;
			k_work_submit_to_queue(&rrsp_wq, &rrsp->status_work);
		} else {
			struct bt_gatt_attr *realtime_rd_attr =
				bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_REALTIME_RD);

			if (bt_gatt_is_subscribed(conn, realtime_rd_attr,
						  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
				if (!rrsp->streaming) {
					rrsp->active_buf =
						bt_ras_rd_buffer_claim(conn, ranging_counter);
					rrsp->active_buf_read_cursor = 0;
					rrsp->segment_counter = 0;
					rrsp->streaming = true;

					k_work_submit_to_queue(&rrsp_wq, &rrsp->send_data_work);
				} else {
					LOG_DBG("Dropped new ranging data.");
				}
			}
		}
	}
}

static void rd_overwritten_handle(struct bt_conn *conn, uint16_t ranging_counter)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);
	struct bt_gatt_attr *ondemand_rd_attr =
		bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_ONDEMAND_RD);

	if (rrsp && bt_gatt_is_subscribed(conn, ondemand_rd_attr,
					  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
		rrsp->overwritten_ranging_counter = ranging_counter;
		rrsp->notify_overwritten = true;
		k_work_submit_to_queue(&rrsp_wq, &rrsp->status_work);
	}
}

static struct bt_ras_rd_buffer_cb rd_buffer_callbacks = {
	.new_ranging_data_received = new_rd_handle,
	.ranging_data_overwritten = rd_overwritten_handle,
};

static int ras_rrsp_init(void)
{
	const struct k_work_queue_config cfg = {.name = "BT RAS RRSP WQ"};

	k_work_queue_init(&rrsp_wq);
	k_work_queue_start(&rrsp_wq, rrsp_wq_stack_area, K_THREAD_STACK_SIZEOF(rrsp_wq_stack_area),
			   RRSP_WQ_PRIORITY, &cfg);

	bt_ras_rd_buffer_cb_register(&rd_buffer_callbacks);

	return 0;
}

SYS_INIT(ras_rrsp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static void ranging_data_notify_sent_cb(struct bt_conn *conn, void *user_data)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (rrsp) {
		LOG_DBG("");
		k_work_submit_to_queue(&rrsp_wq, &rrsp->send_data_work);
	}
}

static void ranging_data_indicate_sent_cb(struct bt_conn *conn,
					  struct bt_gatt_indicate_params *params, uint8_t err)
{
	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	if (rrsp) {
		LOG_DBG("");
		k_work_submit_to_queue(&rrsp_wq, &rrsp->send_data_work);
	}
}

static int ranging_data_notify_or_indicate(struct bt_conn *conn, struct net_buf_simple *buf)
{
	struct bt_gatt_attr *attr;

	struct bt_ras_rrsp *rrsp = rrsp_find(conn);

	__ASSERT_NO_MSG(rrsp);
	ARG_UNUSED(rrsp);

	attr = bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_REALTIME_RD);

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE | BT_GATT_CCC_NOTIFY)) {
		attr = bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_ONDEMAND_RD);
	}

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE | BT_GATT_CCC_NOTIFY)) {
		LOG_WRN("Peer has not enabled Real-time or On-demand ranging data.");
		return -EINVAL;
	}

	__ASSERT_NO_MSG(attr);

	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		struct bt_gatt_notify_params params = {0};

		params.attr = attr;
		params.uuid = NULL;
		params.data = buf->data;
		params.len = buf->len;
		params.func = ranging_data_notify_sent_cb;

		return bt_gatt_notify_cb(conn, &params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE)) {
		struct bt_ras_rrsp *rrsp = rrsp_find(conn);

		__ASSERT_NO_MSG(rrsp);

		rrsp->ranging_data_ind_params.attr = attr;
		rrsp->ranging_data_ind_params.uuid = NULL;
		rrsp->ranging_data_ind_params.data = buf->data;
		rrsp->ranging_data_ind_params.len = buf->len;
		rrsp->ranging_data_ind_params.func = ranging_data_indicate_sent_cb;
		rrsp->ranging_data_ind_params.destroy = NULL;

		return bt_gatt_indicate(conn, &rrsp->ranging_data_ind_params);
	}

	return -EINVAL;
}

static int rascp_indicate(struct bt_conn *conn, struct net_buf_simple *rsp)
{
	struct bt_gatt_attr *attr = bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_CP);

	__ASSERT_NO_MSG(attr);

	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE)) {
		struct bt_ras_rrsp *rrsp = rrsp_find(conn);

		__ASSERT_NO_MSG(rrsp);

		rrsp->rascp_ind_params.attr = attr;
		rrsp->rascp_ind_params.uuid = NULL;
		rrsp->rascp_ind_params.data = rsp->data;
		rrsp->rascp_ind_params.len = rsp->len;
		rrsp->rascp_ind_params.func = NULL;
		rrsp->rascp_ind_params.destroy = NULL;

		return bt_gatt_indicate(conn, &rrsp->rascp_ind_params);
	}

	LOG_WRN("Peer is not subscribed to RAS-CP.");
	return -EINVAL;
}

static int rd_status_notify_or_indicate(struct bt_conn *conn, const struct bt_uuid *uuid,
					uint16_t ranging_counter)
{
	struct bt_gatt_attr *attr = bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, uuid);

	__ASSERT_NO_MSG(attr);

	if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_INDICATE)) {
		struct bt_ras_rrsp *rrsp = rrsp_find(conn);

		__ASSERT_NO_MSG(rrsp);

		rrsp->rd_status_params.attr = attr;
		rrsp->rd_status_params.uuid = NULL;
		rrsp->rd_status_params.data = &ranging_counter;
		rrsp->rd_status_params.len = sizeof(ranging_counter);
		rrsp->rd_status_params.func = NULL;
		rrsp->rd_status_params.destroy = NULL;

		return bt_gatt_indicate(conn, &rrsp->rd_status_params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify(conn, attr, &ranging_counter, sizeof(ranging_counter));
	}

	LOG_WRN("Peer is not subscribed to status characteristic.");
	return -EINVAL;
}

static void rascp_send_complete_rd_rsp(struct bt_conn *conn, uint16_t ranging_counter)
{
	NET_BUF_SIMPLE_DEFINE(rsp, RASCP_CMD_OPCODE_LEN + RASCP_RSP_OPCODE_COMPLETE_RD_RSP_LEN);

	net_buf_simple_add_u8(&rsp, RASCP_RSP_OPCODE_COMPLETE_RD_RSP);
	net_buf_simple_add_le16(&rsp, ranging_counter);
	int err = rascp_indicate(conn, &rsp);

	if (err) {
		LOG_WRN("Failed to send indication: %d", err);
	}
}

static void rascp_send_rsp_code(struct bt_conn *conn, enum rascp_rsp_code rsp_code)
{
	NET_BUF_SIMPLE_DEFINE(rsp, RASCP_CMD_OPCODE_LEN + RASCP_RSP_OPCODE_RSP_CODE_LEN);

	net_buf_simple_add_u8(&rsp, RASCP_RSP_OPCODE_RSP_CODE);
	net_buf_simple_add_u8(&rsp, rsp_code);

	int err = rascp_indicate(conn, &rsp);

	if (err) {
		LOG_WRN("Failed to send indication: %d", err);
	}
}

static void rascp_cmd_handle(struct bt_ras_rrsp *rrsp)
{
	struct net_buf_simple req;

	net_buf_simple_init_with_data(&req, rrsp->rascp_cmd_buf, rrsp->rascp_cmd_len);

	uint8_t opcode = net_buf_simple_pull_u8(&req);
	uint8_t param_len = MIN(req.len, RASCP_CMD_PARAMS_MAX_LEN);

	if (rrsp->streaming) {
		rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_SERVER_BUSY);
		return;
	}

	struct bt_gatt_attr *realtime_rd_attr =
		bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_REALTIME_RD);

	if (bt_gatt_is_subscribed(rrsp->conn, realtime_rd_attr,
				  BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
		LOG_WRN("Peer accessing RAS-CP while RRSP is operating in real-time mode.");
		rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_INVALID_PARAMETER);
		return;
	}

	switch (opcode) {
	case RASCP_OPCODE_GET_RD: {
		if (param_len != sizeof(uint16_t)) {
			LOG_WRN("Invalid length %d", param_len);
			rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_INVALID_PARAMETER);
			return;
		}

		uint16_t ranging_counter = net_buf_simple_pull_le16(&req);

		LOG_DBG("GET_RD %d", ranging_counter);

		struct bt_gatt_attr *attr =
			bt_gatt_find_by_uuid(rrsp_svc.attrs, 0, BT_UUID_RAS_ONDEMAND_RD);

		__ASSERT_NO_MSG(attr);

		if (!bt_gatt_is_subscribed(rrsp->conn, attr, BT_GATT_CCC_INDICATE) &&
		    !bt_gatt_is_subscribed(rrsp->conn, attr, BT_GATT_CCC_NOTIFY)) {
			/* Ranging data can't be sent to the peer. */
			LOG_WRN("Peer not subscribed to ranging data characteristic.");
			rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_INVALID_PARAMETER);
			return;
		}

		if (rrsp->active_buf) {
			/* Disallow getting new data until the active one has been ACKed. */
			rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_SERVER_BUSY);
			return;
		}

		if (!bt_ras_rd_buffer_ready_check(rrsp->conn, ranging_counter)) {
			rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_NO_RECORDS_FOUND);
			return;
		}

		rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_SUCCESS);

		rrsp->active_buf = bt_ras_rd_buffer_claim(rrsp->conn, ranging_counter);
		rrsp->active_buf_read_cursor = 0;
		rrsp->segment_counter = 0;
		rrsp->streaming = true;

		k_work_submit_to_queue(&rrsp_wq, &rrsp->send_data_work);

		break;
	}
	case RASCP_OPCODE_ACK_RD: {
		if (param_len != sizeof(uint16_t)) {
			LOG_WRN("Invalid length %d", param_len);
			rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_INVALID_PARAMETER);
			return;
		}

		uint16_t ranging_counter = net_buf_simple_pull_le16(&req);

		LOG_DBG("ACK_RD %d", ranging_counter);

		if (!rrsp->active_buf || rrsp->active_buf->ranging_counter != ranging_counter) {
			/* Only allow ACKing the currently requested ranging counter. */
			rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_NO_RECORDS_FOUND);
			return;
		}

		k_timer_stop(&rrsp->rascp_timeout);
		rrsp->handle_rascp_timeout = false;

		rrsp->active_buf->acked = true;

		bt_ras_rd_buffer_release(rrsp->active_buf);
		rrsp->active_buf = NULL;
		rrsp->active_buf_read_cursor = 0;

		rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_SUCCESS);

		break;
	}
	default: {
		LOG_INF("Opcode %x invalid or unsupported", opcode);
		rascp_send_rsp_code(rrsp->conn, RASCP_RESPONSE_OPCODE_NOT_SUPPORTED);
		break;
	}
	}
}
