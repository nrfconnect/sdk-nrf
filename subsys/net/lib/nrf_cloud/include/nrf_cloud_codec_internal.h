/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CODEC_INTERNAL_H__
#define NRF_CLOUD_CODEC_INTERNAL_H__

#include <stdbool.h>
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <net/nrf_cloud_defs.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_alerts.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_agps.h>
#endif
#include <net/nrf_cloud_location.h>
#include "cJSON.h"
#include "nrf_cloud_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

enum nrf_cloud_rcv_topic {
	NRF_CLOUD_RCV_TOPIC_GENERAL,
	NRF_CLOUD_RCV_TOPIC_AGPS,
	NRF_CLOUD_RCV_TOPIC_PGPS,
	NRF_CLOUD_RCV_TOPIC_LOCATION,
	/* Unknown/unhandled topic */
	NRF_CLOUD_RCV_TOPIC_UNKNOWN
};

/** @brief Initialize the codec used encoding the data to the cloud. */
int nrf_cloud_codec_init(struct nrf_cloud_os_mem_hooks *hooks);

/** @brief Encode an alert and update the output struct with pointer
 *  to data and its length.  Caller must free the pointer when done,
 *  but only if it is not NULL; when CONFIG_NRF_CLOUD_ALERTS is disabled,
 *  this function returns 0, and sets output->ptr = NULL and output->len = 0.
 */
int nrf_cloud_alert_encode(const struct nrf_cloud_alert_info *alert,
			   struct nrf_cloud_data *output);

/** @brief Encode the sensor data based on the indicated type. */
int nrf_cloud_sensor_data_encode(const struct nrf_cloud_sensor_data *input,
				 struct nrf_cloud_data *output);

/** @brief Encode the sensor data to be sent to the device shadow. */
int nrf_cloud_shadow_data_encode(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output);

/** @brief Encode the user association data based on the indicated type. */
int nrf_cloud_requested_state_decode(const struct nrf_cloud_data *payload,
				     enum nfsm_state *requested_state);

/** @brief Decode data endpoint information. */
int nrf_cloud_data_endpoint_decode(const struct nrf_cloud_data *input,
				   struct nrf_cloud_data *tx_endpoint,
				   struct nrf_cloud_data *rx_endpoint,
				   struct nrf_cloud_data *bulk_endpoint,
				   struct nrf_cloud_data *m_endpoint);

/** @brief Encode state information. */
int nrf_cloud_state_encode(uint32_t reported_state, const bool update_desired_topic,
			   struct nrf_cloud_data *output);

/** @brief Search input for config and encode response if necessary. */
int nrf_cloud_shadow_config_response_encode(struct nrf_cloud_data const *const input,
					    struct nrf_cloud_data *const output,
					    bool *const has_config);

/** @brief Parse input for control section, and return contents and status of it. */
int nrf_cloud_shadow_control_decode(struct nrf_cloud_data const *const input,
				    enum nrf_cloud_ctrl_status *status,
				    struct nrf_cloud_ctrl_data *data);

/** @brief Encode response that we have accepted a shadow delta. */
int nrf_cloud_shadow_control_response_encode(struct nrf_cloud_ctrl_data const *const data,
					     struct nrf_cloud_data *const output);

/** @brief Encode the device status data into a JSON formatted buffer to be saved to
 * the device shadow.
 * The include_state flag controls if the "state" JSON key is included in the output.
 * When calling this function to encode data for use with the UpdateDeviceState nRF Cloud
 * REST endpoint, the "state" key should not be included.
 * The user is responsible for freeing the memory by calling @ref nrf_cloud_device_status_free.
 */
int nrf_cloud_shadow_dev_status_encode(const struct nrf_cloud_device_status * const dev_status,
				       struct nrf_cloud_data * const output,
				       const bool include_state);

/** @brief Encode the device status data as an nRF Cloud device message in the provided
 * cJSON object.
 */
int nrf_cloud_dev_status_json_encode(const struct nrf_cloud_device_status *const dev_status,
				       const int64_t timestamp,
				       cJSON * const msg_obj_out);

/** @brief Free memory allocated by @ref nrf_cloud_shadow_dev_status_encode */
void nrf_cloud_device_status_free(struct nrf_cloud_data *status);

/** @brief Free memory allocated by @ref nrf_cloud_rest_fota_execution_decode */
void nrf_cloud_fota_job_free(struct nrf_cloud_fota_job_info *const job);

/** @brief Parse the response from a FOTA execution request REST call.
 * If successful, memory will be allocated for the data in @ref nrf_cloud_fota_job_info.
 * The user is responsible for freeing the memory by calling @ref nrf_cloud_fota_job_free.
 */
