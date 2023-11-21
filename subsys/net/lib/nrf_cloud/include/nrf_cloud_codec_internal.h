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
#include <zephyr/bluetooth/bluetooth.h>
#include <net/nrf_cloud_defs.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_alert.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_agnss.h>
#endif
#if defined(CONFIG_NRF_MODEM)
#include <nrf_modem_gnss.h>
#endif
#include <net/nrf_cloud_location.h>
#include <net/nrf_cloud_log.h>
#include "cJSON.h"
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_agnss_schema_v1.h"
#include "nrf_cloud_log_internal.h"
#include "nrf_cloud_fota.h"

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

/** Special value indicating this is an nRF Cloud binary format */
#define NRF_CLOUD_BINARY_MAGIC 0x4346526e /* 'nRFC' in little-endian order */

/** Format identifier for remainder of this binary blob */
#define NRF_CLOUD_DICT_LOG_FMT 0x0001

/** @brief Header preceding binary blobs so nRF Cloud can
 *  process them in correct order using ts_ms and sequence fields.
 */
struct nrf_cloud_bin_hdr {
	/** Special marker value indicating this binary blob is a supported type */
	uint32_t magic;
	/** Value indicating the service format, such as a dictionary-based log */
	uint16_t format;
	/** Value for alignment */
	uint16_t pad;
	/** The time at which the log entry was generated */
	int64_t ts;
	/** Monotonically increasing sequence number */
	uint32_t sequence;
} __packed;

/** @brief Structure to receive dynamically allocated strings containing
 *  details for FOTA job update.
 */
struct nrf_cloud_fota_job_update {
	/** REST path or CoAP resource */
	char *url;
	/** Update message to send to the url */
	char *payload;
};

/** @brief Initialize the codec used encoding the data to the cloud. */
int nrf_cloud_codec_init(struct nrf_cloud_os_mem_hooks *hooks);

/** @brief Encode an alert and update the output struct with pointer
 *  to data and its length.  Caller must free the pointer when done,
 *  but only if it is not NULL; when CONFIG_NRF_CLOUD_ALERT is disabled,
 *  this function returns 0, and sets output->ptr = NULL and output->len = 0.
 */
int nrf_cloud_alert_encode(const struct nrf_cloud_alert_info *alert,
			   struct nrf_cloud_data *output);

/** @brief Encode the sensor data based on the indicated type. */
int nrf_cloud_sensor_data_encode(const struct nrf_cloud_sensor_data *input,
				 struct nrf_cloud_data *output);

/** @brief Encode general message of either a given numeric value or, if not NULL,
 *  a string value.  If topic is present, that topic will be used.
 */
int nrf_cloud_encode_message(const char *app_id, double value, const char *str_val,
			     const char *topic, int64_t ts,
			     struct nrf_cloud_data *output);

/** @brief Encode the sensor data to be sent to the device shadow. */
int nrf_cloud_shadow_data_encode(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output);

/** @brief Decode data endpoint information. */
int nrf_cloud_obj_endpoint_decode(const struct nrf_cloud_obj *const desired_obj,
				  struct nrf_cloud_data *tx_endpoint,
				  struct nrf_cloud_data *rx_endpoint,
				  struct nrf_cloud_data *bulk_endpoint,
				  struct nrf_cloud_data *bin_endpoint,
				  struct nrf_cloud_data *m_endpoint);

/** @brief Encode state information. */
int nrf_cloud_state_encode(uint32_t reported_state, const bool update_desired_topic,
			   const bool add_dev_status, struct nrf_cloud_data *output);

/** @brief Decode the shadow data and get the requested FSM state. */
int nrf_cloud_shadow_data_state_decode(const struct nrf_cloud_obj_shadow_data *const input,
				       enum nfsm_state *const requested_state);

/** @brief Decode the accepted shadow data.
 * Decoded data should be freed with @ref nrf_cloud_obj_shadow_accepted_free.
 */
