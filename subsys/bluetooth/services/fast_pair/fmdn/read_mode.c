/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_read_mode, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>
#include <dult.h>

#include "fp_fmdn_auth.h"
#include "fp_fmdn_callbacks.h"
#include "fp_fmdn_dult_integration.h"
#include "fp_fmdn_state.h"
#include "fp_activation.h"
#include "fp_crypto.h"

/* Timeout for the FMDN recovery mode */
#define FMDN_RECOVERY_TIMEOUT (CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT)

BUILD_ASSERT(FMDN_RECOVERY_TIMEOUT > 0);

static const struct bt_fast_pair_fmdn_read_mode_cb *read_mode_cb;

static void fmdn_recovery_timeout_handle(struct k_work *w);

static K_WORK_DELAYABLE_DEFINE(fmdn_recovery_timeout_work,
			       fmdn_recovery_timeout_handle);

static void read_mode_exited(enum bt_fast_pair_fmdn_read_mode mode)
{
	__ASSERT_NO_MSG(
		(mode == BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY) ||
		(mode == BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID));

	if (read_mode_cb && read_mode_cb->exited) {
		read_mode_cb->exited(mode);
	}
}

static void fmdn_recovery_timeout_handle(struct k_work *w)
{
	ARG_UNUSED(w);

	read_mode_exited(BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY);
}

static int dult_id_payload_get(uint8_t *buf, size_t *len)
{
	static const size_t payload_eid_len = 10;
	static const size_t payload_sha_len = 8;
	static const size_t payload_len = payload_eid_len + payload_sha_len;

	int err;
	uint8_t eid[FP_FMDN_STATE_EID_LEN];
	uint8_t local_sha[FP_CRYPTO_SHA256_HASH_LEN];
	uint8_t recovery_key[FP_FMDN_AUTH_KEY_RECOVERY_LEN];
	struct net_buf_simple payload;

	BUILD_ASSERT(payload_len <= CONFIG_DULT_BT_ANOS_ID_PAYLOAD_LEN_MAX);

	if (*len < payload_len) {
		LOG_ERR("FMDN Read Mode: Insufficient Identifier buffer length");
		return -EINVAL;
	}

	err = fp_fmdn_state_eid_read(eid);
	if (err) {
		LOG_ERR("FMDN Read Mode: fp_fmdn_state_eid_read failed: %d", err);
		return err;
	}

	err = fp_fmdn_auth_key_generate(FP_FMDN_AUTH_KEY_TYPE_RECOVERY,
					recovery_key,
					sizeof(recovery_key));
	if (err) {
		LOG_ERR("FMDN Read Mode: fp_fmdn_auth_key_generate failed: %d", err);
		return err;
	}

	err = fp_crypto_hmac_sha256(local_sha,
				    eid,
				    payload_eid_len,
				    recovery_key,
				    sizeof(recovery_key));
	if (err) {
		LOG_ERR("FMDN Read Mode: fp_crypto_hmac_sha256 failed: %d", err);
		return err;
	}

	net_buf_simple_init_with_data(&payload, buf, *len);
	/* Reset the buffer to allow for writing the Identifier. */
	net_buf_simple_reset(&payload);
	net_buf_simple_add_mem(&payload, eid, payload_eid_len);
	net_buf_simple_add_mem(&payload, local_sha, payload_sha_len);

	*len = payload.len;

	return 0;
}

static void dult_id_read_state_exited(void)
{
	read_mode_exited(BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID);
}

static const struct dult_id_read_state_cb dult_id_read_state_cb = {
	.payload_get = dult_id_payload_get,
	.exited = dult_id_read_state_exited,
};

int bt_fast_pair_fmdn_read_mode_cb_register(const struct bt_fast_pair_fmdn_read_mode_cb *cb)
{
	if (bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if (!cb || !cb->exited) {
		return -EINVAL;
	}

	read_mode_cb = cb;

	return 0;
}

static int read_mode_fmdn_recovery_enter(void)
{
	(void) k_work_reschedule(&fmdn_recovery_timeout_work,
				 K_MINUTES(FMDN_RECOVERY_TIMEOUT));

	return 0;
}

static int read_mode_dult_id_enter(void)
{
	int err;

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT)) {
		return -ENOTSUP;
	}

	err = dult_id_read_state_enter(fp_fmdn_dult_integration_user_get());
	if (err) {
		return err;
	}

	return 0;
}

int bt_fast_pair_fmdn_read_mode_enter(enum bt_fast_pair_fmdn_read_mode mode)
{
	if (!bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if ((mode != BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY) &&
	    (mode != BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID)) {
		return -EINVAL;
	}

	if (!fp_fmdn_state_is_provisioned()) {
		return -EACCES;
	}

	switch (mode) {
	case BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY:
		return read_mode_fmdn_recovery_enter();
	case BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID:
		return read_mode_dult_id_enter();
	default:
		/* Unsupported mode: should not happen. */
		__ASSERT_NO_MSG(0);
		return -EINVAL;
	}
}

int fp_fmdn_read_mode_recovery_mode_request(bool *accepted)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (!fp_fmdn_state_is_provisioned()) {
		return -EACCES;
	}

	*accepted = k_work_delayable_is_pending(&fmdn_recovery_timeout_work);

	return 0;
}

static void read_mode_timeout_force(void)
{
	if (k_work_delayable_is_pending(&fmdn_recovery_timeout_work)) {
		(void) k_work_cancel_delayable(&fmdn_recovery_timeout_work);

		fmdn_recovery_timeout_handle(NULL);
	}
}

static void provisioning_state_changed(bool provisioned)
{
	if (!provisioned) {
		read_mode_timeout_force();
	}
}

static struct bt_fast_pair_fmdn_info_cb fmdn_info_cb = {
	.provisioning_state_changed = provisioning_state_changed,
};

static int init(void)
{
	int err;

	err = fp_fmdn_callbacks_info_cb_register(&fmdn_info_cb);
	if (err) {
		LOG_ERR("FMDN: fp_fmdn_callbacks_info_cb_register returned error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT)) {
		err = dult_id_read_state_cb_register(fp_fmdn_dult_integration_user_get(),
						     &dult_id_read_state_cb);
		if (err) {
			LOG_ERR("FMDN: dult_id_read_state_cb_register returned error: %d", err);
			return err;
		}
	}

	return 0;
}

static int uninit(void)
{
	read_mode_timeout_force();

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_read_mode,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      init,
			      uninit);
