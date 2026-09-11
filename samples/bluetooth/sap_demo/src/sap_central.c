/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/sap.h>

#include "demo_protocol.h"

LOG_MODULE_REGISTER(sap_central, CONFIG_BT_SAP_LOG_LEVEL);

#define SAP_SCAN_RETRY_DELAY	  K_MSEC(500)
#define SAP_PROTECTED_RETRY_DELAY K_MSEC(200)
#define SAP_PROTECTED_RETRY_MAX	  5U
#define SAP_RX_QUEUE_DEPTH	  (CONFIG_BT_SAP_MAX_PEERS * 4U)
#define SAP_RX_THREAD_STACK_SIZE  2048
#define SAP_RX_THREAD_PRIORITY	  7

struct sap_handles {
	uint16_t auth;
	uint16_t auth_ccc;
	uint16_t secure_tx;
	uint16_t secure_tx_ccc;
	uint16_t secure_rx;
};

struct sap_central_peer {
	struct bt_conn *conn;
	struct bt_sap_session *session;
	struct sap_handles handles;
	struct bt_gatt_exchange_params mtu_exchange_params;
	struct bt_gatt_subscribe_params auth_sub_params;
	struct bt_gatt_subscribe_params secure_sub_params;
	struct bt_gatt_read_params protected_read_params;
	struct k_work_delayable protected_work;
	uint16_t protected_status_handle;
	uint8_t protected_attempts;
	uint8_t peer_device_id;
	bool in_use;
	bool mtu_ready;
	bool discovery_ready;
	bool auth_subscribed;
	bool secure_subscribed;
	bool sap_auth_started;
	bool authenticated;
};

struct sap_rx_item {
	struct bt_conn *conn;
	uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN];
	uint16_t len;
	bool secure;
};

static void scan_work_fn(struct k_work *work);
static void protected_work_fn(struct k_work *work);
static void discover_sap(struct sap_central_peer *peer);
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad);

static struct bt_sap_context *sap_ctx;
static struct sap_central_peer peers[CONFIG_BT_SAP_MAX_PEERS];
static struct k_work_delayable scan_work;
static struct bt_conn *conn_connecting;
static bool scanning_active;

static const uint8_t sap_service_uuid[] = {BT_UUID_SAP_SERVICE_VAL};

K_MSGQ_DEFINE(sap_rx_msgq, sizeof(struct sap_rx_item), SAP_RX_QUEUE_DEPTH, sizeof(void *));

static struct sap_central_peer *peer_from_session(struct bt_sap_session *session)
{
	for (size_t i = 0U; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].in_use && peers[i].session == session) {
			return &peers[i];
		}
	}

	return NULL;
}

static struct sap_central_peer *peer_from_event(const struct bt_sap_event *event)
{
	struct sap_central_peer *peer = event == NULL ? NULL : event->user_data;

	if (peer == NULL || !peer->in_use) {
		return NULL;
	}

	return peer;
}

static struct sap_central_peer *peer_from_conn(struct bt_conn *conn)
{
	for (size_t i = 0U; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].in_use && peers[i].conn == conn) {
			return &peers[i];
		}
	}

	return NULL;
}

static int queue_frame_rx(struct bt_conn *conn, const void *data, uint16_t len, bool secure)
{
	struct sap_rx_item item = {
		.len = len,
		.secure = secure,
	};
	int err;

	if (conn == NULL || data == NULL) {
		return -EINVAL;
	}

	if (len > sizeof(item.data)) {
		return -EMSGSIZE;
	}

	item.conn = bt_conn_ref(conn);
	memcpy(item.data, data, len);

	err = k_msgq_put(&sap_rx_msgq, &item, K_NO_WAIT);
	if (err != 0) {
		bt_conn_unref(item.conn);
	}

	return err;
}

