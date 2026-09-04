/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PSA_EXT_ECC_H__
#define PSA_EXT_ECC_H__

#include <stdint.h>
#include <stddef.h>

#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup psa_ext_ecc Nordic PSA extension: non-standard EC operations
 * @{
 *
 * @brief Elliptic curve operations that the PSA Crypto API cannot express.
 *
 * These functions are a Nordic extension, not part of the Arm PSA Crypto
 * specification. Two things put them outside PSA:
 *
 * - The curves are not in any @c psa_ecc_family_t. secp160r1 in particular has
 *   no @c PSA_ECC_FAMILY_SECP_R1 encoding at 160 bits.
 * - The operations are raw curve arithmetic. PSA has no algorithm for
 *   "multiply the base point by this scalar", and no way to return the result
 *   of a reduction modulo the group order.
 *
 * @note Despite the @c psa_ext_ prefix and the @c psa_status_t return type,
 *       these calls do @b not go through the PSA core: there is no key object,
 *       no policy, and no entry in @c psa_crypto_driver_wrappers.c. They talk
 *       to the CRACEN PK engine directly. Do not look for a driver-wrapper
 *       entry point, there is none.
 *
 * @note These functions are keyless and stateless, and may be called without
 *       @c psa_crypto_init().
 */

/** @brief Size of a secp160r1 scalar reduced modulo the group order.
 *
 * 21 rather than 20: the secp160r1 group order @c n is 161 bits, so a value
 * reduced modulo @c n does not always fit in the 160-bit field size.
 */
#define PSA_EXT_ECC_SECP160R1_SCALAR_SIZE 21u

/** @brief Size of a secp160r1 affine coordinate. The field prime is 160 bits. */
#define PSA_EXT_ECC_SECP160R1_COORD_SIZE 20u

/** @brief Largest input accepted by psa_ext_ecc_secp160r1_scalar_reduce(). */
#define PSA_EXT_ECC_SECP160R1_MAX_INPUT_SIZE 32u

/** @brief Reduce a big-endian integer modulo the secp160r1 group order.
 *
 * Computes @c output = @c input mod @c n.
 *
 * @param[in]  input         Big-endian integer to reduce.
 * @param[in]  input_length  Length of @p input in bytes, 1 to
 *                           @ref PSA_EXT_ECC_SECP160R1_MAX_INPUT_SIZE.
 * @param[out] output        Big-endian result, left-padded with zeros to
 *                           @p output_size.
 * @param[in]  output_size   Size of @p output, at least
 *                           @ref PSA_EXT_ECC_SECP160R1_SCALAR_SIZE.
 *
 * @retval PSA_SUCCESS                   The reduction was performed.
 * @retval PSA_ERROR_INVALID_ARGUMENT    @p input_length is out of range.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL    @p output_size is too small.
 */
psa_status_t psa_ext_ecc_secp160r1_scalar_reduce(const uint8_t *input, size_t input_length,
						 uint8_t *output, size_t output_size);

/** @brief Multiply the secp160r1 base point by a scalar.
 *
 * Computes @c (x, y) = @c scalar * @c G and returns the x-coordinate only.
 *
 * @p scalar is used as given; it is not reduced. Pass a value already reduced
 * by psa_ext_ecc_secp160r1_scalar_reduce(), which may need all
 * @ref PSA_EXT_ECC_SECP160R1_SCALAR_SIZE bytes.
 *
 * @param[in]  scalar        Big-endian scalar, must be non-zero.
 * @param[in]  scalar_length Length of @p scalar in bytes, 1 to
 *                           @ref PSA_EXT_ECC_SECP160R1_SCALAR_SIZE.
 * @param[out] x             Big-endian x-coordinate of the resulting point.
 * @param[in]  x_size        Size of @p x, at least
 *                           @ref PSA_EXT_ECC_SECP160R1_COORD_SIZE.
 *
 * @retval PSA_SUCCESS                   The point was computed.
 * @retval PSA_ERROR_INVALID_ARGUMENT    @p scalar_length is out of range, or
 *                                       @p scalar is zero.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL    @p x_size is too small.
 */
psa_status_t psa_ext_ecc_secp160r1_scalar_mult_base(const uint8_t *scalar, size_t scalar_length,
						    uint8_t *x, size_t x_size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PSA_EXT_ECC_H__ */
