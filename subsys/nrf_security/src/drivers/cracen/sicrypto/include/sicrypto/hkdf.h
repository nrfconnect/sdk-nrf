/** HMAC Based Key Derivation Function (HKDF).
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_HKDF_HEADER_FILE
#define SICRYPTO_HKDF_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/** Create a task for HKDF key derivation.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      Hash algorithm to use.
 * @param[in] ikm          Input keying material.
 * @param[in] ikmsz        Size in bytes of the \p ikm.
 * @param[in] salt         Optional salt.
 * @param[in] saltsz       Size in bytes of the \p salt. It must be less than or
 *                         equal to the hash algorithm output size. If zero, the
 *                         \p salt input is ignored.
 *
 * This task implements the HMAC based key derivation function HKDF defined in
 * RFC 5869.
 *
 * The \p hashalg argument selects the hash algorithm to use for the HMAC.
 *
 * This implementation restricts the size of the \p salt to be less than or
 * equal to the hash digest size.
 *
 * The \p salt input is optional. When the salt is not needed, the user should
 * call the function with \p saltsz equal to zero.
 *
 * The buffers \p ikm and \p salt must be in DMA memory.
 *
 * This task needs a workmem buffer with size:
 *      1 + 3*hash_digest_size + hash_block_size
 * where all sizes are expressed in bytes.
 */
void si_kdf_create_hkdf(struct sitask *t, const struct sxhashalg *hashalg, const char *ikm,
			size_t ikmsz, const char *salt, size_t saltsz);

#ifdef __cplusplus
}
#endif

#endif