static void sap_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct sap_rx_item item;
		struct sap_central_peer *peer;
		int err;

		(void)k_msgq_get(&sap_rx_msgq, &item, K_FOREVER);

		peer = peer_from_conn(item.conn);
		if (peer != NULL && peer->session != NULL) {
			if (item.secure) {
				err = bt_sap_handle_secure_rx(peer->session, item.data, item.len);
			} else {
				err = bt_sap_handle_auth_rx(peer->session, item.data, item.len);
			}

			if (err != 0) {
				LOG_ERR("SAP %s frame rejected (%d)",
					item.secure ? "secure" : "auth", err);
				(void)bt_conn_disconnect(item.conn, BT_HCI_ERR_AUTH_FAIL);
			}
		}

		bt_conn_unref(item.conn);
	}
}

K_THREAD_DEFINE(sap_rx_thread_id, SAP_RX_THREAD_STACK_SIZE, sap_rx_thread, NULL, NULL, NULL,
		SAP_RX_THREAD_PRIORITY, 0, 0);

static size_t active_peer_count(void)
{
	size_t count = 0U;

	for (size_t i = 0U; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].in_use) {
			count++;
		}
	}

	return count;
}

static int central_write_frame(struct sap_central_peer *peer, uint16_t handle, const uint8_t *data,
			       size_t len)
{
	if (peer == NULL || peer->conn == NULL || handle == 0U) {
		return -ENOTCONN;
	}

	if (len > BT_ATT_MAX_ATTRIBUTE_LEN) {
		return -EMSGSIZE;
	}

	return bt_gatt_write_without_response(peer->conn, handle, data, len, false);
}

static int send_auth(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *data,
		     size_t len)
{
	struct sap_central_peer *peer = peer_from_session(session);

	ARG_UNUSED(msg_type);

	if (peer == NULL) {
		return -ENOTCONN;
	}

	return central_write_frame(peer, peer->handles.auth, data, len);
}

static int send_secure(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *data,
		       size_t len)
{
	struct sap_central_peer *peer = peer_from_session(session);

	ARG_UNUSED(msg_type);

	if (peer == NULL) {
		return -ENOTCONN;
	}

	return central_write_frame(peer, peer->handles.secure_rx, data, len);
}

static void secure_payload_received(const struct bt_sap_event *event, uint8_t msg_type,
				    const uint8_t *data, size_t len)
{
	if (msg_type != SAP_DEMO_MSG_TEXT) {
		LOG_INF("Secure payload from peripheral %u type=0x%02x len=%zu",
			event->peer_device_id, msg_type, len);
		return;
	}

	LOG_INF("Secure payload from peripheral %u: %.*s", event->peer_device_id, (int)len,
		(const char *)data);
}

static bool adv_data_matches_sap(struct bt_data *data, void *user_data)
{
	bool *matched = user_data;

	if (data->type != BT_DATA_UUID128_SOME && data->type != BT_DATA_UUID128_ALL) {
		return true;
	}

	for (size_t i = 0U; (i + sizeof(sap_service_uuid)) <= data->data_len;
	     i += sizeof(sap_service_uuid)) {
		if (memcmp(&data->data[i], sap_service_uuid, sizeof(sap_service_uuid)) == 0) {
			*matched = true;
			return false;
		}
	}

	return true;
}

static int assign_char_handle(const struct bt_gatt_dm *dm, const struct bt_uuid *uuid,
			      uint16_t *value_handle, uint16_t *ccc_handle)
{
	const struct bt_gatt_dm_attr *chrc_attr;
	const struct bt_gatt_dm_attr *ccc_attr;
	struct bt_gatt_chrc *chrc;

	chrc_attr = bt_gatt_dm_char_by_uuid(dm, uuid);
	if (chrc_attr == NULL) {
		return -ENOENT;
	}

	chrc = bt_gatt_dm_attr_chrc_val(chrc_attr);
	if (chrc == NULL) {
		return -ENOENT;
	}

	*value_handle = chrc->value_handle;

	if (ccc_handle != NULL) {
		ccc_attr = bt_gatt_dm_desc_by_uuid(dm, chrc_attr, BT_UUID_GATT_CCC);
		if (ccc_attr == NULL) {
			return -ENOENT;
		}

		*ccc_handle = ccc_attr->handle;
	}

	return 0;
}

