/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <xtensa/xtruntime.h>
#include <zephyr/irq_nextlevel.h>
#include <xtensa/hal.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <adsp_shim.h>
#include <adsp-clk.h>
#include <cavs-idc.h>
#include "soc.h"

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <zephyr/sw_isr_table.h>
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc);

#ifndef CONFIG_SOC_INTEL_CAVS_V15
# define SHIM_GPDMA_BASE_OFFSET   0x6500
# define SHIM_GPDMA_BASE(x)       (SHIM_GPDMA_BASE_OFFSET + (x) * 0x100)
# define SHIM_GPDMA_CLKCTL(x)     (SHIM_GPDMA_BASE(x) + 0x4)
# define SHIM_CLKCTL_LPGPDMAFDCGB BIT(0)

#endif


__imr void power_init(void)
{
#ifdef CONFIG_SOC_INTEL_CAVS_V15
	/* HP domain clocked by PLL
	 * LP domain clocked by PLL
	 * DSP Core 0 PLL Clock Select divide by 1
	 * DSP Core 1 PLL Clock Select divide by 1
	 * High Power Domain PLL Clock Select device by 2
	 * Low Power Domain PLL Clock Select device by 4
	 * Disable Tensilica Core Prevent Audio PLL Shutdown (TCPAPLLS)
	 * Disable Tensilica Core Prevent Local Clock Gating (Core 0)
	 * Disable Tensilica Core Prevent Local Clock Gating (Core 1)
	 *   - Disabling "prevent clock gating" means allowing clock gating
	 */
	CAVS_SHIM.clkctl = CAVS_CLKCTL_LMPCS;

	/* Rewrite the low power sequencing control bits */
	CAVS_SHIM.lpsctl = CAVS_SHIM.lpsctl;
#else
	/* Request HP ring oscillator and
	 * wait for status to indicate it's ready.
	 */
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_RHROSCC;
	while ((CAVS_SHIM.clkctl & CAVS_CLKCTL_RHROSCC) != CAVS_CLKCTL_RHROSCC) {
		k_busy_wait(10);
	}

	/* Request HP Ring Oscillator
	 * Select HP Ring Oscillator
	 * High Power Domain PLL Clock Select device by 2
	 * Low Power Domain PLL Clock Select device by 4
	 * Disable Tensilica Core(s) Prevent Local Clock Gating
	 *   - Disabling "prevent clock gating" means allowing clock gating
	 */
	CAVS_SHIM.clkctl = (CAVS_CLKCTL_RHROSCC |
			    CAVS_CLKCTL_OCS |
			    CAVS_CLKCTL_LMCS);

	/* Prevent LP GPDMA 0 & 1 clock gating */
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(0));
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(1));

	/* Disable power gating for first cores */
	CAVS_SHIM.pwrctl |= CAVS_PWRCTL_TCPDSPPG(0);

	/* On cAVS 1.8+, we must demand ownership of the timestamping
	 * and clock generator registers.  Lacking the former will
	 * prevent wall clock timer interrupts from arriving, even
	 * though the device itself is operational.
	 */
	sys_write32(GENO_MDIVOSEL | GENO_DIOPTOSEL, DSP_INIT_GENO);
	sys_write32(IOPO_DMIC_FLAG | IOPO_I2SSEL_MASK, DSP_INIT_IOPO);
#endif
}
