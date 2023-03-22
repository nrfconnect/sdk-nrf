/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_LOCATION_H_
#define NRF_CLOUD_LOCATION_H_

/** @file nrf_cloud_location.h
 * @brief Module to provide nRF Cloud location support to nRF9160 SiP.
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <net/nrf_cloud.h>
#include <net/wifi_location_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_location nRF Cloud location
 * @{
 */

/** Omit the RSSI value when submitting a location request */
#define NRF_CLOUD_LOCATION_WIFI_OMIT_RSSI	(INT8_MAX)
/** Omit the channel value when submitting a location request */
#define NRF_CLOUD_LOCATION_WIFI_OMIT_CHAN	(0)

/** Minimum number of access points required by nRF Cloud */
#define NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN 2

/** @brief Location request type */
enum nrf_cloud_location_type {
	LOCATION_TYPE_SINGLE_CELL,
	LOCATION_TYPE_MULTI_CELL,
	LOCATION_TYPE_WIFI,

	LOCATION_TYPE__INVALID
};

/** @brief Location request result */
struct nrf_cloud_location_result {
	/** The service used to fulfill the location request */
	enum nrf_cloud_location_type type;
	/** Latitude */
	double lat;
	/** Longitude */
	double lon;
	/** The radius of the uncertainty circle around the location in meters.
	 * Also known as Horizontal Positioning Error (HPE).
	 */
	uint32_t unc;
	/** Error value received from nRF Cloud. NRF_CLOUD_ERROR_NONE on success. */
	enum nrf_cloud_error err;
};

/** Omit the timing advance value when submitting a location request */
#define NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV	LTE_LC_CELL_TIMING_ADVANCE_INVALID
/** Omit the RSRQ value when submitting a location request */
#define NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ	LTE_LC_CELL_RSRQ_INVALID
/** Omit the RSRP value when submitting a location request */
#define NRF_CLOUD_LOCATION_CELL_OMIT_RSRP	LTE_LC_CELL_RSRP_INVALID
/** Omit the EARFN value when submitting a location request */
#define NRF_CLOUD_LOCATION_CELL_OMIT_EARFCN	UINT32_MAX

/** Maximum value for timing advance parameter */
#define NRF_CLOUD_LOCATION_CELL_TIME_ADV_MAX	LTE_LC_CELL_TIMING_ADVANCE_MAX

#if defined(CONFIG_NRF_CLOUD_MQTT)
/**
 * @brief Cloud location result handler function type.
 *
 * @param pos   Location result.
 */
typedef void (*nrf_cloud_location_response_t)(const struct nrf_cloud_location_result *const pos);

/** @brief Perform an nRF Cloud location request over MQTT.
 *
 * @param cells_inf Cell info. The required network parameters are
 *                  cell identifier, mcc, mnc and tac. The required neighboring
 *                  cell parameters are E-ARFCN and physical cell identity.
 *                  The parameters for time diff and measurement time are not used.
 *                  The remaining parameters are optional; including them may improve
 *                  location accuracy. To omit a request parameter, use the appropriate
 *                  `NRF_CLOUD_LOCATION_CELL_OMIT_` define.
 *                  Can be NULL if Wi-Fi info is provided.
 * @param wifi_inf Wi-Fi info. The MAC address is the only required parameter
 *                 for each item.
 *                 To omit a request parameter, use the appropriate
 *                 `NRF_CLOUD_LOCATION_WIFI_OMIT_` define.
 *                 Can be NULL if cell info is provided.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @param cb Callback function to receive parsed location result. Only used when
 *           request_loc is true. If NULL, JSON result will be sent to the cloud event
 *           handler as an NRF_CLOUD_EVT_RX_DATA_LOCATION event.
 * @retval 0       Request sent successfully.
 * @retval -EACCES Cloud connection is not established; wait for @ref NRF_CLOUD_EVT_READY.
 * @retval -EDOM The number of access points in the Wi-Fi-only request was smaller than
 *               the minimum required value NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN.
 * @return A negative value indicates an error.
 */
int nrf_cloud_location_request(const struct lte_lc_cells_info *const cells_inf,
			       const struct wifi_scan_info *const wifi_inf,
			       const bool request_loc, nrf_cloud_location_response_t cb);
#endif /* CONFIG_NRF_CLOUD_MQTT */

/** @brief Process location data received from nRF Cloud over MQTT or REST.
 *
 * @param buf Data received from nRF Cloud.
 * @param result Parsed results.
 *
 * @retval 0 If processed successfully and location found.
 * @retval 1 If processed successfully but no location data found. This
 *           indicates that the data is not a location response.
 * @retval -EFAULT An nRF Cloud error code was processed.
 * @return A negative value indicates an error.
 */
int nrf_cloud_location_process(const char *buf, struct nrf_cloud_location_result *result);

/** @brief Get the cellular network information from the modem that is required
 *        for a single-cell location request.
 *
 * @param cell_inf Cellular information obtained from the modem.
 *
 * @retval 0 If cellular informat was obtained successfully.
 * @return A negative value indicates an error.
 */
int nrf_cloud_location_scell_data_get(struct lte_lc_cell *const cell_inf);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_LOCATION_H_ */
