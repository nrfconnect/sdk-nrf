/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <string.h>

#include "psa/crypto_types.h"
#include "tfm_plat_crypto_keys.h"

enum tfm_plat_err_t
tfm_plat_get_huk_derived_key(const uint8_t *label, size_t label_size,
			     const uint8_t *context, size_t context_size,
			     uint8_t *key, size_t key_size)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}


enum tfm_plat_err_t tfm_plat_builtin_key_get_usage(psa_key_id_t key_id,
                                                   mbedtls_key_owner_id_t user,
                                                   psa_key_usage_t *usage)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}

enum tfm_plat_err_t tfm_plat_builtin_key_get_lifetime_and_slot(
    mbedtls_svc_key_id_t key_id,
    psa_key_lifetime_t *lifetime,
    psa_drv_slot_number_t *slot_number)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}

enum tfm_plat_err_t tfm_plat_load_builtin_keys(void)
{
	/* No keys to load */
	return TFM_PLAT_ERR_SUCCESS;
}
