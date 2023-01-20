/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/stats/stats.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include "fmfu_mgmt_internal.h"
#include <mgmt/fmfu_mgmt.h>
#include <mgmt/fmfu_mgmt_stat.h>

STATS_SECT_START(smp_com_param)
STATS_SECT_ENTRY(frame_max)
STATS_SECT_ENTRY(pack_max)
STATS_SECT_END;

/* Assign a name to each stat. */
STATS_NAME_START(smp_com_param)
STATS_NAME(smp_com_param, frame_max)
STATS_NAME(smp_com_param, pack_max)
STATS_NAME_END(smp_com_param);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_com_param) smp_com_param;

int fmfu_mgmt_stat_init(void)
{
	/* Register/start the stat service */
	int rc = STATS_INIT_AND_REG(smp_com_param, STATS_SIZE_32, "smp_com");

	STATS_INCN(smp_com_param, frame_max, SMP_UART_BUFFER_SIZE);
	STATS_INCN(smp_com_param, pack_max, SMP_PACKET_MTU);

	return rc;
}
