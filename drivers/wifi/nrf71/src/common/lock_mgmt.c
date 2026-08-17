/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Lock management functions for the nRF71 driver.
 */

#include <common/mem_mgmt.h>
#include <common/lock_mgmt.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

void *nrf_wifi_lock_alloc(void)
{
	struct k_mutex *lock;

	lock = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*lock));
	if (!lock) {
		LOG_ERR("%s: Unable to allocate memory for lock", __func__);
		return NULL;
	}

	return lock;
}

void nrf_wifi_lock_free(void *lock)
{
	if (lock) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, lock);
	}
}

void nrf_wifi_lock_init(void *lock)
{
	k_mutex_init(lock);
}

void nrf_wifi_lock_take(void *lock)
{
	k_mutex_lock(lock, K_FOREVER);
}

void nrf_wifi_lock_rel(void *lock)
{
	k_mutex_unlock(lock);
}

void nrf_wifi_lock_irq_take(void *lock, unsigned long *flags)
{
	ARG_UNUSED(flags);
	k_mutex_lock(lock, K_FOREVER);
}

void nrf_wifi_lock_irq_rel(void *lock, unsigned long *flags)
{
	ARG_UNUSED(flags);
	k_mutex_unlock(lock);
}
