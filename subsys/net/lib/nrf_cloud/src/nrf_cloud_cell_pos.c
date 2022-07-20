/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/socket.h>
#include <nrf_socket.h>

#include <cJSON.h>
#include <cJSON_os.h>
#include <net/nrf_cloud_cell_pos.h>
#include "nrf_cloud_fsm.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_cell_pos, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* Forward declarations */
int nrf_cloud_cell_pos_request_json_get(const struct lte_lc_cells_info *const cells_inf,
					const bool request_loc, cJSON **req_obj_out);

#include "nrf_cloud_codec.h"
#include "nrf_cloud_transport.h"

#define CELL_POS_JSON_CELL_LOC_KEY_DOREPLY	"doReply"

int nrf_cloud_cell_pos_request(const struct lte_lc_cells_info *const cells_inf,
			       const bool request_loc, nrf_cloud_cell_pos_response_t cb)
{
	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	int err = 0;
	cJSON *cell_pos_req_obj = NULL;

	err = nrf_cloud_cell_pos_request_json_get(cells_inf, request_loc, &cell_pos_req_obj);
	if (!err) {
		if (request_loc) {
			nfsm_set_cell_pos_response_cb(cb);
		}

		err = json_send_to_cloud(cell_pos_req_obj);
	}

	cJSON_Delete(cell_pos_req_obj);
	return err;
}

int nrf_cloud_cell_pos_request_json_get(const struct lte_lc_cells_info *const cells_inf,
					const bool request_loc, cJSON **req_obj_out)
{
	int err = 0;
	*req_obj_out = json_create_req_obj(NRF_CLOUD_JSON_APPID_VAL_CELL_POS,
					   NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	cJSON *data_obj = cJSON_AddObjectToObject(*req_obj_out, NRF_CLOUD_JSON_DATA_KEY);

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

	return 0;

cleanup:
	cJSON_Delete(*req_obj_out);
	return err;
}

int nrf_cloud_cell_pos_process(const char *buf, struct nrf_cloud_cell_pos_result *result)
{
	int err;

	if (!result) {
		return -EINVAL;
	}

	err = nrf_cloud_parse_cell_pos_response(buf, result);
	if (err == -EFAULT) {
		LOG_ERR("nRF Cloud cell-based location error: %d",
			result->err);
	} else if (err < 0) {
		LOG_ERR("Error processing cell-based location: %d", err);
	}

	return err;
}
