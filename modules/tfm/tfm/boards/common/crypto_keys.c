/*
 * Copyright (c) 2017-2020 Arm Limited. All rights reserved.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <string.h>

#include "psa/crypto_types.h"
#include "tfm_plat_crypto_keys.h"
#include <hw_unique_key.h>

#ifdef NRF5340_XXAA_APPLICATION
#define TFM_KEY_LEN_BYTES  32
#elif defined(NRF9160_XXAA)
#define TFM_KEY_LEN_BYTES  16
#endif

extern const psa_ecc_family_t initial_attestation_curve_type;
extern const uint8_t  initial_attestation_private_key[];
extern const uint32_t initial_attestation_private_key_size;

enum tfm_plat_err_t tfm_plat_get_huk_derived_key(const uint8_t *label,
		size_t label_size, const uint8_t *context, size_t context_size,
		uint8_t *key, size_t key_size)
{
	if (key_size > TFM_KEY_LEN_BYTES) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	int err  = hw_unique_key_derive_key(HUK_KEYSLOT_MKEK, context,
			context_size, label, label_size, key, key_size);

	return err;
}

enum tfm_plat_err_t
tfm_plat_get_initial_attest_key(uint8_t *key_buf, uint32_t size,
		struct ecc_key_t *ecc_key, psa_ecc_family_t *curve_type)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}
