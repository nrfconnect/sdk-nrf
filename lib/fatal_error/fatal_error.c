/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <arch/cpu.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <fatal.h>

#if defined(CONFIG_IS_SPM) && \
	defined(CONFIG_SPM_SERVICE_NS_HANDLER_FROM_SPM_FAULT)
#include <secure_services.h>
#endif

LOG_MODULE_REGISTER(fatal_error, CONFIG_FATAL_ERROR_LOG_LEVEL);

extern void sys_arch_reboot(int type);

void k_sys_fatal_error_handler(unsigned int reason,
			       const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);
	ARG_UNUSED(reason);

	LOG_PANIC();

#if defined(CONFIG_IS_SPM) && \
	defined(CONFIG_SPM_SERVICE_NS_HANDLER_FROM_SPM_FAULT)
	z_spm_ns_fatal_error_handler();
#endif

	if (IS_ENABLED(CONFIG_RESET_ON_FATAL_ERROR)) {
		LOG_ERR("Resetting system");
		sys_arch_reboot(0);
	} else {
		LOG_ERR("Halting system");
		for (;;) {
			/* Spin endlessly */
		}
	}

	CODE_UNREACHABLE;
}
