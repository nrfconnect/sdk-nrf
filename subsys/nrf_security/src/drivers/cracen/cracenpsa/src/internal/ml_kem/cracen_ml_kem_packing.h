/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Encoding and compression of the CRACEN ML-KEM (FIPS 203) implementation.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * ByteEncode and ByteDecode (FIPS 203, Algorithms 5 and 6, respectively) both stream d bits
 * per coefficient through the shared bit primitives of cracen_pqc_bits.h.
 *
 * A 32-byte message is the d = 1 case: Compress_1 maps a coefficient to the bit
 * it is closest to, and Decompress_1 maps a bit to 0 or to (q+1)/2.
 */

#ifndef CRACEN_ML_KEM_PACKING_H
#define CRACEN_ML_KEM_PACKING_H

#include "cracen_ml_kem_internal.h"

#include <stdbool.h>

/** @brief Encode an array of 12-bit integers into a byte array.
 *         ByteEncode_12 (FIPS 203, Algorithm 5).
 *
 * @param[in] poly  Polynomial with coefficients in (-q, q).
 * @param[out] out  Buffer of ML_KEM_POLY_PACKED_SZ_BYTES bytes.
 */
void cracen_ml_kem_poly_pack(const ml_kem_poly_t *poly, uint8_t *out);

/** @brief Decode a byte array into an array of 12-bit integers.
 *         ByteDecode_12 (FIPS 203, Algorithm 6).
 *
 * The 12 bits of each coefficient are taken as they are, without the reduction
 * modulo q that Algorithm 6 applies for d = 12. The decoded coefficients are
 * therefore in range [0, q), only for input produced by cracen_ml_kem_poly_pack()
 * or accepted by the modulus check of Section 7.2.
 *
 * @param[in] in     Buffer of ML_KEM_POLY_PACKED_SZ_BYTES bytes.
 * @param[out] poly  Decoded polynomial; coefficients in [0, 2^12).
 */
void cracen_ml_kem_poly_unpack(const uint8_t *in, ml_kem_poly_t *poly);

/** @brief Execute Compress_d followed by ByteEncode_d (FIPS 203, Section 4.2.1).
 *
 * @param[in] poly  Polynomial with coefficients in (-q, q).
 * @param[in] d     Bits per coefficient: du, dv, or 1 for a message.
 * @param[out] out  Buffer of 32 * @p d bytes.
 */
void cracen_ml_kem_poly_compress_encode(const ml_kem_poly_t *poly, uint8_t d, uint8_t *out);

/** @brief Execute ByteDecode_d followed by Decompress_d (FIPS 203, Section 4.2.1).
 *
 * @param[in] in     Buffer of 32 * @p d bytes.
 * @param[in] d      Bits per coefficient: du, dv, or 1 for a message.
 * @param[out] poly  Decompressed polynomial; coefficients in [0, q).
 */
void cracen_ml_kem_poly_decode_decompress(const uint8_t *in, uint8_t d, ml_kem_poly_t *poly);

#endif /* CRACEN_ML_KEM_PACKING_H */
