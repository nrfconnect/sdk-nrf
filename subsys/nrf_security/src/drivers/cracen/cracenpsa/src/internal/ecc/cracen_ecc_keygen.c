/* ECC key pair generation.
 * Based on FIPS 186-4, section B.4.2 "Key Pair Generation by Testing
 * Candidates".
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <silexpk/cmddefs/ecc.h>
#include <silexpk/ec_curves.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include "cracen_ecc_keygen.h"
#include <cracen/common.h>

#define MAX_ECC_ATTEMPTS 10

int ecc_genpubkey(const uint8_t *priv_key, uint8_t *pub_key, const struct sx_pk_ecurve *curve)
{
	const uint8_t **outputs;
	sx_pk_req req;
	struct sx_pk_inops_ecp_mult inputs;
	int opsz;
	int status = SX_ERR_CORRUPTION_DETECTED;
	int attempts = 0;

	opsz = sx_pk_curve_opsize(curve);

	/* Acquire CRACEN once for all retry attempts */
	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_ECC_PTMUL);

	do {
		if (attempts > 0) {
			sx_pk_set_cmd(&req, SX_PK_CMD_ECC_PTMUL);
		}

		status = sx_pk_list_ecc_inslots(&req, curve, 0, (struct sx_pk_slot *)&inputs);
		if (status != SX_OK) {
			sx_pk_release_req(&req);
			return status;
		}

		/* Write the private key (random) into ba414ep device memory */
		sx_wrpkmem(inputs.k.addr, priv_key, opsz);
		sx_pk_write_curve_gen(&req, curve, inputs.px, inputs.py);

		sx_pk_run(&req);

		status = sx_pk_wait(&req);

		/* When countermeasures are used, the operation may fail with error code
		 * SX_ERR_NOT_INVERTIBLE. In this case we can try again.
		 */
		if (status == SX_ERR_NOT_INVERTIBLE) {
			if (++attempts == MAX_ECC_ATTEMPTS) {
				sx_pk_release_req(&req);
				return SX_ERR_TOO_MANY_ATTEMPTS;
			}
		}
	} while (status == SX_ERR_NOT_INVERTIBLE);

	if (status == SX_OK) {
		outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
		sx_rdpkmem(pub_key, outputs[0], opsz);
		sx_rdpkmem(pub_key + opsz, outputs[1], opsz);
	}
	sx_pk_release_req(&req);
	return status;
}

int ecc_genprivkey(const struct sx_pk_ecurve *curve, uint8_t *priv_key, size_t priv_key_size)
{
	int status;
	int opsz = sx_pk_curve_opsize(curve);
	const uint8_t *curve_n = (const uint8_t *)sx_pk_curve_order(curve);
	size_t key_size = (size_t)sx_pk_curve_opsize(curve);

	if (priv_key_size < key_size) {
		return SX_ERR_OUTPUT_BUFFER_TOO_SMALL;
	}

	/* generate private key, a random number in [1, n-1], where n is the curve
	 * order
	 */
	status = cracen_get_rnd_in_range(curve_n, opsz, priv_key);

	return status;
}
