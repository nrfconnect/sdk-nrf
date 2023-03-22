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

#include "nrf_cloud_fsm.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"

LOG_MODULE_REGISTER(nrf_cloud_location, CONFIG_NRF_CLOUD_LOG_LEVEL);

int nrf_cloud_location_request(const struct lte_lc_cells_info *const cells_inf,
			       const struct wifi_scan_info *const wifi_inf,
			       const bool request_loc, nrf_cloud_location_response_t cb)
{
	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	int err = 0;
	cJSON *location_req_obj = cJSON_CreateObject();

	err = nrf_cloud_location_request_msg_json_encode(cells_inf, wifi_inf,
							 request_loc, location_req_obj);
	if (!err) {
		if (request_loc) {
			nfsm_set_location_response_cb(cb);
		}

		err = json_send_to_cloud(location_req_obj);
	}

	cJSON_Delete(location_req_obj);
	return err;
}

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
