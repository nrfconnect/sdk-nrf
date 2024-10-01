/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Memfault Diagnostic GATT Service (MDS)
 */

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/mds.h>

#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>

#include <memfault/config.h>
#include <memfault/core/platform/device_info.h>
#include <memfault/core/data_packetizer.h>

LOG_MODULE_REGISTER(mds, CONFIG_BT_MDS_LOG_LEVEL);

#ifndef CONFIG_BT_MDS_PERM_RW
#define CONFIG_BT_MDS_PERM_RW 0
#endif
#ifndef CONFIG_BT_MDS_PERM_RW_ENCRYPT
#define CONFIG_BT_MDS_PERM_RW_ENCRYPT 0
#endif
#ifndef CONFIG_BT_MDS_PERM_RW_AUTHEN
#define CONFIG_BT_MDS_PERM_RW_AUTHEN 0
#endif

#define MDS_GATT_PERM_READ (                                                \
	CONFIG_BT_MDS_PERM_RW_AUTHEN ?                                      \
		BT_GATT_PERM_READ_AUTHEN : (CONFIG_BT_MDS_PERM_RW_ENCRYPT ? \
			BT_GATT_PERM_READ_ENCRYPT : BT_GATT_PERM_READ)      \
	)

#define MDS_GATT_PERM_WRITE (                                                \
	CONFIG_BT_MDS_PERM_RW_AUTHEN ?                                       \
		BT_GATT_PERM_WRITE_AUTHEN : (CONFIG_BT_MDS_PERM_RW_ENCRYPT ? \
			BT_GATT_PERM_WRITE_ENCRYPT : BT_GATT_PERM_WRITE)     \
	)

#define MDS_MAX_URI_LENGTH CONFIG_BT_MDS_MAX_URI_LENGTH

#define MDS_URI_BASE \
	MEMFAULT_HTTP_APIS_DEFAULT_SCHEME "://" MEMFAULT_HTTP_CHUNKS_API_HOST "/api/v0/chunks/"

#define MDS_AUTH_KEY "Memfault-Project-Key:" CONFIG_MEMFAULT_NCS_PROJECT_KEY

#define DATA_POLL_INTERVAL CONFIG_BT_MDS_DATA_POLL_INTERVAL

/* Memfault chunk number maximum value. Chunk number should overlaps after reaching this value. */
#define MDS_CHUNK_NUMBER_MAX_VALUE BIT_MASK(5)

#define MAX_PIPELINE CONFIG_BT_MDS_PIPELINE_COUNT

#define STREAM_ENABLED BIT(0)

/* Application error code defined by the MDS.
 * According to Bluetooth Core Specification, Vol 3, Part F, Section 3.4.1.
 */
enum mds_att_error {
	MDS_ATT_ERROR_CLIENT_ALREADY_SUBSCRIBED = 0x80,
	MDS_ATT_ERROR_CLIENT_NOT_SUBSCRIBED     = 0x81
};

enum data_export_mode {
	DATA_EXPORT_MODE_STREAMING_DISABLE = 0x00,
	DATA_EXPORT_MODE_STREAMING_ENABLE  = 0x01
};

enum mds_read_char {
	MDS_READ_CHAR_SUPPORTED_FEATURES,
	MDS_READ_CHAR_DEVICE_IDENTIFIER,
	MDS_READ_CHAR_DATA_URI,
	MDS_READ_CHAR_AUTHORIZATION
};
struct mds {
	const struct bt_mds_cb *cb;
	struct bt_conn *conn;
	atomic_t send_cnt;
	atomic_t stream_state;
	uint8_t chunk_number;
};

struct mds_data_export_nfy {
	uint8_t chunk_number:5;
	uint8_t rfu:3;
	uint8_t data[];
};

struct mds_subscription {
	bool subscribed;
	const struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
};

static struct mds mds_instance = {
	.send_cnt = ATOMIC_INIT(MAX_PIPELINE)
};

static void mds_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(mds_work, mds_work_handler);

/* System workqueue thread cannot be preempted to ensure proper service operations.
 * Make sure here that system workqueue is a cooperative thread.
 */
BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0);


