/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Workmem layout for the 'coprime check' task:
 * - result of the modular reduction (size: min(asz, bsz)).
 *
 * The algorithm implemented in this task is based on the fact that:
 * a, b are coprime <==> GCD(a, b) = 1
 * <==> a is invertible in the integers modulo b.
 *
 * High level description of the algorithm:
 * - at least one of the two input numbers 'a', 'b' must be odd
 * - swap 'a' and 'b', if necessary, so that 'b' is the smallest of the odd numbers
 * - if the size of 'a' is greater than the size of 'b':
 *     - 'a' is reduced modulo 'b', using the SilexPK command
 *     SX_PK_CMD_ODD_MOD_REDUCE
 * - 'a' is inverted modulo 'b', using the SilexPK command SX_PK_CMD_ODD_MOD_INV
 * - status code at the end of the task
 *     - SX_OK: 'a' and 'b' are coprime
 *     - SX_ERR_NOT_INVERTIBLE: 'a' and 'b' are not coprime
 *     - other value: an error has occurred.
 *
 * The modular reduction operation is done for performance reasons: a large modular
 * reduction + a short modular inversion is faster than a large modular inversion.
 * Modular reduction is always performed, except when one input is even and shorter
 * than the odd input. The result of the modular reduction is placed in workmem.
 * Requesting that workmem size be equal to min(asz, bsz) works in any case.
 */


#include <internal/rsa/cracen_rsa_coprime_check.h>

#include <stddef.h>
#include <silexpk/iomem.h>
#include <silexpk/core.h>
#include <silexpk/cmddefs/modmath.h>
#include <cracen/statuscodes.h>
#include <cracen_psa_primitives.h>

/* Perform modular inversion of a, using b as the modulo. */
static int modular_inversion_run(sx_pk_req *req, struct cracen_coprimecheck *coprimecheck)
{
	struct sx_pk_inops_mod_single_op_cmd inputs;
	size_t bsz = coprimecheck->bsz;
	size_t asz = coprimecheck->asz;
	int sizes[] = {bsz, asz};

	/* b is odd, we can use command SX_PK_CMD_ODD_MOD_INV */
	sx_pk_set_cmd(req, SX_PK_CMD_ODD_MOD_INV);

	int status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}

	sx_wrpkmem(inputs.n.addr, coprimecheck->b, bsz);
	sx_wrpkmem(inputs.b.addr, coprimecheck->a, asz);

	sx_pk_run(req);

	return sx_pk_wait(req);
}

/* Perform modular reduction of a, using b as the modulo. */
static int modular_reduction_run(sx_pk_req *req, uint8_t *workmem,
				 struct cracen_coprimecheck *coprimecheck)
{
	struct sx_pk_inops_mod_single_op_cmd inputs;
	size_t bsz = coprimecheck->bsz;
	size_t asz = coprimecheck->asz;

	sx_pk_set_cmd(req, SX_PK_CMD_ODD_MOD_REDUCE);

	/* give the largest size for both inputs, in this case it is asz */
	int sizes[] = {asz, asz};

	int status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}

	/* Copy modulo and operand to the device memory slots. Note that the
	 * modulo b must be placed at the end of its memory slot.
	 */
	sx_clrpkmem(inputs.n.addr, asz - bsz);
	sx_wrpkmem(inputs.n.addr + asz - bsz, coprimecheck->b, bsz);
	sx_wrpkmem(inputs.b.addr, coprimecheck->a, asz);

	sx_pk_run(req);
	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	/* Read the result of the modular reduction */
	const uint8_t **outputs = sx_pk_get_output_ops(req);
	int opsz = sx_pk_get_opsize(req);

	/* copy to workmem the result of the modular reduction, to be found
	 * at the end of the device memory slots
	 */
	sx_rdpkmem(workmem, outputs[0] + opsz - bsz, bsz);

	/* update the task's parameters related to a */
	coprimecheck->a = workmem;
	coprimecheck->asz = bsz;

	return SX_OK;
}

int coprime_check_run(sx_pk_req *req, uint8_t *workmem, size_t workmemsz, const uint8_t *a,
		      size_t asz, const uint8_t *b, size_t bsz)
{
	int status;
	int a_is_odd;
	int b_is_odd;
	size_t minworkmemsz;
	struct cracen_coprimecheck coprimecheck;

	/* sizes must not be zero */
	if ((asz == 0) || (bsz == 0)) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	a_is_odd = a[asz - 1] & 1;
	b_is_odd = b[bsz - 1] & 1;

	/* at least one of the two input numbers a, b must be odd */
	if ((!a_is_odd) && (!b_is_odd)) {
		return SX_ERR_NOT_INVERTIBLE;
	}

	/* error if either input is 0 */
	if ((asz == 0) || (bsz == 0)) {
		return SX_ERR_INVALID_ARG;
	}

	/* the most significant byte in both inputs must not be zero */
	if ((a[0] == 0) || (b[0] == 0)) {
		return SX_ERR_INVALID_ARG;
	}

	/* swap a and b, if necessary, so that b is odd and, if possible, b is
	 * shorter than a
	 */
	if ((!b_is_odd) || (a_is_odd && (asz < bsz))) {
		const uint8_t *tmp = a;
		size_t tmpsz = asz;

		a = b;
		asz = bsz;
		b = tmp;
		bsz = tmpsz;
	}
	minworkmemsz = (asz > bsz) ? bsz : asz;
	if (workmemsz < minworkmemsz) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	coprimecheck.a = a;
	coprimecheck.asz = asz;
	coprimecheck.b = b;
	coprimecheck.bsz = bsz;

	if (asz > bsz) {
		/* First do modular reduction, then inversion */
		status = modular_reduction_run(req, workmem, &coprimecheck);
		if (status != SX_OK) {
			return status;
		}
	}

	return modular_inversion_run(req, &coprimecheck);
}

int cracen_coprime_check(uint8_t *workmem, size_t workmemsz, const uint8_t *a, size_t asz,
			 const uint8_t *b, size_t bsz)
{
	sx_pk_req req;
	int status;

	sx_pk_acquire_hw(&req);
	sx_pk_set_cmd(&req, SX_PK_CMD_ODD_MOD_INV);

	status = coprime_check_run(&req, workmem, workmemsz, a, asz, b, bsz);

	sx_pk_release_req(&req);

	return status;
}
