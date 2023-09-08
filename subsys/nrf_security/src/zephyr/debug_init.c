/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <mbedtls/debug.h>

static int _mbedtls_init(void)
{
#if defined(CONFIG_MBEDTLS_DEBUG_LEVEL)
	mbedtls_debug_set_threshold(CONFIG_MBEDTLS_DEBUG_LEVEL);
#endif

	return 0;
}

SYS_INIT(_mbedtls_init, POST_KERNEL, 0);
