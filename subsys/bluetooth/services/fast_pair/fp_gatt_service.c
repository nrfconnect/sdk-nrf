/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/rand32.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_registration_data.h"
#include "fp_common.h"
#include "fp_crypto.h"
#include "fp_keys.h"
#include "fp_auth.h"

/* Fast Pair GATT Service UUIDs defined by the Fast Pair specification. */
#define BT_UUID_FAST_PAIR		BT_UUID_DECLARE_16(FP_SERVICE_UUID)
#define BT_UUID_FAST_PAIR_MODEL_ID \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1233, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))
#define BT_UUID_FAST_PAIR_KEY_BASED_PAIRING \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1234, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))
#define BT_UUID_FAST_PAIR_PASSKEY \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1235, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))
#define BT_UUID_FAST_PAIR_ACCOUNT_KEY \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1236, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))

/* Make sure that MTU value of at least 83 is used (recommended by the Fast Pair specification). */
#define FP_RECOMMENDED_MTU	83
BUILD_ASSERT(BT_L2CAP_TX_MTU >= FP_RECOMMENDED_MTU);
BUILD_ASSERT(BT_L2CAP_RX_MTU >= FP_RECOMMENDED_MTU);

#define MSB_BIT(n)		BIT(7 - n)

/* Fast Pair message type. */
enum fp_msg_type {
	/* Key-based Pairing Request. */
	FP_MSG_KEY_BASED_PAIRING_REQ    = 0x00,

	/* Key-based Pairing Response. */
	FP_MSG_KEY_BASED_PAIRING_RSP    = 0x01,

	/* Seeker's Passkey. */
	FP_MSG_SEEKERS_PASSKEY          = 0x02,

	/* Provider's Passkey. */
	FP_MSG_PROVIDERS_PASSKEY        = 0x03,

	/* Action request. */
	FP_MSG_ACTION_REQ               = 0x10,
};

struct msg_key_based_pairing_req {
	uint8_t msg_type;
	uint8_t fp_flags;
	uint8_t provider_address[BT_ADDR_SIZE];
	uint8_t seeker_address[BT_ADDR_SIZE];
};

struct msg_seekers_passkey {
	uint8_t msg_type;
	uint32_t passkey;
};


static int fp_gatt_rsp_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      struct net_buf_simple *rsp)
{
	uint8_t rsp_enc[FP_CRYPTO_AES128_BLOCK_LEN];
	uint8_t *salt;
	size_t salt_len;
	int err = 0;

	if (net_buf_simple_max_len(rsp) != FP_CRYPTO_AES128_BLOCK_LEN) {
		LOG_ERR("Unsupported response size %zu", net_buf_simple_max_len(rsp));
		return -ENOTSUP;
	}

	salt_len = net_buf_simple_tailroom(rsp);
	salt = net_buf_simple_add(rsp, salt_len);

	/* Fill remaining part of the response with random salt. */
	err = sys_csrand_get(salt, salt_len);
	if (err) {
		LOG_WRN("Failed to generate salt for GATT response: err=%d", err);
		return err;
	}

	err = fp_keys_encrypt(conn, rsp_enc,
			      net_buf_simple_pull_mem(rsp, FP_CRYPTO_AES128_BLOCK_LEN));
	if (err) {
		LOG_WRN("GATT response encrypt failed: err=%d", err);
		return err;
	}

	err = bt_gatt_notify(conn, attr, rsp_enc, sizeof(rsp_enc));
	if (err) {
		LOG_ERR("GATT response notify failed: err=%d", err);
	}

	return err;
}

