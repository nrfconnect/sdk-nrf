/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <errno.h>
#include <aarch32/cortex_m/tz.h>
#include <power/reboot.h>
#include <sys/util.h>
#include <autoconf.h>
#include <string.h>
#include <bl_validation.h>
#include <aarch32/cortex_m/cmse.h>

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
 *
 * These should not be called directly. Instead call them through their wrapper
 * functions, e.g. call spm_request_read_nse() via spm_request_read().
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

static bool ptr_in_secure_area(intptr_t ptr)
{
	return arm_cmse_addr_is_secure(ptr) == 1;
}

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
	uint32_t start;
	size_t size;
};


__TZ_NONSECURE_ENTRY_FUNC
int spm_request_read_nse(void *destination, uint32_t addr, size_t len)
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

	if (ptr_in_secure_area((intptr_t)destination)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ranges); i++) {
		uint32_t start = ranges[i].start;
		uint32_t size = ranges[i].size;

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
void spm_request_system_reboot_nse(void)
{
	sys_reboot(SYS_REBOOT_COLD);
}
#endif /* CONFIG_SPM_SERVICE_REBOOT */


#ifdef CONFIG_SPM_SERVICE_RNG
__TZ_NONSECURE_ENTRY_FUNC
int spm_request_random_number_nse(uint8_t *output, size_t len, size_t *olen)
{
	int err = -EINVAL;

	if (ptr_in_secure_area((intptr_t)output) ||
	    ptr_in_secure_area((intptr_t)olen)) {
		return -EINVAL;
	}

	if (len != MBEDTLS_ENTROPY_MAX_GATHER) {
		return -EINVAL;
	}

	err = mbedtls_hardware_poll(NULL, output, len, olen);
	return err;
}
#endif /* CONFIG_SPM_SERVICE_RNG */

#ifdef CONFIG_SPM_SERVICE_S0_ACTIVE
__TZ_NONSECURE_ENTRY_FUNC
int spm_s0_active(uint32_t s0_address, uint32_t s1_address, bool *s0_active)
{
	const struct fw_info *s0;
	const struct fw_info *s1;
	bool s0_valid;
	bool s1_valid;

	if (ptr_in_secure_area((intptr_t)s0_active)) {
		return -EINVAL;
	}

	s0 = fw_info_find(s0_address);
	s1 = fw_info_find(s1_address);

	s0_valid = (s0 != NULL) && (s0->valid == CONFIG_FW_INFO_VALID_VAL);
	s1_valid = (s1 != NULL) && (s1->valid == CONFIG_FW_INFO_VALID_VAL);

	if (!s1_valid && !s0_valid) {
		return -EINVAL;
	} else if (!s1_valid) {
		*s0_active = true;
	} else if (!s0_valid) {
		*s0_active = false;
	} else {
		*s0_active = s0->version >= s1->version;
	}

	return 0;
}
#endif /* CONFIG_SPM_SERVICE_S0_ACTIVE */

#ifdef CONFIG_SPM_SERVICE_FIND_FIRMWARE_INFO
__TZ_NONSECURE_ENTRY_FUNC
int spm_firmware_info_nse(uint32_t fw_address, struct fw_info *info)
{
	const struct fw_info *tmp_info;

	if (info == NULL) {
		return -EINVAL;
	}

	/* Ensure that fw_address is within secure area */
	if (!ptr_in_secure_area(fw_address)) {
		return -EINVAL;
	}

	/* Ensure that *info is in non-secure RAM */
	if (ptr_in_secure_area((intptr_t)info)) {
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


#ifdef CONFIG_SPM_SERVICE_PREVALIDATE
__TZ_NONSECURE_ENTRY_FUNC
int spm_prevalidate_b1_upgrade_nse(uint32_t dst_addr, uint32_t src_addr)
{
	if (!bl_validate_firmware_available()) {
		return -ENOTSUP;
	}
	bool result = bl_validate_firmware(dst_addr, src_addr);
	return result;
}
#endif /* CONFIG_SPM_SERVICE_PREVALIDATE */


#ifdef CONFIG_SPM_SERVICE_BUSY_WAIT
__TZ_NONSECURE_ENTRY_FUNC
void spm_busy_wait_nse(uint32_t busy_wait_us)
{
	k_busy_wait(busy_wait_us);
}
#endif /* CONFIG_SPM_SERVICE_BUSY_WAIT */

#if CONFIG_SPM_SERVICE_NS_HANDLER_FROM_SPM_FAULT
#include <secure_services.h>

static spm_ns_on_fatal_error_t ns_on_fatal_handler;

__TZ_NONSECURE_ENTRY_FUNC
int spm_set_ns_fatal_error_handler_nse(spm_ns_on_fatal_error_t handler)
{
	ns_on_fatal_handler = handler;

	return 0;
}

void z_spm_ns_fatal_error_handler(void)
{
	if (!ns_on_fatal_handler) {
		return;
	}

	TZ_NONSECURE_FUNC_PTR_DECLARE(fatal_handler_ns);
	fatal_handler_ns = TZ_NONSECURE_FUNC_PTR_CREATE(ns_on_fatal_handler);

	if (TZ_NONSECURE_FUNC_PTR_IS_NS(fatal_handler_ns)) {
		fatal_handler_ns();
	}
}
#endif /* CONFIG_SPM_SERVICE_NS_HANDLER_FROM_SPM_FAULT */
