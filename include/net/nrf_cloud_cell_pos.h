/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CELL_POS_H_
#define NRF_CLOUD_CELL_POS_H_

/** @file nrf_cloud_cell_pos.h
 * @brief Module to provide nRF Cloud cellular positioning support to nRF9160 SiP.
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <net/nrf_cloud.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_cell_pos nRF Cloud cellular positioning
 * @{
 */

/** @brief Cellular positioning request type */
enum nrf_cloud_cell_pos_type {
	CELL_POS_TYPE_SINGLE,
	CELL_POS_TYPE_MULTI,

	CELL_POS_TYPE__INVALID
};

/** @brief Cellular positioning request result */
struct nrf_cloud_cell_pos_result {
	enum nrf_cloud_cell_pos_type type;
	double lat;
	double lon;
	uint32_t unc;
	/* Error value received from nRF Cloud */
	enum nrf_cloud_error err;
};

/** @defgroup nrf_cloud_cell_pos_omit Omit item from cellular positioning request.
 * @{
 */
#define NRF_CLOUD_CELL_POS_OMIT_TIME_ADV	LTE_LC_CELL_TIMING_ADVANCE_INVALID
#define NRF_CLOUD_CELL_POS_OMIT_RSRQ		LTE_LC_CELL_RSRQ_INVALID
#define NRF_CLOUD_CELL_POS_OMIT_RSRP		LTE_LC_CELL_RSRP_INVALID
#define NRF_CLOUD_CELL_POS_OMIT_EARFCN		UINT32_MAX
/** @} */

#define NRF_CLOUD_CELL_POS_TIME_ADV_MAX		LTE_LC_CELL_TIMING_ADVANCE_MAX

#if defined(CONFIG_NRF_CLOUD_MQTT)
/**
 * @brief Cloud cellular positioning result handler function type.
 *
 * @param pos   Cellular positioning result.
 */
typedef void (*nrf_cloud_cell_pos_response_t)(const struct nrf_cloud_cell_pos_result *const pos);

/**@brief Perform an nRF Cloud cellular positioning request via MQTT using
 * the provided cell info.
 *
 * @param cells_inf Pointer to cell info. The required network parameters are
 *                  cell identifier, mcc, mnc and tac. The required neighboring
 *                  cell parameters are E-ARFCN and physical cell identity.
 *                  The parameters for time diff and measurement time are not used.
 *                  The remaining parameters are optional; including them may improve
 *                  location accuracy. To omit a request parameter, use the appropriate
 *                  define in @ref nrf_cloud_rest_pgps_omit.
 *                  If NULL, current (single) cell tower data will be requested from
 *                  the modem and used in the request.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @param cb Callback function to receive parsed cell pos result. Only used when
 *           request_loc is true. If NULL, JSON result will be sent to the cloud event
 *           handler as an NRF_CLOUD_EVT_RX_DATA_CELL_POS event.
 * @retval 0       Request sent successfully.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @return A negative value indicates an error.
 */
int nrf_cloud_cell_pos_request(const struct lte_lc_cells_info * const cells_inf,
			       const bool request_loc, nrf_cloud_cell_pos_response_t cb);
#endif /* CONFIG_NRF_CLOUD_MQTT */

/**@brief Get the reference to a cJSON object containing a cell position request.
 *
 * @param cells_inf Pointer to cell info.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @param req_obj_out Pointer used to get the reference to the generated cJSON object.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_cell_pos_request_json_get(const struct lte_lc_cells_info *const cells_inf,
					const bool request_loc, cJSON **req_obj_out);

/**@brief Processes cellular positioning data received from nRF Cloud via MQTT or REST.
 *
 * @param buf Pointer to data received from nRF Cloud.
 * @param result Pointer to buffer for parsing result.
 *
 * @retval 0 If processed successfully and cell-based location found.
 * @retval 1 If processed successfully but no cell-based location data found. This
 *           indicates that the data is not a cell-based location response.
 * @retval -EFAULT An nRF Cloud error code was processed.
 * @return A negative value indicates an error.
 */
int nrf_cloud_cell_pos_process(const char *buf, struct nrf_cloud_cell_pos_result *result);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CELL_POS_H_ */
