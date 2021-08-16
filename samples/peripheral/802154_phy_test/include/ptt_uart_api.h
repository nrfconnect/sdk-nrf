/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: UART interface intended for external usage */

#ifndef PTT_UART_API_H__
#define PTT_UART_API_H__

#include "ptt_types.h"

/** @brief Initialize library UART processing module
 *
 *  @param send_cb - callback provided by application to send packets via UART
 *
 *  @return none
 */
void ptt_uart_init(ptt_uart_send_cb send_cb);

/** @brief Uninitialize library UART processing module
 *
 *  @param none
 *
 *  @return none
 */
void ptt_uart_uninit(void);

/** @brief Push received packet to library for further processing
 *
 *  @param pkt - received packet
 *  @param len - length of the packet
 *
 *  @return none
 */
void ptt_uart_push_packet(const uint8_t *pkt, ptt_pkt_len_t len);

#endif /* PTT_UART_API_H__ */
