/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#include "dect_phy_api_scheduler_integration.h"
#include "dect_phy_api_scheduler.h"

#include "dect_phy_ctrl.h"

void dect_phy_api_scheduler_suspended_evt_send(void)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_SCHEDULER_SUSPENDED);
}

void dect_phy_api_scheduler_resumed_evt_send(void)
{
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_SCHEDULER_RESUMED);
}

int dect_phy_api_scheduler_mdm_op_req_failed_evt_send(
	struct dect_phy_common_op_completed_params *params)
{
	struct dect_phy_common_op_completed_params ctrl_op_completed_params;
	int ret;

	ctrl_op_completed_params.handle = params->handle;
	ctrl_op_completed_params.time = params->time;
	ctrl_op_completed_params.temperature = params->temperature;
	ctrl_op_completed_params.status = params->status;

	ret = dect_phy_ctrl_msgq_data_op_add(DECT_PHY_CTRL_OP_PHY_API_MDM_COMPLETED,
					     (void *)&ctrl_op_completed_params,
					     sizeof(struct dect_phy_common_op_completed_params));

	return ret;
}