static ssize_t supported_feature_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	static const uint8_t supported_features;

	LOG_DBG("MDS supported feature read, handle: %u, conn: %p", bt_gatt_attr_get_handle(attr),
		(void *)conn);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &supported_features,
				 sizeof(supported_features));
}

static ssize_t device_identifier_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len, uint16_t offset)
{
	sMemfaultDeviceInfo info;
	size_t device_identifier_length;

	LOG_DBG("MDS Device Identifier characteristic read, handle: %u, conn: %p",
		bt_gatt_attr_get_handle(attr), (void *)conn);

	memfault_platform_get_device_info(&info);

	device_identifier_length = strlen(info.device_serial);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, info.device_serial,
				 device_identifier_length);
}

static ssize_t data_uri_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	sMemfaultDeviceInfo info;
	char uri[MDS_MAX_URI_LENGTH];
	size_t uri_base_length = strlen(MDS_URI_BASE);
	size_t uri_sn_length;
	size_t uri_length;

	LOG_DBG("MDS Data URI characteristic read, handle: %u, conn: %p",
		bt_gatt_attr_get_handle(attr), (void *)conn);

	memfault_platform_get_device_info(&info);

	uri_sn_length = strlen(info.device_serial);
	uri_length = uri_base_length + uri_sn_length;

	if (uri_length > sizeof(uri)) {
		LOG_ERR("Too long URI");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(uri, MDS_URI_BASE, uri_base_length);
	memcpy(&uri[uri_base_length], info.device_serial, uri_sn_length);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, uri,
				 uri_length);
}

static ssize_t authorization_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	static const char *auth_key = MDS_AUTH_KEY;
	size_t auth_key_len = strlen(auth_key);

	LOG_DBG("MDS Authorization characteristic read, handle: %u, conn: %p",
		bt_gatt_attr_get_handle(attr), (void *)conn);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, auth_key, auth_key_len);
}

static ssize_t mds_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	ssize_t ret;
	enum mds_read_char characteristic = (enum mds_read_char)attr->user_data;

	if (mds_instance.cb && mds_instance.cb->access_enable) {
		if (!mds_instance.cb->access_enable(conn)) {
			return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
		}
	}

	switch (characteristic) {
	case MDS_READ_CHAR_SUPPORTED_FEATURES:
		ret = supported_feature_read(conn, attr, buf, len, offset);
		break;

	case MDS_READ_CHAR_DEVICE_IDENTIFIER:
		ret = device_identifier_read(conn, attr, buf, len, offset);
		break;

	case MDS_READ_CHAR_DATA_URI:
		ret = data_uri_read(conn, attr, buf, len, offset);
		break;

	case MDS_READ_CHAR_AUTHORIZATION:
		ret = authorization_read(conn, attr, buf, len, offset);
		break;

	default:
		ret = BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		break;
	}

	return ret;
}

static void stream_enable(struct bt_conn *conn)
{
	/* Check if stream is already enabled. */
	if (atomic_test_and_set_bit(&mds_instance.stream_state, STREAM_ENABLED)) {
		return;
	}

	if (!mds_instance.conn) {
		mds_instance.conn = conn;
	}

	k_work_schedule(&mds_work, K_NO_WAIT);
}

static void stream_disable(struct bt_conn *conn)
{
	atomic_clear_bit(&mds_instance.stream_state, STREAM_ENABLED);

	/* This context cannot be preempted by the system workqueue so we can cancel workqueue here
	 * without checking a return value.
	 */
	(void)k_work_cancel_delayable(&mds_work);
}

static bool stream_mode_handler(struct bt_conn *conn, enum data_export_mode stream_mode)
{
	bool valid = true;

	switch (stream_mode) {
	case DATA_EXPORT_MODE_STREAMING_ENABLE:
		stream_enable(conn);
		break;

	case DATA_EXPORT_MODE_STREAMING_DISABLE:
		stream_disable(conn);
		break;

	default:
		valid = false;
		break;
	}

	return valid;
}

