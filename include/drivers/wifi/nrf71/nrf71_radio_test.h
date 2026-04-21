/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf71_radio_test.h
 * @brief Public API for nRF71 Wi-Fi radio-test mode.
 *
 * Exposes only the types and helpers required by the radio-test sample.
 */

#ifndef NRF71_RADIO_TEST_H_
#define NRF71_RADIO_TEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#include <radio_test/fmac_api.h>
#include <nrf71_wifi_ctrl.h>
#include <coex.h>

#define NRF71_NUM_RF_PARAM_ADDRS 22

struct nrf_wifi_ctx_zep {
	void *drv_priv_zep;
	void *rpu_ctx;
	struct rpu_conf_params conf_params;
	bool rf_test_run;
	unsigned char rf_test;
	unsigned char *extended_capa;
	unsigned char *extended_capa_mask;
	unsigned int extended_capa_len;
	struct k_mutex rpu_lock;
#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
	bool rpu_recovery_in_progress;
	unsigned long last_rpu_recovery_time_ms;
	unsigned int rpu_recovery_retries;
	int rpu_recovery_success;
	int rpu_recovery_failure;
	int wdt_irq_received;
	int wdt_irq_ignored;
#endif
	unsigned int phy_rf_params_addr[NRF71_NUM_RF_PARAM_ADDRS];
	unsigned int vtf_buffer_start_address;
#if defined(CONFIG_NRF71_RAW_DATA_TX) || defined(CONFIG_NRF71_RAW_DATA_RX)
	struct k_sem channel_set_done_sem;
	volatile int channel_set_status;
#endif
};

struct nrf_wifi_drv_priv_zep {
	struct nrf_wifi_fmac_priv *fmac_priv;
	struct nrf_wifi_ctx_zep rpu_ctx_zep;
};

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

enum nrf_wifi_status nrf_wifi_fmac_config_rf_params(void *dev_ctx,
						    unsigned int *rf_params_addr);

enum nrf_wifi_status nrf_wifi_fmac_config_vtf_params(struct nrf_wifi_fmac_dev_ctx *dev_ctx,
						     unsigned int voltage,
						     unsigned int temp,
						     unsigned int x0,
						     unsigned int *vtf_buffer_start_address);

#endif /* NRF71_RADIO_TEST_H_ */
