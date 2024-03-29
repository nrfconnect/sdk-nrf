/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SDFW_BUILTIN_KEYS_H__
#define MOCK_SDFW_BUILTIN_KEYS_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <sdfw/sdfw_builtin_keys.h>

FAKE_VALUE_FUNC(bool, sdfw_builtin_keys_is_builtin, mbedtls_svc_key_id_t);
FAKE_VALUE_FUNC(int, sdfw_builtin_keys_verify_message, mbedtls_svc_key_id_t, psa_algorithm_t,
		const uint8_t *, size_t, const uint8_t *, size_t);

static inline void mock_sdfw_builtin_keys_reset(void)
{
	RESET_FAKE(sdfw_builtin_keys_is_builtin);
	sdfw_builtin_keys_is_builtin_fake.return_val = false;
	RESET_FAKE(sdfw_builtin_keys_verify_message);
}
#endif /* MOCK_SDFW_BUILTIN_KEYS_H__ */
