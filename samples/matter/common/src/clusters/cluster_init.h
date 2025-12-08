/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cluster_init.h
 *
 * @defgroup nrf_matter_cluster_init Matter Cluster Initialization
 *
 * @{
 *
 * @brief Automatic Matter cluster initialization mechanism.
 *
 * This module provides a mechanism for automatic registration and initialization
 * of Matter code-driven clusters. Cluster implementations can use the NRF_MATTER_CLUSTER_INIT
 * macro to register their initialization functions, which will be automatically
 * called during Matter server startup.
 */

/**
 * @brief Cluster initialization callback function type.
 *
 * @return true if initialization succeeded, false otherwise
 */
typedef bool (*nrf_matter_cluster_init_cb_t)(void);

/**
 * @brief Cluster initialization callback entry.
 */
struct nrf_matter_cluster_init_entry {
	/** Cluster initialization callback function */
	nrf_matter_cluster_init_cb_t callback;
	/** Optional null-terminated cluster name for logging purposes */
	const char *name;
};

/**
 * @brief Define a cluster initialization callback.
 *
 * Use this macro in a cluster's .cpp file to register an initialization function
 * that will be automatically called during Matter server startup.
 *
 * The callback function should perform cluster-specific initialization such as:
 * - Unregistering default cluster implementations
 * - Registering custom cluster implementations
 *
 * @note The callback shall be invoked after the Matter server has been initialized
 *       and the cluster registry is available. The callback shall run in the context
 *       of the thread that calls nrf_matter_cluster_init_run_all().
 *
 * @note The source file name is automatically captured at compile time.
 *       Uses __FILE_NAME__ (GCC 12+/Clang) if available, falls back to __FILE__.
 *
 * @param _id Unique identifier for this initialization entry (used for symbol naming)
 * @param _callback Callback function of type bool (*)(void)
 *
 * Example usage:
 * @code
 * static bool MyClusterInit()
 * {
 *     auto &registry = chip::app::CodegenDataModelProvider::Instance().Registry();
 *     // ... cluster registration logic ...
 *     return true;
 * }
 *
 * NRF_MATTER_CLUSTER_INIT(my_cluster, MyClusterInit);
 * @endcode
 */

/* Use __FILE_NAME__ if available (GCC 12+, Clang 9+), otherwise fall back to __FILE__ */
#ifdef __FILE_NAME__
#define _NRF_MATTER_CLUSTER_FILE_NAME __FILE_NAME__
#else
#define _NRF_MATTER_CLUSTER_FILE_NAME __FILE__
#endif

#define NRF_MATTER_CLUSTER_INIT(_id, _callback)                                                              \
	STRUCT_SECTION_ITERABLE(nrf_matter_cluster_init_entry, nrf_matter_cluster_init_##_id) = {                      \
		.callback = _callback,                                                                                 \
		.name = _NRF_MATTER_CLUSTER_FILE_NAME,                                                                 \
	}

/**
 * @brief Run all registered cluster initialization callbacks.
 *
 * This function iterates through all cluster initialization entries registered
 * via NRF_MATTER_CLUSTER_INIT and invokes their callbacks. It should be called
 * after the Matter server has been initialized and the cluster registry is
 * available.
 *
 * @return true if all callbacks succeeded, false if any callback failed
 */
bool nrf_matter_cluster_init_run_all(void);

/** @} */

#ifdef __cplusplus
}
#endif
