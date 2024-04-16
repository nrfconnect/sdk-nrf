/** RSA PKCS1-v1_5 signature generation and verification.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_RSASSA_PKCS1V15_HEADER_FILE
#define SICRYPTO_RSASSA_PKCS1V15_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "sig.h"

/** Signature definitions for RSA PKCS1-v1_5 keys.
 *
 * The following constraints apply when using RSA PKCS1-v1_5 keys:
 *
 * When generating a signature with si_sig_create_sign() or with
 * si_sig_create_sign_digest():
 *  - The task needs a workmem buffer with size at least equal to the hash
 *    digest size.
 *  - The task can use partial processing. For each partial run, the sum of the
 *    sizes of the provided message chunks must be a multiple of the block size
 *    of the selected hash algorithm.
 *
 * When verifying a signature with si_sig_create_verify() or with
 * si_sig_create_verify_digest():
 *  - The task needs a workmem buffer with size at least equal to the hash
 *    digest size.
 *  - The task can use partial processing. For each partial run, the sum of the
 *    sizes of the provided message chunks must be a multiple of the block size
 *    of the selected hash algorithm.
 */
extern const struct si_sig_def *const si_sig_def_rsa_pkcs1v15;

#ifdef __cplusplus
}
#endif

#endif
