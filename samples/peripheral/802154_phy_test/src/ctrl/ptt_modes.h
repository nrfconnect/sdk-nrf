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

/* Purpose: modes declaration intended for usage inside control module */

#ifndef PTT_MODES_H__
#define PTT_MODES_H__

#include "ptt_config.h"
#include "ptt_errors.h"
#include "ptt_types_internal.h"
#include "ptt_utils.h"

/** device modes */
typedef enum
{
    PTT_MODE_ZB_PERF_DUT = 0, /**< device under test mode */
    PTT_MODE_ZB_PERF_CMD,     /**< command mode */
    PTT_MODE_N                /**< mode count */
} ptt_mode_t;

/** @brief Switch current mode to a given
 *
 *  Uninitialize current mode, reset shared resources and initialize a new mode.
 *  If a given mode matches with current mode, current mode resets to state as after initialization.
 *
 *  @param new_mode - a new mode to switch
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_mode_switch(ptt_mode_t new_mode);

/** @brief Initialize default mode
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_mode_default_init(void);

/** @brief Uninitialize current mode
 *
 *  @param none
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_mode_uninit(void);

/** @brief Initialize Zigbee RF Performance Test Plan CMD mode
 *
 *  @param none
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_zb_perf_cmd_mode_init(void);

/** @brief Uninitialize Zigbee RF Performance Test Plan CMD mode
 *
 *  @param none
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_zb_perf_cmd_mode_uninit(void);

/** @brief Initialize Zigbee RF Performance Test Plan DUT mode
 *
 *  @param none
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_zb_perf_dut_mode_init(void);

/** @brief Uninitialize Zigbee RF Performance Test Plan DUT mode
 *
 *  @param none
 *
 *  @return ptt_ret_t returns PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_zb_perf_dut_mode_uninit(void);

#endif /* PTT_MODES_H__ */

