/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <errno.h>
#include <cortex_m/tz.h>
#include <power/reboot.h>
#include <sys/util.h>
#include <autoconf.h>
#include <secure_services.h>
#include <string.h>

#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif
#ifdef CONFIG_SPM_SERVICE_FIND_FIRMWARE_INFO
#include <fw_info.h>
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

#define FICR_BASE               NRF_FICR_S_BASE
#define FICR_PUBLIC_ADDR        (FICR_BASE + 0x204)
#define FICR_PUBLIC_SIZE        0xA1C
#define FICR_RESTRICTED_ADDR    (FICR_BASE + 0x130)
#define FICR_RESTRICTED_SIZE    0x8

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
		{.start = FICR_PUBLIC_ADDR,
		 .size = FICR_PUBLIC_SIZE},
		{.start = FICR_RESTRICTED_ADDR,
		 .size = FICR_RESTRICTED_SIZE},
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


#ifdef CONFIG_SPM_SERVICE_FIND_FIRMWARE_INFO
__TZ_NONSECURE_ENTRY_FUNC
int spm_firmware_info(u32_t fw_address, struct fw_info *info)
{
	const struct fw_info *tmp_info;

	if (info == NULL) {
		return -EINVAL;
	}

	tmp_info = fw_info_find(fw_address);

	if (tmp_info != NULL) {
		memcpy(info, tmp_info, sizeof(*tmp_info));
		return 0;
	}

	return -EFAULT;
}
#endif /* CONFIG_SPM_SERVICE_FIND_FIRMWARE_INFO */
