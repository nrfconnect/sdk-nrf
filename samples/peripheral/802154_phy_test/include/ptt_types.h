/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: PHY Test Tool types */

#ifndef PTT_TYPES_H__
#define PTT_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t ptt_pkt_len_t;

typedef int32_t (*ptt_uart_send_cb)(const uint8_t *pkt, ptt_pkt_len_t len, bool add_crlf);

typedef uint32_t ptt_time_t;
typedef void (*ptt_call_me_cb_t)(ptt_time_t timeout);

typedef uint16_t ptt_ed_t;
typedef int8_t ptt_rssi_t;

typedef uint8_t ptt_rf_rx_error_t;

#define PTT_RF_RX_ERROR_NONE 0x00 /**< there is no error */
#define PTT_RF_RX_ERROR_INVALID_FCS 0x01 /**< received a frame with an invalid checksum */
#define PTT_RF_RX_ERROR_OPERATION_FAILED 0x02 /**< reception failed */

typedef uint8_t ptt_rf_tx_error_t;

#define PTT_RF_TX_ERROR_NONE 0x00 /**< there is no error */
#define PTT_RF_TX_ERROR_INVALID_ACK_FCS 0x01 /**< received ACK frame is valid, but has wrong FCS */
#define PTT_RF_TX_ERROR_NO_ACK 0x02 /**< ACK was not received during the timeout period */
#define PTT_RF_TX_ERROR_OPERATION_FAILED 0x03 /**< transmission failed */

#endif /* PTT_TYPES_H__ */
