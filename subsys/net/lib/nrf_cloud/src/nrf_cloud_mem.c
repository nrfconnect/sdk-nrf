/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "nrf_cloud_codec_internal.h"

LOG_MODULE_REGISTER(nrf_cloud_mem, CONFIG_NRF_CLOUD_LOG_LEVEL);

static struct nrf_cloud_os_mem_hooks used_hooks = { .malloc_fn = k_malloc,
						    .calloc_fn = k_calloc,
						    .free_fn = k_free };

void *nrf_cloud_calloc(size_t count, size_t size)
{
	return used_hooks.calloc_fn(count, size);
}

void *nrf_cloud_malloc(size_t size)
{
	return used_hooks.malloc_fn(size);
}

void nrf_cloud_free(void *ptr)
{
	used_hooks.free_fn(ptr);
}

void nrf_cloud_os_mem_hooks_init(struct nrf_cloud_os_mem_hooks *hooks)
{
	__ASSERT_NO_MSG(hooks != NULL);

	LOG_DBG("Overriding OS mem hooks");

	used_hooks.malloc_fn = hooks->malloc_fn;
	used_hooks.calloc_fn = hooks->calloc_fn;
	used_hooks.free_fn = hooks->free_fn;

	/* Codec hooks need to be the same */
	(void)nrf_cloud_codec_init(hooks);
}
