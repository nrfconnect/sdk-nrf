/** DRBG AES CTR without derivation function.
 *
 * Implementation based on NIST SP 800-90A Rev. 1 "Recommendation for Random
 * Number Generation Using Deterministic Random Bit Generators".
 *
 * @file
 *
 * @copyright Copyright (c) 2020-2021 Silex Insight
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_DRBGCTR_HEADER_FILE
#define SICRYPTO_DRBGCTR_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sitask;

/** Create a task to generate pseudorandom bytes using an DRBG AES CTR.
 *
 * @param[in,out] t       Task structure to use.
 * @param[in] keysz       Size in bytes of the internal AES key.
 * @param[in] entropy     Entropy input required for DRBG instantiation. It must
 *                        be made of true random bytes. Its size must be
 *                        \p keysz + 16 bytes.
 *
 * The \p entropy buffer must be in DMA memory.
 *
 * Allowed values for \p keysz are 16, 24 and 32 bytes (or a subset of them if
 * the underlying HW does not support all three values).
 *
 * After task creation, the user can provide a personalization string with
 * si_task_consume(). The size in bytes of the personalization string must be
 * smaller than or equal to \p keysz + 16.
 *
 * This task needs a workmem buffer with size:
 *      keysz + 144
 * where all sizes are expressed in bytes.
 */
void si_prng_create_drbg_aes_ctr(struct sitask *t, size_t keysz, const char *entropy);

#ifdef __cplusplus
}
#endif

#endif
