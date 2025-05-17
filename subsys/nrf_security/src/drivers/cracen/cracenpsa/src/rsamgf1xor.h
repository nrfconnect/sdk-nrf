/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RSAMGF1XOR_HEADER_FILE
#define RSAMGF1XOR_HEADER_FILE

#include <psa/crypto.h>
#include <stdint.h>
#include <stddef.h>

/** run a MGF1 and xor step in rsa.
 *
 * MGF1 is the mask generation function defined in RFC 8017. This operation computes
 * the mask and then applies it, by bitwise xor, to another input octet string.
 *
 * @param[in] hashalg      The hash algorithm on which MGF1 is based.
 *
 *
 * This task needs a workmem buffer with size:
 *      hash_digest_size + 4
 * where all sizes are expressed in bytes.
 */
int cracen_run_mgf1xor(uint8_t *workmem, size_t workmemsz, const struct sxhashalg *hashalg,
        uint8_t *seed, size_t digestsz, uint8_t *out, size_t out_sz);

#endif
