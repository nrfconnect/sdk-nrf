/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DIGEST_SINK_H__
#define DIGEST_SINK_H__

#include <suit_sink.h>
#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int digest_sink_err_t;

 /**< The compared digests do not match */
#define DIGEST_SINK_ERR_DIGEST_MISMATCH 1

/**
 * @brief Get the digest_sink object
 *
 * @note This function has to be called prior to calling suit_digest_sink_digest_match function
 *
 * @param[in] sink Pointer to sink_stream to be filled
 * @param[in] algorithm Algorithm to be used for digest calculation
 * @param[in] expected_digest digest to be used for comparison against calculated digest value
 * @return int SUIT_PLAT_SUCCESS if success, error code otherwise
 */
suit_plat_err_t suit_digest_sink_get(struct stream_sink *sink, psa_algorithm_t algorithm,
				     const uint8_t *expected_digest);

/**
 * @brief Check if digest matches expected digest value
 *
 * @note A sink has to be initialized with suit_digest_sink_get function call.
 * @note Then data intended for digest calculation has to be fed in using sink's write API function
 * call(s).
 * @note Finally this function can be called to finilize digest calculation and perform its
 * verification.
 *
 * @param[in] ctx Context of a sink used for digest calculation
 *
 * @return SUIT_PLAT_SUCCESS Digest calculation was successful and it matches expected digest
 * @return DIGEST_SINK_ERR_DIGEST_MISMATCH Digest calculation was successful but it does
 * not match expected digest
 * @return SUIT_PLAT_ERR_CRASH Digest could not be calculated or crash during cleanup
 * @return SUIT_PLAT_ERR_INVAL @ctx is NULL
 * @return SUIT_PLAT_ERR_INCORRECT_STATE A sink was not initialized
 * using suit_digest_sink_get function
 */
digest_sink_err_t suit_digest_sink_digest_match(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DIGEST_SINK_H__ */
