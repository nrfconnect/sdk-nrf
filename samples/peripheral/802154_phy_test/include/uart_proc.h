/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: uart interfaces */

#ifndef UART_PROC_H__
#define UART_PROC_H__

#include <stdint.h>

#include "ptt.h"

/**< Amount of bytes requested from driver */
#define UART_BYTES_TO_READ (1u)

/** @brief Initialize UARTE driver and UART module inside PTT library
 *
 *  @param none
 *
 *  @return none
 *
 */
void uart_init(void);

int32_t uart_send(const uint8_t *pkt, ptt_pkt_len_t len, bool add_crlf);

#endif /* UART_PROC_H__ */
