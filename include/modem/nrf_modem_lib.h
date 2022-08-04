/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_MODEM_LIB_H_
#define NRF_MODEM_LIB_H_

#include <zephyr/kernel.h>
#include <nrf_modem.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file nrf_modem_lib.h
 *
 * @defgroup nrf_modem_lib nRF Modem library wrapper
 *
 * @{
 *
 * @brief API of the SMS nRF Modem library wrapper module.
 */

/**
 * @brief Initialize the Modem library.
 *
 * This function synchronously turns on the modem; it could block
 * for a few minutes when the modem firmware is being updated.
 *
 * If your application supports modem firmware updates, consider
 * initializing the library manually to have control of what
 * the application should do while initialization is ongoing.
 *
 * The library has two operation modes, normal mode and full DFU mode.
 * The full DFU mode is used to update the whole modem firmware.
 *
 * When the library is initialized in full DFU mode, all shared memory regions
 * are reserved for the firmware update operation, and no other functionality
 * can be used. In particular, sockets won't be available to the application.
 *
 * To switch between the full DFU mode and normal mode,
 * shutdown the modem with @ref nrf_modem_lib_shutdown() and re-initialize
 * it in the desired operation mode.
 *
 * @param[in] mode Library mode.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_init(enum nrf_modem_mode mode);

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
 * @brief Makes a thread sleep until next time nrf_modem_lib_init() is called.
 *
 * When nrf_modem_lib_shutdown() is called a thread can call this function to be
 * woken up next time nrf_modem_lib_init() is called.
 */
__deprecated void nrf_modem_lib_shutdown_wait(void);

/**
 * @brief Get the last return value of nrf_modem_lib_init.
 *
 * This function can be used to access the last return value of
 * nrf_modem_lib_init. This can be used to check the state of a modem
 * firmware exchange when the Modem library was initialized at boot-time.
 *
 * @return int The last return value of nrf_modem_lib_init.
 */
int nrf_modem_lib_get_init_ret(void);

/**
 * @brief Shutdown the Modem library, releasing its resources.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_shutdown(void);

/**
 * @brief Print diagnostic information for the TX heap.
 */
void nrf_modem_lib_shm_tx_diagnose(void);

/**
 * @brief Print diagnostic information for the library heap.
 */
void nrf_modem_lib_heap_diagnose(void);

/**
 * @brief Modem fault handler.
 *
 * @param[in] fault_info Modem fault information.
 *			 Contains the fault reason and, in some cases, the modem program counter.
 */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_MODEM_LIB_H_ */
