/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/uuid.h>

/* Those headers are included to make a workaround of Android issue with sending old RPA address
 * during Key-based Pairing write.
 */
#include "host/hci_core.h"
#include "common/rpa.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_registration_data.h"
#include "fp_common.h"
#include "fp_crypto.h"
#include "fp_keys.h"
#include "fp_auth.h"
#include "fp_storage_pn.h"

#if defined(CONFIG_BT_FAST_PAIR_FMDN_BEACON_ACTIONS)
#include "fp_fmdn_beacon_actions.h"
#endif

/* Make sure that MTU value of at least 83 is used (recommended by the Fast Pair specification). */
#define FP_RECOMMENDED_MTU			83
BUILD_ASSERT(BT_L2CAP_TX_MTU >= FP_RECOMMENDED_MTU);
BUILD_ASSERT(BT_L2CAP_RX_MTU >= FP_RECOMMENDED_MTU);

#define MSB_BIT(n)				BIT(7 - n)

#define MSG_ACTION_REQ_ADDITIONAL_DATA_MAX_LEN	5

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

enum additional_action {
	ADDITIONAL_ACTION_NOTIFY_PN_BIT_POS,
	ADDITIONAL_ACTION_COUNT,
};

struct msg_kbp_req_data {
	uint8_t seeker_address[BT_ADDR_SIZE];
};

struct msg_action_req_data {
	uint8_t msg_group;
	uint8_t msg_code;
	uint8_t additional_data_len_or_id;
	uint8_t additional_data[MSG_ACTION_REQ_ADDITIONAL_DATA_MAX_LEN];
};

union kbp_write_msg_specific_data {
	struct msg_kbp_req_data kbp_req;
	struct msg_action_req_data action_req;
};

struct msg_kbp_write {
	uint8_t msg_type;
	uint8_t fp_flags;
	uint8_t provider_address[BT_ADDR_SIZE];
	union kbp_write_msg_specific_data data;
};

struct msg_seekers_passkey {
	uint8_t msg_type;
	uint32_t passkey;
};

/* Reference to the Fast Pair information callback structure. */
static const struct bt_fast_pair_info_cb *fast_pair_info_cb;

#if CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID
static ssize_t read_model_id(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset);
#endif /* CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID */

