/** ECDSA signature generation and verification.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_ECDSA_HEADER_FILE
#define SICRYPTO_ECDSA_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "sig.h"

/** Signature definitions for ECDSA keys.
 *
 * The following constraints apply when using ECDSA keys:
 *
 * When generating a signature with si_sig_create_sign() or with
 * si_sig_create_sign_digest():
 *  - The task needs a workmem buffer with a size of at least:
 *          hash_digest_size + curve_op_size
 *    where curve_op_size is the maximum size in bytes of parameters and
 *    operands for the curve being used. All sizes are expressed in bytes.
 *  - The PRNG in the sicrypto environment must have been set up.
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
 *
 * When computing a public key with si_sig_create_pubkey():
 *  - There are no size requirements on the task workmem (the task does not
 *    need a workmem buffer).
 */
extern const struct si_sig_def *const si_sig_def_ecdsa;

/**
 * Signature definitions for ECDSA keys.
 *
 * Same as @ref si_sig_def_ecdsa, but with deterministic ECDSA.
 */
extern const struct si_sig_def *const si_sig_def_ecdsa_deterministic;

#ifdef __cplusplus
}
#endif

#endif
