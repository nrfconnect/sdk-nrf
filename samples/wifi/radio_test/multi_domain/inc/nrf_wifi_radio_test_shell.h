/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief nRF Wi-Fi radio-test mode shell module
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <ctype.h>
#ifndef CONFIG_NRF71_RADIO_TEST
#include <host_rpu_sys_if.h>
#endif
#include <radio_test/fmac_structs.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/rpu_hw_if.h>

enum nrf_wifi_frequency_bands {
	NRF_WIFI_FREQ_BAND_2_4_GHZ = 0,
	NRF_WIFI_FREQ_BAND_5_GHZ,
	NRF_WIFI_FREQ_BAND_6_GHZ,
};

/* RX capture display constants */
#define SAMPLES_PER_LINE 16
#define BYTES_PER_SAMPLE 3
#define BYTES_PER_LINE 48

struct nrf_wifi_ctx_zep_rt {
	struct nrf_wifi_fmac_priv *fmac_priv;
	struct rpu_conf_params conf_params;
};
