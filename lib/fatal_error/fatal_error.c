/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/fatal.h>

LOG_MODULE_REGISTER(fatal_error, CONFIG_FATAL_ERROR_LOG_LEVEL);

extern void sys_arch_reboot(int type);

void k_sys_fatal_error_handler(unsigned int reason,
			       const struct arch_esf *esf)
{
	ARG_UNUSED(esf);
	ARG_UNUSED(reason);

	LOG_PANIC();
	LOG_ERR("Resetting system");
	sys_arch_reboot(0);

	CODE_UNREACHABLE;
}
