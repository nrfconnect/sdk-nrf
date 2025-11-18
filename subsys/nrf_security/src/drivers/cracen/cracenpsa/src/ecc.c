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
#include "ecc.h"
#include "common.h"

#define MAX_ECC_ATTEMPTS 10

int ecc_genpubkey(const uint8_t *priv_key, uint8_t *pub_key, const struct sx_pk_ecurve *curve)
{
	const uint8_t **outputs = NULL;
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecp_mult inputs;
	int opsz;
	int status = SX_ERR_NOT_INVERTIBLE;
	int attempts = 0;

	opsz = sx_pk_curve_opsize(curve);

	while (status == SX_ERR_NOT_INVERTIBLE) {
		pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PTMUL);
		if (pkreq.status) {
			return pkreq.status;
		}
		pkreq.status =
			sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
		if (pkreq.status) {
			return pkreq.status;
		}

		/* Write the private key (random) into ba414ep device memory */
		sx_wrpkmem(inputs.k.addr, priv_key, opsz);
		sx_pk_write_curve_gen(pkreq.req, curve, inputs.px, inputs.py);

		sx_pk_run(pkreq.req);

		status = sx_pk_wait(pkreq.req);
		if (status != SX_OK && status != SX_ERR_NOT_INVERTIBLE) {
			break;
		}
		outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);

		/* When countermeasures are used, the operation may fail with error code
		 * SX_ERR_NOT_INVERTIBLE. In this case we can try again.
		 */
		if (status == SX_ERR_NOT_INVERTIBLE) {
			sx_pk_release_req(pkreq.req);
			if (++attempts == MAX_ECC_ATTEMPTS) {
				status = SX_ERR_TOO_MANY_ATTEMPTS;
			}
		}
	}
	if (status == SX_OK) {
		sx_rdpkmem(pub_key, outputs[0], opsz);
		sx_rdpkmem(pub_key + opsz, outputs[1], opsz);
	}
	sx_pk_release_req(pkreq.req);
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
