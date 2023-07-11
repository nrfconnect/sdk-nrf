/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTM_UART_TWOWIRE_H_
#define DTM_UART_TWOWIRE_H_

#include <stdint.h>

/** @brief Initialize DTM over Two Wire UART interface.
 *
 * @note This function also initializes the DTM module.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int dtm_uart_twowire_init(void);

/** @brief Poll for DTM command.
 *
 * This function polls for Two Wire UART command.
 *
 * @return 16-bit command.
 */
uint16_t dtm_uart_twowire_get(void);

/** @brief Process DTM command and respond.
 *
 * This function processes the DTM command
 * and responds to the tester.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int dtm_uart_twowire_process(uint16_t cmd);

#endif