int nrf_cloud_rest_fota_execution_decode(const char *const response,
					struct nrf_cloud_fota_job_info *const job);

#if defined(CONFIG_NRF_CLOUD_PGPS)
/** @brief Parse the PGPS response (REST and MQTT) from nRF Cloud */
int nrf_cloud_pgps_response_decode(const char *const response,
				   struct nrf_cloud_pgps_result *const result);
#endif

/** @brief Add common [network] modem info to the provided cJSON object */
int nrf_cloud_network_info_json_encode(cJSON * const data_obj);

/** @brief Build a cellular positioning request in the provided cJSON object
 * using the provided cell info
 */
int nrf_cloud_cell_pos_req_json_encode(struct lte_lc_cells_info const *const inf,
				       cJSON * const req_obj_out);

/** @brief Build a location request string using the provided info.
 * If successful, memory will be allocated for the output string and the user is
 * responsible for freeing it using @ref cJSON_free.
 */
int nrf_cloud_location_req_json_encode(struct lte_lc_cells_info const *const cell_info,
				       struct wifi_scan_info const *const wifi_info,
				       char **string_out);

/** @brief Build a WiFi positioning request in the provided cJSON object
 * using the provided WiFi info
 */
int nrf_cloud_wifi_req_json_encode(struct wifi_scan_info const *const wifi,
				   cJSON *const req_obj_out);

/** @brief Get the required information from the modem for a single-cell location request. */
int nrf_cloud_get_single_cell_modem_info(struct lte_lc_cell *const cell_inf);

/** @brief Parse the location response (REST and MQTT) from nRF Cloud. */
int nrf_cloud_location_response_decode(const char *const buf,
				      struct nrf_cloud_location_result *result);

/** @brief Check whether the provided MQTT payload is an nRF Cloud disconnection request */
bool nrf_cloud_disconnection_request_decode(const char *const buf);

/** @brief Obtain a pointer to the string at the specified index in the cJSON array.
 * No memory is allocated, pointer is valid as long as the cJSON array is valid.
 */
int get_string_from_array(const cJSON * const array, const int index,
			  char **string_out);

/** @brief Obtain a pointer to the string of the specified key in the cJSON object.
 * No memory is allocated, pointer is valid as long as the cJSON object is valid.
 */
int get_string_from_obj(const cJSON * const obj, const char *const key,
			char **string_out);

/** @brief Send the cJSON object to nRF Cloud on the d2c topic */
int json_send_to_cloud(cJSON * const request);

/** @brief Create a cJSON object containing the specified appId and messageType.
 * If successful, user is responsible for calling @ref cJSON_Delete to free
 * the cJSON object's memory.
 */
cJSON *json_create_req_obj(const char *const app_id, const char *const msg_type);

/** @brief Parse received REST data for an nRF Cloud error code */
int nrf_cloud_rest_error_decode(const char *const buf, enum nrf_cloud_error *const err);

/** @brief Encode PVT data to be sent to nRF Cloud */
int nrf_cloud_pvt_data_encode(const struct nrf_cloud_gnss_pvt * const pvt,
			      cJSON * const pvt_data_obj);

#if defined(CONFIG_NRF_MODEM)
/** @brief Encode a modem PVT data frame to be sent to nRF Cloud */
int nrf_cloud_modem_pvt_data_encode(const struct nrf_modem_gnss_pvt_data_frame	* const mdm_pvt,
				    cJSON * const pvt_data_obj);
#endif

/** @brief Replace legacy c2d topic with wilcard topic string.
 * Return true, if the topic was modified; otherwise false.
 */
bool nrf_cloud_set_wildcard_c2d_topic(char *const topic, size_t topic_len);

/** @brief Decode a dc receive topic string into an enum value */
enum nrf_cloud_rcv_topic nrf_cloud_dc_rx_topic_decode(const char * const topic);

/** @brief Set the application version that is reported to nRF Cloud if
 * CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS is enabled.
 */
void nrf_cloud_set_app_version(const char * const app_ver);

#ifdef CONFIG_NRF_CLOUD_GATEWAY
typedef int (*gateway_state_handler_t)(void *root_obj);

/** @brief Register a callback, which is called whenever the shadow changes.
 *  The callback is passed a pointer to the shadow JSON document.  The callback
 *  should return 0 to allow further processing of shadow changes in
 *  nrf_cloud_codec.c.  It should return a negative error code only when the
 *  shadow is malformed.
 */
void nrf_cloud_register_gateway_state_handler(gateway_state_handler_t handler);
#endif

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CODEC_INTERNAL_H__ */
