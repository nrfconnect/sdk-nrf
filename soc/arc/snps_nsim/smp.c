/*
 * Copyright (c) 2016, 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This module provides routines to initialize and support board-level hardware
 * for the ARC EM and HS cores in nSIM simulator when SMP is enabled.
 *
 */

#include <zephyr/init.h>
#include <zephyr/arch/arc/v2/arc_connect.h>
#include <zephyr/arch/arc/v2/aux_regs.h>

static int arc_nsim_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t core;
	uint32_t i;

	/* allocate all IDU interrupts to master core */
	core = z_arc_v2_core_id();

	z_arc_connect_idu_disable();

	for (i = 0; i < (CONFIG_NUM_IRQS - ARC_CONNECT_IDU_IRQ_START); i++) {
		z_arc_connect_idu_set_mode(i, ARC_CONNECT_INTRPT_TRIGGER_LEVEL,
			ARC_CONNECT_DISTRI_MODE_ROUND_ROBIN);
		z_arc_connect_idu_set_dest(i, 1 << core);
		z_arc_connect_idu_set_mask(i, 0x0);
	}

	z_arc_connect_idu_enable();

	return 0;
}

SYS_INIT(arc_nsim_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
