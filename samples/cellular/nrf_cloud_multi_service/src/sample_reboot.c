/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log_ctrl.h>
#include <modem/lte_lc.h>

#include "sample_reboot.h"

#define ERROR_REBOOT_S		30
#define NORMAL_REBOOT_S		10

LOG_MODULE_REGISTER(sample_reboot, CONFIG_MULTI_SERVICE_LOG_LEVEL);

/* Called by various subsystems in the sample when a clean reboot is needed. */
void sample_reboot(const bool error)
{
	unsigned int delay_s = error ? ERROR_REBOOT_S  : NORMAL_REBOOT_S;

	LOG_INF("Rebooting in %us%s", delay_s, error ? " due to error" : "...");

#if defined(CONFIG_LTE_LINK_CONTROL)
	if (error) {
		/* We must do this before we reboot, otherwise we might trip LTE boot loop
		 * prevention, and the modem will be temporarily disable itself.
		 */
		(void)lte_lc_power_off();
	}
#endif

	LOG_PANIC();
	k_sleep(K_SECONDS(delay_s));
	sys_reboot(SYS_REBOOT_COLD);
}
