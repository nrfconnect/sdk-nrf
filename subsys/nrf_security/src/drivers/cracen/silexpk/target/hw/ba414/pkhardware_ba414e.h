/**
 * \brief Internal definitions for BA414ep targets.
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PKHARDWARE_BA414E_H
#define PKHARDWARE_BA414E_H

#include <silexpk/core.h>
#include <stdint.h>

struct sx_pk_cmd_def {
	uint32_t cmdcode;
	uint32_t outslots;
	uint32_t inslots;
	uint32_t selected_ptrA;
	uint32_t blind_flags;
};

/** Program the command register of an acceleration request.
 *
 * Operations that list their input slots do this as part of the slot setup.
 * Operations without operands must call this themselves, otherwise the
 * hardware reruns whatever command is still latched in the register.
 *
 * @param[in,out] req The acceleration request.
 * @param op_size Operand size in bytes. Use 1 for operations without operands.
 * @param flags Command flags to write in addition to the command code.
 */
void sx_pk_write_command(sx_pk_req *req, int op_size, uint32_t flags);

void sx_pk_write_curve(sx_pk_req *pk, const struct sx_pk_ecurve *curve);

int sx_pk_count_curve_params(const struct sx_pk_ecurve *curve);

void sx_pk_select_ops(sx_pk_req *req);

#endif
