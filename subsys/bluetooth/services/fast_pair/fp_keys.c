/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>
#include "fp_activation.h"
#include "fp_registration_data.h"
#include "fp_keys.h"
#include "fp_crypto.h"
#include "fp_common.h"
#include "fp_storage_ak.h"
#include "fp_storage_pn.h"

#define EMPTY_AES_KEY_BYTE	0xff
#define FP_ACCOUNT_KEY_PREFIX	0x04

#define FP_KEY_TIMEOUT				K_SECONDS(10)
#define FP_KEY_GEN_FAILURE_MAX_CNT		10
#define FP_KEY_GEN_FAILURE_CNT_RESET_TIMEOUT	K_MINUTES(5)

BUILD_ASSERT(FP_ACCOUNT_KEY_LEN == FP_CRYPTO_AES128_KEY_LEN);

enum wait_for {
	WAIT_FOR_ACCOUNT_KEY_BIT_POS,
	WAIT_FOR_PERSONALIZED_NAME_BIT_POS,
	WAIT_FOR_COUNT,
};

BUILD_ASSERT(__CHAR_BIT__ * sizeof(uint8_t) >= WAIT_FOR_COUNT);

enum fp_state {
	FP_STATE_INITIAL,
	FP_STATE_USE_TEMP_KEY,
	FP_STATE_USE_TEMP_ACCOUNT_KEY,
};

struct fp_procedure {
	struct k_work_delayable timeout;
	enum fp_state state;
	uint8_t wait_for_mask;
	bool pairing_mode;
	uint8_t aes_key[FP_ACCOUNT_KEY_LEN];
};

struct fp_key_gen_account_key_check_context {
	const struct bt_conn *conn;
	struct fp_keys_keygen_params *keygen_params;
};

static uint8_t key_gen_failure_cnt;
static void key_gen_failure_cnt_reset_fn(struct k_work *w);
K_WORK_DELAYABLE_DEFINE(key_gen_failure_cnt_reset, key_gen_failure_cnt_reset_fn);

static bool user_pairing_mode = true;
static struct fp_procedure fp_procedures[CONFIG_BT_MAX_CONN];

static bool is_enabled;


void bt_fast_pair_set_pairing_mode(bool pairing_mode)
{
	user_pairing_mode = pairing_mode;
}

static void key_timeout(struct fp_procedure *proc)
{
	proc->state = FP_STATE_INITIAL;
	proc->wait_for_mask = 0;
	memset(proc->aes_key, EMPTY_AES_KEY_BYTE, sizeof(proc->aes_key));
}

static void invalidate_key(struct fp_procedure *proc)
{
	if (proc->state != FP_STATE_INITIAL) {
		int ret;

		key_timeout(proc);

		ret = k_work_cancel_delayable(&proc->timeout);
		__ASSERT_NO_MSG(ret == 0);
		ARG_UNUSED(ret);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	if (!err) {
		proc->pairing_mode = user_pairing_mode;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	invalidate_key(proc);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static inline bool is_key_generated(enum fp_state state)
{
	switch (state) {
	case FP_STATE_USE_TEMP_KEY:
	case FP_STATE_USE_TEMP_ACCOUNT_KEY:
		return true;

	case FP_STATE_INITIAL:
	default:
		return false;
	}
}

int fp_keys_encrypt(const struct bt_conn *conn, uint8_t *out, const uint8_t *in)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (!is_key_generated(proc->state)) {
		err = -EACCES;
	}

	if (!err) {
		err = fp_crypto_aes128_ecb_encrypt(out, in, proc->aes_key);
	}

	return err;
}

int fp_keys_decrypt(const struct bt_conn *conn, uint8_t *out, const uint8_t *in)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (!is_key_generated(proc->state)) {
		err = -EACCES;
	}

	if (!err) {
		err = fp_crypto_aes128_ecb_decrypt(out, in, proc->aes_key);
	}

	return err;
}

int fp_keys_additional_data_encode(const struct bt_conn *conn, uint8_t *out, const uint8_t *data,
				   size_t data_len, const uint8_t *nonce)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (!is_key_generated(proc->state)) {
		err = -EACCES;
	}

	if (!err) {
		err = fp_crypto_additional_data_encode(out, data, data_len, proc->aes_key, nonce);
	}

	return err;
}

int fp_keys_additional_data_decode(const struct bt_conn *conn, uint8_t *out_data,
				   const uint8_t *in_packet, size_t packet_len)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (!is_key_generated(proc->state)) {
		err = -EACCES;
	}

	if (!err) {
		err = fp_crypto_additional_data_decode(out_data, in_packet, packet_len,
						       proc->aes_key);
	}

	return err;
}

