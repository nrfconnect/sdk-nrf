/*
 * Copyright (c) 2019-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_MODEM_LIB_H_
#define NRF_MODEM_LIB_H_

#include <nrf_modem.h>
#include <zephyr/kernel.h>

#if CONFIG_NRF_MODEM_LIB_MEM_DIAG
#include <zephyr/sys/sys_heap.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file nrf_modem_lib.h
 *
 * @defgroup nrf_modem_lib Modem library integration layer.
 *
 * @{
 *
 * @brief Modem library wrapper.
 */

/**
 * @brief Initialize the Modem library and turn on the modem.
 *
 * The operation can take a few minutes when a firmware update is scheduled.
 *
 * To switch between the bootloader mode and normal operating mode, shutdown the modem
 * with @ref nrf_modem_lib_shutdown() and re-initialize it in the desired mode.
 * Use @ref nrf_modem_lib_init() to initialize in normal mode and
 * @ref nrf_modem_lib_bootloader_init() to initialize the Modem library in bootloader mode.
 *
 * @retval Zero on success.
 *
 * @retval -NRF_EPERM The Modem library is already initialized.
 * @retval -NRF_EFAULT @c init_params is @c NULL.
 * @retval -NRF_ENOLCK Not enough semaphores.
 * @retval -NRF_ENOMEM Not enough shared memory.
 * @retval -NRF_EINVAL Control region size is incorrect or missing handlers in @c init_params.
 * @retval -NRF_ENOTSUPP RPC version mismatch.
 * @retval -NRF_ETIMEDOUT Operation timed out.
 * @retval -NRF_ACCESS Modem firmware authentication failure.
 * @retval -NRF_EAGAIN Modem device firmware upgrade failure.
 *		       DFU is scheduled at next initialization.
 * @retval -NRF_EIO Modem device firmware upgrade failure.
 *		    Reprogramming the modem firmware is necessary to recover.
 */
int nrf_modem_lib_init(void);

/**
 * @brief Initialize the Modem library in bootloader mode and turn on the modem.
 *
 * When the modem is initialized in bootloader mode, no other functionality is available. In
 * particular, networking sockets and AT commands won't be available.
 *
 * To switch between the bootloader mode and normal operating mode, shutdown the modem
 * with @ref nrf_modem_lib_shutdown() and re-initialize it in the desired mode.
 * Use @ref nrf_modem_lib_init() to initialize in normal mode and
 * @ref nrf_modem_lib_bootloader_init() to initialize the Modem library in bootloader mode.
 *
 * @retval Zero on success.
 *
 * @retval -NRF_EPERM The Modem library is already initialized.
 * @retval -NRF_EFAULT @c init_params is @c NULL.
 * @retval -NRF_ENOLCK Not enough semaphores.
 * @retval -NRF_ENOMEM Not enough shared memory.
 * @retval -NRF_EINVAL Missing handler in @c init_params.
 * @retval -NRF_EACCES Bad root digest.
 * @retval -NRF_ETIMEDOUT Operation timed out.
 * @retval -NRF_EIO Bootloader fault.
 * @retval -NRF_ENOSYS Operation not available.
 */
int nrf_modem_lib_bootloader_init(void);

/**
 * @brief Shutdown the Modem library and turn off the modem.
 *
 * @note The modem must be put in minimal function mode before being shut down.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_shutdown(void);

/**
 * @brief Modem library dfu callback struct.
 */
struct nrf_modem_lib_dfu_cb {
	/**
	 * @brief Callback function.
	 * @param dfu_res The return value of nrf_modem_init()
	 * @param ctx User-defined context
	 */
	void (*callback)(int dfu_res, void *ctx);
	/** User defined context */
	void *context;
};

/**
 * @brief Modem library initialization callback struct.
 */
struct nrf_modem_lib_init_cb {
	/**
	 * @brief Callback function.
	 * @param ret The return value of nrf_modem_init()
	 * @param ctx User-defined context
	 */
	void (*callback)(int ret, void *ctx);
	/** User defined context */
	void *context;
};

/**
 * @brief Modem library shutdown callback struct.
 */
struct nrf_modem_lib_shutdown_cb {
	/**
	 * @brief Callback function.
	 * @param ctx User-defined context
	 */
	void (*callback)(void *ctx);
	/** User defined context */
	void *context;
};

/**
 * @brief AT CFUN callback entry.
 */
