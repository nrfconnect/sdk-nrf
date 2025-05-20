/** Ed448 signature-related operations.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_ED448_HEADER_FILE
#define SICRYPTO_ED448_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "sig.h"

/** Signature definitions for Ed448 keys.
 *
 * The following constraints apply when using Ed448 keys:
 *
 * When generating a signature with si_sig_create_sign():
 *  - The task needs a workmem buffer of at least 285 bytes.
 *  - The message to be signed must be given with a single call to
 *    si_task_consume().
 *  - The buffer for private key material must be in DMA memory.
 *  - The signature buffer must be in DMA memory.
 *
 * When verifying a signature with si_sig_create_verify():
 *  - The task needs a workmem buffer of at least 114 bytes.
 *  - The buffer for public key material must be in DMA memory.
 *  - The signature buffer must be in DMA memory.
 *  - The task can use partial processing. For the first partial run, the sum of
 *    the sizes of the provided message chunks plus 124 must be a multiple of
 *    136 bytes. For any other partial run, the sum of the sizes of the provided
 *    message chunks must be a multiple of 136 bytes.
 *
 * When computing a public key with si_sig_create_pubkey():
 *  - The task needs a workmem buffer of at least 114 bytes.
 *  - The buffer for private key material must be in DMA memory.
 */
extern const struct si_sig_def *const si_sig_def_ed448;

#ifdef __cplusplus
}
#endif

#endif
