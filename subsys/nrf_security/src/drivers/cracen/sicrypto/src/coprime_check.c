/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#include <stddef.h>
#include <silexpk/iomem.h>
#include <silexpk/core.h>
#include <silexpk/cmddefs/modmath.h>
#include "../include/sicrypto/sicrypto.h"
#include <cracen/statuscodes.h>
#include "coprime_check.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"

static int modular_inversion_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	sx_pk_release_req(t->pk);

	/* we are not interested in getting the result of the modular inversion,
	 * we only need to know if the inversion was
	 */
	return t->statuscode;
}

/* Perform modular inversion of a, using b as the modulo. */
static void modular_inversion_start(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_mod_single_op_cmd inputs;
	size_t bsz = t->params.coprimecheck.bsz;
	size_t asz = t->params.coprimecheck.asz;
	int sizes[] = {bsz, asz};

	si_wq_run_after(t, &t->params.coprimecheck.wq, modular_inversion_finish);

	/* b is odd, we can use command SX_PK_CMD_ODD_MOD_INV */
	pkreq = sx_pk_acquire_req(SX_PK_CMD_ODD_MOD_INV);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	sx_wrpkmem(inputs.n.addr, t->params.coprimecheck.b, bsz);
	sx_wrpkmem(inputs.b.addr, t->params.coprimecheck.a, asz);

	sx_pk_run(pkreq.req);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
}

static int modular_reduction_finish(struct sitask *t, struct siwq *wq)
{
	const char **outputs = sx_pk_get_output_ops(t->pk);
	int opsz = sx_pk_get_opsize(t->pk);
	size_t bsz = t->params.coprimecheck.bsz;
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* copy to workmem the result of the modular reduction, to be found
	 * at the end of the device memory slots
	 */
	sx_rdpkmem(t->workmem, outputs[0] + opsz - bsz, bsz);

	sx_pk_release_req(t->pk);

	/* update the task's parameters related to a */
	t->params.coprimecheck.a = t->workmem;
	t->params.coprimecheck.asz = bsz;

	modular_inversion_start(t);

	return SX_OK;
}

/* Perform modular reduction of a, using b as the modulo. */
static void modular_reduction_start(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_mod_single_op_cmd inputs;
	size_t bsz = t->params.coprimecheck.bsz;
	size_t asz = t->params.coprimecheck.asz;

	si_wq_run_after(t, &t->params.coprimecheck.wq, modular_reduction_finish);

	/* b is odd, we can use command SX_PK_CMD_ODD_MOD_REDUCE */
	pkreq = sx_pk_acquire_req(SX_PK_CMD_ODD_MOD_REDUCE);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	/* give the largest size for both inputs, in this case it is asz */
	int sizes[] = {asz, asz};

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}

	/* Copy modulo and operand to the device memory slots. Note that the
	 * modulo b must be placed at the end of its memory slot.
	 */
	sx_clrpkmem(inputs.n.addr, asz - bsz);
	sx_wrpkmem(inputs.n.addr + asz - bsz, t->params.coprimecheck.b, bsz);
	sx_wrpkmem(inputs.b.addr, t->params.coprimecheck.a, asz);

	sx_pk_run(pkreq.req);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
}

void si_create_coprime_check(struct sitask *t, const char *a, size_t asz, const char *b, size_t bsz)
{
	int a_is_odd, b_is_odd;
	size_t minworkmemsz;

	/* sizes must not be zero */
	if ((asz == 0) || (bsz == 0)) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	a_is_odd = a[asz - 1] & 1;
	b_is_odd = b[bsz - 1] & 1;

	/* at least one of the two input numbers a, b must be odd */
	if ((!a_is_odd) && (!b_is_odd)) {
		si_task_mark_final(t, SX_ERR_NOT_INVERTIBLE);
		return;
	}

	/* error if either input is 0 */
	if ((asz == 0) || (bsz == 0)) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* the most significant byte in both inputs must not be zero */
	if ((a[0] == 0) || (b[0] == 0)) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* swap a and b, if necessary, so that b is odd and, if possible, b is
	 * shorter than a
	 */
	if ((!b_is_odd) || (a_is_odd && (asz < bsz))) {
		const char *tmp = a;
		size_t tmpsz = asz;

		a = b;
		asz = bsz;
		b = tmp;
		bsz = tmpsz;
	}

	minworkmemsz = (asz > bsz) ? bsz : asz;
	if (t->workmemsz < minworkmemsz) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->params.coprimecheck.a = a;
	t->params.coprimecheck.asz = asz;
	t->params.coprimecheck.b = b;
	t->params.coprimecheck.bsz = bsz;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	if (asz > bsz) {
		t->actions.run = modular_reduction_start;
	} else {
		t->actions.run = modular_inversion_start;
	}
}
