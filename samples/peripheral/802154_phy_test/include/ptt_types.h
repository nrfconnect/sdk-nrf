/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Purpose: PHY Test Tool types */

#ifndef PTT_TYPES_H__
#define PTT_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

typedef bool ptt_bool_t;

typedef uint16_t ptt_pkt_len_t;

typedef int32_t (* ptt_uart_send_cb)(const uint8_t * p_pkt, ptt_pkt_len_t len, bool add_crlf);

typedef uint32_t ptt_time_t;
typedef void (* ptt_call_me_cb_t)(ptt_time_t timeout);

typedef ptt_bool_t ptt_cca_t;
typedef uint16_t ptt_ed_t;
typedef int8_t ptt_rssi_t;

typedef uint8_t ptt_rf_rx_error_t;

#define PTT_RF_RX_ERROR_NONE             0x00 /**< there is no error */
#define PTT_RF_RX_ERROR_INVALID_FCS      0x01 /**< received a frame with an invalid checksum */
#define PTT_RF_RX_ERROR_OPERATION_FAILED 0x02 /**< reception failed */

typedef uint8_t ptt_rf_tx_error_t;

#define PTT_RF_TX_ERROR_NONE             0x00 /**< there is no error */
#define PTT_RF_TX_ERROR_INVALID_ACK_FCS  0x01 /**< received ACK frame is valid, but has wrong FCS */
#define PTT_RF_TX_ERROR_NO_ACK           0x02 /**< ACK frame was not received during the timeout period */
#define PTT_RF_TX_ERROR_OPERATION_FAILED 0x03 /**< transmission failed */

#endif  /* PTT_TYPES_H__ */