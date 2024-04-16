/** RSA private key generation.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_RSA_KEYGEN_HEADER_FILE
#define SICRYPTO_RSA_KEYGEN_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "rsa_keys.h"

struct sitask;

/** Create a task to generate an RSA private key.
 *
 * This task generates an RSA private key for a given key size and a given
 * public exponent.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] pubexp       Public exponent. It is an unsigned integer stored as
 *                         a big endian byte array.
 * @param[in] pubexpsz     Size in bytes of the public exponent.
 * @param[in] keysz        Size in bytes of the RSA key (size of the RSA
 *                         modulus).
 * @param[in,out] privkey  Output key.
 *
 * Prior to calling this task creation function, the si_rsa_key structure
 * pointed by \p privkey must be initialized with either SI_KEY_INIT_RSA() or
 * SI_KEY_INIT_RSACRT(). The generated RSA private key will be:
 *      - in the (n, d) form, if it was initialized with SI_KEY_INIT_RSA()
 *      - in the CRT form (p, q, dp, dq, qinv), if it was initialized with
 *        SI_KEY_INIT_RSACRT().
 *
 * When initializing the key with SI_KEY_INIT_RSA(mod, expon), the macro
 * arguments 'mod' and 'expon' must be pointers to buffers of \p keysz bytes.
 * When initializing the key with SI_KEY_INIT_RSACRT(p, q, dp, dq, qinv), the
 * macro arguments 'p', 'q', 'dp', 'dq' and 'qinv' must be pointers to buffers
 * of \p keysz/2 bytes.
 *
 * The task writes the generated key element values, as unsigned big endian
 * integers, to the buffers provided with SI_KEY_INIT_RSA() or
 * SI_KEY_INIT_RSACRT().
 *
 * The public exponent \p pubexp must be odd. It also must be strictly greater
 * than 2^16 and strictly smaller than 2^256. Its most significant byte must not
 * be zero.
 *
 * The \p keysz must be even and it must be greater than or equal to 256
 * (the minimum key size is 2048 bits).
 *
 * The \p pubexp buffer and the buffers in \p privkey do not need to be in DMA
 * memory.
 *
 * The PRNG in the sicrypto environment must have been set up prior to calling
 * this function.
 *
 * This task needs a workmem buffer with size at least equal to 2*keysz bytes.
 */
void si_rsa_create_genprivkey(struct sitask *t, const char *pubexp, size_t pubexpsz, size_t keysz,
			      struct si_rsa_key *privkey);

void si_rsa_create_check_pq(struct sitask *t, const char *pubexp, size_t pubexpsz, char *p, char *q,
			    size_t candidatesz);

#ifdef __cplusplus
}
#endif

#endif
