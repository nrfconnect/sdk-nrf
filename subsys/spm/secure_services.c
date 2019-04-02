/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <cortex_m/tz.h>
#include <misc/reboot.h>
#include <autoconf.h>

#ifdef CONFIG_SPM_SECURE_SERVICES
#ifdef CONFIG_SPM_SERVICE_REBOOT
/** @brief Implementation of Secure System Reset
 *
 * Currently, this simply calls the Zephyr System Reset function,
 * issuing a system reboot immediately.
 *
 * Note: the function will be located in a Non-Secure
 * Callable region of the Secure Firmware Image.
 */
static void spm_system_reboot(void)
{
	sys_reboot(SYS_REBOOT_WARM);
}

/** @brief Secure Service for issuing a System Reset
 *
 * Secure Entry function for allowing Non-Secure Firmware
 * to request a System Reset.
 *
 * Note: the function will be located in a Non-Secure
 * Callable region of the Secure Firmware Image.
 */
void __TZ_NONSECURE_ENTRY_FUNC spm_request_system_reboot(void)
{
	spm_system_reboot();
}
#endif /* CONFIG_SPM_SERVICE_REBOOT */
#endif /* CONFIG_SPM_SECURE_SERVICES */
