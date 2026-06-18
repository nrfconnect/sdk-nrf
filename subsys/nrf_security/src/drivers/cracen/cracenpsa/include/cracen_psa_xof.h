/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_psa_driver_api
 * @{
 * @brief XOF (Extendable Output Function) operations for the CRACEN PSA driver.
 */

#ifndef CRACEN_PSA_XOF_H
#define CRACEN_PSA_XOF_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include "cracen_psa_primitives.h"

/** @brief Set up an XOF operation.
 *
 * @param[in,out] operation XOF operation context.
 * @param[in] alg           XOF algorithm (PSA_ALG_SHAKE128 or PSA_ALG_SHAKE256).
 *
 * @retval PSA_SUCCESS             The operation was set up successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_xof_setup(cracen_xof_operation_t *operation, psa_algorithm_t alg);

/** @brief Provide a context for a multi-part XOF operation.
 *
 * @param[in,out] operation      XOF operation context.
 * @param[in] context            Buffer containing the context value.
 * @param[in] context_length     Length of the context in bytes.
 *
 * @retval PSA_SUCCESS             The context was set successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The operation does not support context setting.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 */
psa_status_t cracen_xof_set_context(cracen_xof_operation_t *operation, const uint8_t *context,
				    size_t context_length);

/** @brief Feed input to a multi-part XOF operation.
 *
 * @param[in,out] operation  XOF operation context.
 * @param[in] input          Buffer containing the input fragment.
 * @param[in] input_length   Length of the input in bytes.
 *
 * @retval PSA_SUCCESS         The input was processed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_xof_update(cracen_xof_operation_t *operation, const uint8_t *input,
			       size_t input_length);

/** @brief Extract data from an XOF operation.
 *
 * @param[in,out] operation  XOF operation context.
 * @param[out] output        Buffer where the output will be written.
 * @param[in] output_length  Number of output bytes to produce.
 *
 * @retval PSA_SUCCESS         Output was generated successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_xof_output(cracen_xof_operation_t *operation, uint8_t *output,
			       size_t output_length);

/** @brief Abort a XOF operation and wipe its context.
 *
 * @param[in,out] operation XOF operation context.
 *
 * @retval PSA_SUCCESS Always succeeds.
 */
psa_status_t cracen_xof_abort(cracen_xof_operation_t *operation);

/** @} */

#endif /* CRACEN_PSA_XOF_H */