static ssize_t read_model_id(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	uint8_t model_id[FP_REG_DATA_MODEL_ID_LEN];
	ssize_t res;

	if (!fp_reg_data_get_model_id(model_id, sizeof(model_id))) {
		res = bt_gatt_attr_read(conn, attr, buf, len, offset, model_id, sizeof(model_id));
	} else {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	LOG_DBG("Model ID read: res=%d conn=%p", res, (void *)conn);

	return res;
}

static int parse_key_based_pairing_req(const struct bt_conn *conn,
				       struct msg_key_based_pairing_req *parsed_req,
				       struct net_buf_simple *req)
{
	struct bt_conn_info conn_info;
	uint8_t *provider_address;
	uint8_t *seeker_address;
	int err = 0;

	parsed_req->msg_type = net_buf_simple_pull_u8(req);
	parsed_req->fp_flags = net_buf_simple_pull_u8(req);

	provider_address = net_buf_simple_pull_mem(req, sizeof(parsed_req->provider_address));
	sys_memcpy_swap(parsed_req->provider_address, provider_address,
			sizeof(parsed_req->provider_address));

	seeker_address = net_buf_simple_pull_mem(req, sizeof(parsed_req->seeker_address));
	sys_memcpy_swap(parsed_req->seeker_address, seeker_address,
			sizeof(parsed_req->seeker_address));

	if (parsed_req->msg_type != FP_MSG_KEY_BASED_PAIRING_REQ) {
		return -ENOMSG;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		LOG_ERR("Failed to get local conn info %d", err);
		return err;
	}

	if (memcmp(parsed_req->provider_address, &conn_info.le.local->a.val,
	    sizeof(parsed_req->provider_address))) {
		err = -EINVAL;
	}

	return 0;
}

static int handle_key_based_pairing_req(struct bt_conn *conn,
					struct net_buf_simple *rsp,
					struct msg_key_based_pairing_req *parsed_req)
{
	enum fp_kbp_flags {
		FP_KBP_FLAG_DEPRECATED				= MSB_BIT(0),
		FP_KBP_FLAG_INITIATE_BONDING			= MSB_BIT(1),
		FP_KBP_FLAG_NOTIFY_NAME				= MSB_BIT(2),
		FP_KBP_FLAG_RETROACTIVELY_WRITE_ACCOUNT_KEY	= MSB_BIT(3),
	};

	struct bt_conn_info conn_info;
	bool send_pairing_req = false;
	uint8_t *local_addr = NULL;
	int err = 0;

	if (parsed_req->fp_flags & FP_KBP_FLAG_INITIATE_BONDING) {
		/* Ignore Seeker's BR/EDR Address. */
		ARG_UNUSED(parsed_req->seeker_address);

		send_pairing_req = true;
	}

	if (parsed_req->fp_flags & FP_KBP_FLAG_NOTIFY_NAME) {
		/* The notification will not be sent, do not return error. */
		LOG_WRN("Notify name not supported (Key-based Pairing)");
	}

	if (parsed_req->fp_flags & FP_KBP_FLAG_RETROACTIVELY_WRITE_ACCOUNT_KEY) {
		LOG_WRN("Retroactively write account key not supported (Key-based Pairing)");
		return -ENOTSUP;
	}

	err = fp_auth_start(conn, send_pairing_req);
	if (err) {
		LOG_WRN("Failed to start authentication: err=%d (Key-based Pairing)", err);
		return err;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		LOG_ERR("Failed to get local conn info %d", err);
		return err;
	}

	net_buf_simple_add_u8(rsp, FP_MSG_KEY_BASED_PAIRING_RSP);
	local_addr = net_buf_simple_add_mem(rsp, &conn_info.le.local->a.val, BT_ADDR_SIZE);
	sys_mem_swap(local_addr, BT_ADDR_SIZE);

	return 0;
}

static int validate_key_based_pairing_req(const struct bt_conn *conn, const uint8_t *req,
					  void *context)
{
	struct msg_key_based_pairing_req *parsed_req = context;
	struct net_buf_simple req_net_buf;

	net_buf_simple_init_with_data(&req_net_buf, (uint8_t *)req, FP_CRYPTO_AES128_BLOCK_LEN);
	return parse_key_based_pairing_req(conn, parsed_req, &req_net_buf);
}

static ssize_t write_key_based_pairing(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       const void *buf,
				       uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple gatt_write;
	struct fp_keys_keygen_params keygen_params;
	struct msg_key_based_pairing_req parsed_req;
	int err = 0;
	ssize_t res = len;

	NET_BUF_SIMPLE_DEFINE(rsp, FP_CRYPTO_AES128_BLOCK_LEN);

	if (offset != 0) {
		LOG_WRN("Invalid offset: off=%" PRIu16 " (Key-based Pairing)", offset);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto finish;
	}

	if ((len != (FP_CRYPTO_AES128_BLOCK_LEN + FP_CRYPTO_ECDH_PUBLIC_KEY_LEN)) &&
	    (len != FP_CRYPTO_AES128_BLOCK_LEN)) {
		LOG_WRN("Invalid length: len=%" PRIu16 " (Key-based Pairing)", len);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		goto finish;
	}

	net_buf_simple_init_with_data(&gatt_write, (void *)buf, len);

	keygen_params.req_enc = net_buf_simple_pull_mem(&gatt_write, FP_CRYPTO_AES128_BLOCK_LEN);
	if (net_buf_simple_max_len(&gatt_write) == FP_CRYPTO_ECDH_PUBLIC_KEY_LEN) {
		keygen_params.public_key = net_buf_simple_pull_mem(&gatt_write,
								   FP_CRYPTO_ECDH_PUBLIC_KEY_LEN);
	} else {
		keygen_params.public_key = NULL;
	}
	keygen_params.req_validate_cb = validate_key_based_pairing_req;
	keygen_params.context = &parsed_req;

	err = fp_keys_generate_key(conn, &keygen_params);
	if (err) {
		LOG_WRN("Generating keys failed: err=%d (Key-based Pairing)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = handle_key_based_pairing_req(conn, &rsp, &parsed_req);
	if (err) {
		LOG_WRN("Handling request failed: ret=%d (Key-based Pairing)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = fp_gatt_rsp_notify(conn, attr, &rsp);
	if (err) {
		LOG_WRN("Failed to send response: err=%d (Key-based Pairing)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

finish:
	if (res < 0) {
		fp_keys_drop_key(conn);
	}

	LOG_DBG("Key-based Pairing write: res=%d conn=%p", res, (void *)conn);

	return res;
}

static int parse_passkey_req(struct msg_seekers_passkey *parsed_req, struct net_buf_simple *req)
{
	parsed_req->msg_type = net_buf_simple_pull_u8(req);
	parsed_req->passkey = net_buf_simple_pull_be24(req);

	if (parsed_req->msg_type != FP_MSG_SEEKERS_PASSKEY) {
		LOG_WRN("Invalid message type: %" PRIu8 " (Passkey)", parsed_req->msg_type);
		return -ENOMSG;
	}

	return 0;
}

static int handle_passkey_req(struct bt_conn *conn, struct net_buf_simple *rsp,
			      struct msg_seekers_passkey *parsed_req)

{
	int err;
	uint32_t bt_auth_passkey;

	err = fp_auth_cmp_passkey(conn, parsed_req->passkey, &bt_auth_passkey);
	if (err) {
		LOG_WRN("Passkey comparison failed: err=%d (Passkey)", err);
		return err;
	}

	net_buf_simple_add_u8(rsp, FP_MSG_PROVIDERS_PASSKEY);
	net_buf_simple_add_be24(rsp, bt_auth_passkey);

	return 0;
}

static ssize_t write_passkey(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	struct msg_seekers_passkey parsed_req;
	int err = 0;
	ssize_t res = len;

	NET_BUF_SIMPLE_DEFINE(req, FP_CRYPTO_AES128_BLOCK_LEN);
	NET_BUF_SIMPLE_DEFINE(rsp, FP_CRYPTO_AES128_BLOCK_LEN);

	if (offset != 0) {
		LOG_WRN("Invalid offset: off=%" PRIu16 " (Passkey)", offset);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto finish;
	}

	if (len != FP_CRYPTO_AES128_BLOCK_LEN) {
		LOG_WRN("Invalid length: len=%" PRIu16 " (Key-based Pairing)", len);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		goto finish;
	}

	err = fp_keys_decrypt(conn, net_buf_simple_add(&req, FP_CRYPTO_AES128_BLOCK_LEN), buf);
	if (err) {
		LOG_WRN("Decrypt failed: err=%d (Passkey)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = parse_passkey_req(&parsed_req, &req);
	if (err) {
		res = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		goto finish;
	}

	err = handle_passkey_req(conn, &rsp, &parsed_req);
	if (err) {
		LOG_WRN("Handling request failed: err=%d (Passkey)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = fp_gatt_rsp_notify(conn, attr, &rsp);
	if (err) {
		LOG_WRN("Failed to send response: err=%d (Passkey)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

finish:
	if (res < 0) {
		fp_keys_drop_key(conn);
	}

	LOG_DBG("Passkey write: res=%d conn=%p", res, (void *)conn);

	return res;
}

static ssize_t write_account_key(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf,
				 uint16_t len, uint16_t offset, uint8_t flags)
{
	struct fp_account_key account_key;
	int err = 0;
	ssize_t res = len;

	if (offset != 0) {
		LOG_WRN("Invalid offset: off=%" PRIu16 " (Account Key)", offset);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto finish;
	}

	if (len != FP_ACCOUNT_KEY_LEN) {
		LOG_WRN("Invalid length: len=%" PRIu16 " (Account Key)", len);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		goto finish;
	}

	err = fp_keys_decrypt(conn, account_key.key, buf);
	if (err) {
		LOG_WRN("Decrypt failed: err=%d (Account Key)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = fp_keys_store_account_key(conn, &account_key);
	if (err) {
		LOG_WRN("Account Key store failed: err=%d (Account Key)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

finish:
	if (res < 0) {
		fp_keys_drop_key(conn);
	}

	LOG_DBG("Account Key write: res=%d conn=%p", res, (void *)conn);

	return res;
}

BT_GATT_SERVICE_DEFINE(fast_pair_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_FAST_PAIR),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_MODEL_ID,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_model_id, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_KEY_BASED_PAIRING,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_key_based_pairing, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_PASSKEY,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_passkey, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_ACCOUNT_KEY,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, write_account_key, NULL),
);
