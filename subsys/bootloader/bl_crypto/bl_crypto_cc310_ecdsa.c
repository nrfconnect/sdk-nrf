/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <nrf_cc310_bl_ecdsa_verify_secp256r1.h>
#include <nrf_cc310_bl_hash_sha256.h>
#include "bl_crypto_internal.h"
#include "bl_crypto_cc310_common.h"

bool verify_sig(const u8_t *data, u32_t data_len, const u8_t *sig,
		const u8_t *pk)
{
	nrf_cc310_bl_ecdsa_verify_context_secp256r1_t context;
	nrf_cc310_bl_hash_digest_sha256_t hash1;
	nrf_cc310_bl_hash_digest_sha256_t hash2;

	if (!get_hash((u8_t *)&hash1, data, data_len)) {
		return false;
	}

	if (!get_hash((u8_t *)&hash2, hash1, CONFIG_SB_HASH_LEN)) {
		return false;
	}

	if (!cc310_bl_init()) {
		return false;
	}

	cc310_bl_backend_enable();

	bool retval = (nrf_cc310_bl_ecdsa_verify_secp256r1(
			       &context,
			       (nrf_cc310_bl_ecc_public_key_secp256r1_t *)pk,
			       (nrf_cc310_bl_ecc_signature_secp256r1_t *)sig,
			       hash2, CONFIG_SB_HASH_LEN) == CRYS_OK);

	cc310_bl_backend_disable();

	return retval;
}
