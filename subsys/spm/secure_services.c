/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <cortex_m/tz.h>

#if defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)

extern void sys_arch_reboot(int type);

static void spm_system_reboot(void)
{
	sys_arch_reboot(0);
}

/** @brief Secure Service for issuing a System Reset
 *
 *
 * Secure Entry function for allowing Non-Secure firmware
 * to request a System Reset. The function simply calls the
 * Zephyr System Reset function, issuing a system reboot.
 *
 * Note: the function shall be located in a Non-Secure
 * Callable region of the Secure Firmware Image.
 */
void __TZ_NONSECURE_ENTRY_FUNC spm_request_system_reboot(void)
{
	spm_system_reboot();
}
#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS */
