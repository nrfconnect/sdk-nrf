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

/* Purpose: interface part of radio module */

#include <stddef.h>
#include <stdint.h>

#include "ctrl/ptt_ctrl.h"
#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"
#include "ptt.h"

#include "ptt_rf_api.h"
#include "ptt_rf_internal.h"

ptt_rf_ctx_t m_ptt_rf_ctx;

static ptt_ret_t ptt_rf_try_lock(ptt_evt_id_t evt_id);
static inline ptt_bool_t ptt_rf_is_locked(void);
static inline ptt_bool_t ptt_rf_is_locked_by(ptt_evt_id_t evt_id);
static inline ptt_evt_id_t ptt_rf_unlock(void);

static ptt_ret_t ptt_rf_promiscuous_set(ptt_bool_t value);

static void ptt_rf_stat_inc(int8_t rssi, uint8_t lqi);

static void ptt_rf_set_default_channel(void);
static void ptt_rf_set_default_power(void);
static void ptt_rf_set_default_antenna(void);

void ptt_rf_init(void)
{
    PTT_TRACE("%s ->\n", __func__);
    m_ptt_rf_ctx = (ptt_rf_ctx_t){0};
    ptt_rf_unlock();

    ptt_rf_set_default_channel();
    ptt_rf_set_default_power();
    ptt_rf_set_default_antenna();

    /* we need raw PHY packets without any filtering */
    ptt_rf_promiscuous_set(true);
    ptt_rf_receive_ext();
    PTT_TRACE("%s -<\n", __func__);
}

void ptt_rf_uninit(void)
{
    PTT_TRACE("%s \n", __func__);
}

void ptt_rf_reset(void)
{
    PTT_TRACE("%s ->\n", __func__);
    ptt_rf_uninit();
    ptt_rf_init();
    PTT_TRACE("%s -<\n", __func__);
}

static void ptt_rf_set_default_channel(void)
{
    uint32_t  channel_mask;
    ptt_ret_t ret = PTT_RET_SUCCESS;

    /* try to set channel from prod config */
    if (ptt_get_channel_mask_ext(&channel_mask))
    {
        ret = ptt_rf_set_channel_mask(PTT_RF_EVT_UNLOCKED, channel_mask);
        if (PTT_RET_SUCCESS != ret)
        {
            /* set default channel */
            ptt_rf_set_channel(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_CHANNEL);
        }
    }
    else
    {
        /* set default channel */
        ptt_rf_set_channel(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_CHANNEL);
    }
}

static void ptt_rf_set_default_power(void)
{
    int8_t power = PTT_RF_DEFAULT_POWER;

    /* set power from prod config */
    if (ptt_get_power_ext(&power))
    {
        ptt_rf_set_power(PTT_RF_EVT_UNLOCKED, power);
    }
    else
    {
        ptt_rf_set_power(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_POWER);
    }
}

static void ptt_rf_set_default_antenna(void)
{
    uint8_t antenna = PTT_RF_DEFAULT_ANTENNA;

    /* set antenna from prod config */
    if (ptt_get_antenna_ext(&antenna))
    {
        ptt_rf_set_antenna(PTT_RF_EVT_UNLOCKED, antenna);
    }
    else
    {
        ptt_rf_set_antenna(PTT_RF_EVT_UNLOCKED, PTT_RF_DEFAULT_ANTENNA);
    }
}

void ptt_rf_push_packet(const uint8_t * p_pkt, ptt_pkt_len_t len, int8_t rssi, uint8_t lqi)
{
    ptt_rf_stat_inc(rssi, lqi);
    ptt_ctrl_rf_push_packet(p_pkt, len, rssi, lqi);
}

void ptt_rf_tx_finished(void)
{
    if (ptt_rf_is_locked())
    {
        /* have to unlock before passing control out of RF module */
        ptt_evt_id_t evt_id = ptt_rf_unlock();

        ptt_ctrl_rf_tx_finished(evt_id);
    }
    else
    {
        PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
        /* we get event although we didn't send a packet, just pass it */
    }
}

void ptt_rf_tx_failed(ptt_rf_tx_error_t tx_error)
{
    if (ptt_rf_is_locked())
    {
        /* have to unlock before passing control out of RF module */
        ptt_evt_id_t evt_id = ptt_rf_unlock();

        ptt_ctrl_rf_tx_failed(evt_id, tx_error);
    }
    else
    {
        PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
        /* we get event although we didn't send a packet, just pass it */
    }
}

void ptt_rf_rx_failed(ptt_rf_rx_error_t rx_error)
{
    ptt_ctrl_rf_rx_failed(rx_error);
}

void ptt_rf_cca_done(ptt_cca_t result)
{
    if (ptt_rf_is_locked())
    {
        /* have to unlock before passing control out of RF module */
        ptt_evt_id_t evt_id = ptt_rf_unlock();

        ptt_ctrl_rf_cca_done(evt_id, result);
    }
    else
    {
        PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
        /* we get event although we didn't send a packet, just pass it */
    }
}

void ptt_rf_cca_failed(void)
{
    if (ptt_rf_is_locked())
    {
        /* have to unlock before passing control out of RF module */
        ptt_evt_id_t evt_id = ptt_rf_unlock();

        ptt_ctrl_rf_cca_failed(evt_id);
    }
    else
    {
        PTT_TRACE("%s: called, but rf module is not locked\n", __func__);
        /* we get event although we didn't send a packet, just pass it */
    }
}

ptt_ret_t ptt_rf_cca(ptt_evt_id_t evt_id, uint8_t mode)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        /* will be unlocked inside ptt_rf_cca_done/ptt_rf_cca_failed functions */
        if (false == ptt_rf_cca_ext(mode))
        {
            ret = PTT_RET_BUSY;
        }

        if (ret != PTT_RET_SUCCESS)
        {
            ptt_rf_unlock();
        }
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

void ptt_rf_ed_detected(ptt_ed_t result)
{
    if (ptt_rf_is_locked())
    {
        /* have to unlock before passing control out of RF module */
        ptt_evt_id_t evt_id = ptt_rf_unlock();

        ptt_ctrl_rf_ed_detected(evt_id, result);
    }
    else
    {
        PTT_TRACE("%s: called, but rf module does not locked\n", __func__);
        /* we get event although we didn't send a packet, just pass it */
    }
}

void ptt_rf_ed_failed(void)
{
    if (ptt_rf_is_locked())
    {
        /* have to unlock before passing control out of RF module */
        ptt_evt_id_t evt_id = ptt_rf_unlock();

        ptt_ctrl_rf_ed_failed(evt_id);
    }
    else
    {
        PTT_TRACE("%s: called, but rf module does not locked\n", __func__);
        /* we get event although we didn't send a packet, just pass it */
    }
}

ptt_ret_t ptt_rf_ed(ptt_evt_id_t evt_id, uint32_t time_us)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        /* will be unlocked inside ptt_rf_ed_detected/ptt_rf_ed_failed functions */
        if (false == ptt_rf_ed_ext(time_us))
        {
            ret = PTT_RET_BUSY;
        }

        if (ret != PTT_RET_SUCCESS)
        {
            ptt_rf_unlock();
        }
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_rssi_measure_begin(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        if (false == ptt_rf_rssi_measure_begin_ext())
        {
            ret = PTT_RET_BUSY;
        }

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_rssi_last_get(ptt_evt_id_t evt_id, ptt_rssi_t * rssi)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        *rssi = ptt_rf_rssi_last_get_ext();

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_send_packet(ptt_evt_id_t    evt_id,
                             const uint8_t * p_pkt,
                             ptt_pkt_len_t   len)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        if ((NULL == p_pkt) || (0 == len))
        {
            ret = PTT_RET_INVALID_VALUE;
        }

        if (PTT_RET_SUCCESS == ret)
        {
            /* will be unlocked inside ptt_rf_tx_finished/ptt_rf_tx_failed functions */
            if (false == ptt_rf_send_packet_ext(p_pkt, len, m_ptt_rf_ctx.cca_on_tx))
            {
                ret = PTT_RET_BUSY;
            }
        }

        if (ret != PTT_RET_SUCCESS)
        {
            ptt_rf_unlock();
        }
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_set_channel_mask(ptt_evt_id_t evt_id, uint32_t mask)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    uint8_t channel = ptt_rf_convert_channel_mask_to_num(mask);

    if ((channel < PTT_CHANNEL_MIN) || (channel > PTT_CHANNEL_MAX))
    {
        ret = PTT_RET_INVALID_VALUE;
    }
    else
    {
        ret = ptt_rf_set_channel(evt_id, channel);
    }

    return ret;
}

