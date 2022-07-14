/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <string.h>

#include "psa/crypto_types.h"
#include "tfm_plat_crypto_keys.h"
#include <hw_unique_key.h>
#include <nrf_cc3xx_platform_kmu.h>
#include <nrf_cc3xx_platform_identity_key.h>
#include <nrf_cc3xx_platform_defines.h>

#ifdef NRF5340_XXAA_APPLICATION
#define TFM_KEY_LEN_BYTES  32
#elif defined(NRF9160_XXAA)
#define TFM_KEY_LEN_BYTES  16
#endif

enum tfm_plat_err_t
tfm_plat_get_huk_derived_key(const uint8_t *label, size_t label_size,
			     const uint8_t *context, size_t context_size,
			     uint8_t *key, size_t key_size)
{
	if (key_size > TFM_KEY_LEN_BYTES) {
		return TFM_PLAT_ERR_INVALID_INPUT;
	}

	int err  = hw_unique_key_derive_key(HUK_KEYSLOT_MEXT, context,
			context_size, label, label_size, key, key_size);

	if (err) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
tfm_plat_get_initial_attest_key(uint8_t *key_buf, uint32_t size,
                                struct ecc_key_t *ecc_key,
                                psa_ecc_family_t *curve_type)
{
        int err;

        if (size < 32) {
                return TFM_PLAT_ERR_INVALID_INPUT;
        }

        err = nrf_cc3xx_platform_identity_key_retrieve(
                NRF_KMU_SLOT_KIDENT,
                key_buf);

        if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
                return TFM_PLAT_ERR_SYSTEM_ERR;
        }

        ecc_key->priv_key = key_buf;
        ecc_key->priv_key_size = 32;

        ecc_key->pubx_key = NULL;
        ecc_key->pubx_key_size = 0;
        ecc_key->puby_key = NULL;
        ecc_key->puby_key_size = 0;

        /* Set the EC curve type which the key belongs to */
        *curve_type = PSA_ECC_FAMILY_SECP_R1;

        return TFM_PLAT_ERR_SUCCESS;
}
