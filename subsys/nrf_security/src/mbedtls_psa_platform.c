/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa_crypto_core.h"

#if defined(PSA_CRYPTO_DRIVER_CRACEN)
#include "cracen_psa.h"
#endif

psa_status_t mbedtls_psa_platform_get_builtin_key(mbedtls_svc_key_id_t key_id,
						  psa_key_lifetime_t *lifetime,
						  psa_drv_slot_number_t *slot_number)
{
#if defined(PSA_CRYPTO_DRIVER_CRACEN)
	return cracen_get_key_slot(key_id, lifetime, slot_number);
#endif
	return PSA_ERROR_DOES_NOT_EXIST;
}
