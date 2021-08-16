/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: settings intended for external configuration */

#ifndef PTT_CONFIG_H__
#define PTT_CONFIG_H__

#define PTT_EVENT_POOL_N 8u
#define PTT_EVENT_DATA_SIZE 320u /* necessary to keep aligned to 4 */
#define PTT_EVENT_CTX_SIZE 32u /* necessary to keep aligned to 4 */

#define PTT_TIMER_POOL_N 8u

/* will be rounded up to multiplication of 8 symbols (128 us) */
#define PTT_ED_TIME_US 128u

/* timeout for the measurement process to finish */
#define PTT_RSSI_TIME_MS 1u

/* if this value returned by RSSI measurement procedure, error will be printed */
#define PTT_RSSI_ERROR_VALUE INT8_MAX

/* time to wait for packet for `lgetlqi` command */
#define PTT_LQI_DELAY_MS 5000u

#define PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE 125u
#define PTT_CUSTOM_LTX_DEFAULT_REPEATS 1
#define PTT_CUSTOM_LTX_DEFAULT_TIMEOUT_MS 0
#define PTT_CUSTOM_LTX_TIMEOUT_MAX_VALUE INT16_MAX

#define PTT_L_CARRIER_PULSE_MIN 1 /* must fit in int32_t */
#define PTT_L_CARRIER_PULSE_MAX INT16_MAX /* must fit in int32_t */
#define PTT_L_CARRIER_INTERVAL_MIN 0 /* must fit in int32_t */
#define PTT_L_CARRIER_INTERVAL_MAX INT16_MAX /* must fit in int32_t */
#define PTT_L_CARRIER_DURATION_MIN 0 /* must fit in int32_t */
#define PTT_L_CARRIER_DURATION_MAX INT16_MAX /* must fit in int32_t */

#define PTT_L_STREAM_PULSE_MIN 1 /* must fit in int32_t */
#define PTT_L_STREAM_PULSE_MAX INT16_MAX /* must fit in int32_t */
#define PTT_L_STREAM_INTERVAL_MIN 0 /* must fit in int32_t */
#define PTT_L_STREAM_INTERVAL_MAX INT16_MAX /* must fit in int32_t */
#define PTT_L_STREAM_DURATION_MIN 0 /* must fit in int32_t */
#define PTT_L_STREAM_DURATION_MAX INT16_MAX /* must fit in int32_t */

#define PTT_UART_PROMPT_STR ">"

#define PTT_LED_INDICATION_BLINK_TIMEOUT_MS 200

#endif /* PTT_CONFIG_H__ */