int nrf_cloud_obj_shadow_accepted_decode(struct nrf_cloud_obj *const shadow_obj,
					 struct nrf_cloud_obj_shadow_accepted *const accepted);

/** @brief Free the accepted shadow data. */
void nrf_cloud_obj_shadow_accepted_free(struct nrf_cloud_obj_shadow_accepted *const accepted);

/** @brief Decode the delta shadow data.
 * Decoded data should be freed with @ref nrf_cloud_obj_shadow_delta_free.
 */
int nrf_cloud_obj_shadow_delta_decode(struct nrf_cloud_obj *const shadow_obj,
				      struct nrf_cloud_obj_shadow_delta *const delta);

/** @brief Free the delta shadow data. */
void nrf_cloud_obj_shadow_delta_free(struct nrf_cloud_obj_shadow_delta *const delta);

/** @brief Check if the shadow data should be sent to the application. */
bool nrf_cloud_shadow_app_send_check(struct nrf_cloud_obj_shadow_data *const input);

/** @brief Get the control section from the shadow data.
 * The control section will be detached from the input object and should be
 * freed with @ref nrf_cloud_obj_free.
 */
int nrf_cloud_shadow_control_get(struct nrf_cloud_obj_shadow_data *const input,
				 struct nrf_cloud_obj *const ctrl_obj);

/** @brief Parse the control object into the provided control data struct. */
int nrf_cloud_shadow_control_decode(struct nrf_cloud_obj *const ctrl_obj,
				    struct nrf_cloud_ctrl_data *data);

/** @brief Get the current device control state. */
void nrf_cloud_device_control_get(struct nrf_cloud_ctrl_data *const ctrl);

/** @brief Encode response that we have accepted a shadow delta. */
int nrf_cloud_shadow_control_response_encode(struct nrf_cloud_ctrl_data const *const data,
					     bool accept,
					     struct nrf_cloud_data *const output);

/** @brief Parse shadow data for control section. Act on any changes to logging or alerts.
 * If needed, generate output JSON to send back to cloud to confirm change.
 */
int nrf_cloud_shadow_control_process(struct nrf_cloud_obj_shadow_data *const input,
				     struct nrf_cloud_data *const response_out);

/** @brief Parse shadow data for default nRF Cloud shadow content. This data is not
 * needed by CoAP devices, but it needs to be acknowledged to remove the shadow delta.
 */
int nrf_cloud_coap_shadow_default_process(struct nrf_cloud_obj_shadow_data *const input,
					  struct nrf_cloud_data *const response_out);

/** @brief Encode the device status data into a JSON formatted buffer to be saved to
 * the device shadow.
 * The include_state flag controls if the "state" JSON key is included in the output.
 * When calling this function to encode data for use with the UpdateDeviceState nRF Cloud
 * REST endpoint, the "state" key should not be included.
 * The include_reported flag controls if the "reported" JSON key is included in the output.
 * When calling this function to encode data for use with the PATCH /state CoAP endpoint,
 * neither the "state" nor the "reported" keys should be included.
 * The user is responsible for freeing the memory by calling @ref nrf_cloud_device_status_free.
 */
int nrf_cloud_shadow_dev_status_encode(const struct nrf_cloud_device_status *const dev_status,
				       struct nrf_cloud_data *const output,
				       const bool include_state, const bool include_reported);

/** @brief Encode the device status data as an nRF Cloud device message in the provided
 * cJSON object.
 */
int nrf_cloud_dev_status_json_encode(const struct nrf_cloud_device_status *const dev_status,
				       const int64_t timestamp,
				       cJSON * const msg_obj_out);

/** @brief Free memory allocated by @ref nrf_cloud_shadow_dev_status_encode */
void nrf_cloud_device_status_free(struct nrf_cloud_data *status);

/** @brief Free memory allocated by @ref nrf_cloud_rest_fota_execution_decode or
 * @ref nrf_cloud_fota_job_decode
 */
void nrf_cloud_fota_job_free(struct nrf_cloud_fota_job_info *const job);

