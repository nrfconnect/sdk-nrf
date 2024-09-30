/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf53_cpunet_mgmt.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <hal/nrf_spu.h>

static int nrf53_cpunet_init(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	nrf_spu_extdomain_set((NRF_SPU_Type *)DT_REG_ADDR(DT_NODELABEL(spu)), 0, true, false);
#endif /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/*
	 * Building Zephyr with CONFIG_TRUSTED_EXECUTION_SECURE=y implies
	 * building also a Non-Secure image. The Non-Secure image will, in
	 * this case do the remainder of actions to properly configure and
	 * boot the Network MCU.
	 */

	nrf53_cpunet_enable(true);
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(nrf53_cpunet_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
