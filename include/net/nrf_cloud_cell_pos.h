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

#include <zephyr.h>
#include <modem/lte_lc.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_cell_pos nRF Cloud cellular positioning
 * @{
 */

/** @brief Cellular positioning request type */
enum nrf_cloud_cell_pos_type {
	CELL_POS_TYPE_SINGLE,
	CELL_POS_TYPE_MULTI
};

/** @brief Cellular positioning request result */
struct nrf_cloud_cell_pos_result {
	enum nrf_cloud_cell_pos_type type;
	double lat;
	double lon;
	uint32_t unc;
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
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_cell_pos_request(const struct lte_lc_cells_info * const cells_inf,
			       const bool request_loc);
#endif /* CONFIG_NRF_CLOUD_MQTT */

/**@brief Get the reference to a cJSON object containing a cell position request.
 *
 * @param cells_inf Pointer to cell info.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @param req_obj_out Pointer used to get the reference to the generated cJSON object.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_cell_pos_request_json_get(const struct lte_lc_cells_info *const cells_inf,
					const bool request_loc, cJSON **req_obj_out);

/**@brief Processes cellular positioning data received from nRF Cloud via MQTT or REST.
 *
 * @param buf Pointer to data received from nRF Cloud.
 * @param result Pointer to buffer for parsing result.
 *
 * @retval 0 If processed successfully and cell-based location found.
 * @retval 1 If processed successfully but no cell-based location found.
 *         Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_cell_pos_process(const char *buf, struct nrf_cloud_cell_pos_result *result);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CELL_POS_H_ */
