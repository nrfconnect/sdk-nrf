/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing RF parameters for the Wi-Fi driver.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <common/mem_mgmt.h>
#include <common/fw_if/nrf71_wifi_rf.h>
#include <common/rf_params.h>
#include <common/util.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);


static struct rf_hex_param rf_params[NUM_WIFI_PARAMS] = {
	{NRF_WIFI_PARAMS1, NULL, 0},  {NRF_WIFI_PARAMS2, NULL, 0},  {NRF_WIFI_PARAMS3, NULL, 0},
	{NRF_WIFI_PARAMS4, NULL, 0},  {NRF_WIFI_PARAMS5, NULL, 0},  {NRF_WIFI_PARAMS6, NULL, 0},
	{NRF_WIFI_PARAMS7, NULL, 0},  {NRF_WIFI_PARAMS8, NULL, 0},  {NRF_WIFI_PARAMS9, NULL, 0},
	{NRF_WIFI_PARAMS10, NULL, 0}, {NRF_WIFI_PARAMS11, NULL, 0}, {NRF_WIFI_PARAMS12, NULL, 0},
	{NRF_WIFI_PARAMS13, NULL, 0}, {NRF_WIFI_PARAMS14, NULL, 0}, {NRF_WIFI_PARAMS15, NULL, 0},
	{NRF_WIFI_PARAMS16, NULL, 0}, {NRF_WIFI_PARAMS17, NULL, 0}, {NRF_WIFI_PARAMS18, NULL, 0},
	{NRF_WIFI_PARAMS19, NULL, 0}, {NRF_WIFI_PARAMS20, NULL, 0}, {NRF_WIFI_PARAMS21, NULL, 0},
	{NRF_WIFI_PARAMS22, NULL, 0},
};

enum nrf_wifi_status nrf_wifi_fmac_config_rf_params(void *dev_ctx, unsigned int *rf_params_addr)
{
	int index;
	int cleanup_idx;
	size_t str_len;
	int ret;

	for (index = 0; index < NUM_WIFI_PARAMS; index++) {
		if (!rf_params[index].hex_str) {
			continue;
		}
		str_len = strlen(rf_params[index].hex_str);
		rf_params[index].bytes = nrf_wifi_mem_alloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, str_len);
		if (!rf_params[index].bytes) {
			LOG_ERR("%s: Unable to allocate %zu bytes", __func__, str_len);
			goto cleanup;
		}

		ret = nrf_wifi_utils_hex_str_to_val(rf_params[index].bytes, (unsigned int)str_len,
						    (unsigned char *)rf_params[index].hex_str);
		if (ret < 0) {
			LOG_ERR("%s: hex_str_to_val failed", __func__);
			nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, rf_params[index].bytes);
			rf_params[index].bytes = NULL;
			goto cleanup;
		}

		rf_params[index].bytes_len = ret;
		rf_params_addr[index] = (unsigned int)rf_params[index].bytes;
	}
	return NRF_WIFI_STATUS_SUCCESS;

cleanup:
	for (cleanup_idx = 0; cleanup_idx < index; cleanup_idx++) {
		if (rf_params[cleanup_idx].bytes) {
			nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
					  rf_params[cleanup_idx].bytes);
			rf_params[cleanup_idx].bytes = NULL;
		}
	}
	return NRF_WIFI_STATUS_FAIL;
}
