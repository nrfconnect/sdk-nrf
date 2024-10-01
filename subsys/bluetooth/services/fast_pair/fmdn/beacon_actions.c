/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net_buf.h>
#include <zephyr/random/random.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_beacon_actions, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_activation.h"
#include "fp_fmdn_auth.h"
#include "fp_fmdn_callbacks.h"
#include "fp_fmdn_clock.h"
#include "fp_fmdn_read_mode.h"
#include "fp_fmdn_ring.h"
#include "fp_fmdn_state.h"
#include "fp_crypto.h"
#include "fp_storage_ak.h"

#include "beacon_actions_defs.h"

enum beacon_actions_data_id {
	BEACON_ACTIONS_BEACON_PARAMETERS_READ       = 0x00,
	BEACON_ACTIONS_PROVISIONING_STATE_READ      = 0x01,
	BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_SET   = 0x02,
	BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_CLEAR = 0x03,
	BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_READ  = 0x04,
	BEACON_ACTIONS_RING                         = 0x05,
	BEACON_ACTIONS_RINGING_STATE_READ           = 0x06,
	BEACON_ACTIONS_ACTIVATE_UTP_MODE            = 0x07,
	BEACON_ACTIONS_DEACTIVATE_UTP_MODE          = 0x08,
};

enum beacon_actions_att_err {
	BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED = 0x80,
	BEACON_ACTIONS_ATT_ERR_INVALID_VALUE   = 0x81,
	BEACON_ACTIONS_ATT_ERR_NO_USER_CONSENT = 0x82,
};

struct ring_context {
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	uint8_t random_nonce[BEACON_ACTIONS_RANDOM_NONCE_LEN];
};

struct conn_context {
	uint8_t random_nonce[BEACON_ACTIONS_RANDOM_NONCE_LEN];
	bool is_challenge_valid;
};

static struct conn_context conn_contexts[CONFIG_BT_MAX_CONN];
static struct ring_context ring_context;

static bool owner_account_key_check(const struct fp_account_key *account_key, void *context)
{
	int ret;

	ret = fp_storage_ak_is_owner(account_key);

	return (ret > 0) ? true : false;
}

static bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce)
{
	int err;
	uint8_t eik_hash_local[FP_CRYPTO_SHA256_HASH_LEN];
	uint8_t hash_input[FP_FMDN_STATE_EIK_LEN + BEACON_ACTIONS_RANDOM_NONCE_LEN];

	/* Calculate: (Ephemeral Identity Key || random_nonce) */
	err = fp_fmdn_state_eik_read(hash_input);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set/Clear request:"
			" EIK read failed: %d", err);

		return false;
	}
	memcpy(hash_input + FP_FMDN_STATE_EIK_LEN,
	       random_nonce,
	       BEACON_ACTIONS_RANDOM_NONCE_LEN);

	/* Generate local version of EIK Hash. */
	err = fp_crypto_sha256(eik_hash_local, hash_input, sizeof(hash_input));
	if (err) {
		LOG_ERR("Beacon Actions: Hash Comparison: fp_crypto_sha256 failed: %d", err);

		return false;
	}

	return !memcmp(eik_hash_local, eik_hash, EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN);
}

static int beacon_response_send(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const uint8_t *rsp,
				uint16_t rsp_len)
{
	int err;

	LOG_HEXDUMP_DBG(rsp, rsp_len, "Beacon Actions response:");

	err = bt_gatt_notify(conn, attr, rsp, rsp_len);
	if (err) {
		return err;
	}

	return 0;
}

