/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto_psa_key_allocator.h"

#ifdef CONFIG_OPENTHREAD_PSA_NVM_BACKEND_KMU
#include <cracen_psa_kmu.h>

#define OPENTHREAD_KMU_SINGLE_KEY_SLOT_SIZE (2)
/* Reserved for six 128-bit keys, and one 256-bit key */
#define OPENTHREAD_KMU_DEDICATED_SLOTS	    (8)
#endif /* CONFIG_OPENTHREAD_PSA_NVM_BACKEND_KMU */

otError getKeyRef(otCryptoKeyRef *aInputKeyRef, psa_key_attributes_t *aAttributes)
{
	if (!aInputKeyRef) {
		return OT_ERROR_INVALID_ARGS;
	}

#if defined(CONFIG_OPENTHREAD_PSA_NVM_BACKEND_ITS)
	/* In this case we do not need make any adjustments to the input KeyRef or Attributes */
	return OT_ERROR_NONE;

#elif defined(CONFIG_OPENTHREAD_PSA_NVM_BACKEND_KMU)
	/* Exit function if the key is outside the expected range dedicated to persistent lifetime.
	 * Currently we supports only 7 keys defined in the openthread/src/core/crypto/storage.hpp
	 * file. The first six keys are 128-bit length (1 KMU slot), and the last one is 256-bit
	 * length (2 kmu slots). If the OPENTHREAD_KMU_DEDICATED_SLOTS is changed the new
	 * formula for converting keys must be written, because we must take into account the shift
	 * of two slots for ECDSA private key (7th key).
	 */
	if (*aInputKeyRef <= CONFIG_OPENTHREAD_PSA_ITS_NVM_OFFSET ||
	    *aInputKeyRef > CONFIG_OPENTHREAD_PSA_ITS_NVM_OFFSET + OPENTHREAD_KMU_DEDICATED_SLOTS) {
		return OT_ERROR_NONE;
	}

	/* Convert key to KMU slot, currently all keys except ECDSA private key are 128-bit length
	so need one KMU slot. the ECDSA privatekey is the last one on the list, so we can simply
	convert it one by one. Decrease it by 1 to get the correct slot as aInputKeyRef starts
	from 1.
	 */
	*aInputKeyRef = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW,
		CONFIG_OPENTHREAD_KMU_SLOT_START +
			(*aInputKeyRef - CONFIG_OPENTHREAD_PSA_ITS_NVM_OFFSET) - 1);

	/* Convert key attributes to meet KMU requirements */
	if (aAttributes) {

		/* Set the key location and lifetime to persistent and CRACEN KMU */
		psa_set_key_lifetime(aAttributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							  PSA_KEY_PERSISTENCE_DEFAULT,
							  PSA_KEY_LOCATION_CRACEN_KMU));

		/* Convert to a type which KMU currently supports.
		   TODO: remove this part once KMU driver supports these types.
		   PSA_KEY_TYPE_RAW_DATA <-> PSA_KEY_TYPE_AES
		*/
		if (psa_get_key_type(aAttributes) == PSA_KEY_TYPE_RAW_DATA) {
			psa_set_key_type(aAttributes, PSA_KEY_TYPE_AES);
		} else if (psa_get_key_type(aAttributes) == PSA_KEY_TYPE_AES) {
			psa_set_key_type(aAttributes, PSA_KEY_TYPE_RAW_DATA);
		}

		/* Convert algorithm to the current meet KMU limitations:
		0 <-> PSA_ALG_GCM
		PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256) <-> PSA_ALG_ECDSA(PSA_ALG_ANY_HASH)
		TODO: remove this part once KMU driver supports these algorithms:
		*/
		if (psa_get_key_algorithm(aAttributes) == 0) {
			psa_set_key_algorithm(aAttributes, PSA_ALG_GCM);
		} else if (psa_get_key_algorithm(aAttributes) == PSA_ALG_GCM) {
			psa_set_key_algorithm(aAttributes, 0);
		} else if (psa_get_key_algorithm(aAttributes) ==
			   PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256)) {
			psa_set_key_algorithm(aAttributes, PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
		} else if (psa_get_key_algorithm(aAttributes) == PSA_ALG_ECDSA(PSA_ALG_ANY_HASH)) {
			psa_set_key_algorithm(aAttributes,
					      PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
		}
	}
#endif /* CONFIG_OPENTHREAD_PSA_NVM_BACKEND */

	return OT_ERROR_NONE;
}
