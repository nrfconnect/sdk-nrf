/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include "fp_crypto.h"
#include "fp_fhn_auth.h"
#include "fp_fhn_state.h"
#include "fp_storage_ak.h"

#include <zephyr/net_buf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_auth, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#define AUTH_DATA_RSP_END_BYTE     (0x01)
#define AUTH_DATA_RSP_END_BYTE_LEN (1U)

/* Byte length of the fixed fields that precede the Additional Data in the authentication
 * data input: Protocol Major Version || random_nonce || Data ID || Data Length.
 */
#define AUTH_DATA_HDR_LEN                           \
	(1U /* Protocol Major Version */ +          \
	 CONFIG_BT_FAST_PAIR_FHN_RANDOM_NONCE_LEN + \
	 1U /* Data ID */ +                         \
	 1U /* Data Length */)

/* Byte length of the buffer that holds the authentication data input. The Data Length
 * parameter accounts for the authentication segment, which is not a part of the input,
 * hence the subtraction. The additional byte accommodates the end byte that the response
 * encoding appends, so that a response at the maximum supported Data Length also fits.
 */
#define AUTH_DATA_BUF_LEN                                       \
	((CONFIG_BT_FAST_PAIR_FHN_BEACON_ACTIONS_MAX_DATA_LEN - \
	  FP_FHN_AUTH_SEG_LEN) +                                \
	 AUTH_DATA_HDR_LEN +                                    \
	 AUTH_DATA_RSP_END_BYTE_LEN)

#define EIK_DERIVED_KEY_SEED_END_BYTE_LEN (1U)
#define EIK_DERIVED_KEY_SEED_BUF_LEN \
	(FP_FHN_STATE_EIK_LEN + EIK_DERIVED_KEY_SEED_END_BYTE_LEN)

struct account_key_find_context {
	struct net_buf_simple *auth_data_buf;
	const uint8_t *auth_seg;
};

static bool auth_seg_compare(const struct net_buf_simple *auth_data_buf,
			     const uint8_t *key,
			     size_t key_len,
			     const uint8_t *auth_seg)
{
	int err;
	uint8_t local_auth_seg[FP_CRYPTO_SHA256_HASH_LEN];

	err = fp_crypto_hmac_sha256(local_auth_seg,
				    auth_data_buf->data,
				    auth_data_buf->len,
				    key,
				    key_len);
	if (err) {
		LOG_ERR("Authentication: Hash Comparison: fp_crypto_hmac_sha256 failed: %d", err);

		return false;
	}

	return !memcmp(local_auth_seg, auth_seg, FP_FHN_AUTH_SEG_LEN);
}

static bool account_key_find_iterator(const struct fp_account_key *account_key, void *context)
{
	struct account_key_find_context *account_key_find_context = context;

	return auth_seg_compare(account_key_find_context->auth_data_buf,
				account_key->key,
				sizeof(account_key->key),
				account_key_find_context->auth_seg);
}

static int auth_data_encode(struct net_buf_simple *auth_data_buf,
			    const struct fp_fhn_auth_data *auth_data)
{
	size_t add_data_len;

	if (auth_data->data_len < FP_FHN_AUTH_SEG_LEN) {
		LOG_ERR("Authentication: Data Length parameter too small: %u",
			auth_data->data_len);
		__ASSERT_NO_MSG(0);

		return -EINVAL;
	}

	add_data_len = auth_data->data_len - FP_FHN_AUTH_SEG_LEN;

	/* The buffer is sized from CONFIG_BT_FAST_PAIR_FHN_BEACON_ACTIONS_MAX_DATA_LEN, and
	 * the callers reject requests that declare a larger Data Length. Validate against the
	 * buffer that was passed in, so that the encoder cannot overflow it regardless.
	 */
	if (net_buf_simple_tailroom(auth_data_buf) < (AUTH_DATA_HDR_LEN + add_data_len)) {
		LOG_ERR("Authentication: Data Length parameter too large: %u",
			auth_data->data_len);
		__ASSERT_NO_MSG(0);

		return -EINVAL;
	}

	/* Prepare Authentication data input for HMAC-SHA256 operation:
	 * (Protocol Major Version || random_nonce || Data ID || Data length ||
	 * Additional data).
	 */
	net_buf_simple_add_u8(auth_data_buf, CONFIG_BT_FAST_PAIR_FHN_VERSION_MAJOR);
	net_buf_simple_add_mem(auth_data_buf,
			       auth_data->random_nonce,
			       CONFIG_BT_FAST_PAIR_FHN_RANDOM_NONCE_LEN);
	net_buf_simple_add_u8(auth_data_buf, auth_data->data_id);
	net_buf_simple_add_u8(auth_data_buf, auth_data->data_len);

	if (auth_data->add_data) {
		net_buf_simple_add_mem(auth_data_buf, auth_data->add_data, add_data_len);
	}

	return 0;
}

static int auth_data_rsp_encode(struct net_buf_simple *auth_data_buf,
				const struct fp_fhn_auth_data *auth_data)
{
	int err;

	err = auth_data_encode(auth_data_buf, auth_data);
	if (err) {
		return err;
	}

	if (net_buf_simple_tailroom(auth_data_buf) < AUTH_DATA_RSP_END_BYTE_LEN) {
		LOG_ERR("Authentication: no room for the response end byte");
		__ASSERT_NO_MSG(0);

		return -EINVAL;
	}

	/* Append additional end byte to authentication data segment of the response packet. */
	net_buf_simple_add_u8(auth_data_buf, AUTH_DATA_RSP_END_BYTE);

	return 0;
}


