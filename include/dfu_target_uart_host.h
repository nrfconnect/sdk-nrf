/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DFU_TARGET_UART_HOST_H_
#define _DFU_TARGET_UART_HOST_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Starts the UART DFU thread.
 *
 * This does NOT start the DFU process, it allows receiving package from the
 * host.
 *
 * Once this function is called, the UART attached to the DFU process will
 * be required by this thread. It means that it will not be usable by any
 * other part of the application.
 *
 * @param uart The UART used to receive the update binary
 * @param on_done Callback called once the update is done
 * @return 0 on success
 * @return -EINVAL if the provided UART device is not ready
 */
int dfu_target_uart_host_start(const struct device *uart, void (*on_done)(bool success));

/**
 * @brief Stop the UART DFU process
 *
 * Stopping the DFU process will release the attached UART.
 *
 * @return 0 on success, or a negative errno otherwise
 */
int dfu_target_uart_host_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _DFU_TARGET_UART_HOST_H_ */
