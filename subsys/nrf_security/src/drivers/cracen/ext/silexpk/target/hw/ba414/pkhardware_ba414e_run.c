/*
 * Copyright (c) 2020 Silex Insight sa
 * Copyright (c) 2020 Beerten Engineering scs
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <silexpk/core.h>
#include "regs_addr.h"
#include <internal.h>
#include "pkhardware_ba414e.h"
#include "ba414_status.h"
#include "../../include/sx_regs.h"

#include <cracen/membarriers.h>

void sx_pk_run(sx_pk_req *req)
{
	sx_pk_select_ops(req);
	wmb();
	sx_pk_wrreg(&req->regs, PK_REG_CONTROL, PK_RB_CONTROL_START_OP | PK_RB_CONTROL_CLEAR_IRQ);
}

int sx_pk_get_status(sx_pk_req *req)
{
	uint32_t status;

	rmb();
	status = sx_pk_rdreg(&req->regs, PK_REG_STATUS);

	return convert_ba414_status(status);
}
