/** RSA PSS signature scheme.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_RSAPSS_HEADER_FILE
#define SICRYPTO_RSAPSS_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "sig.h"

/** Signature definitions for RSA PSS keys.
 *
 * The following constraints apply when using RSA PSS keys:
 *
 * When generating a signature with si_sig_create_sign() or with
 * si_sig_create_sign_digest():
 *  - The task needs a workmem buffer with a size of at least:
 *          rsa_modulus_size + 2*hash_digest_size + 4
 *    where all sizes are expressed in bytes.
 *  - The PRNG in the sicrypto environment must have been set up.
 *  - The task can use partial processing. For each partial run, the sum of the
 *    sizes of the provided message chunks must be a multiple of the block size
 *    of the selected hash algorithm.
 *
 * When verifying a signature with si_sig_create_verify() or with
 * si_sig_create_verify_digest():
 *  - The task needs a workmem buffer with a size of at least:
 *          rsa_modulus_size + 2*hash_digest_size + 4
 *    where all sizes are expressed in bytes.
 *  - The task can use partial processing. For each partial run, the sum of the
 *    sizes of the provided message chunks must be a multiple of the block size
 *    of the selected hash algorithm.
 */
extern const struct si_sig_def *const si_sig_def_rsa_pss;

#ifdef __cplusplus
}
#endif

#endif
