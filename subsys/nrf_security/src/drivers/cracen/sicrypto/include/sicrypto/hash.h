/** Hash message digest.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_HASH_HEADER_FILE
#define SICRYPTO_HASH_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

struct sitask;
struct sxhashalg;

/** Create a task to compute the hash of a message.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] hashalg       Hash algorithm to use.
 *
 * This task does not need a workmem buffer.
 */
void si_hash_create(struct sitask *t, const struct sxhashalg *hashalg);

#ifdef __cplusplus
}
#endif

#endif
