/** Key Derivation Function KDF2.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_KDF2_HEADER_FILE
#define SICRYPTO_KDF2_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sitask;
struct sxhashalg;

/** Create a task for KDF2 key derivation.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      Hash algorithm to use.
 * @param[in] ikm          Input keying material.
 * @param[in] ikmsz        Size in bytes of \p ikm.
 * @param[in] info         Optional shared info parameter.
 * @param[in] infosz       Size in bytes of \p info.
 *
 * This task implements the KDF2 algorithm, based on ANS X9.63.
 *
 * The \p info argument is defined by ANS X9.63 as some data shared by the
 * entities using the key derivation function.
 *
 * The \p info argument is optional: when the value passed for \p infosz is 0,
 * the \p info input is ignored. In this case, this KDF2 implementation behaves
 * exactly as the KDF2 defined in the ISO 18033-2 standard.
 *
 * The buffers \p ikm and \p info must be in DMA memory.
 *
 * This task needs a workmem buffer with size:
 *      4 + hash_digest_size
 * where all sizes are expressed in bytes.
 */
void si_kdf_create_kdf2(struct sitask *t, const struct sxhashalg *hashalg, const char *ikm,
			size_t ikmsz, const char *info, size_t infosz);

#ifdef __cplusplus
}
#endif

#endif