static ssize_t write_key_based_pairing(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       const void *buf,
				       uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t write_passkey(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t write_account_key(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf,
				 uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t write_additional_data(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf,
				     uint16_t len, uint16_t offset, uint8_t flags);

BT_GATT_SERVICE_DEFINE(fast_pair_svc,
BT_GATT_PRIMARY_SERVICE(BT_FAST_PAIR_UUID_FPS),
#if CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID
	BT_GATT_CHARACTERISTIC(BT_FAST_PAIR_UUID_MODEL_ID,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_model_id, NULL, NULL),
#endif /* CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID */
	BT_GATT_CHARACTERISTIC(BT_FAST_PAIR_UUID_KEY_BASED_PAIRING,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_key_based_pairing, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_FAST_PAIR_UUID_PASSKEY,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_passkey, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_FAST_PAIR_UUID_ACCOUNT_KEY,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, write_account_key, NULL),
#if CONFIG_BT_FAST_PAIR_GATT_SERVICE_ADDITIONAL_DATA
	BT_GATT_CHARACTERISTIC(BT_FAST_PAIR_UUID_ADDITIONAL_DATA,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_additional_data, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#endif /* CONFIG_BT_FAST_PAIR_GATT_SERVICE_ADDITIONAL_DATA */
#if defined(CONFIG_BT_FAST_PAIR_FMDN_BEACON_ACTIONS)
	FP_FMDN_BEACON_ACTIONS_CHARACTERISTIC,
#endif /* CONFIG_BT_FAST_PAIR_FMDN_BEACON_ACTIONS */
);

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

static int notify_personalized_name(struct bt_conn *conn)
{
	int err;
	char pn[FP_STORAGE_PN_BUF_LEN];
	size_t pn_len;
	uint8_t packet[FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN + FP_STORAGE_PN_BUF_LEN - 1];
	size_t packet_len;
	uint8_t nonce[FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN];
	static struct bt_gatt_attr *additional_data_attr;

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_PN)) {
		__ASSERT_NO_MSG(false);
		return -ENOTSUP;
	}

	err = fp_storage_pn_get(pn);
	if (err) {
		LOG_ERR("Failed to get Personalized Name from storage: err=%d", err);
		return err;
	}

	pn_len = strlen(pn);
	if (pn_len == 0) {
		LOG_DBG("Personalized Name is empty");
		return 0;
	}

	packet_len = FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN + pn_len;

	/* Generate random nonce. */
	err = sys_csrand_get(nonce, sizeof(nonce));
	if (err) {
		LOG_ERR("Failed to generate nonce: err=%d", err);
		return err;
	}

	err = fp_keys_additional_data_encode(conn, packet, pn, pn_len, nonce);
	if (err) {
		LOG_ERR("Failed to encode additional data: err=%d", err);
		return err;
	}

	if (!additional_data_attr) {
		additional_data_attr = bt_gatt_find_by_uuid(fast_pair_svc.attrs,
							    fast_pair_svc.attr_count,
							    BT_FAST_PAIR_UUID_ADDITIONAL_DATA);
		__ASSERT_NO_MSG(additional_data_attr != NULL);
	}

	err = bt_gatt_notify(conn, additional_data_attr, packet, packet_len);
	if (err) {
		LOG_ERR("Personalized Name notify failed: err=%d", err);
		return err;
	}

	return packet_len;
}

#if CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID
static ssize_t read_model_id(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	uint8_t model_id[FP_REG_DATA_MODEL_ID_LEN];
	ssize_t res;

	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Model ID read: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled",  res, (void *)conn);
		return res;
	}

	if (!fp_reg_data_get_model_id(model_id, sizeof(model_id))) {
		res = bt_gatt_attr_read(conn, attr, buf, len, offset, model_id, sizeof(model_id));
	} else {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	LOG_DBG("Model ID read: res=%d conn=%p", res, (void *)conn);

	return res;
}
#endif /* CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID */

static int parse_key_based_pairing_write(const struct bt_conn *conn,
					 struct msg_kbp_write *parsed_req,
					 struct net_buf_simple *req)
{
	struct bt_conn_info conn_info;
	uint8_t *provider_address;
	uint8_t *seeker_address;
	uint8_t *additional_data;
	int err = 0;

	parsed_req->msg_type = net_buf_simple_pull_u8(req);
	parsed_req->fp_flags = net_buf_simple_pull_u8(req);

	provider_address = net_buf_simple_pull_mem(req, sizeof(parsed_req->provider_address));
	sys_memcpy_swap(parsed_req->provider_address, provider_address,
			sizeof(parsed_req->provider_address));

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		LOG_ERR("Failed to get local conn info %d", err);
		return err;
	}

	if (memcmp(parsed_req->provider_address, &conn_info.le.local->a.val,
		   sizeof(parsed_req->provider_address))) {
		/* This is a workaround of Android issue with sending old RPA address during
		 * Key-based Pairing write. Normally, error value should be returned at this point.
		 */
		BUILD_ASSERT(sizeof(bt_addr_t) == sizeof(parsed_req->provider_address));
		if (!bt_rpa_irk_matches(bt_dev.irk[conn_info.id],
					(const bt_addr_t *)parsed_req->provider_address)) {
			return -EINVAL;
		}

		LOG_DBG("Fallback has been used to verify Key-based Pairing write.");
	}

	switch (parsed_req->msg_type) {
	case FP_MSG_KEY_BASED_PAIRING_REQ:
		seeker_address = net_buf_simple_pull_mem(req,
						sizeof(parsed_req->data.kbp_req.seeker_address));
		sys_memcpy_swap(parsed_req->data.kbp_req.seeker_address, seeker_address,
				sizeof(parsed_req->data.kbp_req.seeker_address));

		break;

	case FP_MSG_ACTION_REQ:
		parsed_req->data.action_req.msg_group = net_buf_simple_pull_u8(req);
		parsed_req->data.action_req.msg_code = net_buf_simple_pull_u8(req);
		parsed_req->data.action_req.additional_data_len_or_id = net_buf_simple_pull_u8(req);

		additional_data = net_buf_simple_pull_mem(req,
					sizeof(parsed_req->data.action_req.additional_data));
		memcpy(parsed_req->data.action_req.additional_data, additional_data,
		       sizeof(parsed_req->data.action_req.additional_data));

		break;

	default:
		LOG_WRN("Unexpected message type: %" PRIx8 " (Key-based Pairing)",
			parsed_req->msg_type);
		return -EINVAL;
	}

	return err;
}

static int prepare_key_based_pairing_rsp(struct bt_conn *conn, struct net_buf_simple *rsp)
{
	struct bt_conn_info conn_info;
	uint8_t *local_addr = NULL;
	int err;

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

static int handle_key_based_pairing_req(struct bt_conn *conn,
					struct net_buf_simple *rsp,
					struct msg_kbp_write *parsed_req,
					uint8_t *additional_actions)
{
	enum fp_kbp_flags {
		FP_KBP_FLAG_DEPRECATED				= MSB_BIT(0),
		FP_KBP_FLAG_INITIATE_BONDING			= MSB_BIT(1),
		FP_KBP_FLAG_NOTIFY_NAME				= MSB_BIT(2),
		FP_KBP_FLAG_RETROACTIVELY_WRITE_ACCOUNT_KEY	= MSB_BIT(3),
	};

	bool send_pairing_req = false;
	int err;

	if (parsed_req->fp_flags & FP_KBP_FLAG_INITIATE_BONDING) {
		/* Ignore Seeker's BR/EDR Address. */
		ARG_UNUSED(parsed_req->data.kbp_req.seeker_address);

		send_pairing_req = true;
	}

	if (parsed_req->fp_flags & FP_KBP_FLAG_NOTIFY_NAME) {
		if (IS_ENABLED(CONFIG_BT_FAST_PAIR_PN)) {
			WRITE_BIT(*additional_actions, ADDITIONAL_ACTION_NOTIFY_PN_BIT_POS, 1);
		} else {
			/* The notification will not be sent, do not return error. */
			LOG_WRN("Notify name not supported (KBP request)");
		}
	}

	if (parsed_req->fp_flags & FP_KBP_FLAG_RETROACTIVELY_WRITE_ACCOUNT_KEY) {
		LOG_WRN("Retroactively write account key unsupported (KBP request)");
		return -ENOTSUP;
	}

	err = fp_auth_start(conn, send_pairing_req);
	if (err) {
		LOG_WRN("Failed to start authentication: err=%d (KBP request)", err);
		return err;
	}

	err = prepare_key_based_pairing_rsp(conn, rsp);
	if (err) {
		LOG_ERR("Failed to prepare Key-based Pairing response: err=%d (KBP request)", err);
		return err;
	}

	return 0;
}

static int handle_action_req(struct bt_conn *conn,
			     struct net_buf_simple *rsp,
			     struct msg_kbp_write *parsed_req)
{
	enum fp_action_flags {
		FP_ACTION_FLAG_DEVICE_ACTION			= MSB_BIT(0),
		FP_ACTION_FLAG_ADDITIONAL_DATA_TO_FOLLOW	= MSB_BIT(1),
	};

	static const uint8_t personalized_name_data_id = 0x01;

	int err;

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_PN)) {
		ARG_UNUSED(write_additional_data);
		LOG_WRN("Action request not supported (Action request)");
		return -ENOTSUP;
	}

	if (!(parsed_req->fp_flags & FP_ACTION_FLAG_ADDITIONAL_DATA_TO_FOLLOW) ||
	     (parsed_req->fp_flags & FP_ACTION_FLAG_DEVICE_ACTION)) {
		LOG_WRN("Unexpected message flags: %" PRIx8 " (Action request)",
			parsed_req->fp_flags);
		return -ENOMSG;
	}

	if (parsed_req->data.action_req.additional_data_len_or_id != personalized_name_data_id) {
		LOG_WRN("Unexpected Data ID: %" PRIx8 " (Action request)",
			parsed_req->data.action_req.additional_data_len_or_id);
		return -ENOMSG;
	}

	err = fp_keys_wait_for_personalized_name(conn);
	if (err) {
		LOG_ERR("Failed to change state: err=%d (Action request)", err);
		return err;
	}
	LOG_DBG("Waiting for Personalized Name write (Action request)");

	err = prepare_key_based_pairing_rsp(conn, rsp);
	if (err) {
		LOG_ERR("Failed to prepare Key-based Pairing response: err=%d (Action request)",
			err);
		return err;
	}

	return 0;
}

