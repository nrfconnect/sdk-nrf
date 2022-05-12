/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx_clock.h>
#include <init.h>

#define MODULE cpu_boost
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_CPU_BOOST_LOG_LEVEL);

static int boost_cpu_clock(const struct device *dev)
{
	ARG_UNUSED(dev);

	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);

	LOG_INF("Boosting up CPU clock to %u MHz", SystemCoreClock/MHZ(1));

	return 0;
}

SYS_INIT(boost_cpu_clock, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
