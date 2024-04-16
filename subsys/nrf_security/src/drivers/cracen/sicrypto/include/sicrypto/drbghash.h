/** DRBG Hash, based on NIST SP 800-90A Rev. 1.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_DRBGHASH_HEADER_FILE
#define SICRYPTO_DRBGHASH_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sitask;
struct sxhashalg;

#define CRACEN_DRBG_HASH_SHA_1_WORKMEM_SIZE	(181)
#define CRACEN_DRBG_HASH_SHA_2_224_WORKMEM_SIZE (173)
#define CRACEN_DRBG_HASH_SHA_2_256_WORKMEM_SIZE (189)
#define CRACEN_DRBG_HASH_SHA_2_384_WORKMEM_SIZE (405)
#define CRACEN_DRBG_HASH_SHA_2_512_WORKMEM_SIZE (373)

/** Create a task to generate pseudorandom bytes using a DRBG Hash.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] hashalg       Hash algorithm to use.
 * @param[in] entropy       The entropy input required for DRBG instantiation.
 * @param[in] entropysz     Size in bytes of the entropy input.
 *
 * The \p entropy buffer must be in DMA memory. It must contain a byte string
 * obtained from a randomness source. The user is responsible for making sure
 * that the randomness source is adequate, see for example the requirements in
 * NIST SP 800-90A Rev. 1. The \p entropy input must be kept secret.
 *
 * This task needs a workmem buffer whose size depends on the selected hash
 * algorithm:
 * SHA-1                        181 bytes
 * SHA2-224 and SHA2-512/224    173 bytes
 * SHA2-256 and SHA2-512/256    189 bytes
 * SHA2-384                     405 bytes
 * SHA2-512                     373 bytes.
 */
void si_prng_create_drbg_hash(struct sitask *t, const struct sxhashalg *hashalg,
			      const char *entropy, size_t entropysz);

#ifdef __cplusplus
}
#endif

#endif
