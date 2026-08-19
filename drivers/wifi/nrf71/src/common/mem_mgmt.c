/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Memory management functions for the nRF71 driver.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <common/mem_mgmt.h>
#include <common/fw_if/nrf71_wifi_ctrl.h>

/* Memory pool management - unified pool-based API */
#if defined(CONFIG_NRF_WIFI_CONNECT_SCAN_RESULTS_GDRAM)
/* Connect and display scan databases share the control pool but never run at
 * once, so only reserve what the connect one needs beyond the display one.
 */
#define NRF_WIFI_DISP_SCAN_DB                                                                      \
	(CONFIG_NRF_WIFI_SCAN_MAX_BSS_CNT * sizeof(struct umac_display_results))
#define NRF_WIFI_CONN_SCAN_DB CONFIG_NRF_WIFI_CONNECT_SCAN_RESULTS_GDRAM_SIZE
#define NRF_WIFI_CTRL_HEAP_EXTRA                                                                   \
	(NRF_WIFI_CONN_SCAN_DB > NRF_WIFI_DISP_SCAN_DB                                             \
		 ? NRF_WIFI_CONN_SCAN_DB - NRF_WIFI_DISP_SCAN_DB                                   \
		 : 0)
#else
#define NRF_WIFI_CTRL_HEAP_EXTRA 0
#endif
#if defined(CONFIG_NOCACHE_MEMORY)
K_HEAP_DEFINE_NOCACHE(wifi_drv_ctrl_mem_pool,
		      CONFIG_NRF_WIFI_CTRL_HEAP_SIZE + NRF_WIFI_CTRL_HEAP_EXTRA);
K_HEAP_DEFINE_NOCACHE(wifi_drv_data_mem_pool, CONFIG_NRF_WIFI_DATA_HEAP_SIZE);
#else
K_HEAP_DEFINE(wifi_drv_ctrl_mem_pool, CONFIG_NRF_WIFI_CTRL_HEAP_SIZE + NRF_WIFI_CTRL_HEAP_EXTRA);
K_HEAP_DEFINE(wifi_drv_data_mem_pool, CONFIG_NRF_WIFI_DATA_HEAP_SIZE);
#endif /* CONFIG_NOCACHE_MEMORY */
static struct k_heap *const wifi_ctrl_pool = &wifi_drv_ctrl_mem_pool;
static struct k_heap *const wifi_data_pool = &wifi_drv_data_mem_pool;

void nrf_wifi_mem_get_heaps(struct k_heap **ctrl, struct k_heap **data)
{
	if (ctrl != NULL) {
		*ctrl = wifi_ctrl_pool;
	}

	if (data != NULL) {
		*data = wifi_data_pool;
	}
}

void *nrf_wifi_mem_alloc(enum nrf_wifi_mem_pool_type pool_type, size_t size)
{
	size_t size_aligned = WORD_ALIGNED(size);
	struct k_heap *pool;

	switch (pool_type) {
	case NRF_WIFI_MEM_POOL_TYPE_DATA:
		pool = wifi_data_pool;
		break;
	case NRF_WIFI_MEM_POOL_TYPE_CTRL:
	default:
		pool = wifi_ctrl_pool;
		break;
	}

	return k_heap_aligned_alloc(pool, WORD_SIZE, size_aligned, K_FOREVER);
}

void *nrf_wifi_mem_zalloc(enum nrf_wifi_mem_pool_type pool_type, size_t size)
{
	void *ret = nrf_wifi_mem_alloc(pool_type, size);

	if (ret != NULL) {
		(void)memset(ret, 0, size);
	}
	return ret;
}

void nrf_wifi_mem_free(enum nrf_wifi_mem_pool_type pool_type, void *ptr)
{
	struct k_heap *pool;

	if (ptr == NULL) {
		return;
	}

	switch (pool_type) {
	case NRF_WIFI_MEM_POOL_TYPE_DATA:
		pool = wifi_data_pool;
		break;
	case NRF_WIFI_MEM_POOL_TYPE_CTRL:
	default:
		pool = wifi_ctrl_pool;
		break;
	}

	k_heap_free(pool, ptr);
}

void nrf_wifi_mem_cpy(void *dest, const void *src, size_t size)
{
	(void)memcpy(dest, src, size);
}

void nrf_wifi_mem_set(void *ptr, int value, size_t size)
{
	(void)memset(ptr, value, size);
}

int nrf_wifi_mem_cmp(const void *addr1, const void *addr2, size_t size)
{
	return memcmp(addr1, addr2, size);
}
