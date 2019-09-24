/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <autoconf.h>
#include <secure_services.h>
#include <toolchain.h>

/* Overrides the weak ARM implementation:
 * Call into secure firmware if in the non-secure firmware since the non-secure
 * firmware is not allowed to directly reboot the system.
 */
#ifdef CONFIG_SPM_SERVICE_REBOOT
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	spm_request_system_reboot();
}
#endif
