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

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_cell_pos nRF Cloud cellular positioning
 * @{
 */

/** @brief Cellular positioning request type */
enum nrf_cloud_cell_pos_type {
	CELL_POS_TYPE_SINGLE,
	CELL_POS_TYPE_MULTI /* Not yet supported */
};

/** @brief Cellular positioning request result */
struct nrf_cloud_cell_pos_result {
	enum nrf_cloud_cell_pos_type type;
	double lat;
	double lon;
	uint32_t unc;
};

#if defined(CONFIG_NRF_CLOUD_MQTT)
/**@brief Request a cellular positioning query from nRF Cloud via MQTT.
 *
 * @param type        Type of cellular positioning request.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_cell_pos_request(enum nrf_cloud_cell_pos_type type, const bool request_loc);
#endif /* CONFIG_NRF_CLOUD_MQTT */

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