static ssize_t beacon_parameters_read_handle(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	struct fp_account_key account_key;
	int8_t tx_power;
	uint8_t padding[BEACON_PARAMETERS_RSP_PADDING_LEN];
	static const uint8_t req_data_len = BEACON_PARAMETERS_REQ_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = BEACON_PARAMETERS_RSP_PAYLOAD_LEN;
	uint8_t rsp_data_enc[FP_CRYPTO_AES128_BLOCK_LEN];

	NET_BUF_SIMPLE_DEFINE(rsp_data_buf, BEACON_PARAMETERS_RSP_ADD_DATA_LEN);
	NET_BUF_SIMPLE_DEFINE(rsp_buf, BEACON_PARAMETERS_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Beacon Parameters Read request:"
			"Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Perform authentication of the incoming packet. */
	auth_seg = net_buf_simple_pull_mem(req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_BEACON_PARAMETERS_READ;
	auth_data.data_len = req_data_len;

	err = fp_fmdn_auth_account_key_find(&auth_data, auth_seg, &account_key);
	if (err) {
		LOG_ERR("Beacon Actions: Beacon Parameters Read request:"
			" Authentication failed: %d", err);

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Prepare response payload for encryption. */
	tx_power = fp_fmdn_state_tx_power_encode();
	LOG_DBG("Beacon Actions: Beacon Parameters Read request:"
		" setting response TX power to %d [dBm] encoded as 0x%02X",
		tx_power, (uint8_t) tx_power);

	net_buf_simple_add_u8(&rsp_data_buf, tx_power);
	net_buf_simple_add_be32(&rsp_data_buf, fp_fmdn_clock_read());
	net_buf_simple_add_u8(&rsp_data_buf, fp_fmdn_state_ecc_type_encode());
	net_buf_simple_add_u8(&rsp_data_buf, fp_fmdn_ring_comp_num_encode());
	net_buf_simple_add_u8(&rsp_data_buf, fp_fmdn_ring_cap_encode());

	memset(padding, BEACON_PARAMETERS_RSP_PADDING_VAL, sizeof(padding));
	net_buf_simple_add_mem(&rsp_data_buf, padding, sizeof(padding));

	/* Encrypt the payload with AES-ECB-128. */
	__ASSERT(sizeof(rsp_data_enc) == rsp_data_buf.len,
		"Beacon Actions: Beacon Parameters: invalid payload for AES-ECB-128");

	err = fp_crypto_aes128_ecb_encrypt(rsp_data_enc, rsp_data_buf.data, account_key.key);
	if (err) {
		LOG_ERR("Beacon Actions: Beacon Parameters Read request:"
			" fp_crypto_aes128_ecb_encrypt failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Prepare a response. */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_BEACON_PARAMETERS_READ);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = rsp_data_enc;

	err = fp_fmdn_auth_seg_generate(account_key.key,
					sizeof(account_key.key),
					&auth_data,
					auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Beacon Parameters Read request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	net_buf_simple_add_mem(&rsp_buf, rsp_data_enc, sizeof(rsp_data_enc));

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Beacon Parameters Read request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Fire the clock synchronization callback for the application if it is registered. */
	fp_fmdn_callbacks_clock_synced_notify();

	return 0;
}

static ssize_t provisioning_state_read_handle(struct bt_conn *conn,
					      const struct bt_gatt_attr *attr,
					      struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	struct fp_account_key account_key;
	uint8_t eid[PROVISIONING_STATE_RSP_EID_LEN];
	uint8_t provisioning_state_flags;
	const bool provisioned = fp_fmdn_state_is_provisioned();
	static const uint8_t req_data_len = PROVISIONING_STATE_REQ_PAYLOAD_LEN;
	uint8_t rsp_data_len;
	enum provisioning_state_bit_flag {
		PROVISIONING_STATE_BIT_FLAG_EPHEMERAL_IDENTITY_KEY_SET = 0,
		PROVISIONING_STATE_BIT_FLAG_OWNER_ACCOUNT_KEY_MATCH    = 1,
	};

	NET_BUF_SIMPLE_DEFINE(rsp_data_buf, PROVISIONING_STATE_RSP_ADD_DATA_LEN);
	NET_BUF_SIMPLE_DEFINE(rsp_buf, PROVISIONING_STATE_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Provisioning State Read request:"
			"Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Perform authentication of the incoming packet. */
	auth_seg = net_buf_simple_pull_mem(req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_PROVISIONING_STATE_READ;
	auth_data.data_len = req_data_len;

	err = fp_fmdn_auth_account_key_find(&auth_data, auth_seg, &account_key);
	if (err) {
		LOG_ERR("Beacon Actions: Provisioning State Read request:"
			" Authentication failed: %d", err);

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Prepare response payload. */
	rsp_data_len = provisioned ? PROVISIONING_STATE_RSP_PAYLOAD_LEN :
		(PROVISIONING_STATE_RSP_PAYLOAD_LEN - PROVISIONING_STATE_RSP_EID_LEN);

	/* Encode provisioning bit. */
	provisioning_state_flags = 0;
	WRITE_BIT(provisioning_state_flags,
		  PROVISIONING_STATE_BIT_FLAG_EPHEMERAL_IDENTITY_KEY_SET,
		  provisioned);

	/* Encode Owner Account Key bit. */
	err = fp_storage_ak_is_owner(&account_key);
	if (err < 0) {
		LOG_ERR("Beacon Actions: Provisioning State Read request:"
			" Check for Owner Account Key failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);

	}
	WRITE_BIT(provisioning_state_flags,
		  PROVISIONING_STATE_BIT_FLAG_OWNER_ACCOUNT_KEY_MATCH,
		  err);

	/* Append the provisioning state bitmap to the response buffer. */
	net_buf_simple_add_u8(&rsp_data_buf, provisioning_state_flags);

	if (provisioned) {
		err = fp_fmdn_state_eid_read(eid);
		if (err) {
			LOG_ERR("Beacon Actions: Provisioning State Read request:"
				" EID read failed: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		net_buf_simple_add_mem(&rsp_data_buf, eid, sizeof(eid));
	}

	/* Prepare a response. */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_PROVISIONING_STATE_READ);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = rsp_data_buf.data;

	err = fp_fmdn_auth_seg_generate(account_key.key,
					sizeof(account_key.key),
					&auth_data,
					auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Provisioning State Read request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	net_buf_simple_add_mem(&rsp_buf, rsp_data_buf.data, rsp_data_buf.len);

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Provisioning State Read request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t ephemeral_identity_key_set_handle(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr,
						 struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	struct fp_account_key account_key;
	uint8_t *encrypted_eik;
	uint8_t new_eik[EPHEMERAL_IDENTITY_KEY_SET_REQ_EIK_LEN];
	const bool provisioned = fp_fmdn_state_is_provisioned();
	const uint8_t req_data_len = provisioned ?
		EPHEMERAL_IDENTITY_KEY_SET_REQ_PROVISIONED_PAYLOAD_LEN :
		EPHEMERAL_IDENTITY_KEY_SET_REQ_UNPROVISIONED_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = EPHEMERAL_IDENTITY_KEY_SET_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_buf, EPHEMERAL_IDENTITY_KEY_SET_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			"Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Perform authentication of the incoming packet. */
	auth_seg = net_buf_simple_pull_mem(req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_SET;
	auth_data.data_len = req_data_len;
	auth_data.add_data = req_data_buf->data;

	err = fp_fmdn_auth_account_key_find(&auth_data, auth_seg, &account_key);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			" Authentication failed: %d", err);

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	err = fp_storage_ak_is_owner(&account_key);
	if (err <= 0) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request: %s", (err == 0) ?
			"Account Key does not belong to the owner" :
			" Check for Owner Account Key failed");

		if (err == 0) {
			return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
		} else {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}

	encrypted_eik = net_buf_simple_pull_mem(req_data_buf,
						EPHEMERAL_IDENTITY_KEY_SET_REQ_EIK_LEN);
	if (provisioned) {
		uint8_t *current_eik_hash;
		uint8_t *random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;

		current_eik_hash = net_buf_simple_pull_mem(
			req_data_buf, EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN);

		if (!eik_hash_compare(current_eik_hash, random_nonce)) {
			LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
				" Current EIK hash does not match");

			return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
		}
	}

	/* Recover the Ephemeral Identity Key (EIK) by AES-ECB-128 with the matched
	 * Account Key.
	 */
	err = fp_crypto_aes128_ecb_decrypt(new_eik, encrypted_eik, account_key.key);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			" Could not decrypt the first part of EIK:"
			" fp_crypto_aes128_ecb_decrypt failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_crypto_aes128_ecb_decrypt(new_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   encrypted_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   account_key.key);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			" Could not decrypt the second part of EIK:"
			" fp_crypto_aes128_ecb_decrypt failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_fmdn_state_eik_provision(new_eik);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			" Beacon State provision failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Prepare a response. */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_SET);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = NULL;

	err = fp_fmdn_auth_seg_generate(account_key.key,
					sizeof(account_key.key),
					&auth_data,
					auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Set request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t ephemeral_identity_key_clear_handle(struct bt_conn *conn,
						   const struct bt_gatt_attr *attr,
						   struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	struct fp_account_key account_key;
	uint8_t *current_eik_hash;
	uint8_t *random_nonce;
	const bool provisioned = fp_fmdn_state_is_provisioned();
	static const uint8_t req_data_len = EPHEMERAL_IDENTITY_KEY_CLEAR_REQ_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = EPHEMERAL_IDENTITY_KEY_CLEAR_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_buf, EPHEMERAL_IDENTITY_KEY_CLEAR_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			"Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	if (!provisioned) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			" Beacon is not provisioned");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Perform authentication of the incoming packet. */
	auth_seg = net_buf_simple_pull_mem(req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_CLEAR;
	auth_data.data_len = req_data_len;
	auth_data.add_data = req_data_buf->data;

	err = fp_fmdn_auth_account_key_find(&auth_data, auth_seg, &account_key);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			" Authentication failed: %d", err);

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	err = fp_storage_ak_is_owner(&account_key);
	if (err <= 0) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request: %s", (err == 0) ?
			"Account Key does not belong to the owner" :
			" Check for Owner Account Key failed");

		if (err == 0) {
			return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
		} else {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}

	current_eik_hash = net_buf_simple_pull_mem(
		req_data_buf, EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN);
	random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;

	if (!eik_hash_compare(current_eik_hash, random_nonce)) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			" Current EIK hash does not match");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	err = fp_fmdn_state_eik_provision(NULL);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			" Beacon State unprovision failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Clear the ringing context to prevent the module from sending ringing
	 * GATT notifications with an invalid authetnication segment as the Ring Key
	 * gets invalidated together with its source key: EIK.
	 */
	memset(&ring_context, 0, sizeof(ring_context));

	/* Prepare a response. */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_CLEAR);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = NULL;

	err = fp_fmdn_auth_seg_generate(account_key.key,
					sizeof(account_key.key),
					&auth_data,
					auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Clear request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t ephemeral_identity_key_read_handle(struct bt_conn *conn,
						  const struct bt_gatt_attr *attr,
						  struct net_buf_simple *req_data_buf)
{
	int err;
	bool user_consent;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	uint8_t recovery_key[FP_FMDN_AUTH_KEY_RECOVERY_LEN];
	struct fp_account_key owner_account_key;
	uint8_t eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	uint8_t encrypted_eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	static const uint8_t req_data_len = EPHEMERAL_IDENTITY_KEY_READ_REQ_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = EPHEMERAL_IDENTITY_KEY_READ_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_data_buf, EPHEMERAL_IDENTITY_KEY_READ_RSP_ADD_DATA_LEN);
	NET_BUF_SIMPLE_DEFINE(rsp_buf, EPHEMERAL_IDENTITY_KEY_READ_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			"Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Perform authentication of the incoming packet. */
	err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_RECOVERY,
					recovery_key,
					sizeof(recovery_key));
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" fp_fmdn_auth_key_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	auth_seg = net_buf_simple_pull_mem(req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_READ;
	auth_data.data_len = req_data_len;

	if (!fp_fmdn_auth_seg_validate(recovery_key, sizeof(recovery_key), &auth_data, auth_seg)) {
		LOG_WRN("Beacon Actions: Ephemeral Identity Key Read request:"
			" Recovery hash does not match");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Check the user consent. */
	err = fp_fmdn_read_mode_recovery_mode_request(&user_consent);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" fp_fmdn_read_mode_recovery_mode_request failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (!user_consent) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" No user consent");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_NO_USER_CONSENT);
	}

	/* Fetch and encrypt the EIK. */
	err = fp_fmdn_state_eik_read(eik);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" EIK read failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_storage_ak_find(&owner_account_key, owner_account_key_check, NULL);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" Could not find the Owner Account Key: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_crypto_aes128_ecb_encrypt(encrypted_eik, eik, owner_account_key.key);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" Could not encrypt the first part of EIK:"
			" fp_crypto_aes128_ecb_encrypt failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_crypto_aes128_ecb_encrypt(encrypted_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   owner_account_key.key);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" Could not encrypt the second part of EIK:"
			" fp_crypto_aes128_ecb_encrypt failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Prepare response payload. */
	net_buf_simple_add_mem(&rsp_data_buf, encrypted_eik, sizeof(encrypted_eik));

	/* Prepare a response */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_READ);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = rsp_data_buf.data;

	err = fp_fmdn_auth_seg_generate(recovery_key,
					sizeof(recovery_key),
					&auth_data,
					auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Provisioning State Read request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	net_buf_simple_add_mem(&rsp_buf, rsp_data_buf.data, rsp_data_buf.len);

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Ephemeral Identity Key Read request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t ring_handle(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   struct net_buf_simple *req_data_buf)
{
	int err;
	bool perform_auth;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	uint8_t ring_key[FP_FMDN_AUTH_KEY_RING_LEN];
	struct bt_fast_pair_fmdn_ring_req_param ring_param;
	static const uint8_t req_data_len = RING_REQ_PAYLOAD_LEN;

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Ring request: Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Pull the Authentication Segment field even when the authentication procedure
	 * is omitted to correctly parse the request fields that follow.
	 */
	auth_seg = net_buf_simple_pull_mem(
		req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	perform_auth = !fp_fmdn_state_utp_mode_ring_auth_skip();
	if (perform_auth) {
		/* Perform authentication of the incoming packet. */
		err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_RING,
						ring_key,
						sizeof(ring_key));
		if (err) {
			LOG_ERR("Beacon Actions: Ring request:"
				" fp_fmdn_auth_key_generate failed: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		memset(&auth_data, 0, sizeof(auth_data));
		auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
		auth_data.data_id = BEACON_ACTIONS_RING;
		auth_data.data_len = req_data_len;
		auth_data.add_data = req_data_buf->data;

		if (!fp_fmdn_auth_seg_validate(ring_key, sizeof(ring_key), &auth_data, auth_seg)) {
			LOG_WRN("Beacon Actions: Ring request: Ring hash does not match");

			return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
		}
	} else {
		LOG_DBG("Beacon Actions: Ring request: skipping authentication");
	}

	/* Parse the ringing parameters. */
	ring_param.active_comp_bm = net_buf_simple_pull_u8(req_data_buf);
	ring_param.timeout = net_buf_simple_pull_be16(req_data_buf);
	ring_param.volume = net_buf_simple_pull_u8(req_data_buf);

	/* Validate the number of ringing components if authentication is enabled. */
	if (perform_auth && !fp_fmdn_ring_is_active_comp_bm_supported(ring_param.active_comp_bm)) {
		LOG_ERR("Beacon Actions: Ring request:"
			" the request state does not match the number of ringing components");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Update the ringing context. */
	BUILD_ASSERT(sizeof(ring_context.random_nonce) ==
		     sizeof(conn_contexts[bt_conn_index(conn)].random_nonce));

	ring_context.conn = conn;
	ring_context.attr = attr;
	memcpy(ring_context.random_nonce,
	       conn_contexts[bt_conn_index(conn)].random_nonce,
	       sizeof(ring_context.random_nonce));

	/* Pass the request to the Ring module. */
	err = fp_fmdn_ring_req_handle(BT_FAST_PAIR_FMDN_RING_SRC_FMDN_BT_GATT, &ring_param);
	if (err == -EINVAL) {
		LOG_ERR("Beacon Actions: Ring request: Invalid ring data");

		memset(&ring_context, 0, sizeof(ring_context));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	} else if (err) {
		LOG_ERR("Beacon Actions: Ring request failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t ringing_state_read_handle(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	uint8_t ring_key[FP_FMDN_AUTH_KEY_RING_LEN];
	static const uint8_t req_data_len = RINGING_STATE_READ_REQ_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = RINGING_STATE_READ_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_data_buf, RINGING_STATE_READ_RSP_ADD_DATA_LEN);
	NET_BUF_SIMPLE_DEFINE(rsp_buf, RINGING_STATE_READ_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Ringing State Read request:"
			" Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Perform authentication of the incoming packet. */
	err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_RING,
					ring_key,
					sizeof(ring_key));
	if (err) {
		LOG_ERR("Beacon Actions: Ringing State Read request:"
			" fp_fmdn_auth_key_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	auth_seg = net_buf_simple_pull_mem(
		req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_RINGING_STATE_READ;
	auth_data.data_len = req_data_len;

	if (!fp_fmdn_auth_seg_validate(ring_key, sizeof(ring_key), &auth_data, auth_seg)) {
		LOG_WRN("Beacon Actions: Ringing State Read request:"
			" Ring hash does not match");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Prepare response payload. */
	net_buf_simple_add_u8(&rsp_data_buf, fp_fmdn_ring_active_comp_bm_get());
	net_buf_simple_add_be16(&rsp_data_buf, fp_fmdn_ring_timeout_get());

	/* Prepare a response */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_RINGING_STATE_READ);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = rsp_data_buf.data;

	err = fp_fmdn_auth_seg_generate(ring_key, sizeof(ring_key), &auth_data, auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Ring State Change response:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	net_buf_simple_add_mem(&rsp_buf, rsp_data_buf.data, rsp_data_buf.len);

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Ringing State Read request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t activate_utp_mode_handle(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	uint8_t utp_key[FP_FMDN_AUTH_KEY_UTP_LEN];
	uint8_t control_flags = 0;
	uint8_t req_data_len = ACTIVATE_UTP_MODE_REQ_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = ACTIVATE_UTP_MODE_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_buf, ACTIVATE_UTP_MODE_RSP_LEN);

	/* If no control flag is being set, it is valid to omit this field in the request. */
	if ((req_data_len - ACTIVATE_UTP_MODE_REQ_CONTROL_FLAGS_LEN) ==
		net_buf_simple_max_len(req_data_buf)) {
		req_data_len -= ACTIVATE_UTP_MODE_REQ_CONTROL_FLAGS_LEN;
	}

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	if (!fp_fmdn_state_is_provisioned()) {
		LOG_ERR("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" Device is not provisioned");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Perform authentication of the incoming packet. */
	err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_UTP, utp_key, sizeof(utp_key));
	if (err) {
		LOG_ERR("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" fp_fmdn_auth_key_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	auth_seg = net_buf_simple_pull_mem(
		req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	auth_data.data_id = BEACON_ACTIONS_ACTIVATE_UTP_MODE;
	auth_data.data_len = req_data_len;
	auth_data.add_data = req_data_buf->data;

	if (!fp_fmdn_auth_seg_validate(utp_key, sizeof(utp_key), &auth_data, auth_seg)) {
		LOG_WRN("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" UTP hash does not match");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	if (net_buf_simple_max_len(req_data_buf) > 0) {
		control_flags = net_buf_simple_pull_u8(req_data_buf);
	}

	/* Manage the UTP mode in the FMDN State module. */
	err = fp_fmdn_state_utp_mode_activate(control_flags);
	if (err) {
		LOG_ERR("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" fp_fmdn_state_utp_mode_activate failed: %d", err);

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	/* Prepare a response */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_ACTIVATE_UTP_MODE);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = NULL;

	err = fp_fmdn_auth_seg_generate(utp_key, sizeof(utp_key), &auth_data, auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Activate Unwanted Tracking Protection mode request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static ssize_t deactivate_utp_mode_handle(struct bt_conn *conn,
					  const struct bt_gatt_attr *attr,
					  struct net_buf_simple *req_data_buf)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	uint8_t utp_key[FP_FMDN_AUTH_KEY_UTP_LEN];
	uint8_t *current_eik_hash;
	uint8_t *random_nonce = conn_contexts[bt_conn_index(conn)].random_nonce;
	static const uint8_t req_data_len = DEACTIVATE_UTP_MODE_REQ_PAYLOAD_LEN;
	static const uint8_t rsp_data_len = DEACTIVATE_UTP_MODE_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_buf, DEACTIVATE_UTP_MODE_RSP_LEN);

	if (req_data_len != net_buf_simple_max_len(req_data_buf)) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" Incorrect length: %d!=%d",
			req_data_len, net_buf_simple_max_len(req_data_buf));

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
	}

	if (!fp_fmdn_state_is_provisioned()) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" Device is not provisioned");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Perform authentication of the incoming packet. */
	err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_UTP, utp_key, sizeof(utp_key));
	if (err) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" fp_fmdn_auth_key_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	auth_seg = net_buf_simple_pull_mem(
		req_data_buf, BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN);

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = random_nonce;
	auth_data.data_id = BEACON_ACTIONS_DEACTIVATE_UTP_MODE;
	auth_data.data_len = req_data_len;
	auth_data.add_data = req_data_buf->data;

	if (!fp_fmdn_auth_seg_validate(utp_key, sizeof(utp_key), &auth_data, auth_seg)) {
		LOG_WRN("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" UTP hash does not match");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	current_eik_hash = net_buf_simple_pull_mem(
		req_data_buf, DEACTIVATE_UTP_MODE_REQ_EIK_HASH_LEN);

	if (!eik_hash_compare(current_eik_hash, random_nonce)) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" Current EIK hash does not match");

		return BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
	}

	/* Manage the UTP mode in the FMDN State module. */
	err = fp_fmdn_state_utp_mode_deactivate();
	if (err) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" fp_fmdn_state_utp_mode_deactivate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Prepare a response */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_DEACTIVATE_UTP_MODE);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = NULL;

	err = fp_fmdn_auth_seg_generate(utp_key, sizeof(utp_key), &auth_data, auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	/* Send a response notification. */
	err = beacon_response_send(conn, attr, rsp_buf.data, rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Deactivate Unwanted Tracking Protection mode request:"
			" GATT response notify failed: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

int bt_fast_pair_fmdn_ring_state_update(
	enum bt_fast_pair_fmdn_ring_src src,
	const struct bt_fast_pair_fmdn_ring_state_param *param)
{
	int err;
	uint8_t *auth_seg;
	struct fp_fmdn_auth_data auth_data;
	uint8_t ring_key[FP_FMDN_AUTH_KEY_RING_LEN];
	static const uint8_t rsp_data_len = RING_RSP_PAYLOAD_LEN;

	NET_BUF_SIMPLE_DEFINE(rsp_buf, RING_RSP_LEN);
	NET_BUF_SIMPLE_DEFINE(rsp_data_buf, RING_RSP_ADD_DATA_LEN);

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!bt_fast_pair_is_ready()) {
		LOG_ERR("Beacon Actions: Ring State Change response: Fast Pair disabled");
		return -EALREADY;
	}

	err = fp_fmdn_ring_state_param_set(src, param);
	if (err) {
		LOG_ERR("Beacon Actions: Ring State Change response:"
			" fp_fmdn_ring_state_param_set failed: %d", err);

		return err;
	}

	/* Check if connected peers should be notified about the ring state change. */
	if (!fp_fmdn_state_is_provisioned()) {
		return 0;
	}

	if (!ring_context.conn) {
		return 0;
	}

	/* Prepare response payload. */
	net_buf_simple_add_u8(&rsp_data_buf, param->trigger);
	net_buf_simple_add_u8(&rsp_data_buf, fp_fmdn_ring_active_comp_bm_get());
	net_buf_simple_add_be16(&rsp_data_buf, fp_fmdn_ring_timeout_get());

	/* Prepare a response. */
	net_buf_simple_add_u8(&rsp_buf, BEACON_ACTIONS_RING);
	net_buf_simple_add_u8(&rsp_buf, rsp_data_len);

	err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_RING,
					ring_key,
					sizeof(ring_key));
	if (err) {
		LOG_ERR("Beacon Actions: Ring State Change response:"
			" fp_fmdn_auth_key_generate failed: %d", err);

		return err;
	}

	memset(&auth_data, 0, sizeof(auth_data));
	auth_data.random_nonce = ring_context.random_nonce;
	auth_data.data_id = BEACON_ACTIONS_RING;
	auth_data.data_len = rsp_data_len;
	auth_data.add_data = rsp_data_buf.data;

	auth_seg = net_buf_simple_add(&rsp_buf, BEACON_ACTIONS_RSP_AUTH_SEG_LEN);

	err = fp_fmdn_auth_seg_generate(ring_key, sizeof(ring_key), &auth_data, auth_seg);
	if (err) {
		LOG_ERR("Beacon Actions: Ring State Change response:"
			" fp_fmdn_auth_seg_generate failed: %d", err);

		return err;
	}

	net_buf_simple_add_mem(&rsp_buf, rsp_data_buf.data, rsp_data_buf.len);

	/* Send a response notification. */
	err = beacon_response_send(ring_context.conn,
				   ring_context.attr,
				   rsp_buf.data,
				   rsp_buf.len);
	if (err) {
		LOG_ERR("Beacon Actions: Ring State Change response:"
			" GATT response notify failed: %d", err);

		return err;
	}

	return 0;
}

ssize_t fp_fmdn_beacon_actions_read(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf,
				    uint16_t len,
				    uint16_t offset)
{
	int err;
	ssize_t res;
	struct conn_context *conn_context;
	uint8_t rsp[BEACON_ACTIONS_READ_RSP_LEN];

	BUILD_ASSERT((sizeof(conn_context->random_nonce) +
		     BEACON_ACTIONS_READ_RSP_VERSION_LEN) ==
		     BEACON_ACTIONS_READ_RSP_LEN);

	/* It is assumed that this callback executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	LOG_DBG("Beacon Actions GATT Read Request");

	/* Do not perform any action if Fast Pair is not ready. */
	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Beacon Actions read: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled", res, (void *)conn);
		return res;
	}

	conn_context = &conn_contexts[bt_conn_index(conn)];

	err = sys_csrand_get(conn_context->random_nonce, sizeof(conn_context->random_nonce));
	if (err) {
		LOG_ERR("Beacon Actions: failed to generate random nonce: err=%d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	rsp[0] = CONFIG_BT_FAST_PAIR_FMDN_VERSION_MAJOR;
	memcpy(&rsp[BEACON_ACTIONS_READ_RSP_VERSION_LEN],
	       conn_context->random_nonce,
	       sizeof(conn_context->random_nonce));

	res = bt_gatt_attr_read(conn, attr, buf, len, offset, rsp, sizeof(rsp));
	if (res == sizeof(rsp)) {
		conn_context->is_challenge_valid = true;
		LOG_HEXDUMP_DBG(conn_context->random_nonce, sizeof(conn_context->random_nonce),
				"Beacon Actions: challenge-response enabled for the next write:");
	}

	LOG_DBG("Beacon Actions read: res=%d conn=%p", res, (void *)conn);

	return res;
}

ssize_t fp_fmdn_beacon_actions_write(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf,
				     uint16_t len,
				     uint16_t offset,
				     uint8_t flags)
{
	struct net_buf_simple fmdn_beacon_actions_buf;
	struct conn_context *conn_context;
	uint8_t data_id;
	uint8_t data_len;
	ssize_t res;

	/* It is assumed that this callback executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	LOG_HEXDUMP_DBG(buf, len, "Beacon Actions GATT Write Request:");

	/* Do not perform any action if Fast Pair is not ready. */
	if (!bt_fast_pair_is_ready()) {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		LOG_INF("Beacon Actions write: res=%d conn=%p, "
			"Return error because Fast Pair is not enabled", res, (void *)conn);
		return res;
	}

	conn_context = &conn_contexts[bt_conn_index(conn)];

	if (conn_context->is_challenge_valid) {
		LOG_DBG("Beacon Actions: consuming random nonce read operation");
		conn_context->is_challenge_valid = false;
	} else {
		LOG_ERR("Beacon Actions: failed to enable challenge-response");
		res = BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_UNAUTHENTICATED);
		goto finish;
	}

	if (len < BEACON_ACTIONS_HEADER_LEN) {
		LOG_ERR("Beacon Actions: request header too short: len=%d", len);
		res = BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
		goto finish;
	}

	net_buf_simple_init_with_data(&fmdn_beacon_actions_buf, (void *) buf, len);
	data_id = net_buf_simple_pull_u8(&fmdn_beacon_actions_buf);
	data_len = net_buf_simple_pull_u8(&fmdn_beacon_actions_buf);

	if (data_len != net_buf_simple_max_len(&fmdn_beacon_actions_buf)) {
		LOG_ERR("Beacon Actions: request with incorrect length: %d!=%d",
			data_len, net_buf_simple_max_len(&fmdn_beacon_actions_buf));
		res = BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
		goto finish;
	}

	switch (data_id) {
	case BEACON_ACTIONS_BEACON_PARAMETERS_READ:
		res = beacon_parameters_read_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_PROVISIONING_STATE_READ:
		res = provisioning_state_read_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_SET:
		res = ephemeral_identity_key_set_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_CLEAR:
		res = ephemeral_identity_key_clear_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_EPHEMERAL_IDENTITY_KEY_READ:
		res = ephemeral_identity_key_read_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_RING:
		res = ring_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_RINGING_STATE_READ:
		res = ringing_state_read_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_ACTIVATE_UTP_MODE:
		res = activate_utp_mode_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	case BEACON_ACTIONS_DEACTIVATE_UTP_MODE:
		res = deactivate_utp_mode_handle(conn, attr, &fmdn_beacon_actions_buf);
		break;
	default:
		LOG_ERR("Beacon Actions: unrecognized request: data_id=%d", data_id);
		res = BT_GATT_ERR(BEACON_ACTIONS_ATT_ERR_INVALID_VALUE);
		goto finish;
	}

	if (res == 0) {
		res = len;
	}
finish:
	LOG_DBG("Beacon Actions write: res=%d conn=%p", res, (void *)conn);

	return res;
}

void fp_fmdn_beacon_actions_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_INF("Beacon Actions CCCD write, handle: %u, value: 0x%04X",
		attr->handle, value);
}

static void beacon_actions_disconnected(struct bt_conn *conn, uint8_t reason)
{
	uint8_t conn_index;

	if (!bt_fast_pair_is_ready()) {
		return;
	}

	conn_index = bt_conn_index(conn);

	memset(&conn_contexts[conn_index], 0, sizeof(conn_contexts[conn_index]));
	if (ring_context.conn == conn) {
		memset(&ring_context, 0, sizeof(ring_context));
	}
}

BT_CONN_CB_DEFINE(beacon_actions_conn_callbacks) = {
	.disconnected = beacon_actions_disconnected,
};

static int fp_fmdn_beacon_actions_init(void)
{
	/* intentionally left empty */

	return 0;
}

static int fp_fmdn_beacon_actions_uninit(void)
{
	memset(conn_contexts, 0, sizeof(conn_contexts));
	memset(&ring_context, 0, sizeof(ring_context));

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_beacon_actions,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      fp_fmdn_beacon_actions_init,
			      fp_fmdn_beacon_actions_uninit);
