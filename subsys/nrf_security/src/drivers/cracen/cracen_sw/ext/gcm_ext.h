/**
 * \file gcm_ext.h
 *
 * \brief This file contains GCM definitions and functions.
 *
 * The Galois/Counter Mode (GCM) for 128-bit block ciphers is defined
 * in <em>D. McGrew, J. Viega, The Galois/Counter Mode of Operation
 * (GCM), Natl. Inst. Stand. Technol.</em>
 *
 * For more information on GCM, see <em>NIST SP 800-38D: Recommendation for
 * Block Cipher Modes of Operation: Galois/Counter Mode (GCM) and GMAC</em>.
 *
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

/* Copied from mbed TLS, modified to contain GF(2^128) operation only */

#ifndef MBEDTLS_GCM_EXT_H
#define MBEDTLS_GCM_EXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Only small Shoup's table is supported now */
#define GCM_EXT_HTABLE_SIZE 16

int gcm_ext_gen_table(const uint8_t *h, uint64_t H[16][2]);
void gcm_ext_mult(const uint64_t H[16][2], const unsigned char x[16],
		  unsigned char output[16]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_GCM_EXT_H */
