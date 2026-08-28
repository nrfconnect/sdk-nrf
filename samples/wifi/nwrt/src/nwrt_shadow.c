/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nwrt_conf.h>
#include <nwrt_shadow.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/sys/barrier.h>

struct nwrt_shadow_common nwrt_shadow_common Z_GENERIC_SECTION(
    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(nwrt_shadow_common_sram)));

struct nwrt_shadow_tx nwrt_shadow_tx Z_GENERIC_SECTION(
    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(nwrt_shadow_tx_sram)));

struct nwrt_shadow_rx nwrt_shadow_rx Z_GENERIC_SECTION(
    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(nwrt_shadow_rx_sram)));

uint8_t nwrt_rx_cap_buf[NWRT_RX_CAP_SIZE] Z_GENERIC_SECTION(
    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(nwrt_rx_cap_sram)));

struct rpu_rt_op_stats nwrt_shadow_stats;

static uint32_t nwrt_shadow_last_common_seq;
static uint32_t nwrt_shadow_last_tx_seq;
static uint32_t nwrt_shadow_last_rx_seq;

static uint32_t nwrt_shadow_active_common_submit;
static uint32_t nwrt_shadow_active_tx_submit;
static uint32_t nwrt_shadow_active_rx_submit;

static void nwrt_shadow_fill_defaults(void)
{
    nwrt_shadow_common.submit   = 0U;
    nwrt_shadow_common.result   = 0;
    nwrt_shadow_common.ack      = 0U;
    nwrt_shadow_common.band_idx = 0U;
    nwrt_shadow_common.channel  = 1U;

    nwrt_shadow_tx.submit           = 0U;
    nwrt_shadow_tx.result           = 0;
    nwrt_shadow_tx.ack              = 0U;
    nwrt_shadow_tx.tx_power         = 30U;
    nwrt_shadow_tx.tx_tone_freq     = 0;
    nwrt_shadow_tx.tx_pkt_len       = 1400U;
    nwrt_shadow_tx.tx_pkt_num       = -1;
    nwrt_shadow_tx.tx_pkt_mcs       = 0;
    nwrt_shadow_tx.tx_pkt_rate      = 6;
    nwrt_shadow_tx.tx_pkt_tput_mode = 0U;
    nwrt_shadow_tx.enable_tx        = 0U;
    nwrt_shadow_tx.tx_tone          = 0U;

    nwrt_shadow_rx.submit             = 0U;
    nwrt_shadow_rx.result             = 0;
    nwrt_shadow_rx.ack                = 0U;
    nwrt_shadow_rx.cap_timeout_status = 0U;
    nwrt_shadow_rx.lna_gain           = 0U;
    nwrt_shadow_rx.bb_gain            = 0U;
    nwrt_shadow_rx.capture_length     = 0U;
    nwrt_shadow_rx.capture_timeout    = 0U;
    nwrt_shadow_rx.enable_rx            = 0U;
    nwrt_shadow_rx.rx_cap_type        = NWRT_RX_CAP_TYPE_ADC;
}

void nwrt_shadow_init(void)
{
    __ASSERT((uintptr_t)&nwrt_shadow_common == NWRT_SHADOW_COMMON_BASE,
             "nwrt_shadow_common not at NWRT_SHADOW_COMMON_BASE");
    __ASSERT((uintptr_t)&nwrt_shadow_tx == NWRT_SHADOW_TX_BASE,
             "nwrt_shadow_tx not at NWRT_SHADOW_TX_BASE");
    __ASSERT((uintptr_t)&nwrt_shadow_rx == NWRT_SHADOW_RX_BASE,
             "nwrt_shadow_rx not at NWRT_SHADOW_RX_BASE");

    memset(&nwrt_shadow_common, 0, sizeof(nwrt_shadow_common));
    memset(&nwrt_shadow_tx, 0, sizeof(nwrt_shadow_tx));
    memset(&nwrt_shadow_rx, 0, sizeof(nwrt_shadow_rx));
    nwrt_shadow_fill_defaults();
    nwrt_shadow_common.magic   = NWRT_SHADOW_MAGIC;
    nwrt_shadow_common.version = NWRT_SHADOW_VERSION;
    nwrt_shadow_last_common_seq = 0U;
    nwrt_shadow_last_tx_seq     = 0U;
    nwrt_shadow_last_rx_seq     = 0U;
    nwrt_shadow_active_common_submit = 0U;
    nwrt_shadow_active_tx_submit     = 0U;
    nwrt_shadow_active_rx_submit     = 0U;
}

