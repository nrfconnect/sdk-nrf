/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_AUTH_H_
#define _FP_FMDN_AUTH_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup fp_fmdn_auth Fast Pair FMDN Authentication
 * @brief Internal API for Fast Pair FMDN Authentication
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "fp_common.h"
#include <zephyr/types.h>

/* Length in bytes of the authentication segment. */
#define FP_FMDN_AUTH_SEG_LEN (CONFIG_BT_FAST_PAIR_FMDN_AUTH_SEG_LEN)

/* Length in bytes of the Recovery Key. */
#define FP_FMDN_AUTH_KEY_RECOVERY_LEN (8U)
/* Length in bytes of the Ring Key. */
#define FP_FMDN_AUTH_KEY_RING_LEN (8U)
/* Length in bytes of the Unwanted Tracking Protection (UTP) Key. */
#define FP_FMDN_AUTH_KEY_UTP_LEN (8U)

/** Authentication key types */
enum fp_fmdn_auth_key_type {
	/* Recovery Key type */
	FP_FMDN_AUTH_KEY_TYPE_RECOVERY,

	/* Ring Key type */
	FP_FMDN_AUTH_KEY_TYPE_RING,

	/* Unwanted Tracking Protection (UTP) Key type */
	FP_FMDN_AUTH_KEY_TYPE_UTP,
};

/** Generate the Authentication Key which is used to create authentication segments.
 *  The Authentication Key is derived from the Ephemeral Identity Key (EIK).
 *
 * @param[in]  key_type Type of the Authentication Key.
 * @param[out] auth_key The generated Authentication Key.
 * @param[in]  auth_key_len Length of the Authentication Key.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_auth_key_generate(enum fp_fmdn_auth_key_type key_type,
			      uint8_t *auth_key,
			      size_t auth_key_len);

/** Authentication seed data */
struct fp_fmdn_auth_data {
	/** Random Nonce */
	const uint8_t *random_nonce;

	/** Data ID */
	uint8_t data_id;

	/** Data Length */
	uint8_t data_len;

	/** Additional Data */
	const uint8_t *add_data;
};

/** Validate if the authentication segment is correct. This function generates
 *  the local authentication segment from the Authentication Key and the
 *  authentication seed data. This local segment is compared against the
 *  remote segment.
 *
 * @param[in] key       The Authentication Key.
 * @param[in] key_len   Length of the Authentication Key.
 * @param[in] auth_data Authentication seed data.
 * @param[in] auth_seg  Remote authentication segment.
 *
 * @return True if the authentication segment is correct, False Otherwise.
 */
bool fp_fmdn_auth_seg_validate(const uint8_t *key,
			       size_t key_len,
			       const struct fp_fmdn_auth_data *auth_data,
			       const uint8_t auth_seg[FP_FMDN_AUTH_SEG_LEN]);

/** Generate the local authentication segment from the Authentication Key
 *  and the authentication seed data. This segment is used by the remote peer
 *  to authenticate the local peer. The authentication seed is extended by one
 *  byte with a fixed value to make it different from the seed used by the
 *  @ref fp_fmdn_auth_seg_validate.
 *
 * @param[in]  key       The Authentication Key.
 * @param[in]  key_len   Length of the Authentication Key.
 * @param[in]  auth_data Authentication seed data.
 * @param[out] auth_seg  Local authentication segment.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_auth_seg_generate(const uint8_t *key,
			      size_t key_len,
			      struct fp_fmdn_auth_data *auth_data,
			      uint8_t auth_seg[FP_FMDN_AUTH_SEG_LEN]);

/** Find the Account Key that matches the remote authentication segment.
 *  This function generates the local authentication segment from the
 *  Account Key and the authentication seed data. This local segment is compared
 *  against the remote segment. The operation is repeated for every Account Key
 *  until a matching key is found.
 *
 * @param[in]  auth_data   Authentication seed data.
 * @param[in]  auth_seg    Remote authentication segment.
 * @param[out] account_key The matching Account Key.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_auth_account_key_find(const struct fp_fmdn_auth_data *auth_data,
				  const uint8_t auth_seg[FP_FMDN_AUTH_SEG_LEN],
				  struct fp_account_key *account_key);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_AUTH_H_ */
