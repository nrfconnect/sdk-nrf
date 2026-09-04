/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief XOF helpers shared by the CRACEN lattice-based algorithms
 *        (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * Both FIPS 203 and FIPS 204 define their hash functions as a SHAKE over the
 * concatenation of a few short inputs. This helper performs the whole
 * setup/absorb/squeeze/abort sequence in one call, so callers do not repeat it.
 *
 * Rejection sampling, which squeezes an unknown amount of output, still needs a
 * live @c cracen_xof_operation_t and must use the @c cracen_xof_* API directly.
 * Note that the hardware cannot continue an output stream, so such a caller
 * should squeeze in a few large blocks rather than byte by byte.
 */

#ifndef CRACEN_PQC_XOF_H
#define CRACEN_PQC_XOF_H

#include <psa/crypto_types.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Absorb a list of input chunks and squeeze a fixed amount of output.
 *         Chunks are absorbed in order, so the result equals the XOF of their
 *         concatenation. Chunks that are NULL or of length 0 are skipped.
 *
 * @param[in] alg           XOF algorithm: PSA_ALG_SHAKE128 or PSA_ALG_SHAKE256.
 * @param[in] chunks        Array of @p chunk_count input buffers.
 * @param[in] chunk_lengths Length in bytes of each input buffer.
 * @param[in] chunk_count   Number of input buffers, must be the same
 *                          for @p chunks and @p chunk_lengths.
 * @param[out] output       Buffer where the output is to be written.
 * @param[in] output_length Number of bytes to squeeze.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_pqc_xof_compute(psa_algorithm_t alg, const uint8_t *const *chunks,
				    const size_t *chunk_lengths, size_t chunk_count,
				    uint8_t *output, size_t output_length);

#endif /* CRACEN_PQC_XOF_H */
