/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <cortex_m/tz.h>
#include <misc/reboot.h>
#include <misc/util.h>
#include <autoconf.h>
#include <secure_services.h>
#include <string.h>

#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif

/*
 * Secure Entry functions to allow access to secure services from non-secure
 * firmware.
 *
 * Note: the function will be located in a Non-Secure
 * Callable region of the Secure Firmware Image.
 */

#ifdef CONFIG_SPM_SERVICE_RNG
#ifdef MBEDTLS_CONFIG_FILE
#include MBEDTLS_CONFIG_FILE
#else
#include "mbedtls/config.h"
#endif /* MBEDTLS_CONFIG_FILE */

#include <mbedtls/platform.h>
#include <mbedtls/entropy_poll.h>
#endif /* CONFIG_SPM_SERVICE_RNG */


int spm_secure_services_init(void)
{
	int err = 0;

#ifdef CONFIG_SPM_SERVICE_RNG
	mbedtls_platform_context platform_ctx = {0};
	err = mbedtls_platform_setup(&platform_ctx);
#endif
	return err;
}


#ifdef CONFIG_SPM_SERVICE_READ
struct read_range {
	u32_t start;
	size_t size;
};

__TZ_NONSECURE_ENTRY_FUNC
int spm_request_read(void *destination, u32_t addr, size_t len)
{
	static const struct read_range ranges[] = {
#ifdef PM_MCUBOOT_ADDRESS
		/* Allow reads of mcuboot metadata */
		{.start = PM_MCUBOOT_PAD_ADDRESS,
		 .size = PM_MCUBOOT_PAD_SIZE},
#endif
	};

	if (destination == NULL || len <= 0) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ranges); i++) {
		u32_t start = ranges[i].start;
		u32_t size = ranges[i].size;

		if (addr >= start && addr + len <= start + size) {
			memcpy(destination, (const void *)addr, len);
			return 0;
		}
	}

	return -EPERM;
}
#endif /* CONFIG_SPM_SERVICE_READ */

#ifdef CONFIG_SPM_SERVICE_REBOOT
__TZ_NONSECURE_ENTRY_FUNC
void spm_request_system_reboot(void)
{
	sys_reboot(SYS_REBOOT_COLD);
}
#endif /* CONFIG_SPM_SERVICE_REBOOT */


#ifdef CONFIG_SPM_SERVICE_RNG
__TZ_NONSECURE_ENTRY_FUNC
int spm_request_random_number(u8_t *output, size_t len, size_t *olen)
{
	int err;

	if (len != MBEDTLS_ENTROPY_MAX_GATHER) {
		return -EINVAL;
	}

	err = mbedtls_hardware_poll(NULL, output, len, olen);
	return err;
}
#endif /* CONFIG_SPM_SERVICE_RNG */
