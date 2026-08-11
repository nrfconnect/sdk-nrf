/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/pqc/cracen_pqc_xof.h>

#include <cracen_psa_primitives.h>
#include <cracen_psa_xof.h>

psa_status_t cracen_pqc_xof_compute(psa_algorithm_t alg, const uint8_t *const *chunks,
				    const size_t *chunk_lengths, size_t chunk_count,
				    uint8_t *output, size_t output_length)
{
	cracen_xof_operation_t operation;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	status = cracen_xof_setup(&operation, alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	for (size_t i = 0; i < chunk_count; i++) {
		if (chunks[i] == NULL || chunk_lengths[i] == 0) {
			continue;
		}

		status = cracen_xof_update(&operation, chunks[i], chunk_lengths[i]);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	status = cracen_xof_output(&operation, output, output_length);

exit:
	(void)cracen_xof_abort(&operation);
	return status;
}