struct nrf_modem_lib_at_cfun_cb {
	/** CFUN callback. */
	void (*callback)(int mode, void *ctx);
	/** User defined context */
	void *context;
};

/**
 * @brief Define a callback for DFU result @ref nrf_modem_lib_init calls.
 *
 * @note The @c NRF_MODEM_LIB_ON_DFU_RES callback can be used to subscribe to the result of a modem
 * DFU operation.
 *
 * @param name Callback name
 * @param _callback Callback function name
 * @param _context User-defined context for the callback
 */
#define NRF_MODEM_LIB_ON_DFU_RES(name, _callback, _context)                                        \
	static void _callback(int dfu_res, void *ctx);                                             \
	STRUCT_SECTION_ITERABLE(nrf_modem_lib_dfu_cb, nrf_modem_dfu_hook_##name) = {               \
		.callback = _callback,                                                             \
		.context = _context,                                                               \
	};

/**
 * @brief Define a callback for @ref nrf_modem_lib_init calls.
 *
 * The callback function @p _callback is invoked after the library has been initialized.
 *
 * @note The @c NRF_MODEM_LIB_ON_INIT callback can be used to perform modem and library
 * configurations that require the modem to be turned on in offline mode. It cannot be used to
 * change the modem functional mode. Calls to @c lte_lc_connect and CFUN AT calls are not
 * allowed, and must be done after @c nrf_modem_lib_init has returned. If a library needs to
 * perform operations after the link is up, it can use the link controller and subscribe to a
 * @c LTE_LC_ON_CFUN callback.
 *
 * @param name Callback name
 * @param _callback Callback function name
 * @param _context User-defined context for the callback
 */
#define NRF_MODEM_LIB_ON_INIT(name, _callback, _context)                                           \
	static void _callback(int ret, void *ctx);                                                 \
	STRUCT_SECTION_ITERABLE(nrf_modem_lib_init_cb, nrf_modem_hook_##name) = {                  \
		.callback = _callback,                                                             \
		.context = _context,                                                               \
	};

/**
 * @brief Define a callback for @ref nrf_modem_lib_shutdown calls.
 *
 * The callback function @p _callback is invoked before the library is shutdown.
 *
 * @param name Callback name
 * @param _callback Callback function name
 * @param _context User-defined context for the callback
 */
#define NRF_MODEM_LIB_ON_SHUTDOWN(name, _callback, _context)                                       \
	static void _callback(void *ctx);                                                          \
	STRUCT_SECTION_ITERABLE(nrf_modem_lib_shutdown_cb, nrf_modem_hook_##name) = {              \
		.callback = _callback,                                                             \
		.context = _context,                                                               \
	};

/**
 * @brief Define a callback for successful AT CFUN calls.
 *
 * @param name Callback name
 * @param _callback Callback function name
 * @param _context User-defined context for the callback
 */
#define NRF_MODEM_LIB_ON_CFUN(name, _callback, _context)                                           \
	static void _callback(int mode, void *ctx);                                                \
	STRUCT_SECTION_ITERABLE(nrf_modem_lib_at_cfun_cb, nrf_modem_at_cfun_hook_##name) = {       \
		.callback = _callback,                                                             \
		.context = _context,                                                               \
	};

/**
 * @brief Modem fault handler.
 *
 * @param[in] fault_info Modem fault information.
 *			 Contains the fault reason and, in some cases, the modem program counter.
 */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info);

#if defined(CONFIG_NRF_MODEM_LIB_FAULT_STRERROR) || defined(__DOXYGEN__)
/**
 * @brief Retrieve a statically allocated textual description of a given fault.
 *
 * @param fault The fault.
 * @return const char* Textual description of the given fault.
 */
const char *nrf_modem_lib_fault_strerror(int fault);
#endif

#if defined(CONFIG_NRF_MODEM_LIB_MEM_DIAG) || defined(__DOXYGEN__)
struct nrf_modem_lib_diag_stats {
	struct {
		struct sys_memory_stats heap;
		uint32_t failed_allocs;
	} library;
	struct {
		struct sys_memory_stats heap;
		uint32_t failed_allocs;
	} shmem;
};
/**
 * @brief Retrieve heap runtime statistics.
 *
 * Retrieve runtime statistics for the shared memory and library heaps.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_diag_stats_get(struct nrf_modem_lib_diag_stats *stats);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_MODEM_LIB_H_ */
