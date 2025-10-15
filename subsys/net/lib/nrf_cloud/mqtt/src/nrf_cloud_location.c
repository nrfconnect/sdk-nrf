/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/socket.h>
#include <net/nrf_cloud_location.h>
#include <net/nrf_cloud_codec.h>

#include "nrf_cloud_fsm.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_transport.h"
#include <date_time.h>

int nrf_cloud_location_request(const struct lte_lc_cells_info *const cells_inf,
			       const struct wifi_scan_info *const wifi_inf,
			       const struct nrf_cloud_location_config *const config,
			       nrf_cloud_location_response_t cb)
{
	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	int err = 0;
	int64_t timestamp = 0;

	date_time_now(&timestamp);

	NRF_CLOUD_OBJ_JSON_DEFINE(location_req_obj);

	err = nrf_cloud_obj_location_request_create_timestamped(
		&location_req_obj, cells_inf, wifi_inf, config, timestamp);
	if (!err) {
		if (!config || (config->do_reply)) {
			nfsm_set_location_response_cb(cb);
		}

		err = json_send_to_cloud(location_req_obj.json);
	}

	(void)nrf_cloud_obj_free(&location_req_obj);
	return err;
}