static int nwrt_shadow_boot_init(void)
{
    nwrt_shadow_init();
    return 0;
}

SYS_INIT(nwrt_shadow_boot_init, PRE_KERNEL_2, 0);

static int32_t shadow_lock(void)
{
    if (nwrt_fmac_dev_ctx() == NULL)
    {
        return NWRT_SHADOW_ERR_NODEV;
    }

    return (int32_t)k_mutex_lock(&nwrt_wifi_ctx()->rpu_lock, K_FOREVER);
}

static void shadow_unlock(void)
{
    k_mutex_unlock(&nwrt_wifi_ctx()->rpu_lock);
}

static bool shadow_layout_valid(void)
{
    return nwrt_shadow_common.magic == NWRT_SHADOW_MAGIC &&
           nwrt_shadow_common.version == NWRT_SHADOW_VERSION;
}

static void shadow_apply_tx(struct rpu_conf_params * conf)
{
    const struct nwrt_shadow_tx * tx = &nwrt_shadow_tx;

    conf->tx_power         = (unsigned int)tx->tx_power;
    conf->tx_tone_freq     = (signed char)tx->tx_tone_freq;
    conf->tx_pkt_len       = (unsigned short)tx->tx_pkt_len;
    conf->tx_pkt_num       = (signed int)tx->tx_pkt_num;
    conf->tx_pkt_mcs       = (signed char)tx->tx_pkt_mcs;
    conf->tx_pkt_rate      = (signed char)tx->tx_pkt_rate;
    conf->tx_pkt_tput_mode = (unsigned char)tx->tx_pkt_tput_mode;
}

static void shadow_apply_rx(struct rpu_conf_params * conf)
{
    const struct nwrt_shadow_rx * rx = &nwrt_shadow_rx;

    conf->lna_gain        = (unsigned char)rx->lna_gain;
    conf->bb_gain         = (unsigned char)rx->bb_gain;
    conf->capture_length  = (unsigned short int)rx->capture_length;
    conf->capture_timeout = (unsigned short int)rx->capture_timeout;
}

static int32_t shadow_do_prog_tx(uint32_t enable)
{
    enum nrf_wifi_status     status;
    struct rpu_conf_params * conf = nwrt_conf_params();

    if (enable > 1U)
    {
        return NWRT_SHADOW_ERR_INVAL;
    }

    if (enable && !nwrt_conf_idle())
    {
        return NWRT_SHADOW_ERR_BUSY;
    }

    conf->tx = (unsigned char)enable;

    status = nwrt_wifi_rt_fmac_prog_tx(nwrt_fmac_dev_ctx(), conf);

    return (status == NRF_WIFI_STATUS_SUCCESS) ? 0 : NWRT_SHADOW_ERR_IO;
}

static int32_t shadow_do_prog_rx(uint32_t enable)
{
    enum nrf_wifi_status     status;
    struct rpu_conf_params * conf = nwrt_conf_params();

    if (enable > 1U)
    {
        return NWRT_SHADOW_ERR_INVAL;
    }

    if (enable && !nwrt_conf_idle())
    {
        return NWRT_SHADOW_ERR_BUSY;
    }

    conf->rx = (unsigned char)enable;

    status = nwrt_wifi_rt_fmac_prog_rx(nwrt_fmac_dev_ctx(), conf);

    return (status == NRF_WIFI_STATUS_SUCCESS) ? 0 : NWRT_SHADOW_ERR_IO;
}

