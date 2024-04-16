/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <silexpk/core.h>
#include <internal.h>
#include <silexpk/ik.h>

/** Get status for IK operation */
int sx_ik_read_status(sx_pk_req *req);

/** Check if cmd is IK cmd*/
int sx_pk_is_ik_cmd(sx_pk_req *req);

/** Update the capabilities with IK capabilities */
void sx_pk_read_ik_capabilities(struct sx_regs *regs, struct sx_pk_capabilities *caps);

struct sx_regs *sx_pk_get_regs(void);

struct sx_pk_capabilities *sx_pk_get_caps(void);
