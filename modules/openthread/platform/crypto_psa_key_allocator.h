/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENTHREAD_PLATFORM_CRYPTO_PSA_KEY_ALLOCATOR_H_
#define OPENTHREAD_PLATFORM_CRYPTO_PSA_KEY_ALLOCATOR_H_

#include <psa/crypto.h>
#include <openthread/platform/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A macro helper for using the getKeyRef function.
 * This macro automatically returns the error code if the function fails.
 */
#define GET_KEY_REF(keyRef, attributes)                                                            \
	do {                                                                                       \
		otError err = getKeyRef((keyRef), (attributes));                                   \
		if (err != OT_ERROR_NONE) {                                                        \
			return err;                                                                \
		}                                                                                  \
	} while (0)

/**
 * @brief Returns a PSA key reference.
 *
 * These function searches the chosen PSA NVM backend and return a PSA key with the specified
 * attributes.
 *
 * The input aInputKeyRef should contain already defined key reference, and
 * the functions will attempt to re-allocate the key to the chosen PSA nvm backend.
 *
 * @param[out] aInputKeyRef Pointer to store the allocated key reference
 * @param[in] aAttributes Pointer to PSA key attributes structure defining the key properties
 *
 * @returns OT_ERROR_NONE if key allocation was successful
 *          OT_ERROR_FAILED if key allocation failed
 *          OT_ERROR_INVALID_ARGS if invalid parameters were provided
 */
otError getKeyRef(otCryptoKeyRef *aInputKeyRef, psa_key_attributes_t *aAttributes);

#ifdef __cplusplus
}
#endif

#endif /* OPENTHREAD_PLATFORM_CRYPTO_PSA_KEY_ALLOCATOR_H_ */
