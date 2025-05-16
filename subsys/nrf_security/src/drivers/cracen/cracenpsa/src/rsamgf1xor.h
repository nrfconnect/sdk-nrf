/*
 *  Copyright (c) 2025 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RSAMGF1XOR_HEADER_FILE
#define RSAMGF1XOR_HEADER_FILE

#include <sxsymcrypt/hashdefs.h>
#include <stdint.h>
#include <stddef.h>

/** run a MGF1 and xor step in rsa.
 *
 * MGF1 is the mask generation function defined in RFC 8017. This operation computes
 * the mask using the given hash algorithm and applies it by bitwise xor to the output buffer.
 *
 * @param[in] workmem     Temporary buffer used for intermediate computations.
 * @param[in] workmemsz   Size of the workmem buffer in bytes. Must be at least digestsz + 4.
 * @param[in] hashalg     The hash algorithm on which MGF1 is based.
 * @param[in] seed        The input seed used for mask generation.
 * @param[in] digestsz    Size of the hash digest in bytes.
 * @param[in,out]  xorinout   Output buffer to which the mask is applied via xor.
 * @param[in] out_sz      Length of the output buffer in bytes.
 *
 * This function needs a workmem buffer with size:
 *      hash_digest_size + 4
 * where all sizes are expressed in bytes.
 */
int cracen_run_mgf1xor(uint8_t *workmem, size_t workmemsz, const struct sxhashalg *hashalg,
		       uint8_t *seed, size_t digestsz, uint8_t *out, size_t out_sz);

#endif
