/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_PROVISIONING_MEM_H__
#define NRF_PROVISIONING_MEM_H__

#if defined(CONFIG_NRF_PROVISIONING_HEAP_SYSTEM)
#include <stdlib.h>
#define nrf_provisioning_malloc malloc
#define nrf_provisioning_free   free
#else
#include <zephyr/kernel.h>
#define nrf_provisioning_malloc k_malloc
#define nrf_provisioning_free   k_free
#endif

#endif /* NRF_PROVISIONING_MEM_H__ */