static int key_gen_public_key(const struct bt_conn *conn,
			      struct fp_keys_keygen_params *keygen_params)
{
	int err;
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	uint8_t req[FP_CRYPTO_AES128_BLOCK_LEN];
	uint8_t priv_key[FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN];
	uint8_t ecdh_secret[FP_CRYPTO_ECDH_SHARED_KEY_LEN];

	err = fp_get_anti_spoofing_priv_key(priv_key, sizeof(priv_key));

	if (!err) {
		err = fp_crypto_ecdh_shared_secret(ecdh_secret, keygen_params->public_key,
						   priv_key);
	}

	if (!err) {
		err = fp_crypto_aes_key_compute(proc->aes_key, ecdh_secret);
	}

	if (!err) {
		err = fp_keys_decrypt(conn, req, keygen_params->req_enc);
	}

	if (!err) {
		err = keygen_params->req_validate_cb(conn, req, keygen_params->context);
	}

	return err;
}

static bool key_gen_account_key_check(const struct fp_account_key *account_key, void *context)
{
	int err;
	uint8_t req[FP_CRYPTO_AES128_BLOCK_LEN];
	struct fp_key_gen_account_key_check_context *ak_check_context = context;
	const struct bt_conn *conn = ak_check_context->conn;
	struct fp_keys_keygen_params *keygen_params = ak_check_context->keygen_params;
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	memcpy(proc->aes_key, account_key->key, FP_ACCOUNT_KEY_LEN);

	err = fp_keys_decrypt(conn, req, keygen_params->req_enc);
	if (err) {
		return false;
	}

	err = keygen_params->req_validate_cb(conn, req, keygen_params->context);
	if (err) {
		return false;
	}

	return true;
}

static int key_gen_account_key(const struct bt_conn *conn,
			       struct fp_keys_keygen_params *keygen_params)
{
	struct fp_key_gen_account_key_check_context context = {
		.conn = conn,
		.keygen_params = keygen_params,
	};

	/* This function call assigns the Account Key internally to the Fast Pair Keys
	 * module. The assignment happens in the provided callback method.
	 */
	return fp_storage_ak_find(NULL, key_gen_account_key_check, &context);
}

int fp_keys_generate_key(const struct bt_conn *conn, struct fp_keys_keygen_params *keygen_params)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (key_gen_failure_cnt >= FP_KEY_GEN_FAILURE_MAX_CNT) {
		return -EACCES;
	}

	if (proc->state != FP_STATE_INITIAL) {
		LOG_WRN("Invalid state during key generation");
		return -EACCES;
	}

	if (keygen_params->public_key) {
		/* Update state to ensure that key could be used for decryption. */
		proc->state = FP_STATE_USE_TEMP_KEY;

		if (proc->pairing_mode) {
			err = key_gen_public_key(conn, keygen_params);
		} else {
			/* Ignore write and exit. */
			proc->state = FP_STATE_INITIAL;
			return -EACCES;
		}
	} else {
		/* Update state to ensure that key could be used for decryption. */
		proc->state = FP_STATE_USE_TEMP_ACCOUNT_KEY;

		err = key_gen_account_key(conn, keygen_params);
	}

	if (err) {
		invalidate_key(proc);
		key_gen_failure_cnt++;
		if (key_gen_failure_cnt >= FP_KEY_GEN_FAILURE_MAX_CNT) {
			int ret;

			LOG_WRN("Key generation failure limit exceeded");
			ret = k_work_schedule(&key_gen_failure_cnt_reset,
					      FP_KEY_GEN_FAILURE_CNT_RESET_TIMEOUT);
			__ASSERT_NO_MSG(ret == 1);
			ARG_UNUSED(ret);
		}
	} else {
		int ret;

		key_gen_failure_cnt = 0;
		__ASSERT_NO_MSG(is_key_generated(proc->state));

		ret = k_work_schedule(&proc->timeout, FP_KEY_TIMEOUT);
		__ASSERT_NO_MSG(ret == 1);
		ARG_UNUSED(ret);
	}

	return err;
}

int fp_keys_store_account_key(const struct bt_conn *conn, const struct fp_account_key *account_key)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (!(proc->wait_for_mask & BIT(WAIT_FOR_ACCOUNT_KEY_BIT_POS))) {
		LOG_WRN("Invalid state during Account Key write");
		return -EACCES;
	}
	WRITE_BIT(proc->wait_for_mask, WAIT_FOR_ACCOUNT_KEY_BIT_POS, 0);

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_PN)) {
		/* Invalidate Key to ensure that requests will be rejected until procedure is
		 * restarted.
		 */
		invalidate_key(proc);
	} else {
		WRITE_BIT(proc->wait_for_mask, WAIT_FOR_PERSONALIZED_NAME_BIT_POS, 1);
	}

	if (account_key->key[0] != FP_ACCOUNT_KEY_PREFIX) {
		LOG_WRN("Received invalid Account Key");
		return -EINVAL;
	}
	err = fp_storage_ak_save(account_key);
	if (!err) {
		LOG_DBG("Account Key stored");
	} else {
		LOG_WRN("Store account key error: err=%d", err);
	}

	return err;
}