static int validate_key_based_pairing_write(const struct bt_conn *conn, const uint8_t *req,
					    void *context)
{
	struct msg_kbp_write *parsed_req = context;
	struct net_buf_simple req_net_buf;

	net_buf_simple_init_with_data(&req_net_buf, (uint8_t *)req, FP_CRYPTO_AES128_BLOCK_LEN);
	return parse_key_based_pairing_write(conn, parsed_req, &req_net_buf);
}

static int perform_additional_actions(struct bt_conn *conn, uint8_t additional_actions)
{
	if (additional_actions & BIT(ADDITIONAL_ACTION_NOTIFY_PN_BIT_POS)) {
		int len;

		len = notify_personalized_name(conn);
		if (len < 0) {
			LOG_ERR("Failed to notify Personalized Name: err=%d (Key-based Pairing)",
				len);
			return len;
		}

		if (len == 0) {
			LOG_DBG("Personalized Name was not notified (Key-based Pairing)");
		} else {
			LOG_INF("Notified Personalized Name (Key-based Pairing)");
		}
	}

	return 0;
}

static ssize_t write_key_based_pairing(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       const void *buf,
				       uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple gatt_write;
	struct fp_keys_keygen_params keygen_params;
	struct msg_kbp_write parsed_req;
	int err = 0;
	ssize_t res = len;
	uint8_t additional_actions = 0;

	NET_BUF_SIMPLE_DEFINE(rsp, FP_CRYPTO_AES128_BLOCK_LEN);

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Key-based Pairing write: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled", res, (void *)conn);
		return res;
	}

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
	} else if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING)) {
		LOG_WRN("This operation requires support for the subsequent pairing feature."
			"Enable the CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING Kconfig to support it");
		res = BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
		goto finish;
	} else {
		keygen_params.public_key = NULL;
	}
	keygen_params.req_validate_cb = validate_key_based_pairing_write;
	keygen_params.context = &parsed_req;

	err = fp_keys_generate_key(conn, &keygen_params);
	if (err) {
		LOG_WRN("Generating keys failed: err=%d (Key-based Pairing)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	switch (parsed_req.msg_type) {
	case FP_MSG_KEY_BASED_PAIRING_REQ:
		err = handle_key_based_pairing_req(conn, &rsp, &parsed_req, &additional_actions);
		break;

	case FP_MSG_ACTION_REQ:
		err = handle_action_req(conn, &rsp, &parsed_req);
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
	if (err) {
		LOG_WRN("Handling request failed: ret=%d (Key-based Pairing)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = fp_gatt_rsp_notify(conn, attr, &rsp);
	if (err) {
		LOG_WRN("Failed to send response: err=%d (Key-based Pairing)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	err = perform_additional_actions(conn, additional_actions);
	if (err) {
		LOG_WRN("Failed to perform additional actions: err=%d (Key-based Pairing)", err);
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
		LOG_WRN("Invalid message type: %" PRIx8 " (Passkey)", parsed_req->msg_type);
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

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Passkey write: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled", res, (void *)conn);
		return res;
	}

	if (offset != 0) {
		LOG_WRN("Invalid offset: off=%" PRIu16 " (Passkey)", offset);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto finish;
	}

	if (len != FP_CRYPTO_AES128_BLOCK_LEN) {
		LOG_WRN("Invalid length: len=%" PRIu16 " (Passkey)", len);
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
		goto finish;
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

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Account Key write: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled", res, (void *)conn);
		return res;
	}

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

	err = fp_auth_finalize(conn);
	if (err) {
		LOG_WRN("Authentication finalization failed: err=%d (Account Key)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
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
	} else if (fast_pair_info_cb && fast_pair_info_cb->account_key_written) {
		fast_pair_info_cb->account_key_written(conn);
	}

	LOG_DBG("Account Key write: res=%d conn=%p", res, (void *)conn);

	return res;
}

static ssize_t write_additional_data(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf,
				     uint16_t len, uint16_t offset, uint8_t flags)
{
	/* The only expected Additional Data write is with Personalized Name. fp_keys module will
	 * return an error if it receives Personalized Name store request in inappropriate state.
	 */
	size_t data_len = len - FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN;
	uint8_t decoded_data[data_len + sizeof(char)];
	int err = 0;
	ssize_t res = len;

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Additional Data write: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled", res, (void *)conn);
		return res;
	}

	if (offset != 0) {
		LOG_WRN("Invalid offset: off=%" PRIu16 " (Additional Data)", offset);
		res = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
		goto finish;
	}

	err = fp_keys_additional_data_decode(conn, decoded_data, buf, len);
	if (err) {
		LOG_WRN("Decrypt failed: err=%d (Additional Data)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

	/* Received data is assumed to be a Personalized Name string. Terminate the string. */
	decoded_data[data_len] = '\0';

	LOG_DBG("Received following Personalized Name: %s (Additional Data)", decoded_data);

	err = fp_keys_store_personalized_name(conn, decoded_data);
	if (err) {
		LOG_WRN("Personalized Name store failed: err=%d (Additional Data)", err);
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		goto finish;
	}

finish:
	if (res < 0) {
		fp_keys_drop_key(conn);
	}

	LOG_DBG("Additional Data write: res=%d conn=%p", res, (void *)conn);

	return res;
}

int bt_fast_pair_info_cb_register(const struct bt_fast_pair_info_cb *cb)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (bt_fast_pair_is_ready()) {
		return -EACCES;
	}

	if (!cb) {
		return -EINVAL;
	}

	if (fast_pair_info_cb) {
		return -EALREADY;
	}

	fast_pair_info_cb = cb;

	return 0;
}
