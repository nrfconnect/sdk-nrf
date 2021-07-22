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

/* Purpose: RF interfaces */

#ifndef RF_PROC_H__
#define RF_PROC_H__

#include <stdint.h>

#include "ptt_rf_api.h"

/**< Size of PHY header */
#define RF_PHR_SIZE      (1u)

/**< Size of frame check sequence */
#define RF_FCS_SIZE      (2u)

/**< Maximum size of PSDU */
#define RF_PSDU_MAX_SIZE (127u)

/**< Start of PSDU payload */
#define RF_PSDU_START    (1u)

/** @brief NRF radio driver initialization
 *
 *  @param none
 *
 *  @return none
 */
void rf_init(void);

/** @brief NRF radio driver deinitialization
 *
 *  @param none
 *
 *  @return none
 */
void rf_uninit(void);

/** @brief Thread function for Radio handling
 *
 *  @param none
 *
 *  @return none
 *
 */
void rf_thread(void);

/**< element of received packets array */
typedef struct
{
    uint8_t     * p_rf_buf; /**< pointer to buffer inside radio driver with a received packet */
    uint8_t     * p_data;   /**< pointer to payload */
    ptt_pkt_len_t length;   /**< size of payload of received packet */
    int8_t        rssi;     /**< RSSI */
    uint8_t       lqi;      /**< LQI */
} rf_rx_pkt_t;

#endif  /* RF_PROC_H__ */