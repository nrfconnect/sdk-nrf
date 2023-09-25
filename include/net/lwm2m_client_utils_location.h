/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file lwm2m_client_utils_location.h
 * @defgroup lwm2m_client_utils_location LwM2M client utilities location
 * @{
 * @brief API for the LwM2M based LOCATION
 */

#ifndef LWM2M_CLIENT_UTILS_LOCATION_H__
#define LWM2M_CLIENT_UTILS_LOCATION_H__

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <zephyr/net/lwm2m.h>
#include <modem/lte_lc.h>

#define LOCATION_ASSIST_NEEDS_UTC			BIT(0)
#define LOCATION_ASSIST_NEEDS_EPHEMERIES		BIT(1)
#define LOCATION_ASSIST_NEEDS_ALMANAC			BIT(2)
#define LOCATION_ASSIST_NEEDS_KLOBUCHAR			BIT(3)
#define LOCATION_ASSIST_NEEDS_NEQUICK			BIT(4)
#define LOCATION_ASSIST_NEEDS_TOW			BIT(5)
#define LOCATION_ASSIST_NEEDS_CLOCK			BIT(6)
#define LOCATION_ASSIST_NEEDS_LOCATION			BIT(7)
#define LOCATION_ASSIST_NEEDS_INTEGRITY			BIT(8)

#define LOCATION_ASSIST_RESULT_CODE_OK			0
#define LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR	-1
#define LOCATION_ASSIST_RESULT_CODE_TEMP_ERR		1

/**
 * @typedef location_assistance_result_code_cb_t
 * @brief Callback for location assistance result.
 *
 * This callback is called whenever there is a new result in the location assistance and the
 * location assistance resend handler has been initialized.
 *
 * @param result_code Contains following possible result codes:
 *                    LOCATION_ASSIST_RESULT_CODE_OK when there are no problems
 *                    LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR when there is a permanent error
 *                    between LwM2M-server and the nrfCloud. Location assistance library will no
 *                    longer send any requests. Device needs reboot for assistance library to
 *                    resume operation.
 *                    LOCATION_ASSIST_RESULT_CODE_TEMP_ERR when there is a temporary error between
 *                    LwM2M-server and the nrfCloud. The library automatically uses exponential
 *                    backoff for the retries.
 */
typedef void (*location_assistance_result_code_cb_t)(int32_t result_code);

/**
 * @brief Set the location assistance result code calback
 *
 * @param cb callback function to call when there is a new result from location assistance.
 */
void location_assistance_set_result_code_cb(location_assistance_result_code_cb_t cb);

/**
 * @brief Set the A-GPS request mask
 *
 * @param agps_req The A-GPS request coming from the GNSS module.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_agps_set_mask(const struct nrf_modem_gnss_agnss_data_frame *agps_req);

/**
 * @brief Send the A-GPS assistance request to LwM2M server
 *
 * @param ctx LwM2M client context for sending the data.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_agps_request_send(struct lwm2m_ctx *ctx);

/**
 * @brief Send the Ground Fix request to LwM2M server
 *
 * @param ctx LwM2M client context for sending the data.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_ground_fix_request_send(struct lwm2m_ctx *ctx);

/**
 * @brief Send the P-GPS assistance request to LwM2M server
 *
 * @param ctx LwM2M client context for sending the data.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_pgps_request_send(struct lwm2m_ctx *ctx);

/**
 * @brief Initialize the location assistance library resend handler.
 *        Handler will handle the result code from server and schedule resending in
 *        case of temporary error in server using an exponential backoff.
 *
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_init_resend_handler(void);

/**
 * @brief Initialize the location assistance event handler
 *
 * @param ctx LwM2M client context for sending the data.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_event_handler_init(struct lwm2m_ctx *ctx);

#define GNSS_ASSIST_OBJECT_ID 33625

/**
 * @brief Set an A-GPS assistance request mask for the object
 *
 * @param request_mask A bitmask containing the requested parameters from the server
 */
void location_assist_agps_request_set(uint32_t request_mask);

/**
 * @brief Set the satellite elevation mask angle above the ground. Satellites
 *        below the angle will be filtered in the response.
 *
 * @param elevation_mask Elevation mask angle in degrees above the ground level
 */
void location_assist_agps_set_elevation_mask(int32_t elevation_mask);

/**
 * @brief Get the satellite elevation mask currently stored in the resource.
 *
 * @return Returns the currently stored elevation mask as degrees above the ground level.
 */
int32_t location_assist_agps_get_elevation_mask(void);

/**
 * @brief Set prediction count for the P-GPS query
 *
 * @param predictions Number of predictions requested
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assist_pgps_set_prediction_count(int32_t predictions);

/**
 * @brief Set prediction interval as minutes for the P-GPS query
 *
 * @param interval Time in minutes between each query
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assist_pgps_set_prediction_interval(int32_t interval);

/**
 * @brief Set starting GPS day as days since GPS epoch. Setting the day as 0,
 *        will use the default value for the request which will be current GPS day.
 *
 * @param gps_day Day as a GPS days since GPS epoch.
 */
void location_assist_pgps_set_start_gps_day(int32_t gps_day);

/**
 * @brief Get the GPS start day stored currently in the resource.
 *
 * @return Returns the currently stored value for GPS start day.
 */
int32_t location_assist_pgps_get_start_gps_day(void);

/**
 * @brief Set the GPS start time in seconds.
 *
 * @param start_time Time of day in seconds.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assist_pgps_set_start_time(int32_t start_time);

/**
 * @brief Get the result code of the location request.
 *
 * @return int32_t Returns a result code from the LwM2M server.
 */
int32_t location_assist_gnss_get_result_code(void);

#define GROUND_FIX_OBJECT_ID 33626
/**
 * @brief Set if the server should report the location back to the object
 *        after it has attained it.
 *
 * @param report_back Boolean value showing if the server should report the location back.
 */
void ground_fix_set_report_back(bool report_back);

/**
 * @brief Get the result code of the location request.
 *
 * @return int32_t Returns a result code from the LwM2M server.
 */
int32_t ground_fix_get_result_code(void);

#define VISIBLE_WIFI_AP_OBJECT_ID 33627

#define ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID 10256
int lwm2m_signal_meas_info_inst_id_to_index(uint16_t obj_inst_id);
int lwm2m_signal_meas_info_index_to_inst_id(int index);

/**
 * @brief Update the ECID-Signal Measurement Info objects with the recent neighbor cell data.
 *
 * @param cells Struct containing all the cell neighbors.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int lwm2m_update_signal_meas_objects(const struct lte_lc_cells_info *const cells);

/**
 * @brief Register the handler for the Neighbor cell scanning
 *
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int lwm2m_ncell_handler_register(void);
void lwm2m_ncell_schedule_measurement(void);

/**
 * @brief Request a Wi-Fi scan for nearby access points.
 *
 * The Wi-Fi access points are added to the Visible Wi-Fi Access Point objects (ID 33627)
 * and used for the Wi-Fi based location when sending a ground fix request.
 *
 * @return Returns a negative error code (errno.h) indicating
 *         the reason for failure or 0 for success.
 */
int lwm2m_wifi_request_scan(void);

#endif /* LWM2M_CLIENT_UTILS_LOCATION_H__ */

/**@} */
