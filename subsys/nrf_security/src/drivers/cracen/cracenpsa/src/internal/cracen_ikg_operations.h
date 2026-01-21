/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_ikg
 * @{
 * @brief Internal Key Generation (IKG) support for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * This module provides APIs for generating and using identity keys through
 * the CRACEN hardware's Internal Key Generation (IKG) feature.
 */

#ifndef CRACEN_IKG_OPERATIONS_H
#define CRACEN_IKG_OPERATIONS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <psa/crypto.h>

/** @brief Sign a message using an identity key.
 *
 * @param[in] identity_key_index Index of the identity key to use.
 * @param[in] hashalg            Hash algorithm to use for signing.
 * @param[in] curve              Elliptic curve parameters.
 * @param[in] message            Message to sign.
 * @param[in] message_length     Length of the message in bytes.
 * @param[out] signature         Buffer to store the signature.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_IK_NOT_READY The IK module is not ready.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ikg_sign_message(int identity_key_index, const struct sxhashalg *hashalg,
			    const struct sx_pk_ecurve *curve, const uint8_t *message,
			    size_t message_length, uint8_t *signature);

/** @brief Sign a digest using an identity key.
 *
 * @param[in] identity_key_index Index of the identity key to use.
 * @param[in] hashalg            Hash algorithm to use for message digest.
 * @param[in] curve              Elliptic curve parameters.
 * @param[in] digest             Digest to sign.
 * @param[in] digest_length      Length of the digest in bytes.
 * @param[out] signature         Buffer to store the signature.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_IK_NOT_READY The IK module is not ready.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ikg_sign_digest(int identity_key_index, const struct sxhashalg *hashalg,
			   const struct sx_pk_ecurve *curve, const uint8_t *digest,
			   size_t digest_length, uint8_t *signature);

/** @brief Create a public key from an identity key.
 *
 * @param[in] identity_key_index Index of the identity key to use.
 * @param[out] pub_key           Buffer to store the public key.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_IK_NOT_READY The IK module is not ready.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ikg_create_pub_key(int identity_key_index, uint8_t *pub_key);

/** @} */

#endif /* CRACEN_IKG_OPERATIONS_H */
