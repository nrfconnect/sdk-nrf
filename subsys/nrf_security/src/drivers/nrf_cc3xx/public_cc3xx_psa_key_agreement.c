/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

/** \file cc3xx_psa_key_agreement.c
 *
 * This file contains the implementation of the entry points associated to the
 * raw key agreement (i.e. ECDH) as described by the PSA Cryptoprocessor Driver
 * interface specification
 *
 */

#include "cc3xx_psa_key_agreement.h"
#include "cc3xx_ecdh_util.h"
#include <zephyr/sys/util.h>

/** \defgroup psa_key_agreement PSA driver entry points for raw key agreement
 *
 *  Entry points for raw key agreement as described by the PSA Cryptoprocessor
 *  Driver interface specification
 *
 *  @{
 */
psa_status_t cc3xx_key_agreement(const psa_key_attributes_t *attributes,
				 const uint8_t *priv_key, size_t priv_key_size,
				 const uint8_t *publ_key, size_t publ_key_size,
				 uint8_t *output, size_t output_size, size_t *output_length,
				 psa_algorithm_t alg)
{
	psa_status_t ret = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_key_bits_t key_bits = psa_get_key_bits(attributes);
	psa_ecc_family_t curve = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);

	if (output_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Will be used as an input output parameter for the internal calls */
	*output_length = output_size;

	switch (alg) {
	case PSA_ALG_ECDH:
		/* CC-312 only supports the X25519 function w/bit size m = 255 */
		if (IS_ENABLED(PSA_NEED_CC3XX_ECDH_MONTGOMERY_255) &&
		    curve == PSA_ECC_FAMILY_MONTGOMERY && key_bits == 255) {
			ret = cc3xx_ecdh_calc_secret_mont(priv_key, priv_key_size,
							  publ_key, publ_key_size,
							  output, output_length);
		} else if (IS_ENABLED(PSA_NEED_CC3XX_ECDH_WEIERSTRASS) &&
			   PSA_ECC_FAMILY_IS_WEIERSTRASS(curve)) {
			ret = cc3xx_ecdh_calc_secret_wrst(curve, key_bits,
							  priv_key, priv_key_size,
							  publ_key, publ_key_size,
							  output, output_size, output_length);
		} else {
			/* ECC type did not match supported features */
			ret = PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	default:
		*output_length = 0;
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (ret != PSA_SUCCESS) {
		*output_length = 0;
	}

	return ret;
}
/** @} */