/** @brief Free memory allocated by @ref nrf_cloud_fota_job_update_create */
void nrf_cloud_fota_job_update_free(struct nrf_cloud_fota_job_update *update);

/** @brief Create an nF Cloud REST or CoAP FOTA job update url and payload. */
int nrf_cloud_fota_job_update_create(const char *const device_id,
				     const char *const job_id,
				     const enum nrf_cloud_fota_status status,
				     const char * const details,
				     struct nrf_cloud_fota_job_update *update);

/** @brief Create an nRF Cloud MQTT FOTA job update payload which is to be sent
 * on the FOTA jobs update topic.
 * If successful, memory is allocated for the provided object.
 * The @ref nrf_cloud_obj_free function should be called when finished with the object.
 */
int nrf_cloud_obj_fota_job_update_create(struct nrf_cloud_obj *const obj,
					 const struct nrf_cloud_fota_job *const update);

#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
/** @brief Create an nRF Cloud MQTT BLE FOTA job request payload which is to be sent
 * on the FOTA jobs request topic.
 * If successful, memory is allocated for the provided object.
 * The @ref nrf_cloud_obj_free function should be called when finished with the object.
 */
int nrf_cloud_obj_fota_ble_job_request_create(struct nrf_cloud_obj *const obj,
					       const bt_addr_t *const ble_id);

/** @brief Create an nRF Cloud MQTT BLE FOTA job update payload which is to be sent
 * on the BLE FOTA jobs update topic.
 * If successful, memory is allocated for the provided object.
 * The @ref nrf_cloud_obj_free function should be called when finished with the object.
 */
int nrf_cloud_obj_fota_ble_job_update_create(struct nrf_cloud_obj *const obj,
					     const struct nrf_cloud_fota_ble_job *const ble_job,
					     const enum nrf_cloud_fota_status status);
#endif

/** @brief Parse the response from a FOTA execution request REST call.
 * If successful, memory will be allocated for the data in @ref nrf_cloud_fota_job_info.
 * The user is responsible for freeing the memory by calling @ref nrf_cloud_fota_job_free.
 */
int nrf_cloud_rest_fota_execution_decode(const char *const response,
					 struct nrf_cloud_fota_job_info *const job);

/** @brief Parse the data received on the MQTT FOTA topic.
 * Memory will be allocated for the data in @ref nrf_cloud_fota_job_info.
 * The user is responsible for freeing the memory by calling @ref nrf_cloud_fota_job_free.
 */
int nrf_cloud_fota_job_decode(struct nrf_cloud_fota_job_info *const job_info,
			      bt_addr_t *const ble_id,
			      const struct nrf_cloud_data *const input);

/** @brief Add cellular network info to the provided cJSON object.
 * If the cell_inf parameter is NULL, the codec will obtain the current network
 * info from the modem.
 */
int nrf_cloud_cell_info_json_encode(cJSON * const data_obj,
				    const struct lte_lc_cell * const cell_inf);

/** @brief Build a cellular positioning request in the provided cJSON object
 * using the provided cell info
 */
int nrf_cloud_cell_pos_req_json_encode(struct lte_lc_cells_info const *const inf,
				       cJSON * const req_obj_out);

/** @brief Add the location request data payload to the provided initialized object */
int nrf_cloud_obj_location_request_payload_add(struct nrf_cloud_obj *const obj,
					       struct lte_lc_cells_info const *const cells_inf,
					       struct wifi_scan_info const *const wifi_inf);

/** @brief Build a Wi-Fi positioning request in the provided cJSON object using the provided
 * Wi-Fi info. Local MAC addresses are not included in the request.
 *
 * @retval 0 Success.
 * @retval -ENODATA Access point (non-local) count less than NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN.
 * @return -ENOMEM Out of memory.
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
int json_array_str_get(const cJSON * const array, const int index, char **string_out);

/** @brief Obtain the integer value at the specified index in the cJSON array. */
int json_array_num_get(const cJSON * const array, const int index, int *number_out);

