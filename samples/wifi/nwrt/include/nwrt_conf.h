/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nwrt_conf.h
 * @brief Driver conf access and FMAC wrappers for nRF71 radio-test.
 *
 * Live configuration and driver context come from the Zephyr nRF71 radio-test
 * driver (@c rt_drv_priv). Shadow handlers copy host-written params here, then call
 * the @c nwrt_wifi_rt_fmac_* wrappers below.
 */

#ifndef NWRT_CONF_H__
#define NWRT_CONF_H__

#include <radio_test/fmac_api.h>
#include <radio_test/fmac_structs.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(CONFIG_NWRT_SWD_ONLY)
#include <zephyr/kernel.h>

struct nrf_wifi_fmac_dev_ctx;

/** Minimal context for SWD-only builds (no nRF71 Wi-Fi driver). */
struct nrf_wifi_ctx_zep
{
    struct rpu_conf_params conf_params;
    bool                   rf_test_run;
    unsigned char          rf_test;
    struct k_mutex         rpu_lock;
};
#else
#include <radio_test/core.h>
#endif /* CONFIG_NWRT_SWD_ONLY */

/**
 * @brief Pointer to live radio-test configuration in the Wi-Fi driver.
 *
 * @return Address of @c rt_drv_priv.drv_ctx.conf_params.
 */
struct rpu_conf_params * nwrt_conf_params(void);

/**
 * @brief Pointer to the FMAC device context, or @c NULL before RPU bring-up.
 *
 * @return @c rpu_ctx from the Zephyr Wi-Fi driver, or @c NULL if not ready.
 */
struct nrf_wifi_fmac_dev_ctx * nwrt_fmac_dev_ctx(void);

/**
 * @brief Pointer to the Zephyr Wi-Fi driver context (includes RPU mutex).
 *
 * @return Address of @c rt_drv_priv.drv_ctx.
 */
#if defined(CONFIG_NWRT_SWD_ONLY)
struct nrf_wifi_ctx_zep * nwrt_wifi_ctx(void);
#else
struct nrf_wifi_rt_drv_ctx * nwrt_wifi_ctx(void);
#endif

/**
 * @brief Reset @c rpu_conf_params to radio-test defaults.
 *
 * Sets @c op_mode to @c RPU_OP_MODE_RADIO_TEST, loads RF/VTF addresses from FMAC,
 * and applies factory-style TX/RX defaults. Preserves country code if already set.
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_conf_init_defaults(void);

/**
 * @brief Initialize RF test mode on the given band and channel.
 *
 * Calls @ref nwrt_conf_init_defaults(), sets @c chan from @p band_idx and
 * @p channel, then invokes @c nrf_wifi_rt_fmac_radio_test_init().
 *
 * @param band_idx Band index: @c 0 = 2.4 GHz, @c 1 = 5 GHz, @c 2 = 6 GHz
 *                 (@c op_band = @c 1 << band_idx).
 * @param channel  Primary channel number valid for the selected band.
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_radio_test_init(uint8_t band_idx, uint32_t channel);

/**
 * @brief Check whether no RF test operation is currently active.
 *
 * @return @c true if TX, RX, and @c rf_test_run are all idle; @c false otherwise.
 */
bool nwrt_conf_idle(void);

/**
 * @brief Start or stop programmed packet TX (FMAC radio-test API).
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param params       Radio-test configuration (@ref nwrt_conf_params()).
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_wifi_rt_fmac_prog_tx(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                               struct rpu_conf_params *       params);

/**
 * @brief Start or stop programmed RX (FMAC radio-test API).
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param params       Radio-test configuration (@ref nwrt_conf_params()).
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_wifi_rt_fmac_prog_rx(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                               struct rpu_conf_params *       params);

/**
 * @brief Run an RX capture into a host-visible buffer (FMAC radio-test API).
 *
 * @param fmac_dev_ctx   FMAC device context.
 * @param rf_test_type   Capture type (@c enum nrf_wifi_rf_test).
 * @param cap_data       Sample buffer (e.g. @ref nwrt_rx_cap_buf).
 * @param num_samples    Number of samples to capture.
 * @param capture_timeout Capture timeout (driver units).
 * @param lna_gain       LNA gain.
 * @param bb_gain        Baseband gain.
 * @param timeout_status Output: driver timeout status byte.
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_wifi_rt_fmac_rf_test_rx_cap(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                      enum nrf_wifi_rf_test          rf_test_type,
                                                      void * cap_data, uint16_t num_samples,
                                                      uint16_t capture_timeout, uint8_t lna_gain,
                                                      uint8_t bb_gain, uint8_t * timeout_status);

/**
 * @brief Enable or disable CW tone TX (FMAC radio-test API).
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param enable       @c 1 to enable tone, @c 0 to disable.
 * @param tone_freq    Tone frequency offset.
 * @param tx_power     TX power for the tone.
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_wifi_rt_fmac_rf_test_tx_tone(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                       uint8_t enable, int8_t tone_freq,
                                                       int8_t tx_power);

/**
 * @brief Read RX/TX statistics for the current RF test session (FMAC API).
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param op_mode      RPU operation mode (typically @c RPU_OP_MODE_RADIO_TEST).
 * @param stats        Output statistics structure.
 *
 * @return @c NRF_WIFI_STATUS_SUCCESS on success.
 */
enum nrf_wifi_status nwrt_wifi_rt_fmac_stats_get(struct nrf_wifi_fmac_dev_ctx * fmac_dev_ctx,
                                                 enum rpu_op_mode               op_mode,
                                                 struct rpu_rt_op_stats *       stats);

#endif /* NWRT_CONF_H__ */
