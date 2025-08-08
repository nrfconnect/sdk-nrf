/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file includes configuration of power limit table and power map table.
 */

#ifndef _VENDOR_RADIO_DEFAULT_CONF_H_
#define _VENDOR_RADIO_DEFAULT_CONF_H_

#ifndef POWER_LIMIT_TABLE_5
#define POWER_LIMIT_TABLE_5                                                     \
    {                                                                           \
        { "1", 7, 2},                                                           \
        {                                                                       \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 }, \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 },                 \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                 \
                                                                                \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 }, \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 },                 \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                 \
        }                                                                       \
    }
#endif

#ifndef POWER_LIMIT_TABLE_NRF_DK
#define POWER_LIMIT_TABLE_NRF_DK                                                \
    {                                                                           \
        { "1", 7, 2},                                                           \
        {                                                                       \
            { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },                 \
            { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 }, \
            { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },                 \
            { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },                 \
            { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },                 \
            { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },                 \
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                 \
                                                                                \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 }, \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 },                 \
            { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 },                 \
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                 \
        }                                                                       \
    }
#endif

/**
 * @def DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED
 *
 * The default values for PSEMI FEM power map channel configuration with FEM enabled.
 *
 */
#define DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED                    \
    {                                                                      \
        .entries = {                                                       \
            {.actual_power = 0, .radio_tx_power_db = -8, .att_state = 12}, \
            {.actual_power = 1, .radio_tx_power_db = -4, .att_state = 15}, \
            {.actual_power = 2, .radio_tx_power_db = -4, .att_state = 14}, \
            {.actual_power = 3, .radio_tx_power_db = -4, .att_state = 13}, \
            {.actual_power = 4, .radio_tx_power_db = -4, .att_state = 12}, \
            {.actual_power = 5, .radio_tx_power_db = 0, .att_state = 15},  \
            {.actual_power = 6, .radio_tx_power_db = 0, .att_state = 14},  \
            {.actual_power = 7, .radio_tx_power_db = 0, .att_state = 13},  \
            {.actual_power = 8, .radio_tx_power_db = 0, .att_state = 12},  \
            {.actual_power = 9, .radio_tx_power_db = 0, .att_state = 11},  \
            {.actual_power = 10, .radio_tx_power_db = 0, .att_state = 10}, \
            {.actual_power = 11, .radio_tx_power_db = 0, .att_state = 9},  \
            {.actual_power = 12, .radio_tx_power_db = 0, .att_state = 8},  \
            {.actual_power = 13, .radio_tx_power_db = 0, .att_state = 7},  \
            {.actual_power = 14, .radio_tx_power_db = 0, .att_state = 6},  \
            {.actual_power = 15, .radio_tx_power_db = 0, .att_state = 5},  \
            {.actual_power = 16, .radio_tx_power_db = 0, .att_state = 4},  \
            {.actual_power = 17, .radio_tx_power_db = 0, .att_state = 3},  \
            {.actual_power = 18, .radio_tx_power_db = 0, .att_state = 2},  \
            {.actual_power = 19, .radio_tx_power_db = 0, .att_state = 1},  \
            {.actual_power = 20, .radio_tx_power_db = 0, .att_state = 0},  \
            {.actual_power = 21, .radio_tx_power_db = 4, .att_state = 3}   \
        }                                                                  \
    }

/**
 * @def DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED
 *
 * The default values for PSEMI FEM power map channel configuration with FEM disabled.
 *
 */
#define DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED                 \
    {                                                                    \
        .entries = {                                                     \
            {.actual_power = 0, .radio_tx_power_db = 0, .att_state = 0}, \
            {.actual_power = 0, .radio_tx_power_db = 0, .att_state = 0}, \
            {.actual_power = 2, .radio_tx_power_db = 2, .att_state = 0}, \
            {.actual_power = 3, .radio_tx_power_db = 3, .att_state = 0}, \
            {.actual_power = 4, .radio_tx_power_db = 4, .att_state = 0}, \
            {.actual_power = 5, .radio_tx_power_db = 5, .att_state = 0}, \
            {.actual_power = 6, .radio_tx_power_db = 6, .att_state = 0}, \
            {.actual_power = 7, .radio_tx_power_db = 7, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}, \
            {.actual_power = 8, .radio_tx_power_db = 8, .att_state = 0}  \
        }                                                                \
    }

/**
 * @def DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_DISABLED
 *
 * The default power map table configuration.
 *
 */
#ifndef DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_DISABLED
#define DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_DISABLED            \
    {                                                         \
        .version  = 2,                                        \
        .channels = {                                         \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_DISABLED, \
        }                                                     \
    }
#endif

/**
 * @def DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_ENABLED
 *
 * The default power map table configuration.
 *
 */
#ifndef DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_ENABLED
#define DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_ENABLED            \
    {                                                        \
        .version  = 2,                                       \
        .channels = {                                        \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
            DEFAULT_POWER_MAP_FOR_CHANNEL_FEM_PSEMI_ENABLED, \
        }                                                    \
    }
#endif

#endif // _VENDOR_RADIO_DEFAULT_CONF_H_
