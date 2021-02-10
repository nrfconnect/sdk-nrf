/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <nrf_modem.h>

/**
 * @brief Initialize the Modem library.
 *
 * This function synchronously turns on the modem; it could block
 * for a few minutes when the modem firmware is being updated.
 *
 * If you application supports modem firmware updates, consider
 * initializing the library manually to have control of what
 * the application should do while initialization is ongoing.
 *
 * The library has two operation modes, normal mode and full DFU mode.
 * The full DFU mode is used to update the whole modem firmware.
 *
 * When the library is initialized in full DFU mode, all shared memory regions
 * are reserverd for the firmware update operation, and no other functionality
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
int nrf_modem_lib_init(enum nrf_modem_mode_t mode);

/**
 * @brief Makes a thread sleep until next time nrf_modem_lib_init() is called.
 *
 * When nrf_modem_lib_shutdown() is called a thread can call this function to be
 * woken up next time nrf_modem_lib_init() is called.
 */
void nrf_modem_lib_shutdown_wait(void);

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

#ifdef __cplusplus
}
#endif