int fp_keys_store_personalized_name(const struct bt_conn *conn, const char *pn)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (!(proc->wait_for_mask & BIT(WAIT_FOR_PERSONALIZED_NAME_BIT_POS))) {
		LOG_WRN("Invalid state during Personalized Name write");
		return -EACCES;
	}
	WRITE_BIT(proc->wait_for_mask, WAIT_FOR_PERSONALIZED_NAME_BIT_POS, 0);

	/* Invalidate Key to ensure that requests will be rejected until procedure is restarted. */
	invalidate_key(proc);

	err = fp_storage_pn_save(pn);
	if (!err) {
		LOG_DBG("Personalized Name stored");
	} else {
		LOG_ERR("Store Personalized Name error: err=%d", err);
	}

	return err;
}

int fp_keys_wait_for_personalized_name(const struct bt_conn *conn)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	/* The function is meant to be used only if we expect a Personalized Name write after
	 * receiving an Action Request.
	 */
	if (proc->state != FP_STATE_USE_TEMP_ACCOUNT_KEY) {
		LOG_ERR("Invalid state to wait for personalized name");
		return -EACCES;
	}

	__ASSERT_NO_MSG(!(proc->wait_for_mask & BIT(WAIT_FOR_ACCOUNT_KEY_BIT_POS)));

	if (proc->wait_for_mask & BIT(WAIT_FOR_PERSONALIZED_NAME_BIT_POS)) {
		LOG_ERR("Already waiting for personalized name");
		return -EALREADY;
	}

	WRITE_BIT(proc->wait_for_mask, WAIT_FOR_PERSONALIZED_NAME_BIT_POS, 1);

	return 0;
}

void fp_keys_bt_auth_progress(const struct bt_conn *conn, bool authenticated)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	if (is_key_generated(proc->state)) {
		int ret;

		ret = k_work_cancel_delayable(&proc->timeout);
		__ASSERT_NO_MSG(ret == 0);
		ret = k_work_schedule(&proc->timeout, FP_KEY_TIMEOUT);
		__ASSERT_NO_MSG(ret == 1);
		ARG_UNUSED(ret);

		if (authenticated) {
			if (proc->wait_for_mask & BIT(WAIT_FOR_ACCOUNT_KEY_BIT_POS)) {
				LOG_ERR("Already waiting for account key write");
			} else {
				WRITE_BIT(proc->wait_for_mask, WAIT_FOR_ACCOUNT_KEY_BIT_POS, 1);
			}
		}
	}
}

void fp_keys_drop_key(const struct bt_conn *conn)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	invalidate_key(&fp_procedures[bt_conn_index(conn)]);
}

static void key_gen_failure_cnt_reset_fn(struct k_work *w)
{
	key_gen_failure_cnt = 0;
	LOG_INF("Key generation failure counter reset");
}

static void timeout_fn(struct k_work *w)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	struct fp_procedure *proc = CONTAINER_OF(w, struct fp_procedure, timeout.work);

	__ASSERT_NO_MSG(is_key_generated(proc->state));

	LOG_INF("Key discarded (timeout)");
	key_timeout(proc);
}

static int fp_keys_init(void)
{
	if (is_enabled) {
		LOG_WRN("fp_keys module already initialized");
		return 0;
	}

	static bool timeout_works_initialized;

	for (size_t i = 0; i < ARRAY_SIZE(fp_procedures); i++) {
		struct fp_procedure *proc = &fp_procedures[i];

		proc->state = FP_STATE_INITIAL;
		memset(proc->aes_key, EMPTY_AES_KEY_BYTE, sizeof(proc->aes_key));
		if (!timeout_works_initialized) {
			k_work_init_delayable(&proc->timeout, timeout_fn);
		}
		proc->wait_for_mask = 0;
		proc->pairing_mode = false;
	}

	if (!timeout_works_initialized) {
		timeout_works_initialized = true;
	}

	is_enabled = true;

	return 0;
}

static int fp_keys_uninit(void)
{
	if (!is_enabled) {
		LOG_WRN("fp_auth module already uninitialized");
		return 0;
	}

	is_enabled = false;

	for (size_t i = 0; i < ARRAY_SIZE(fp_procedures); i++) {
		struct fp_procedure *proc = &fp_procedures[i];
		int ret;

		ret = k_work_cancel_delayable(&proc->timeout);
		__ASSERT_NO_MSG(ret == 0);
		ARG_UNUSED(ret);
	}

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_keys, FP_ACTIVATION_INIT_PRIORITY_DEFAULT, fp_keys_init,
			      fp_keys_uninit);