static int eik_derived_auth_key_generate(uint8_t seed_end_byte,
					 uint8_t *eik_derived_key,
					 size_t eik_derived_key_len)
{
	int err;
	uint8_t *eik;
	uint8_t eik_derived_key_full[FP_CRYPTO_SHA256_HASH_LEN];

	NET_BUF_SIMPLE_DEFINE(eik_derived_key_seed_buf, EIK_DERIVED_KEY_SEED_BUF_LEN);

	/* Calculate the EIK Derived Key as the first 8 bytes of the following operation:
	 * SHA256(Ephemeral Identity Key || seed end byte)
	 */
	eik = net_buf_simple_add(&eik_derived_key_seed_buf, FP_FHN_STATE_EIK_LEN);
	net_buf_simple_add_u8(&eik_derived_key_seed_buf, seed_end_byte);

	__ASSERT(eik_derived_key_seed_buf.len == EIK_DERIVED_KEY_SEED_BUF_LEN,
		"Authentication: key generation: incorrect seed buf length");

	err = fp_fhn_state_eik_read(eik);
	if (err) {
		LOG_ERR("Authentication: key generation: EIK read failed: %d", err);

		return err;
	}

	err = fp_crypto_sha256(eik_derived_key_full,
			       eik_derived_key_seed_buf.data,
			       eik_derived_key_seed_buf.len);
	if (err) {
		LOG_ERR("Authentication: key generation: fp_crypto_sha256 failed: %d", err);

		return err;
	}

	memcpy(eik_derived_key, eik_derived_key_full, eik_derived_key_len);

	return 0;
}

int fp_fhn_auth_key_generate(enum fp_fhn_auth_key_type key_type,
			     uint8_t *auth_key,
			     size_t auth_key_len)
{
	int err;
	uint8_t seed_end_byte;
	enum {
		KEY_SEED_END_BYTE_RECOVERY = 0x01,
		KEY_SEED_END_BYTE_RING     = 0x02,
		KEY_SEED_END_BYTE_UTP      = 0x03,
	};

	switch (key_type) {
	case FP_FHN_AUTH_KEY_TYPE_RECOVERY:
		seed_end_byte = KEY_SEED_END_BYTE_RECOVERY;
		break;
	case FP_FHN_AUTH_KEY_TYPE_RING:
		seed_end_byte = KEY_SEED_END_BYTE_RING;
		break;
	case FP_FHN_AUTH_KEY_TYPE_UTP:
		seed_end_byte = KEY_SEED_END_BYTE_UTP;
		break;
	default:
		__ASSERT(0, "Authentication: key generation: unsupported key type");
		return -ENOTSUP;
	}

	err = eik_derived_auth_key_generate(seed_end_byte, auth_key, auth_key_len);
	if (err) {
		return err;
	}

	return 0;
}

bool fp_fhn_auth_seg_validate(const uint8_t *key, size_t key_len,
			      const struct fp_fhn_auth_data *auth_data,
			      const uint8_t auth_seg[FP_FHN_AUTH_SEG_LEN])
{
	int err;

	NET_BUF_SIMPLE_DEFINE(auth_data_buf, AUTH_DATA_BUF_LEN);

	/* Encode the input data for the authentication segment. */
	err = auth_data_encode(&auth_data_buf, auth_data);
	if (err) {
		return false;
	}

	return auth_seg_compare(&auth_data_buf, key, key_len, auth_seg);
}

int fp_fhn_auth_seg_generate(const uint8_t *key, size_t key_len,
			     struct fp_fhn_auth_data *auth_data,
			     uint8_t auth_seg[FP_FHN_AUTH_SEG_LEN])
{
	int err;
	uint8_t local_auth_seg[FP_CRYPTO_SHA256_HASH_LEN];

	NET_BUF_SIMPLE_DEFINE(auth_data_buf, AUTH_DATA_BUF_LEN);

	/* Encode the input data for the authentication segment. */
	err = auth_data_rsp_encode(&auth_data_buf, auth_data);
	if (err) {
		return err;
	}

	/* Generate the authentication segment. */
	err = fp_crypto_hmac_sha256(local_auth_seg,
				    auth_data_buf.data,
				    auth_data_buf.len,
				    key,
				    key_len);
	if (err) {
		LOG_ERR("Authentication: authentication segment generation:"
			" fp_crypto_hmac_sha256 failed: %d", err);

		return err;
	}

	/* Copy the first 8 bytes to the destination buffer. */
	memcpy(auth_seg, local_auth_seg, FP_FHN_AUTH_SEG_LEN);

	return 0;
}

int fp_fhn_auth_account_key_find(const struct fp_fhn_auth_data *auth_data,
				 const uint8_t auth_seg[FP_FHN_AUTH_SEG_LEN],
				 struct fp_account_key *account_key)
{
	int err;
	struct account_key_find_context account_key_find_context = {0};

	NET_BUF_SIMPLE_DEFINE(auth_data_buf, AUTH_DATA_BUF_LEN);

	account_key_find_context.auth_data_buf = &auth_data_buf;
	account_key_find_context.auth_seg = auth_seg;

	err = auth_data_encode(account_key_find_context.auth_data_buf, auth_data);
	if (err) {
		return err;
	}

	err = fp_storage_ak_find(account_key,
				 account_key_find_iterator,
				 &account_key_find_context);
	if (err) {
		return err;
	}

	return 0;
}
