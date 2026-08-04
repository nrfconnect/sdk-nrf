/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <common/fw_if/nrf71_wifi_rf.h>
#include <nwrt_conf.h>
#include <vtf_monitoring/vtf_monitoring.h>
#include <string.h>

enum nrf_wifi_status nwrt_conf_init_defaults(void)
{
    enum nrf_wifi_status           status                                  = NRF_WIFI_STATUS_FAIL;
    struct rpu_conf_params *       conf                                    = nwrt_conf_params();
    struct nrf_wifi_fmac_dev_ctx * dev                                     = nwrt_fmac_dev_ctx();
    uint8_t                        country_code[NRF_WIFI_COUNTRY_CODE_LEN] = {0};

    if (dev == NULL)
    {
        return NRF_WIFI_STATUS_FAIL;
    }

    if (strlen(conf->country_code))
    {
        memcpy(country_code, conf->country_code, NRF_WIFI_COUNTRY_CODE_LEN);
    }

    memset(conf, 0, sizeof(*conf));

    conf->op_mode = RPU_OP_MODE_RADIO_TEST;

    {
        uint32_t rf_params_tmp[NUM_WIFI_PARAMS];

        status = nrf_wifi_fmac_config_rf_params(dev, (unsigned int *)rf_params_tmp);
        if (status != NRF_WIFI_STATUS_SUCCESS)
        {
            goto out;
        }

        memcpy(conf->rf_params_addr, rf_params_tmp, sizeof(conf->rf_params_addr));

        /* Point the firmware at the live VTF snapshot region maintained by the
         * vtf_monitoring subsystem. The battery-voltage entry is the first of
         * the three consecutive words (voltage, temperature, frequency) the
         * firmware reads; the preceding initialization word is not included.
         */
        conf->vtf_buffer_addr = (unsigned int)&vtf_snapshots[VTF_CH_BATTERY_VOLTAGE];
    }

    conf->tx_pkt_nss             = 1;
    conf->tx_pkt_gap_us          = 0;
    conf->tx_power               = MAX_TX_PWR_SYS_TEST;
    conf->chan.primary_num       = 1;
    conf->tx_mode                = 1;
    conf->tx_pkt_num             = -1;
    conf->tx_pkt_len             = 1400;
    conf->tx_pkt_preamble        = 0;
    conf->tx_pkt_rate            = 6;
    conf->he_ltf                 = 2;
    conf->he_gi                  = 2;
    conf->aux_adc_input_chain_id = 1;
    conf->ru_tone                = 26;
    conf->ru_index               = 1;
    conf->tx_pkt_cw              = 15;
    conf->phy_calib              = NRF_WIFI_DEF_PHY_CALIB;

    if (strlen(country_code))
    {
        memcpy(conf->country_code, country_code, NRF_WIFI_COUNTRY_CODE_LEN);
    }
    else
    {
        memcpy(conf->country_code, "00", NRF_WIFI_COUNTRY_CODE_LEN);
    }

out:
    return status;
}

enum nrf_wifi_status nwrt_radio_test_init(uint8_t band_idx, uint32_t channel)
{
    enum nrf_wifi_status           status;
    struct rpu_conf_params *       conf = nwrt_conf_params();
    struct nrf_wifi_fmac_dev_ctx * dev  = nwrt_fmac_dev_ctx();

    if (dev == NULL)
    {
        return NRF_WIFI_STATUS_FAIL;
    }

    status = nwrt_conf_init_defaults();
    if (status != NRF_WIFI_STATUS_SUCCESS)
    {
        return status;
    }

    conf->chan.op_band     = (unsigned char)(1U << band_idx);
    conf->chan.primary_num = (unsigned int)channel;

    return nrf_wifi_rt_fmac_radio_test_init(dev, conf);
}

struct rpu_conf_params * nwrt_conf_params(void)
{
    return &rt_drv_priv.drv_ctx.conf_params;
}

struct nrf_wifi_fmac_dev_ctx * nwrt_fmac_dev_ctx(void)
{
    return rt_drv_priv.drv_ctx.rpu_ctx;
}

struct nrf_wifi_rt_drv_ctx * nwrt_wifi_ctx(void)
{
    return &rt_drv_priv.drv_ctx;
}

bool nwrt_conf_idle(void)
{
    struct nrf_wifi_rt_drv_ctx * ctx = nwrt_wifi_ctx();

    if (ctx->conf_params.rx || ctx->conf_params.tx || ctx->rf_test_run)
    {
        return false;
    }

    return true;
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_prog_tx(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                               struct rpu_conf_params *       params)
{
    return nrf_wifi_rt_fmac_prog_tx(fmac_dev_ctx, params);
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_prog_rx(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                               struct rpu_conf_params *       params)
{
    return nrf_wifi_rt_fmac_prog_rx(fmac_dev_ctx, params);
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_rf_test_rx_cap(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                      enum nrf_wifi_rf_test          rf_test_type,
                                                      void * cap_data, uint16_t num_samples,
                                                      uint16_t capture_timeout, uint8_t lna_gain,
                                                      uint8_t bb_gain, uint8_t * timeout_status)
{
    struct rpu_conf_params * conf = nwrt_conf_params();

    return nrf_wifi_rt_fmac_rf_test_rx_cap(fmac_dev_ctx, rf_test_type, cap_data, num_samples,
                                           capture_timeout, lna_gain, bb_gain,
                                           conf->ed_thresh_ofdm, conf->ed_thresh_dsss,
                                           timeout_status);
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_rf_test_tx_tone(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                       uint8_t enable, int8_t tone_freq,
                                                       int8_t tx_power)
{
    struct rpu_conf_params * conf = nwrt_conf_params();

    return nrf_wifi_rt_fmac_rf_test_tx_tone(fmac_dev_ctx, enable, tone_freq, tx_power,
                                            conf->tx_tone_type, conf->tx_tone_dc_offset_i,
                                            conf->tx_tone_dc_offset_q);
}

enum nrf_wifi_status nwrt_wifi_rt_fmac_stats_get(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                 enum rpu_op_mode               op_mode,
                                                 struct rpu_rt_op_stats *       stats)
{
    return nrf_wifi_rt_fmac_stats_get(fmac_dev_ctx, op_mode, stats);
}
