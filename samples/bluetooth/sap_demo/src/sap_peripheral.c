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
#include <zephyr/sys/printk.h>

#include <bluetooth/services/sap.h>

#include "demo_protocol.h"

LOG_MODULE_REGISTER(sap_peripheral, CONFIG_BT_SAP_LOG_LEVEL);

#define SAP_ADV_RETRY_DELAY	 K_MSEC(500)
#define SAP_RX_QUEUE_DEPTH	 4U
#define SAP_RX_THREAD_STACK_SIZE 2048
#define SAP_RX_THREAD_PRIORITY	 7
#define SAP_GATT_WRITE_PERM	 BT_GATT_PERM_WRITE

struct sap_rx_item {
	struct bt_conn *conn;
	uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN];
	uint16_t len;
	bool secure;
};

static ssize_t sap_auth_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t sap_secure_rx_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t protected_status_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     void *buf, uint16_t len, uint16_t offset);
static void advertising_start(void);

static struct bt_sap_context *sap_ctx;
static struct bt_conn *active_conn;
static struct bt_sap_session *active_session;
static struct k_work_delayable advertising_work;
static uint8_t local_device_id;
static bool auth_notify_enabled;
static bool secure_notify_enabled;
static bool protected_registered;

K_MSGQ_DEFINE(sap_rx_msgq, sizeof(struct sap_rx_item), SAP_RX_QUEUE_DEPTH, sizeof(void *));

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SAP_SERVICE_VAL),
};

static void auth_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	auth_notify_enabled = (value & BT_GATT_CCC_NOTIFY) != 0U;
}

static void secure_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	secure_notify_enabled = (value & BT_GATT_CCC_NOTIFY) != 0U;
}

BT_GATT_SERVICE_DEFINE(sap_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_SAP_SERVICE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_SAP_AUTH,
					      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY,
					      SAP_GATT_WRITE_PERM, NULL, sap_auth_write, NULL),
		       BT_GATT_CCC(auth_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_SAP_SECURE_TX, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(secure_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_SAP_SECURE_RX,
					      BT_GATT_CHRC_WRITE_WITHOUT_RESP, SAP_GATT_WRITE_PERM,
					      NULL, sap_secure_rx_write, NULL));

enum sap_svc_attr_index {
	SAP_SVC_ATTR_AUTH_VALUE = 2,
	SAP_SVC_ATTR_SECURE_TX_VALUE = 5,
};

static struct bt_gatt_attr protected_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_SAP_DEMO_PROTECTED_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_SAP_DEMO_PROTECTED_STATUS, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, protected_status_read, NULL, NULL),
};

static struct bt_gatt_service protected_svc = BT_GATT_SERVICE(protected_attrs);

static int peripheral_notify_frame(struct bt_sap_session *session, const struct bt_gatt_attr *attr,
				   const uint8_t *data, size_t len)
{
	if (active_conn == NULL || session == NULL || session != active_session) {
		return -ENOTCONN;
	}

	if (len > BT_ATT_MAX_ATTRIBUTE_LEN) {
		return -EMSGSIZE;
	}

	return bt_gatt_notify(active_conn, attr, data, len);
}

static int send_auth(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *data,
		     size_t len)
{
	ARG_UNUSED(msg_type);

	if (!auth_notify_enabled) {
		return -EAGAIN;
	}

	return peripheral_notify_frame(session, &sap_svc.attrs[SAP_SVC_ATTR_AUTH_VALUE], data, len);
}

static int send_secure(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *data,
		       size_t len)
{
	ARG_UNUSED(msg_type);

	if (!secure_notify_enabled) {
		return -EAGAIN;
	}

	return peripheral_notify_frame(session, &sap_svc.attrs[SAP_SVC_ATTR_SECURE_TX_VALUE], data,
				       len);
}

static void secure_payload_received(const struct bt_sap_event *event, uint8_t msg_type,
				    const uint8_t *data, size_t len)
{
	if (msg_type != SAP_DEMO_MSG_TEXT) {
		LOG_INF("Secure payload from central %u type=0x%02x len=%zu", event->peer_device_id,
			msg_type, len);
		return;
	}

	LOG_INF("Secure payload from central: %.*s", (int)len, (const char *)data);
}

static struct bt_sap_session *session_for_conn(struct bt_conn *conn)
{
	if (active_session == NULL || active_conn == NULL || conn != active_conn) {
		return NULL;
	}

	return active_session;
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
		struct bt_sap_session *session;
		int err;

		(void)k_msgq_get(&sap_rx_msgq, &item, K_FOREVER);