static ssize_t data_export_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	enum data_export_mode mode;

	LOG_DBG("MDS Data Export characteristic write, handle %u, conn: %p",
		bt_gatt_attr_get_handle(attr), (void *)conn);

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		LOG_DBG("MDS Data Export notification are disabled");
		return BT_GATT_ERR(MDS_ATT_ERROR_CLIENT_NOT_SUBSCRIBED);
	}

	if (mds_instance.cb && mds_instance.cb->access_enable) {
		if (!mds_instance.cb->access_enable(conn)) {
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
		}
	}

	if (offset != 0) {
		LOG_WRN("MDS Data Export invalid write offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(uint8_t)) {
		LOG_WRN("MDS Data Export invalid attribute length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	mode = (enum data_export_mode)(((uint8_t *)buf)[0]);

	if (!stream_mode_handler(conn, mode)) {
		LOG_WRN("MDS Data Export characteristic write invalid value");
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}

	return len;
}

static void data_export_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("MDS Data Export CCCD changed, handle: %u, value: 0x%04X",
		bt_gatt_attr_get_handle(attr), value);
}

static void conn_subscription_check(struct bt_conn *conn, void *user_data)
{
	struct mds_subscription *mds_subscription = user_data;

	if (bt_gatt_is_subscribed(conn, mds_subscription->attr, BT_GATT_CCC_NOTIFY)) {
		mds_subscription->conn = conn;
		mds_subscription->subscribed = true;
	}
}

static ssize_t data_export_ccc_write(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, uint16_t value)
{
	struct mds_subscription mds_subscription = {
		.attr = attr
	};

	if ((value != BT_GATT_CCC_NOTIFY) && (value != 0)) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	/* Allow only one subscription to the Memfault Data Export characteristic. */
	bt_conn_foreach(BT_CONN_TYPE_LE, conn_subscription_check, &mds_subscription);

	if (mds_subscription.subscribed && (mds_subscription.conn != conn)) {
		LOG_WRN("Memfault Data Export characteristic is already subscribed");
		return BT_GATT_ERR(MDS_ATT_ERROR_CLIENT_ALREADY_SUBSCRIBED);
	}

	if (mds_instance.cb && mds_instance.cb->access_enable) {
		if (!mds_instance.cb->access_enable(conn)) {
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
		}
	}

	if ((value == 0) && (conn == mds_instance.conn)) {
		stream_disable(conn);
	}

	return sizeof(value);
}

static struct _bt_gatt_ccc mds_data_export_ccc = BT_GATT_CCC_INITIALIZER(data_export_ccc_changed,
									 data_export_ccc_write,
									 NULL);

BT_GATT_SERVICE_DEFINE(mds_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_MEMFAULT_DIAG),
	BT_GATT_CHARACTERISTIC(BT_UUID_MDS_SUPPORTED_FEATURES, BT_GATT_CHRC_READ,
			       MDS_GATT_PERM_READ, mds_read, NULL,
			       (void *)MDS_READ_CHAR_SUPPORTED_FEATURES),
	BT_GATT_CHARACTERISTIC(BT_UUID_MDS_DEVICE_IDENTIFIER, BT_GATT_CHRC_READ,
			       MDS_GATT_PERM_READ, mds_read, NULL,
			       (void *)MDS_READ_CHAR_DEVICE_IDENTIFIER),
	BT_GATT_CHARACTERISTIC(BT_UUID_MDS_DATA_URI, BT_GATT_CHRC_READ,
			       MDS_GATT_PERM_READ, mds_read, NULL, (void *)MDS_READ_CHAR_DATA_URI),
	BT_GATT_CHARACTERISTIC(BT_UUID_MDS_AUTHORIZATION, BT_GATT_CHRC_READ,
			       MDS_GATT_PERM_READ, mds_read, NULL,
			       (void *)MDS_READ_CHAR_AUTHORIZATION),
	BT_GATT_CHARACTERISTIC(BT_UUID_MDS_DATA_EXPORT, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
			       MDS_GATT_PERM_WRITE, NULL, data_export_write,
			       NULL),
	BT_GATT_CCC_MANAGED(&mds_data_export_ccc, (MDS_GATT_PERM_READ | MDS_GATT_PERM_WRITE))
);

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn == mds_instance.conn) {
		stream_disable(conn);

		/* Clean up connection */
		mds_instance.conn = NULL;
		mds_instance.chunk_number = 0;

		atomic_set(&mds_instance.send_cnt, MAX_PIPELINE);

		/* It is called from a workqueue context, so checking the error codes is no needed,
		 * here.
		 */
		k_work_cancel_delayable(&mds_work);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static void mds_sent_cb(struct bt_conn *conn, void *user_data)
{
	atomic_inc(&mds_instance.send_cnt);
	k_work_reschedule(&mds_work, K_NO_WAIT);
}

static size_t chunk_data_length_get(struct bt_conn *conn)
{
	static const size_t att_header_length = 0x03;

	size_t length;

	if (!conn) {
		return 0;
	}

	length = bt_gatt_get_mtu(conn);
	if (length < (att_header_length + sizeof(struct mds_data_export_nfy))) {
		LOG_ERR("MTU value too low: %d or link is disconnected", length);
		return 0;
	}

	/* According to Bluetooth Core Specification (Vol 3, Part F, Section 3.4.7.1),
	 * maximum supported length of the notification is (ATT_MTU - 3).
	 */
	length -= att_header_length;
	length -= sizeof(struct mds_data_export_nfy);

	return length;
}

static int chunk_send(struct bt_conn *conn, struct net_buf_simple *buf)
{
	struct bt_gatt_notify_params params = {0};
	static struct bt_gatt_attr *attr;

	__ASSERT(conn, "Invalid parameters");
	__ASSERT(buf, "Invalid parameters");

	if (!attr) {
		attr = bt_gatt_find_by_uuid(mds_svc.attrs, mds_svc.attr_count,
					    BT_UUID_MDS_DATA_EXPORT);
	}

	__ASSERT_NO_MSG(attr);

	params.attr = attr;
	params.data = buf->data;
	params.len = buf->len;
	params.func = mds_sent_cb;

	return bt_gatt_notify_cb(conn, &params);
}

static uint8_t chunk_number_update(uint8_t chunk_number)
{
	return ++chunk_number & MDS_CHUNK_NUMBER_MAX_VALUE;
}

static int mds_data_send(struct bt_conn *conn)
{
	int err;
	bool data_available;
	struct mds_data_export_nfy *data_export_nfy;
	size_t chunk_max_size = chunk_data_length_get(mds_instance.conn);
	size_t chunk_size = chunk_max_size;

	NET_BUF_SIMPLE_DEFINE(buf, (sizeof(struct mds_data_export_nfy) + chunk_max_size));

	data_export_nfy = net_buf_simple_add(&buf,
					sizeof(struct mds_data_export_nfy) + chunk_max_size);
	if (!data_export_nfy) {
		LOG_ERR("Cannot allocate place for memfault chunk data");
		return -ENOMEM;
	}

	data_available = memfault_packetizer_get_chunk(data_export_nfy->data, &chunk_size);

	if (data_available) {
		data_export_nfy->chunk_number = mds_instance.chunk_number;
		data_export_nfy->rfu = 0;

		/* If chunk size is smaller than maximum chunk size then shrink the buffer to actual
		 * packet size.
		 */
		(void)net_buf_simple_remove_mem(&buf, (chunk_max_size - chunk_size));

		err = chunk_send(conn, &buf);
		if (err) {
			memfault_packetizer_abort();
			LOG_WRN("Failed to send Memfault diagnostic chunk, err %d", err);

			return err;
		}

		LOG_DBG("Memfault diagnostic data chunk %d successfully sent",
			mds_instance.chunk_number);
		mds_instance.chunk_number = chunk_number_update(mds_instance.chunk_number);

		return chunk_size;
	}

	return 0;
}

static void mds_work_handler(struct k_work *work)
{
	int err;
	bool data_available;

	if (!atomic_test_bit(&mds_instance.stream_state, STREAM_ENABLED)) {
		return;
	}

	err = mds_data_send(mds_instance.conn);
	if (err > 0) {
		atomic_dec(&mds_instance.send_cnt);
	}

	data_available = memfault_packetizer_data_available();
	if (data_available) {
		if (atomic_get(&mds_instance.send_cnt)) {
			k_work_reschedule(&mds_work, K_NO_WAIT);
		}

		return;
	}

	/* Reschedule the workqueue here to check if there is any new Memfault data. */
	k_work_reschedule(&mds_work, K_MSEC(DATA_POLL_INTERVAL));
}

int bt_mds_cb_register(const struct bt_mds_cb *cb)
{
	if (mds_instance.cb) {
		return -EALREADY;
	}

	mds_instance.cb = cb;

	return 0;
}
