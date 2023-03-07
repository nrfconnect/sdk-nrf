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


/** @brief Modem library mode */
enum nrf_modem_mode {
	/** Normal operation mode */
	NORMAL_MODE,
	/** Bootloader (full DFU) mode */
	BOOTLOADER_MODE,
};

/**
 * @brief Initialize the Modem library and turn on the modem.
 *
 * This function initializes all integration components of the nrf_modem library
 * and turns on the modem. The operation can take a few minutes when a firmware update
 * is scheduled. If your application supports modem firmware updates, consider initializing
 * the library manually to have control of what the application should do in the meantime.
 *
 * The library can initialize the modem in normal mode or bootloader mode.
 *
 * The bootloader mode is used to update the whole modem firmware.
 * When modem is initialized in bootloader mode, no other functionality is available.
 * In particular, networking sockets and AT commands won't be available.
 *
 * To switch between the bootloader mode and normal mode, shutdown the modem
 * with @ref nrf_modem_lib_shutdown() and re-initialize it in the desired mode.
 *
 * @param[in] mode Initialization mode.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_init(enum nrf_modem_mode mode);

/**
 * @brief Shutdown the Modem library and turn off the modem.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_shutdown(void);

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
 * @brief Define a callback for @ref nrf_modem_lib_init calls.
 *
 * The callback function @p _callback is invoked after the library has been initialized.
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
 * @brief Modem fault handler.
 *
 * @param[in] fault_info Modem fault information.
 *			 Contains the fault reason and, in some cases, the modem program counter.
 */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info);

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

#endif /* defined(CONFIG_NRF_MODEM_LIB_MEM_DIAG) || defined(__DOXYGEN__) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_MODEM_LIB_H_ */