		session = session_for_conn(item.conn);
		if (session != NULL) {
			err = item.secure ? bt_sap_handle_secure_rx(session, item.data, item.len)
					  : bt_sap_handle_auth_rx(session, item.data, item.len);
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

static ssize_t handle_frame_rx(struct bt_conn *conn, const void *buf, uint16_t len, bool secure)
{
	struct bt_sap_session *session = session_for_conn(conn);
	int err;

	if (session == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	err = queue_frame_rx(conn, buf, len, secure);
	if (err != 0) {
		if (err == -ENOMSG) {
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		if (err == -EMSGSIZE) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	return len;
}

static ssize_t sap_auth_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return handle_frame_rx(conn, buf, len, false);
}

static ssize_t sap_secure_rx_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return handle_frame_rx(conn, buf, len, true);
}

static ssize_t protected_status_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     void *buf, uint16_t len, uint16_t offset)
{
	char status[32];
	int status_len;

	ARG_UNUSED(attr);

	status_len = snprintk(status, sizeof(status), "peripheral-%u-ready-p1", local_device_id);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, status, status_len);
}

static void protected_service_disable(void)
{
	if (!protected_registered) {
		return;
	}

	(void)bt_gatt_service_unregister(&protected_svc);
	protected_registered = false;
	LOG_INF("Protected service unregistered");
}

static void protected_service_enable(void)
{
	int err;

	if (protected_registered) {
		return;
	}

	err = bt_gatt_service_register(&protected_svc);
	if (err != 0) {
		LOG_ERR("Failed to register protected service (%d)", err);
		return;
	}

	protected_registered = true;
	LOG_INF("Protected service registered");
}

static void send_initial_payload(void)
{
	char payload[32];
	int len;
	int err;

	if (active_session == NULL) {
		return;
	}

	len = snprintk(payload, sizeof(payload), "peripheral-%u-ready-p1", local_device_id);
	err = bt_sap_send_secure(active_session, SAP_DEMO_MSG_TEXT, payload, len);
	if (err != 0) {
		LOG_ERR("Failed to send secure payload (%d)", err);
	}
}

static bool sap_event_matches_active_session(const struct bt_sap_event *event)
{
	return event->role == SAP_ROLE_PERIPHERAL && active_session != NULL &&
	       event->user_data == active_session;
}

static void sap_authenticated(const struct bt_sap_event *event)
{
	if (!sap_event_matches_active_session(event)) {
		return;
	}

	LOG_INF("SAP authenticated with central %u", event->peer_device_id);
	protected_service_enable();
	send_initial_payload();
}

static void sap_failed(const struct bt_sap_event *event)
{
	if (!sap_event_matches_active_session(event)) {
		return;
	}

	LOG_ERR("SAP authentication failed (%d)", event->reason);
	if (active_conn != NULL) {
		(void)bt_conn_disconnect(active_conn, BT_HCI_ERR_AUTH_FAIL);
	}
}

static void sap_disconnected(const struct bt_sap_event *event)
{
	if (!sap_event_matches_active_session(event)) {
		return;
	}

	protected_service_disable();
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		LOG_ERR("Connection failed (%u)", err);
		(void)k_work_reschedule(&advertising_work, SAP_ADV_RETRY_DELAY);
		return;
	}

	if (active_conn != NULL) {
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		return;
	}

	active_conn = bt_conn_ref(conn);
	active_session = bt_sap_on_connected(sap_ctx, conn);
	if (active_session == NULL) {
		LOG_ERR("No SAP session available");
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	bt_sap_session_set_user_data(active_session, active_session);

	LOG_INF("Central connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != active_conn) {
		return;
	}

	LOG_INF("Central disconnected (reason %u)", reason);

	protected_service_disable();

	bt_sap_on_disconnected(sap_ctx, conn);
	active_session = NULL;

	bt_conn_unref(active_conn);
	active_conn = NULL;
	auth_notify_enabled = false;
	secure_notify_enabled = false;

	(void)k_work_reschedule(&advertising_work, K_NO_WAIT);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void advertising_start(void)
{
	int err;

	if (active_conn != NULL) {
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err == -EALREADY) {
		return;
	}

	if (err != 0) {
		LOG_ERR("Advertising failed to start (%d)", err);
		(void)k_work_reschedule(&advertising_work, SAP_ADV_RETRY_DELAY);
		return;
	}

	LOG_INF("Advertising started");
}

static void advertising_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	advertising_start();
}

int sap_peripheral_run(const struct bt_sap_policy *policy)
{
	struct bt_sap_cb callbacks = {
		.send_auth = send_auth,
		.send_secure = send_secure,
		.secure_payload_received = secure_payload_received,
		.authenticated = sap_authenticated,
		.failed = sap_failed,
		.disconnected = sap_disconnected,
	};
	int err;

	local_device_id = policy->local_credential->cert.body.device_id;
	k_work_init_delayable(&advertising_work, advertising_work_fn);

	err = bt_sap_init(&sap_ctx, SAP_ROLE_PERIPHERAL, policy, &callbacks);
	if (err != 0) {
		LOG_ERR("SAP init failed (%d)", err);
		return err;
	}

	advertising_start();

	return 0;
}
