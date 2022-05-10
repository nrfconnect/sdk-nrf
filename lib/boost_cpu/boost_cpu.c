/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <zephyr.h>
#include <nrfx_clock.h>

#if CONFIG_BOOST_CPU_CLOCK_AT_STARTUP
#include <init.h>
#endif

LOG_MODULE_REGISTER(boost_cpu, CONFIG_BOOST_CPU_LOG_LEVEL);

void cpu_boost(bool enable)
{
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       enable ? NRF_CLOCK_HFCLK_DIV_1 : NRF_CLOCK_HFCLK_DIV_2);

	LOG_DBG("Boosting up CPU clock to %u MHz", SystemCoreClock/MHZ(1));
}

#if CONFIG_BOOST_CPU_CLOCK_AT_STARTUP

static int boost_cpu_clock(const struct device *dev)
{
	ARG_UNUSED(dev);

	cpu_boost(true);

	return 0;
}

SYS_INIT(boost_cpu_clock, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
