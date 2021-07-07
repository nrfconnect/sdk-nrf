/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <device.h>
#include <net/socket.h>
#include <nrf_socket.h>

#include <cJSON.h>
#include <cJSON_os.h>
#include <net/nrf_cloud_cell_pos.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_cell_pos, CONFIG_NRF_CLOUD_LOG_LEVEL);

#include "nrf_cloud_codec.h"
#if defined(CONFIG_NRF_CLOUD_MQTT)
#include "nrf_cloud_transport.h"

#define CELL_POS_JSON_CELL_LOC_KEY_DOREPLY	"doReply"

int nrf_cloud_cell_pos_request(const struct lte_lc_cells_info * const cells_inf,
			       const bool request_loc)
{
	int err = 0;
	cJSON *cell_pos_req_obj = json_create_req_obj(NRF_CLOUD_JSON_APPID_VAL_CELL_POS,
						      NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	cJSON *data_obj = cJSON_AddObjectToObject(cell_pos_req_obj, NRF_CLOUD_JSON_DATA_KEY);

	if (!data_obj) {
		err = -ENOMEM;
		goto cleanup;
	}

	if (cells_inf) {
		/* Add provided cell info to the data obj */
		err = nrf_cloud_format_cell_pos_req_json(cells_inf, 1, data_obj);
	} else {
		/* Fetch modem info and add to the data obj */
		err = nrf_cloud_format_single_cell_pos_req_json(data_obj);
	}

	if (err) {
		goto cleanup;
	}

	/* By default, nRF Cloud will send the location to the device */
	if (!request_loc &&
	    !cJSON_AddNumberToObjectCS(data_obj, CELL_POS_JSON_CELL_LOC_KEY_DOREPLY, 0)) {
		err = -ENOMEM;
		goto cleanup;
	}

	err = json_send_to_cloud(cell_pos_req_obj);

cleanup:
	cJSON_Delete(cell_pos_req_obj);
	return err;
}
#endif /* CONFIG_NRF_CLOUD_MQTT */

int nrf_cloud_cell_pos_process(const char *buf, struct nrf_cloud_cell_pos_result *result)
{
	int err;

	if (!result) {
		return -EINVAL;
	}

	err = nrf_cloud_parse_cell_pos_response(buf, result);
	if (err < 0) {
		LOG_ERR("Error processing cell-based location: %d", err);
	}

	return err;
}