static int assign_sap_handles(const struct bt_gatt_dm *dm, struct sap_handles *handles)
{
	int err;

	err = assign_char_handle(dm, BT_UUID_SAP_AUTH, &handles->auth, &handles->auth_ccc);
	if (err != 0) {
		return err;
	}

	err = assign_char_handle(dm, BT_UUID_SAP_SECURE_TX, &handles->secure_tx,
				 &handles->secure_tx_ccc);
	if (err != 0) {
		return err;
	}

	return assign_char_handle(dm, BT_UUID_SAP_SECURE_RX, &handles->secure_rx, NULL);
}

static void start_sap_auth_if_ready(struct sap_central_peer *peer)
{
	int err;

	if (peer == NULL || peer->sap_auth_started || peer->session == NULL) {
		return;
	}

	if (!peer->mtu_ready || !peer->discovery_ready || !peer->auth_subscribed ||
	    !peer->secure_subscribed) {
		return;
	}

	peer->sap_auth_started = true;
	err = bt_sap_start(peer->session);
	if (err != 0) {
		LOG_ERR("SAP start failed (%d)", err);
		(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	struct sap_central_peer *peer =
		CONTAINER_OF(params, struct sap_central_peer, mtu_exchange_params);
	uint16_t mtu;

	if (!peer->in_use || peer->conn != conn) {
		return;
	}

	if (err != 0U) {
		LOG_ERR("MTU exchange failed (%u)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	mtu = bt_gatt_get_mtu(conn);
	if (mtu < SAP_REQUIRED_ATT_MTU) {
		LOG_ERR("SAP requires ATT MTU %u, negotiated %u", SAP_REQUIRED_ATT_MTU, mtu);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	peer->mtu_ready = true;
	LOG_INF("ATT MTU %u ready for SAP", mtu);
	discover_sap(peer);
}

static void start_mtu_exchange(struct sap_central_peer *peer)
{
	uint16_t mtu = bt_gatt_get_mtu(peer->conn);
	int err;

	if (mtu >= SAP_REQUIRED_ATT_MTU) {
		peer->mtu_ready = true;
		discover_sap(peer);
		return;
	}

	peer->mtu_exchange_params.func = mtu_exchange_cb;
	err = bt_gatt_exchange_mtu(peer->conn, &peer->mtu_exchange_params);
	if (err == 0) {
		return;
	}

	if (err == -EALREADY) {
		mtu = bt_gatt_get_mtu(peer->conn);
		if (mtu >= SAP_REQUIRED_ATT_MTU) {
			peer->mtu_ready = true;
			discover_sap(peer);
			return;
		}
	}

	LOG_ERR("Failed to start MTU exchange (%d)", err);
	(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static uint8_t auth_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	struct sap_central_peer *peer =
		CONTAINER_OF(params, struct sap_central_peer, auth_sub_params);
	int err;

	if (data == NULL) {
		peer->auth_subscribed = false;
		return BT_GATT_ITER_STOP;
	}

	err = queue_frame_rx(conn, data, length, false);
	if (err != 0) {
		LOG_ERR("Fatal SAP auth RX queue failure (%d)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t secure_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	struct sap_central_peer *peer =
		CONTAINER_OF(params, struct sap_central_peer, secure_sub_params);
	int err;

	if (data == NULL) {
		peer->secure_subscribed = false;
		return BT_GATT_ITER_STOP;
	}

	err = queue_frame_rx(conn, data, length, true);
	if (err != 0) {
		LOG_ERR("Fatal SAP secure RX queue failure (%d)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void auth_subscribe_cb(struct bt_conn *conn, uint8_t err,
			      struct bt_gatt_subscribe_params *params)
{
	struct sap_central_peer *peer =
		CONTAINER_OF(params, struct sap_central_peer, auth_sub_params);

	if (err != 0U) {
		LOG_ERR("SAP auth subscription failed (%u)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	peer->auth_subscribed = true;
	start_sap_auth_if_ready(peer);
}

static void secure_subscribe_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_subscribe_params *params)
{
	struct sap_central_peer *peer =
		CONTAINER_OF(params, struct sap_central_peer, secure_sub_params);

	if (err != 0U) {
		LOG_ERR("SAP secure subscription failed (%u)", err);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	peer->secure_subscribed = true;
	start_sap_auth_if_ready(peer);
}

static void sap_discovered(struct bt_gatt_dm *dm, void *context)
{
	struct sap_central_peer *peer = context;
	int err;

	err = assign_sap_handles(dm, &peer->handles);
	if (err != 0) {
		bt_gatt_dm_data_release(dm);
		LOG_ERR("Failed to assign SAP handles (%d)", err);
		(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	bt_gatt_dm_data_release(dm);
	peer->discovery_ready = true;

	peer->auth_sub_params.notify = auth_notify_cb;
	peer->auth_sub_params.subscribe = auth_subscribe_cb;
	peer->auth_sub_params.value = BT_GATT_CCC_NOTIFY;
	peer->auth_sub_params.value_handle = peer->handles.auth;
	peer->auth_sub_params.ccc_handle = peer->handles.auth_ccc;
	atomic_set_bit(peer->auth_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(peer->conn, &peer->auth_sub_params);
	if (err != 0) {
		LOG_ERR("Failed to subscribe to SAP auth (%d)", err);
		(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	peer->secure_sub_params.notify = secure_notify_cb;
	peer->secure_sub_params.subscribe = secure_subscribe_cb;
	peer->secure_sub_params.value = BT_GATT_CCC_NOTIFY;
	peer->secure_sub_params.value_handle = peer->handles.secure_tx;
	peer->secure_sub_params.ccc_handle = peer->handles.secure_tx_ccc;
	atomic_set_bit(peer->secure_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(peer->conn, &peer->secure_sub_params);
	if (err != 0) {
		LOG_ERR("Failed to subscribe to SAP secure TX (%d)", err);
		(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void sap_service_not_found(struct bt_conn *conn, void *context)
{
	ARG_UNUSED(context);
	LOG_ERR("SAP service not found");
	(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void sap_discovery_error(struct bt_conn *conn, int err, void *context)
{
	ARG_UNUSED(context);
	LOG_ERR("SAP service discovery failed (%d)", err);
	(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static const struct bt_gatt_dm_cb sap_dm_cb = {
	.completed = sap_discovered,
	.service_not_found = sap_service_not_found,
	.error_found = sap_discovery_error,
};

static uint8_t protected_read_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_read_params *params, const void *data,
				 uint16_t length)
{
	ARG_UNUSED(params);

	if (err != 0U) {
		LOG_ERR("Protected service read failed (%u)", err);
		return BT_GATT_ITER_STOP;
	}

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	LOG_INF("Protected service payload: %.*s", length, (const char *)data);

	return BT_GATT_ITER_STOP;
}

static void protected_discovered(struct bt_gatt_dm *dm, void *context)
{
	struct sap_central_peer *peer = context;
	const struct bt_gatt_dm_attr *chrc_attr;
	struct bt_gatt_chrc *chrc;
	int err;

	chrc_attr = bt_gatt_dm_char_by_uuid(dm, BT_UUID_SAP_DEMO_PROTECTED_STATUS);
	if (chrc_attr == NULL) {
		bt_gatt_dm_data_release(dm);
		LOG_ERR("Protected status characteristic not found");
		return;
	}

	chrc = bt_gatt_dm_attr_chrc_val(chrc_attr);
	if (chrc == NULL) {
		bt_gatt_dm_data_release(dm);
		LOG_ERR("Protected status characteristic was malformed");
		return;
	}

	peer->protected_status_handle = chrc->value_handle;
	peer->protected_attempts = 0U;
	bt_gatt_dm_data_release(dm);

	memset(&peer->protected_read_params, 0, sizeof(peer->protected_read_params));
	peer->protected_read_params.func = protected_read_cb;
	peer->protected_read_params.handle_count = 1U;
	peer->protected_read_params.single.handle = peer->protected_status_handle;
	peer->protected_read_params.single.offset = 0U;

	err = bt_gatt_read(peer->conn, &peer->protected_read_params);
	if (err != 0) {
		LOG_ERR("Failed to read protected status (%d)", err);
	}
}

static void protected_service_not_found(struct bt_conn *conn, void *context)
{
	struct sap_central_peer *peer = context;

	ARG_UNUSED(conn);

	if (peer->protected_attempts < SAP_PROTECTED_RETRY_MAX) {
		(void)k_work_reschedule(&peer->protected_work, SAP_PROTECTED_RETRY_DELAY);
		return;
	}

	LOG_WRN("Protected service was not discovered");
}

static void protected_discovery_error(struct bt_conn *conn, int err, void *context)
{
	struct sap_central_peer *peer = context;

	ARG_UNUSED(conn);
	LOG_ERR("Protected service discovery failed (%d)", err);

	if (peer->protected_attempts < SAP_PROTECTED_RETRY_MAX) {
		(void)k_work_reschedule(&peer->protected_work, SAP_PROTECTED_RETRY_DELAY);
	}
}

static const struct bt_gatt_dm_cb protected_dm_cb = {
	.completed = protected_discovered,
	.service_not_found = protected_service_not_found,
	.error_found = protected_discovery_error,
};

static void protected_work_fn(struct k_work *work)
{
	struct sap_central_peer *peer = CONTAINER_OF(k_work_delayable_from_work(work),
						     struct sap_central_peer, protected_work);
	int err;

	if (!peer->in_use || peer->conn == NULL || peer->session == NULL || !peer->authenticated) {
		return;
	}

	peer->protected_attempts++;
	err = bt_gatt_dm_start(peer->conn, BT_UUID_SAP_DEMO_PROTECTED_SERVICE, &protected_dm_cb,
			       peer);
	if (err != 0) {
		LOG_ERR("Failed to start protected service discovery (%d)", err);
		if (peer->protected_attempts < SAP_PROTECTED_RETRY_MAX) {
			(void)k_work_reschedule(&peer->protected_work, SAP_PROTECTED_RETRY_DELAY);
		}
	}
}

static void discover_sap(struct sap_central_peer *peer)
{
	int err;

	err = bt_gatt_dm_start(peer->conn, BT_UUID_SAP_SERVICE, &sap_dm_cb, peer);
	if (err != 0) {
		LOG_ERR("Failed to start SAP discovery (%d)", err);
		(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void send_initial_payload(struct sap_central_peer *peer)
{
	char payload[16];
	int len;
	int err;

	len = snprintk(payload, sizeof(payload), "hello-%u", peer->peer_device_id);
	err = bt_sap_send_secure(peer->session, SAP_DEMO_MSG_TEXT, payload, len);
	if (err != 0) {
		LOG_ERR("Failed to send secure payload (%d)", err);
	}
}

static void sap_authenticated(const struct bt_sap_event *event)
{
	struct sap_central_peer *peer = peer_from_event(event);

	if (peer == NULL || !peer->in_use) {
		return;
	}

	peer->authenticated = true;
	peer->peer_device_id = event->peer_device_id;
	LOG_INF("SAP authenticated with peripheral %u", event->peer_device_id);
	send_initial_payload(peer);
	(void)k_work_reschedule(&peer->protected_work, SAP_PROTECTED_RETRY_DELAY);
}

static void sap_failed(const struct bt_sap_event *event)
{
	struct sap_central_peer *peer = peer_from_event(event);

	if (peer == NULL || !peer->in_use) {
		return;
	}

	peer->authenticated = false;
	LOG_ERR("SAP authentication failed for peripheral %u (%d)", event->peer_device_id,
		event->reason);
	if (peer->conn != NULL) {
		(void)bt_conn_disconnect(peer->conn, BT_HCI_ERR_AUTH_FAIL);
	}
}

static void peer_clear(struct sap_central_peer *peer)
{
	struct k_work_sync sync;

	(void)k_work_cancel_delayable_sync(&peer->protected_work, &sync);

	if (peer->conn != NULL) {
		bt_conn_unref(peer->conn);
	}

	memset(peer, 0, sizeof(*peer));
}

static bool conn_is_central(struct bt_conn *conn)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) != 0) {
		return false;
	}

	return info.role == BT_CONN_ROLE_CENTRAL;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct sap_central_peer *peer = NULL;
	bool was_connecting = (conn == conn_connecting);

	if (was_connecting) {
		bt_conn_unref(conn_connecting);
		conn_connecting = NULL;
	}

	if (!was_connecting && !conn_is_central(conn)) {
		return;
	}

	if (err != 0U) {
		LOG_ERR("Connection failed (0x%02x)", err);
		(void)k_work_reschedule(&scan_work, SAP_SCAN_RETRY_DELAY);
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(peers); i++) {
		if (!peers[i].in_use) {
			peer = &peers[i];
			memset(peer, 0, sizeof(*peer));
			peer->in_use = true;
			peer->conn = bt_conn_ref(conn);
			k_work_init_delayable(&peer->protected_work, protected_work_fn);
			break;
		}
	}

	if (peer == NULL) {
		LOG_ERR("No peer slots left");
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		return;
	}

	peer->session = bt_sap_on_connected(sap_ctx, conn);
	if (peer->session == NULL) {
		LOG_ERR("No SAP session available");
		peer_clear(peer);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		return;
	}

	bt_sap_session_set_user_data(peer->session, peer);

	LOG_INF("Peripheral connected");

	start_mtu_exchange(peer);
	(void)k_work_reschedule(&scan_work, SAP_SCAN_RETRY_DELAY);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct sap_central_peer *peer = peer_from_conn(conn);

	if (conn == conn_connecting) {
		bt_conn_unref(conn_connecting);
		conn_connecting = NULL;
	}

	if (peer == NULL) {
		(void)k_work_reschedule(&scan_work, SAP_SCAN_RETRY_DELAY);
		return;
	}

	LOG_INF("Peripheral disconnected (reason %u)", reason);

	if (peer->session != NULL) {
		bt_sap_on_disconnected(sap_ctx, conn);
	}

	peer_clear(peer);
	(void)k_work_reschedule(&scan_work, SAP_SCAN_RETRY_DELAY);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_conn *existing;
	char addr_str[BT_ADDR_LE_STR_LEN];
	bool matched = false;
	int err;

	if (conn_connecting != NULL || active_peer_count() >= CONFIG_BT_SAP_MAX_PEERS) {
		return;
	}

	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND &&
	    type != BT_GAP_ADV_TYPE_EXT_ADV) {
		return;
	}

	bt_data_parse(ad, adv_data_matches_sap, &matched);
	if (!matched) {
		return;
	}

	existing = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (existing != NULL) {
		bt_conn_unref(existing);
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	LOG_INF("Found SAP peripheral %s RSSI %d", addr_str, rssi);

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_ERR("Failed to stop scanning (%d)", err);
		return;
	}

	scanning_active = false;

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&conn_connecting);
	if (err != 0) {
		LOG_ERR("Create connection failed (%d)", err);
		conn_connecting = NULL;
		(void)k_work_reschedule(&scan_work, SAP_SCAN_RETRY_DELAY);
	}
}

static void scan_work_fn(struct k_work *work)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	ARG_UNUSED(work);

	if (scanning_active || conn_connecting != NULL ||
	    active_peer_count() >= CONFIG_BT_SAP_MAX_PEERS) {
		return;
	}

	err = bt_le_scan_start(&scan_param, device_found);
	if (err == -EALREADY) {
		scanning_active = true;
		return;
	}

	if (err != 0) {
		LOG_ERR("Scan start failed (%d)", err);
		(void)k_work_reschedule(&scan_work, SAP_SCAN_RETRY_DELAY);
		return;
	}

	scanning_active = true;
	LOG_INF("Scanning started");
}

int sap_central_run(const struct bt_sap_policy *policy)
{
	struct bt_sap_cb callbacks = {
		.send_auth = send_auth,
		.send_secure = send_secure,
		.secure_payload_received = secure_payload_received,
		.authenticated = sap_authenticated,
		.failed = sap_failed,
	};
	int err;

	k_work_init_delayable(&scan_work, scan_work_fn);

	err = bt_sap_init(&sap_ctx, SAP_ROLE_CENTRAL, policy, &callbacks);
	if (err != 0) {
		LOG_ERR("SAP init failed (%d)", err);
		return err;
	}

	(void)k_work_reschedule(&scan_work, K_NO_WAIT);

	return 0;
}
