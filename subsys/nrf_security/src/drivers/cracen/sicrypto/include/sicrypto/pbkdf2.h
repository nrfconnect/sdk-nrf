/** Password Based Key Derivation Function (PBKDF2).
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_PBKDF2_HEADER_FILE
#define SICRYPTO_PBKDF2_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/** Create a task for PBKDF2 key derivation.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      Hash algorithm to use.
 * @param[in] password     Password.
 * @param[in] passwordsz   Size in bytes of the \p password.
 * @param[in] salt         Salt.
 * @param[in] saltsz       Size in bytes of the \p salt.
 * @param[in] iterations   Number of iterations to perform.
 *
 * This task implements the password based key derivation function PBKDF2
 * defined in RFC 8018. It uses HMAC as the underlying pseudorandom function.
 * The \p hashalg argument selects the hash algorithm to use for the HMAC.
 *
 * The buffers \p password and \p salt must be in DMA memory.
 *
 * This task needs a workmem buffer with size:
 *      4 + 4*hash_digest_size + hash_block_size
 * where all sizes are expressed in bytes.
 */
void si_kdf_create_pbkdf2(struct sitask *t, const struct sxhashalg *hashalg, const char *password,
			  size_t passwordsz, const char *salt, size_t saltsz, size_t iterations);

#ifdef __cplusplus
}
#endif

#endif