/** @brief Add a string value to the provided cJSON array. */
int json_array_str_add(cJSON * const array, const char * const string);

/** @brief Add an integer value to the provided cJSON array. */
int json_array_num_add(cJSON * const array, const int number);

/** @brief Obtain a pointer to the string of the specified key in the cJSON object.
 * No memory is allocated, pointer is valid as long as the cJSON object is valid.
 */
int get_string_from_obj(const cJSON * const obj, const char * const key,
			char **string_out);

/** @brief Get the number value of the specified key in the cJSON object. */
int get_num_from_obj(const cJSON *const obj, const char *const key,
		     double *num_out);

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
int nrf_cloud_pvt_data_encode(const struct nrf_cloud_gnss_pvt *const pvt,
			      cJSON * const pvt_data_obj);

/** @brief Replace legacy c2d topic with wilcard topic string.
 * Return true, if the topic was modified; otherwise false.
 */
bool nrf_cloud_set_wildcard_c2d_topic(char *const topic, size_t topic_len);

/** @brief Decode a dc receive topic string into an enum value */
enum nrf_cloud_rcv_topic nrf_cloud_dc_rx_topic_decode(const char *const topic);

/** @brief Set the application version that is reported to nRF Cloud if
 * CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS is enabled.
 */
void nrf_cloud_set_app_version(const char *const app_ver);

/** @brief Encode the data payload of an nRF Cloud A-GNSS request into the provided object */
int nrf_cloud_agnss_req_data_json_encode(const enum nrf_cloud_agnss_type *const types,
					const size_t type_count,
					const struct lte_lc_cell *const cell_inf,
					const bool fetch_cell_inf,
					const bool filtered_ephem, const uint8_t mask_angle,
					cJSON * const data_obj_out);

#if defined(CONFIG_NRF_MODEM)
/** @brief Encode a modem PVT data frame to be sent to nRF Cloud */
int nrf_cloud_modem_pvt_data_encode(const struct nrf_modem_gnss_pvt_data_frame	*const mdm_pvt,
				    cJSON * const pvt_data_obj);
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
/** @brief Build A-GNSS type array based on request.
 */
int nrf_cloud_agnss_type_array_get(const struct nrf_modem_gnss_agnss_data_frame *const request,
				  enum nrf_cloud_agnss_type *array, const size_t array_size);

/** @brief Encode an A-GNSS request device message to be sent to nRF Cloud */
int nrf_cloud_agnss_req_json_encode(const struct nrf_modem_gnss_agnss_data_frame *const request,
				   cJSON * const agnss_req_obj_out);
#endif /* CONFIG_NRF_CLOUD_AGNSS || CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
/** @brief Parse the PGPS response (REST and MQTT) from nRF Cloud */
int nrf_cloud_pgps_response_decode(const char *const response,
				   struct nrf_cloud_pgps_result *const result);

/** @brief Encode the data payload of an nRF Cloud P-GPS request into the provided object */
int nrf_cloud_pgps_req_data_json_encode(const struct gps_pgps_request *const request,
					cJSON * const data_obj_out);
#endif

/** @brief Convert a JSON payload to a parameterized URL for REST. Converts only objects at
 * the root level. Converts only integers and integer arrays, strings, and boolean values.
 * String values are assumed to be URL-compatible.
 */
int nrf_cloud_json_to_url_params_convert(char *const buf, const size_t buf_size,
					 const cJSON * const obj);

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

/** @brief Encode a log output buffer for transport to the cloud */
int nrf_cloud_log_json_encode(struct nrf_cloud_log_context *ctx, uint8_t *buf, size_t size,
			 struct nrf_cloud_data *output);

/** @brief Return the appId string equivalent to the specified sensor type, otherwise NULL. */
const char *nrf_cloud_sensor_app_id_lookup(enum nrf_cloud_sensor type);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CODEC_INTERNAL_H__ */
