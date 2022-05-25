/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_registration_data.h"
#include "fp_keys.h"
#include "fp_crypto.h"
#include "fp_common.h"
#include "fp_storage.h"

#define EMPTY_AES_KEY_BYTE	0xff
#define FP_ACCOUNT_KEY_PREFIX	0x04

#define FP_KEY_TIMEOUT				K_SECONDS(10)
#define FP_KEY_GEN_FAILURE_MAX_CNT		10
#define FP_KEY_GEN_FAILURE_CNT_RESET_TIMEOUT	K_MINUTES(5)

enum fp_state {
	FP_STATE_INITIAL,
	FP_STATE_USE_TEMP_KEY,
	FP_STATE_WAIT_FOR_ACCOUNT_KEY,
};

struct fp_procedure {
	struct k_work_delayable timeout;
	enum fp_state state;
	uint8_t key_gen_failure_cnt;
	bool pairing_mode;
	uint8_t aes_key[FP_CRYPTO_ACCOUNT_KEY_LEN];
};

static bool user_pairing_mode = true;
static struct fp_procedure fp_procedures[CONFIG_BT_MAX_CONN];


void bt_fast_pair_set_pairing_mode(bool pairing_mode)
{
	user_pairing_mode = pairing_mode;
}

static void invalidate_key(struct fp_procedure *proc)
{
	if (proc->state != FP_STATE_INITIAL) {
		proc->state = FP_STATE_INITIAL;
		memset(proc->aes_key, EMPTY_AES_KEY_BYTE, sizeof(proc->aes_key));
		k_work_cancel_delayable(&proc->timeout);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	if (!err) {
		proc->pairing_mode = user_pairing_mode;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	invalidate_key(proc);
	proc->key_gen_failure_cnt = 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int fp_keys_encrypt(struct bt_conn *conn, uint8_t *out, const uint8_t *in)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	switch (proc->state) {
	case FP_STATE_USE_TEMP_KEY:
	case FP_STATE_WAIT_FOR_ACCOUNT_KEY:
		break;

	case FP_STATE_INITIAL:
	default:
		err = -EACCES;
		break;
	}

	if (!err) {
		err = fp_crypto_aes128_encrypt(out, in, proc->aes_key);
	}

	return err;
}

int fp_keys_decrypt(struct bt_conn *conn, uint8_t *out, const uint8_t *in)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	switch (proc->state) {
	case FP_STATE_USE_TEMP_KEY:
	case FP_STATE_WAIT_FOR_ACCOUNT_KEY:
		break;

	case FP_STATE_INITIAL:
	default:
		err = -EACCES;
		break;
	}

	if (!err) {
		err = fp_crypto_aes128_decrypt(out, in, proc->aes_key);
	}

	return err;
}

static int key_gen_public_key(struct bt_conn *conn, struct fp_keys_keygen_params *keygen_params)
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

static int key_gen_account_key(struct bt_conn *conn, struct fp_keys_keygen_params *keygen_params)
{
	int err;
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	uint8_t req[FP_CRYPTO_AES128_BLOCK_LEN];
	uint8_t ak[CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX][FP_CRYPTO_ACCOUNT_KEY_LEN];
	size_t ak_cnt = CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX;

	err = fp_storage_account_keys_get(ak, &ak_cnt);
	if (err) {
		return err;
	}

	for (size_t i = 0; i < ak_cnt; i++) {
		memcpy(proc->aes_key, ak[i], FP_CRYPTO_ACCOUNT_KEY_LEN);

		err = fp_keys_decrypt(conn, req, keygen_params->req_enc);
		if (!err) {
			err = keygen_params->req_validate_cb(conn, req, keygen_params->context);
		}

		if (!err) {
			/* Key was found. */
			break;
		}
	}

	return err;
}

int fp_keys_generate_key(struct bt_conn *conn, struct fp_keys_keygen_params *keygen_params)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (proc->key_gen_failure_cnt >= FP_KEY_GEN_FAILURE_MAX_CNT) {
		return -EACCES;
	}

	if (proc->state != FP_STATE_INITIAL) {
		LOG_WRN("Invalid state during key generation");
		return -EACCES;
	}

	/* Update state to ensure that key could be used for decryption. */
	proc->state = FP_STATE_USE_TEMP_KEY;

	if (keygen_params->public_key) {
		if (proc->pairing_mode) {
			err = key_gen_public_key(conn, keygen_params);
		} else {
			err = -EACCES;
		}
	} else {
		err = key_gen_account_key(conn, keygen_params);
	}

	if (err) {
		invalidate_key(proc);
		proc->key_gen_failure_cnt++;
		if (proc->key_gen_failure_cnt >= FP_KEY_GEN_FAILURE_MAX_CNT) {
			LOG_WRN("Key generation failure limit exceeded");
			k_work_reschedule(&proc->timeout, FP_KEY_GEN_FAILURE_CNT_RESET_TIMEOUT);
		}
	} else {
		proc->key_gen_failure_cnt = 0;
		if (proc->state == FP_STATE_USE_TEMP_KEY) {
			k_work_reschedule(&proc->timeout, FP_KEY_TIMEOUT);
		}
	}

	return err;
}

int fp_keys_store_account_key(struct bt_conn *conn, const uint8_t *account_key)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];
	int err = 0;

	if (proc->state != FP_STATE_WAIT_FOR_ACCOUNT_KEY) {
		LOG_WRN("Invalid state during Account Key write");
		return -EACCES;
	}

	/* Invalidate Key to ensure that requests will be rejected until procedure is restarted. */
	invalidate_key(proc);

	if (account_key[0] != FP_ACCOUNT_KEY_PREFIX) {
		LOG_WRN("Received invalid Account Key");
		return -EINVAL;
	}
	err = fp_storage_account_key_save(account_key);
	if (!err) {
		LOG_DBG("Account Key stored");
	} else {
		LOG_WRN("Store account key error: err=%d", err);
	}

	return err;
}

void fp_keys_bt_auth_progress(struct bt_conn *conn, bool authenticated)
{
	struct fp_procedure *proc = &fp_procedures[bt_conn_index(conn)];

	if (proc->state == FP_STATE_USE_TEMP_KEY) {
		k_work_reschedule(&proc->timeout, FP_KEY_TIMEOUT);
		if (authenticated) {
			proc->state = FP_STATE_WAIT_FOR_ACCOUNT_KEY;
		}
	}
}

void fp_keys_drop_key(struct bt_conn *conn)
{
	invalidate_key(&fp_procedures[bt_conn_index(conn)]);
}

static void timeout_fn(struct k_work *w)
{
	struct fp_procedure *proc = CONTAINER_OF(w, struct fp_procedure, timeout);

	if (proc->key_gen_failure_cnt >= FP_KEY_GEN_FAILURE_MAX_CNT) {
		proc->key_gen_failure_cnt = 0;
	} else {
		LOG_WRN("Key discarded (timeout)");
		invalidate_key(proc);
	}
}

static int fp_keys_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	for (size_t i = 0; i < ARRAY_SIZE(fp_procedures); i++) {
		struct fp_procedure *proc = &fp_procedures[i];

		proc->state = FP_STATE_INITIAL;
		memset(proc->aes_key, EMPTY_AES_KEY_BYTE, sizeof(proc->aes_key));
		k_work_init_delayable(&proc->timeout, timeout_fn);
	}

	return 0;
}

SYS_INIT(fp_keys_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