ptt_ret_t ptt_rf_set_channel(ptt_evt_id_t evt_id, uint8_t channel)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        if ((channel < PTT_CHANNEL_MIN) || (channel > PTT_CHANNEL_MAX))
        {
            ret = PTT_RET_INVALID_VALUE;
        }

        if (PTT_RET_SUCCESS == ret)
        {
            m_ptt_rf_ctx.channel = channel;
            ptt_rf_set_channel_ext(m_ptt_rf_ctx.channel);
        }

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_set_short_address(ptt_evt_id_t evt_id, const uint8_t * p_short_addr)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        ptt_rf_set_short_address_ext(p_short_addr);

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_set_extended_address(ptt_evt_id_t evt_id, const uint8_t * p_extended_addr)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        ptt_rf_set_extended_address_ext(p_extended_addr);

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_set_pan_id(ptt_evt_id_t evt_id, const uint8_t * p_pan_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        ptt_rf_set_pan_id_ext(p_pan_id);

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

uint8_t ptt_rf_convert_channel_mask_to_num(uint32_t mask)
{
    uint8_t channel_num = 0;

    for (uint8_t i = PTT_CHANNEL_MIN; i <= PTT_CHANNEL_MAX; i++)
    {
        if (((mask >> i) & 1u) == 1u)
        {
            channel_num = i;
            break;
        }
    }

    return channel_num;
}

uint8_t ptt_rf_get_channel(void)
{
    return m_ptt_rf_ctx.channel;
}

ptt_ret_t ptt_rf_set_power(ptt_evt_id_t evt_id, int8_t power)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        ptt_rf_set_power_ext(power);
        m_ptt_rf_ctx.power = power;

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

int8_t ptt_rf_get_power(void)
{
    m_ptt_rf_ctx.power = ptt_rf_get_power_ext();
    return m_ptt_rf_ctx.power;
}

ptt_ret_t ptt_rf_set_antenna(ptt_evt_id_t evt_id, uint8_t antenna)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        ptt_rf_set_antenna_ext(antenna);
        m_ptt_rf_ctx.antenna = antenna;

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

uint8_t ptt_rf_get_antenna(void)
{
    m_ptt_rf_ctx.antenna = ptt_rf_get_antenna_ext();
    return m_ptt_rf_ctx.antenna;
}

ptt_ret_t ptt_rf_start_statistic(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        m_ptt_rf_ctx.stat         = (ptt_rf_stat_t){0};
        m_ptt_rf_ctx.stat_enabled = true;
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_end_statistic(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (ptt_rf_is_locked_by(evt_id))
    {
        m_ptt_rf_ctx.stat_enabled = false;

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_rf_stat_t ptt_rf_get_stat_report(void)
{
    return m_ptt_rf_ctx.stat;
}

ptt_ret_t ptt_rf_start_modulated_stream(ptt_evt_id_t    evt_id,
                                        const uint8_t * p_pkt,
                                        ptt_pkt_len_t   len)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        if ((NULL == p_pkt) || (0 == len))
        {
            ret = PTT_RET_INVALID_VALUE;
        }

        if (PTT_RET_SUCCESS == ret)
        {
            /* will be unlocked inside ptt_rf_stop_modulated_stream */
            if (false == ptt_rf_modulated_stream_ext(p_pkt, len))
            {
                ret = PTT_RET_BUSY;
            }
        }

        if (ret != PTT_RET_SUCCESS)
        {
            ptt_rf_unlock();
        }
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_stop_modulated_stream(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (ptt_rf_is_locked_by(evt_id))
    {
        if (false == ptt_rf_receive_ext())
        {
            ret = PTT_RET_BUSY;
        }

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_start_continuous_carrier(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (PTT_RET_SUCCESS == ptt_rf_try_lock(evt_id))
    {
        /* will be unlocked inside ptt_rf_stop_continuous_stream */
        if (false == ptt_rf_continuous_carrier_ext())
        {
            ret = PTT_RET_BUSY;

            ptt_rf_unlock();
        }
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_stop_continuous_carrier(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (ptt_rf_is_locked_by(evt_id))
    {
        if (false == ptt_rf_receive_ext())
        {
            ret = PTT_RET_BUSY;
        }

        ptt_rf_unlock();
    }
    else
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

static ptt_ret_t ptt_rf_promiscuous_set(ptt_bool_t value)
{
    ptt_rf_promiscuous_set_ext(value);

    return 0;
}

ptt_ret_t ptt_rf_receive(void)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (ptt_rf_is_locked() ||
        (false == ptt_rf_receive_ext()))
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

ptt_ret_t ptt_rf_sleep(void)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (ptt_rf_is_locked() ||
        (false == ptt_rf_sleep_ext()))
    {
        ret = PTT_RET_BUSY;
    }

    return ret;
}

static inline ptt_evt_id_t ptt_rf_unlock(void)
{
    ptt_evt_id_t evt_id = m_ptt_rf_ctx.evt_lock;

    m_ptt_rf_ctx.evt_lock = PTT_RF_EVT_UNLOCKED;

    return evt_id;
}

static inline ptt_bool_t ptt_rf_is_locked(void)
{
    return (PTT_RF_EVT_UNLOCKED == m_ptt_rf_ctx.evt_lock) ? false : true;
}

static inline ptt_bool_t ptt_rf_is_locked_by(ptt_evt_id_t evt_id)
{
    return (evt_id == m_ptt_rf_ctx.evt_lock) ? true : false;
}

static ptt_ret_t ptt_rf_try_lock(ptt_evt_id_t evt_id)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (ptt_rf_is_locked())
    {
        ret = PTT_RET_BUSY;
    }

    if (PTT_RET_SUCCESS == ret)
    {
        m_ptt_rf_ctx.evt_lock = evt_id;
    }

    return ret;
}

static void ptt_rf_stat_inc(int8_t rssi, uint8_t lqi)
{
    if (m_ptt_rf_ctx.stat_enabled)
    {
        m_ptt_rf_ctx.stat.total_pkts++;
        /* according to B.3.10:
           Assumes no RSSI values measured that are greater than zero dBm */
        if (rssi > 0)
        {
            rssi = 0;
        }
        m_ptt_rf_ctx.stat.total_rssi += (rssi * (-1));
        m_ptt_rf_ctx.stat.total_lqi  += lqi;
    }
}

inline ptt_ltx_payload_t * ptt_rf_get_custom_ltx_payload(void)
{
    return &(m_ptt_rf_ctx.ltx_payload);
}