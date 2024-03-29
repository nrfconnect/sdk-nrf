/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_CRYPTO_H__
#define MOCK_SUIT_CRYPTO_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <psa/crypto.h>

FAKE_VALUE_FUNC(psa_status_t, psa_hash_setup, psa_hash_operation_t *, psa_algorithm_t);
FAKE_VALUE_FUNC(psa_status_t, psa_hash_update, psa_hash_operation_t *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(psa_status_t, psa_hash_abort, psa_hash_operation_t *);
FAKE_VALUE_FUNC(psa_status_t, psa_hash_verify, psa_hash_operation_t *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(psa_status_t, psa_verify_message, mbedtls_svc_key_id_t, psa_algorithm_t,
		const uint8_t *, size_t, const uint8_t *, size_t);

static inline void mock_suit_crypto_reset(void)
{
	RESET_FAKE(psa_hash_setup);
	RESET_FAKE(psa_hash_update);
	RESET_FAKE(psa_hash_abort);
	RESET_FAKE(psa_hash_verify);
	RESET_FAKE(psa_verify_message);
}

#endif /* MOCK_SUIT_CRYPTO_H__ */
