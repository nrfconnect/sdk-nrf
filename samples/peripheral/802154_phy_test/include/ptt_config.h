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

/* Purpose: settings intended for external configuration */

#ifndef PTT_CONFIG_H__
#define PTT_CONFIG_H__

#define PTT_EVENT_POOL_N                    8u
#define PTT_EVENT_DATA_SIZE                 320u /* necessary to keep aligned to 4 */
#define PTT_EVENT_CTX_SIZE                  32u  /* necessary to keep aligned to 4 */

#define PTT_TIMER_POOL_N                    8u

#define PTT_ED_TIME_US                      128u     /* will be rounded up to multiplication of 8 symbols (128 us) */

#define PTT_RSSI_TIME_MS                    1u       /* timeout for the measurement process to finish */
#define PTT_RSSI_ERROR_VALUE                INT8_MAX /* if this value returned by RSSI measurement procedure, error will be printed */

#define PTT_LQI_DELAY_MS                    5000u    /* time to wait for packet for `lgetlqi` command */

#define PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE     125u
#define PTT_CUSTOM_LTX_DEFAULT_REPEATS      1
#define PTT_CUSTOM_LTX_DEFAULT_TIMEOUT_MS   0
#define PTT_CUSTOM_LTX_TIMEOUT_MAX_VALUE    INT16_MAX

#define PTT_L_CARRIER_PULSE_MIN             1         /* must fit in int32_t */
#define PTT_L_CARRIER_PULSE_MAX             INT16_MAX /* must fit in int32_t */
#define PTT_L_CARRIER_INTERVAL_MIN          0         /* must fit in int32_t */
#define PTT_L_CARRIER_INTERVAL_MAX          INT16_MAX /* must fit in int32_t */
#define PTT_L_CARRIER_DURATION_MIN          0         /* must fit in int32_t */
#define PTT_L_CARRIER_DURATION_MAX          INT16_MAX /* must fit in int32_t */

#define PTT_L_STREAM_PULSE_MIN              1         /* must fit in int32_t */
#define PTT_L_STREAM_PULSE_MAX              INT16_MAX /* must fit in int32_t */
#define PTT_L_STREAM_INTERVAL_MIN           0         /* must fit in int32_t */
#define PTT_L_STREAM_INTERVAL_MAX           INT16_MAX /* must fit in int32_t */
#define PTT_L_STREAM_DURATION_MIN           0         /* must fit in int32_t */
#define PTT_L_STREAM_DURATION_MAX           INT16_MAX /* must fit in int32_t */

#define PTT_UART_PROMPT_STR                 ">"

#define PTT_LED_INDICATION_BLINK_TIMEOUT_MS 200

#endif /* PTT_CONFIG_H__ */