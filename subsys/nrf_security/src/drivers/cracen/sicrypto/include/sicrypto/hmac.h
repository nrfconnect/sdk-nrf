/** HMAC (Keyed-Hash Message Authentication Code) based on FIPS 198-1.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_HMAC_HEADER_FILE
#define SICRYPTO_HMAC_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <sxsymcrypt/hash.h>
#include "sicrypto.h"

/** Create a task for HMAC computation.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      Hash algorithm to use.
 * @param[in] key          Secret key.
 * @param[in] keysz        Size in bytes of the key.
 *
 * This function supports any key size. The HMAC task internally performs the
 * necessary key preparation steps, according to FIPS 198-1.
 *
 * The \p key buffer must be in DMA memory.
 *
 * This task needs a workmem buffer with size:
 *      hash_block_size + hash_digest_size
 * where all sizes are expressed in bytes.
 */
void si_mac_create_hmac(struct sitask *t, const struct sxhashalg *hashalg, const char *key,
			size_t keysz);

#ifdef __cplusplus
}
#endif

#endif
