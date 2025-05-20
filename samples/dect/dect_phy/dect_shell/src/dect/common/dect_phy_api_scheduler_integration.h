/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_API_SCHEDULER_INTEGRATION_H
#define DECT_PHY_API_SCHEDULER_INTEGRATION_H

#include <zephyr/kernel.h>

#include "desh_print.h"

#include "dect_common.h"
#include "dect_phy_api_scheduler.h"

/* "callbacks" from scheduler that needs unblocking implementation */
int dect_phy_api_scheduler_mdm_op_req_failed_evt_send(
	struct dect_phy_common_op_completed_params *params);

/* Informs that scheduler is suspended/resumed accordingly */
void dect_phy_api_scheduler_suspended_evt_send(void);
void dect_phy_api_scheduler_resumed_evt_send(void);

#endif /* DECT_PHY_API_SCHEDULER_INTEGRATION_H */