static int32_t shadow_do_rx_cap(void)
{
    enum nrf_wifi_status      status;
#if defined(CONFIG_NWRT_SWD_ONLY)
    struct nrf_wifi_ctx_zep * ctx = nwrt_wifi_ctx();
#else
    struct nrf_wifi_rt_drv_ctx * ctx = nwrt_wifi_ctx();
#endif
    uint32_t                  cap_type    = nwrt_shadow_rx.rx_cap_type;
    uint32_t                  num_samples = nwrt_shadow_rx.capture_length;
    uint32_t                  cap_bytes;
    uint8_t                   timeout_status;

    if (cap_type > NWRT_RX_CAP_TYPE_DYN_PKT)
    {
        return NWRT_SHADOW_ERR_INVAL;
    }

    if (num_samples == 0U || num_samples > NWRT_RX_CAP_MAX_SAMPLES)
    {
        return NWRT_SHADOW_ERR_INVAL;
    }

    if (cap_type == NWRT_RX_CAP_TYPE_DYN_PKT && nwrt_shadow_rx.capture_timeout == 0U)
    {
        return NWRT_SHADOW_ERR_INVAL;
    }

    if (!nwrt_conf_idle())
    {
        return NWRT_SHADOW_ERR_BUSY;
    }

    cap_bytes = num_samples * 3U;
    memset(nwrt_rx_cap_buf, 0, cap_bytes);

    ctx->rf_test_run = true;
    ctx->rf_test     = (unsigned char)cap_type;

    status = nwrt_wifi_rt_fmac_rf_test_rx_cap(
        nwrt_fmac_dev_ctx(), (enum nrf_wifi_rf_test)cap_type, nwrt_rx_cap_buf,
        (uint16_t)num_samples, (uint16_t)nwrt_shadow_rx.capture_timeout,
        (uint8_t)nwrt_shadow_rx.lna_gain, (uint8_t)nwrt_shadow_rx.bb_gain, &timeout_status);

    ctx->rf_test_run = false;
    ctx->rf_test     = NRF_WIFI_RF_TEST_MAX;

    nwrt_shadow_rx.cap_timeout_status = timeout_status;

    if (status != NRF_WIFI_STATUS_SUCCESS)
    {
        return NWRT_SHADOW_ERR_IO;
    }

    return 0;
}

static int32_t shadow_do_tx_tone(uint32_t enable)
{
    enum nrf_wifi_status      status;
#if defined(CONFIG_NWRT_SWD_ONLY)
    struct nrf_wifi_ctx_zep * ctx = nwrt_wifi_ctx();
#else
    struct nrf_wifi_rt_drv_ctx * ctx = nwrt_wifi_ctx();
#endif
    struct rpu_conf_params *  conf      = nwrt_conf_params();
    int8_t                    tone_freq = conf->tx_tone_freq;
    int8_t                    tx_pwr    = (int8_t)conf->tx_power;

    if (enable > 1U)
    {
        return NWRT_SHADOW_ERR_INVAL;
    }

    if (enable && !nwrt_conf_idle())
    return NWRT_SHADOW_ERR_BUSY;
    {
    }

    status =
        nwrt_wifi_rt_fmac_rf_test_tx_tone(nwrt_fmac_dev_ctx(), (uint8_t)enable, tone_freq, tx_pwr);
    if (status != NRF_WIFI_STATUS_SUCCESS)
    {
        return NWRT_SHADOW_ERR_IO;
    }

    if (enable)
    {
        ctx->rf_test_run = true;
        ctx->rf_test     = NRF_WIFI_RF_TEST_TX_TONE;
    }
    else
    {
        ctx->rf_test_run = false;
        ctx->rf_test     = NRF_WIFI_RF_TEST_MAX;
    }

    return 0;
}

static bool shadow_command_new(uint32_t submit, uint32_t active_submit)
{
    if (NWRT_SHADOW_SUBMIT_PENDING(submit) == 0U)
    {
        return false;
    }

    if (submit == active_submit)
    {
        return false;
    }

    return true;
}

static uint32_t shadow_resolve_seq(uint32_t submit, uint32_t * last_seq)
{
    uint32_t seq = NWRT_SHADOW_SUBMIT_SEQ(submit);

    /* seq=0: ATE-style submit (pending only). seq!=0: lab debug (poll ack). */
    if (seq == 0U)
    {
        seq = (*last_seq + 1U) & 0xFFFFU;
        if (seq == 0U)
        {
            seq = 1U;
        }
        return seq;
    }

    return seq;
}

static void shadow_mark_running(int32_t * result)
{
    barrier_dmem_fence_full();
    *result = NWRT_SHADOW_RESULT_RUNNING;
    barrier_dmem_fence_full();
}

static void shadow_complete_slot(uint32_t * submit_word, int32_t * result_word, uint32_t * ack_word,
                                 uint32_t * last_seq, uint32_t * active_submit, uint32_t seq,
                                 int32_t result)
{
    barrier_dmem_fence_full();
    *result_word = result;
    barrier_dmem_fence_full();
    *ack_word       = seq;
    *last_seq       = seq;
    *submit_word    = 0U;
    *active_submit  = 0U;
}

