/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <modem/location.h>
#include <net/lwm2m_client_utils_location.h>

#include "cloud_lwm2m.h"
#include "mosh_print.h"
#include "mosh_defines.h"
#include "location_srv_ext.h"

/* Location Assistance resource IDs */
#define GROUND_FIX_SEND_LOCATION_BACK	0
#define GROUND_FIX_RESULT_CODE		1
#define GROUND_FIX_LATITUDE		2
#define GROUND_FIX_LONGITUDE		3
#define GROUND_FIX_ACCURACY		4

#if defined(CONFIG_NRF_CLOUD_AGNSS)
void location_srv_ext_agnss_handle(const struct nrf_modem_gnss_agnss_data_frame *agnss_req)
{
	int err;

	if (!cloud_lwm2m_is_connected()) {
		mosh_error("LwM2M not connected, can't request A-GNSS data");
		return;
	}

	location_assistance_agnss_set_mask(agnss_req);

	while ((err = location_assistance_agnss_request_send(cloud_lwm2m_client_ctx_get())) ==
	       -EAGAIN) {
		/* LwM2M client utils library is currently handling a P-GPS data request, need to
		 * wait until it has been completed.
		 */
		k_sleep(K_SECONDS(1));
	}
	if (err) {
		mosh_error("Failed to request A-GNSS data, err: %d", err);
	}
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
void location_srv_ext_pgps_handle(const struct gps_pgps_request *pgps_req)
{
	int err = -1;

	if (!cloud_lwm2m_is_connected()) {
		mosh_error("LwM2M not connected, can't request P-GPS data");
		return;
	}

	err = location_assist_pgps_set_prediction_count(pgps_req->prediction_count);
	err |= location_assist_pgps_set_prediction_interval(pgps_req->prediction_period_min);
	location_assist_pgps_set_start_gps_day(pgps_req->gps_day);
	err |= location_assist_pgps_set_start_time(pgps_req->gps_time_of_day);
	if (err) {
		mosh_error("Failed to set P-GPS request parameters");
		return;
	}

	while ((err = location_assistance_pgps_request_send(cloud_lwm2m_client_ctx_get())) ==
	       -EAGAIN) {
		/* LwM2M client utils library is currently handling an A-GNSS data request, need to
		 * wait until it has been completed.
		 */
		k_sleep(K_SECONDS(1));
	}
	if (err) {
		mosh_error("Failed to request P-GPS data, err: %d", err);
	}
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
static int location_ctrl_lwm2m_cell_result_cb(uint16_t obj_inst_id,
					      uint16_t res_id, uint16_t res_inst_id,
					      uint8_t *data, uint16_t data_len,
					      bool last_block, size_t total_size)
{
	int err;
	int32_t result;
	struct location_data location = {0};
	double temp_accuracy;

	if (data_len != sizeof(int32_t)) {
		mosh_error("Unexpected data length in callback");
		err = -1;
		goto exit;
	}

	result = (int32_t)*data;
	switch (result) {
	case LOCATION_ASSIST_RESULT_CODE_OK:
		break;

	case LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR:
	case LOCATION_ASSIST_RESULT_CODE_TEMP_ERR:
	default:
		mosh_error("LwM2M server failed to get location");
		err = -1;
		goto exit;
	}

	err = lwm2m_get_f64(&LWM2M_OBJ(GROUND_FIX_OBJECT_ID, 0, GROUND_FIX_LATITUDE),
			      &location.latitude);
	if (err) {
		mosh_error("Getting latitude from LwM2M engine failed, err: %d", err);
		goto exit;
	}

	err = lwm2m_get_f64(&LWM2M_OBJ(GROUND_FIX_OBJECT_ID, 0, GROUND_FIX_LONGITUDE),
			      &location.longitude);
	if (err) {
		mosh_error("Getting longitude from LwM2M engine failed, err: %d", err);
		goto exit;
	}

	err = lwm2m_get_f64(&LWM2M_OBJ(GROUND_FIX_OBJECT_ID, 0, GROUND_FIX_ACCURACY),
			      &temp_accuracy);
	if (err) {
		mosh_error("Getting accuracy from LwM2M engine failed, err: %d", err);
		goto exit;
	}
	location.accuracy = temp_accuracy;

exit:
	if (err) {
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_ERROR, NULL);
	} else {
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_SUCCESS, &location);
	}

	return err;
}

void location_srv_ext_cellular_handle(
	const struct lte_lc_cells_info *cell_info,
	bool cloud_resp_enabled)
{
	int err = -1;

	if (!cloud_lwm2m_is_connected()) {
		mosh_error("LwM2M not connected, can't send cellular location request");
		goto exit;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(GROUND_FIX_OBJECT_ID, 0,
							    GROUND_FIX_RESULT_CODE),
						 location_ctrl_lwm2m_cell_result_cb);
	if (err) {
		mosh_error("Failed to register post write callback, err: %d", err);
	}

	ground_fix_set_report_back(cloud_resp_enabled);

	err = lwm2m_update_signal_meas_objects(cell_info);
	if (err) {
		mosh_error("Failed to update LwM2M signal meas object, err: %d", err);
		goto exit;
	}

	err = location_assistance_ground_fix_request_send(cloud_lwm2m_client_ctx_get());
	if (err) {
		mosh_error("Failed to send cellular location request, err: %d", err);
		goto exit;
	}

exit:
	if (err) {
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_ERROR, NULL);
	} else if (!cloud_resp_enabled) {
		/* If there is no error and response from the cloud is not expected */
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_UNKNOWN, NULL);
	}
}
#endif /* CONFIG_LOCATION_METHOD_CELLULAR */

void location_srv_ext_cloud_location_handle(
	const struct location_data_cloud *cloud_location_request,
	bool cloud_resp_enabled)
{
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	if (cloud_location_request->wifi_data != NULL) {
		mosh_error("No LwM2M support for Wi-Fi positioning.");
	}
#endif /* CONFIG_LOCATION_METHOD_WIFI */

	if (cloud_location_request->cell_data != NULL) {
		location_srv_ext_cellular_handle(
			cloud_location_request->cell_data, cloud_resp_enabled);
	}
}
