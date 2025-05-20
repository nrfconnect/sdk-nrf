/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <errno.h>
#include <stdbool.h>
#include "nrf_security_mutexes.h"

#ifdef NRF_SECURITY_MUTEX_IMPLEMENTATION
#include <zephyr/kernel.h>
#endif

#ifdef NRF_SECURITY_MUTEX_IMPLEMENTATION
int nrf_security_mutex_lock(nrf_security_mutex_t mutex)
{
	int ret = k_mutex_lock(mutex, K_FOREVER);

	__ASSERT_NO_MSG(ret == 0);
	return ret;
}

int nrf_security_mutex_unlock(nrf_security_mutex_t mutex)
{
	int ret = k_mutex_unlock(mutex);

	__ASSERT_NO_MSG(ret == 0);
	return ret;
}

#else
int nrf_security_mutex_lock(nrf_security_mutex_t mutex)
{
	(void)mutex;
	return 0;
}

int nrf_security_mutex_unlock(nrf_security_mutex_t mutex)
{
	(void)mutex;
	return 0;
}
#endif
