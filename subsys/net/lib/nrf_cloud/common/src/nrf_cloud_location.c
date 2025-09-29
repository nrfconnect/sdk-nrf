/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <net/nrf_cloud_location.h>
#include <net/nrf_cloud_codec.h>

#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"

LOG_MODULE_REGISTER(nrf_cloud_location, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST)
BUILD_ASSERT(CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST_BUFFER_SIZE >= NRF_CLOUD_ANCHOR_LIST_BUF_MIN_SZ,
	     "CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST_BUFFER_SIZE is not large enough");
#endif

int nrf_cloud_location_scell_data_get(struct lte_lc_cell *const cell_inf)
{
	return nrf_cloud_get_single_cell_modem_info(cell_inf);
}

int nrf_cloud_location_process(const char *buf, struct nrf_cloud_location_result *result)
{
	int err;

	if (!result) {
		return -EINVAL;
	}

	err = nrf_cloud_location_response_decode(buf, result);
	if (err == -EFAULT) {
		LOG_ERR("nRF Cloud location error: %d", result->err);
	} else if (err < 0) {
		LOG_ERR("Error processing location result: %d", err);
	}

	return err;
}
