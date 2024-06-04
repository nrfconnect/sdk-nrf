/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <errno.h>
#include <stdbool.h>
#include <nrf_security_mutexes.h>

#if !defined(__NRF_TFM__)
#include <zephyr/kernel.h>
#endif

#if defined(CONFIG_MULTITHREADING) && !defined(__NRF_TFM__)
int nrf_security_mutex_lock(nrf_security_mutex_t mutex)
{
	return k_mutex_lock(mutex, K_FOREVER);
}

int nrf_security_mutex_unlock(nrf_security_mutex_t mutex)
{
	return k_mutex_unlock(mutex);
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
