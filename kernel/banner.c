/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <version.h>
#include <zephyr_commit.h>
#include <ncs_version.h>
#include <ncs_commit.h>

#if defined(CONFIG_NCS_APPLICATION_BOOT_BANNER_STRING)
#include <app_version.h>
#if defined(NCS_APPLICATION_BOOT_BANNER_GIT_REPO)
#include <app_commit.h>
#endif
#endif

#if defined(CONFIG_NCS_APPLICATION_BOOT_BANNER_STRING)
#define NCS_PREFIX "Using "
#else
#define NCS_PREFIX "Booting "
#endif

void boot_banner(void)
{
#if defined(CONFIG_BOOT_DELAY) && (CONFIG_BOOT_DELAY > 0)
	printk("*** Delaying boot by " STRINGIFY(CONFIG_BOOT_DELAY) "ms... ***\n");
	k_busy_wait(CONFIG_BOOT_DELAY * USEC_PER_MSEC);
#endif

#if defined(CONFIG_NCS_APPLICATION_BOOT_BANNER_STRING)
	printk("*** Booting " CONFIG_NCS_APPLICATION_BOOT_BANNER_STRING " v"
	       APP_VERSION_STRING
#if defined(NCS_APPLICATION_BOOT_BANNER_GIT_REPO)
	       "-" APP_COMMIT_STRING
#else
	       " - unknown commit"
#endif
	       " ***\n");
#endif

#if defined(CONFIG_NCS_NCS_BOOT_BANNER_STRING)
	printk("*** " NCS_PREFIX CONFIG_NCS_NCS_BOOT_BANNER_STRING " v"
	       NCS_VERSION_STRING "-" NCS_COMMIT_STRING " ***\n");
#endif

#if defined(CONFIG_NCS_ZEPHYR_BOOT_BANNER_STRING)
	printk("*** Using " CONFIG_NCS_ZEPHYR_BOOT_BANNER_STRING " v"
	       KERNEL_VERSION_STRING "-" ZEPHYR_COMMIT_STRING " ***\n");
#endif
}