static void shadow_process_slot(uint32_t * submit_word, int32_t * result_word, uint32_t * ack_word,
                                uint32_t * last_seq, uint32_t * active_submit,
                                int32_t (*dispatch)(uint32_t pending), bool check_layout)
{
    uint32_t submit  = *submit_word;
    uint32_t pending = NWRT_SHADOW_SUBMIT_PENDING(submit);
    uint32_t seq;
    int32_t  lock_rc;
    int32_t  result = NWRT_SHADOW_ERR_INVAL;

    if (!shadow_command_new(submit, *active_submit))
    {
        return;
    }

    seq = shadow_resolve_seq(submit, last_seq);

    if (check_layout && !shadow_layout_valid())
    {
        goto complete;
    }

    *active_submit = submit;
    shadow_mark_running(result_word);

    lock_rc = shadow_lock();
    if (lock_rc != 0)
    {
        result = lock_rc;
        goto complete;
    }

    result = dispatch(pending);

    shadow_unlock();

complete:
    shadow_complete_slot(submit_word, result_word, ack_word, last_seq, active_submit, seq, result);
}

static int32_t shadow_dispatch_common(uint32_t pending)
{
    enum nrf_wifi_status status;

    if (pending & NWRT_COMMON_PENDING_CONF_INIT)
    {
        if (!nwrt_conf_idle())
        {
            return NWRT_SHADOW_ERR_BUSY;
        }

        status = nwrt_conf_init_defaults();
        if (status != NRF_WIFI_STATUS_SUCCESS)
        {
            return NWRT_SHADOW_ERR_IO;
        }
    }

    if (pending & NWRT_COMMON_PENDING_RADIO_INIT)
    {
        if (!nwrt_conf_idle())
        {
            return NWRT_SHADOW_ERR_BUSY;
        }

        status = nwrt_radio_test_init((uint8_t)nwrt_shadow_common.band_idx,
                                      nwrt_shadow_common.channel);
        if (status != NRF_WIFI_STATUS_SUCCESS)
        {
            return NWRT_SHADOW_ERR_IO;
        }
    }

    return 0;
}

static int32_t shadow_dispatch_tx(uint32_t pending)
{
    int32_t                  result = 0;
    struct rpu_conf_params * conf   = nwrt_conf_params();

    if (pending & NWRT_TX_PENDING_APPLY)
    {
        shadow_apply_tx(conf);
    }

    if (pending & NWRT_TX_PENDING_PROG)
    {
        result = shadow_do_prog_tx(nwrt_shadow_tx.enable_tx);
        if (result != 0)
        {
            return result;
        }
    }

    if (pending & NWRT_TX_PENDING_TONE)
    {
        result = shadow_do_tx_tone(nwrt_shadow_tx.tx_tone);
        if (result != 0)
        {
            return result;
        }
    }

    return 0;
}

static int32_t shadow_dispatch_rx(uint32_t pending)
{
    enum nrf_wifi_status status;
    int32_t              result = 0;

    if (pending & NWRT_RX_PENDING_APPLY)
    {
        shadow_apply_rx(nwrt_conf_params());
    }

    if (pending & NWRT_RX_PENDING_PROG)
    {
        result = shadow_do_prog_rx(nwrt_shadow_rx.enable_rx);
        if (result != 0)
        {
            return result;
        }
    }

    if (pending & NWRT_RX_PENDING_STATS)
    {
        status = nwrt_wifi_rt_fmac_stats_get(nwrt_fmac_dev_ctx(), RPU_OP_MODE_RADIO_TEST,
                                             &nwrt_shadow_stats);
        if (status != NRF_WIFI_STATUS_SUCCESS)
        {
            return NWRT_SHADOW_ERR_IO;
        }
    }

    if (pending & NWRT_RX_PENDING_RX_CAP)
    {
        result = shadow_do_rx_cap();
        if (result != 0)
        {
            return result;
        }
    }

    return 0;
}

void nwrt_shadow_poll(void)
{
    shadow_process_slot(&nwrt_shadow_common.submit, &nwrt_shadow_common.result,
                        &nwrt_shadow_common.ack, &nwrt_shadow_last_common_seq,
                        &nwrt_shadow_active_common_submit, shadow_dispatch_common, true);

    shadow_process_slot(&nwrt_shadow_tx.submit, &nwrt_shadow_tx.result, &nwrt_shadow_tx.ack,
                        &nwrt_shadow_last_tx_seq, &nwrt_shadow_active_tx_submit,
                        shadow_dispatch_tx, true);

    shadow_process_slot(&nwrt_shadow_rx.submit, &nwrt_shadow_rx.result, &nwrt_shadow_rx.ack,
                        &nwrt_shadow_last_rx_seq, &nwrt_shadow_active_rx_submit,
                        shadow_dispatch_rx, true);
}
