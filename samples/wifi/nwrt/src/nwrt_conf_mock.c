/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief SWD-only mock of nwrt_conf.c (CONFIG_NWRT_SWD_ONLY).
 *
 * Provides the same nwrt_conf_* / nwrt_wifi_rt_fmac_* API as nwrt_conf.c but
 * without linking the nRF71 Wi-Fi driver (CONFIG_WIFI=n). Shadow handlers in
 * nwrt_shadow.c stay unchanged; shadow writes via nwrt_conf_params() into a
 * local rpu_conf_params, FMAC calls return FAIL, and nwrt_fmac_dev_ctx() is NULL
 * so radio commands surface -19 (NODEV).
 *
 * Build with prj_swd_only.conf (see README-SWD.md).
 */

#include <nwrt_conf.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

static struct nrf_wifi_ctx_zep nwrt_mock_ctx;

static enum nrf_wifi_status nwrt_mock_fail(void)
{
    return NRF_WIFI_STATUS_FAIL;
}

enum nrf_wifi_status nwrt_conf_init_defaults(void)
{
    memset(&nwrt_mock_ctx.conf_params, 0, sizeof(nwrt_mock_ctx.conf_params));
    nwrt_mock_ctx.conf_params.op_mode = RPU_OP_MODE_RADIO_TEST;
    memcpy(nwrt_mock_ctx.conf_params.country_code, "00", NRF_WIFI_COUNTRY_CODE_LEN);
    return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nwrt_radio_test_init(uint8_t band_idx, uint32_t channel)
{
    ARG_UNUSED(band_idx);
    ARG_UNUSED(channel);
    return nwrt_mock_fail();
}

struct rpu_conf_params * nwrt_conf_params(void)
{
    return &nwrt_mock_ctx.conf_params;
}

struct nrf_wifi_fmac_dev_ctx * nwrt_fmac_dev_ctx(void)
{
    return NULL;
}

struct nrf_wifi_ctx_zep * nwrt_wifi_ctx(void)
{
    return &nwrt_mock_ctx;
}

bool nwrt_conf_idle(void)
{
    return true;
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_prog_tx(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                               struct rpu_conf_params *       params)
{
    ARG_UNUSED(fmac_dev_ctx);
    ARG_UNUSED(params);
    return nwrt_mock_fail();
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_prog_rx(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                               struct rpu_conf_params *       params)
{
    ARG_UNUSED(fmac_dev_ctx);
    ARG_UNUSED(params);
    return nwrt_mock_fail();
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_rf_test_rx_cap(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                      enum nrf_wifi_rf_test          rf_test_type,
                                                      void * cap_data, uint16_t num_samples,
                                                      uint16_t capture_timeout, uint8_t lna_gain,
                                                      uint8_t bb_gain, uint8_t * timeout_status)
{
    ARG_UNUSED(fmac_dev_ctx);
    ARG_UNUSED(rf_test_type);
    ARG_UNUSED(cap_data);
    ARG_UNUSED(num_samples);
    ARG_UNUSED(capture_timeout);
    ARG_UNUSED(lna_gain);
    ARG_UNUSED(bb_gain);
    ARG_UNUSED(timeout_status);
    return nwrt_mock_fail();
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_rf_test_tx_tone(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                       uint8_t enable, int8_t tone_freq,
                                                       int8_t tx_power)
{
    ARG_UNUSED(fmac_dev_ctx);
    ARG_UNUSED(enable);
    ARG_UNUSED(tone_freq);
    ARG_UNUSED(tx_power);
    return nwrt_mock_fail();
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_stats_get(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                 enum rpu_op_mode               op_mode,
                                                 struct rpu_rt_op_stats *       stats)
{
    ARG_UNUSED(fmac_dev_ctx);
    ARG_UNUSED(op_mode);
    ARG_UNUSED(stats);
    return nwrt_mock_fail();
}

static int nwrt_mock_init(void)
{
    k_mutex_init(&nwrt_mock_ctx.rpu_lock);
    return 0;
}

SYS_INIT(nwrt_mock_init, POST_KERNEL, 0);
