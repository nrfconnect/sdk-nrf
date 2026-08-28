/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing VTF parameters for the Wi-Fi driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <common/mem_mgmt.h>
#include <common/vtf.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct nrf_wifi_vtf_params_host {
	unsigned int voltage;
	unsigned int temp;
	unsigned int x0_freq;
};

enum nrf_wifi_status nrf_wifi_fmac_config_vtf_params(struct nrf_wifi_fmac_dev_ctx *dev_ctx,
						     unsigned int voltage, unsigned int temp,
						     unsigned int x0,
						     unsigned int *vtf_buffer_start_address)
{
	struct nrf_wifi_vtf_params_host *vtf_buf;

	if (!vtf_buffer_start_address) {
		return NRF_WIFI_STATUS_FAIL;
	}

	vtf_buf = nrf_wifi_mem_alloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*vtf_buf));
	if (!vtf_buf) {
		LOG_ERR("%s: Unable to allocate memory for VTF params", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	vtf_buf->voltage = voltage;
	vtf_buf->temp = temp;
	vtf_buf->x0_freq = x0;
	*vtf_buffer_start_address = (unsigned int)vtf_buf;
	return NRF_WIFI_STATUS_SUCCESS;
}
