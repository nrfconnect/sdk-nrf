/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CODEC_H__
#define NRF_CLOUD_CODEC_H__

#include <stdbool.h>
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <net/nrf_cloud.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_agps.h>
#endif
#include <net/nrf_cloud_cell_pos.h>
#include "cJSON.h"
#include "nrf_cloud_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* nRF Cloud appID values */
#define NRF_CLOUD_JSON_APPID_KEY		"appId"
#define NRF_CLOUD_JSON_APPID_VAL_AGPS		"AGPS"
#define NRF_CLOUD_JSON_APPID_VAL_PGPS		"PGPS"
#define NRF_CLOUD_JSON_APPID_VAL_GPS		"GPS"
#define NRF_CLOUD_JSON_APPID_VAL_CELL_POS	"CELL_POS"
#define NRF_CLOUD_JSON_APPID_VAL_DEVICE		"DEVICE"
#define NRF_CLOUD_JSON_APPID_VAL_FLIP		"FLIP"
#define NRF_CLOUD_JSON_APPID_VAL_BTN		"BUTTON"
#define NRF_CLOUD_JSON_APPID_VAL_TEMP		"TEMP"
#define NRF_CLOUD_JSON_APPID_VAL_HUMID		"HUMID"
#define NRF_CLOUD_JSON_APPID_VAL_AIR_PRESS	"AIR_PRESS"
#define NRF_CLOUD_JSON_APPID_VAL_AIR_QUAL	"AIR_QUAL"
#define NRF_CLOUD_JSON_APPID_VAL_RSRP		"RSRP"
#define NRF_CLOUD_JSON_APPID_VAL_LIGHT		"LIGHT"

#define NRF_CLOUD_JSON_MSG_TYPE_KEY		"messageType"
#define NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA	"DATA"
#define NRF_CLOUD_JSON_MSG_TYPE_VAL_DISCONNECT	"DISCON"
#define NRF_CLOUD_JSON_MSG_MAX_LEN_DISCONNECT   200

#define NRF_CLOUD_JSON_DATA_KEY			"data"
#define NRF_CLOUD_JSON_ERR_KEY			"err"

#define NRF_CLOUD_JSON_FULFILL_KEY		"fulfilledWith"

#define NRF_CLOUD_JSON_FILTERED_KEY		"filtered"
#define NRF_CLOUD_JSON_ELEVATION_MASK_KEY	"mask"

#define NRF_CLOUD_BULK_MSG_TOPIC		"/bulk"

/* Modem info key text */
#define NRF_CLOUD_JSON_MCC_KEY			"mcc"
#define NRF_CLOUD_JSON_MNC_KEY			"mnc"
#define NRF_CLOUD_JSON_AREA_CODE_KEY		"tac"
#define NRF_CLOUD_JSON_CELL_ID_KEY		"eci"
#define NRF_CLOUD_JSON_PHYCID_KEY		"phycid"

/* Cellular positioning */
#define NRF_CLOUD_CELL_POS_JSON_KEY_LAT		"lat"
#define NRF_CLOUD_CELL_POS_JSON_KEY_LON		"lon"
#define NRF_CLOUD_CELL_POS_JSON_KEY_UNCERT	"uncertainty"

#define NRF_CLOUD_CELL_POS_JSON_KEY_LTE		"lte"
#define NRF_CLOUD_CELL_POS_JSON_KEY_ECI		NRF_CLOUD_JSON_CELL_ID_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_MCC		NRF_CLOUD_JSON_MCC_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_MNC		NRF_CLOUD_JSON_MNC_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_TAC		NRF_CLOUD_JSON_AREA_CODE_KEY
#define NRF_CLOUD_CELL_POS_JSON_KEY_AGE		"age"
#define NRF_CLOUD_CELL_POS_JSON_KEY_T_ADV	"adv"

#define NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN	"earfcn"
#define NRF_CLOUD_CELL_POS_JSON_KEY_PCI		"pci"
#define NRF_CLOUD_CELL_POS_JSON_KEY_NBORS	"nmr"
#define NRF_CLOUD_CELL_POS_JSON_KEY_RSRP	"rsrp"
#define NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ	"rsrq"

#define NRF_CLOUD_CELL_POS_TYPE_VAL_SCELL	"SCELL"
#define NRF_CLOUD_CELL_POS_TYPE_VAL_MCELL	"MCELL"

/* P-GPS */
#define NRF_CLOUD_JSON_PGPS_PRED_COUNT		"predictionCount"
#define NRF_CLOUD_JSON_PGPS_INT_MIN		"predictionIntervalMinutes"
#define NRF_CLOUD_JSON_PGPS_GPS_DAY		"startGpsDay"
#define NRF_CLOUD_JSON_PGPS_GPS_TIME		"startGpsTimeOfDaySeconds"
#define NRF_CLOUD_PGPS_RCV_ARRAY_IDX_HOST	0
#define NRF_CLOUD_PGPS_RCV_ARRAY_IDX_PATH	1
#define NRF_CLOUD_PGPS_RCV_REST_HOST		"host"
#define NRF_CLOUD_PGPS_RCV_REST_PATH		"path"

#define NRF_CLOUD_MSG_TIMESTAMP_KEY		"ts"

/* FOTA */
#define NRF_CLOUD_FOTA_TYPE_MODEM_DELTA		"MODEM"
#define NRF_CLOUD_FOTA_TYPE_MODEM_FULL		"MDM_FULL"
#define NRF_CLOUD_FOTA_TYPE_BOOT		"BOOT"
#define NRF_CLOUD_FOTA_TYPE_APP			"APP"
#define NRF_CLOUD_FOTA_REST_KEY_JOB_DOC		"jobDocument"
#define NRF_CLOUD_FOTA_REST_KEY_JOB_ID		"jobId"
#define NRF_CLOUD_FOTA_REST_KEY_PATH		"path"
#define NRF_CLOUD_FOTA_REST_KEY_HOST		"host"
#define NRF_CLOUD_FOTA_REST_KEY_TYPE		"firmwareType"
#define NRF_CLOUD_FOTA_REST_KEY_SIZE		"fileSize"
#define NRF_CLOUD_FOTA_REST_KEY_VER		"version"

/* REST */
#define NRF_CLOUD_REST_ERROR_CODE_KEY		"code"
#define NRF_CLOUD_REST_ERROR_MSG_KEY		"message"

/**@brief Initialize the codec used encoding the data to the cloud. */
int nrf_cloud_codec_init(void);

/**@brief Encode the sensor data based on the indicated type. */
int nrf_cloud_encode_sensor_data(const struct nrf_cloud_sensor_data *input,
				 struct nrf_cloud_data *output);

/**@brief Encode the sensor data to be sent to the device shadow. */
int nrf_cloud_encode_shadow_data(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output);

/**@brief Encode the user association data based on the indicated type. */
int nrf_cloud_decode_requested_state(const struct nrf_cloud_data *payload,
				     enum nfsm_state *requested_state);

/**@brief Decodes data endpoint information. */
int nrf_cloud_decode_data_endpoint(const struct nrf_cloud_data *input,
				   struct nrf_cloud_data *tx_endpoint,
				   struct nrf_cloud_data *rx_endpoint,
				   struct nrf_cloud_data *bulk_endpoint,
				   struct nrf_cloud_data *m_endpoint);

/** @brief Encodes state information. */
int nrf_cloud_encode_state(uint32_t reported_state, struct nrf_cloud_data *output);

/** @brief Search input for config and encode response if necessary. */
int nrf_cloud_encode_config_response(struct nrf_cloud_data const *const input,
				     struct nrf_cloud_data *const output,
				     bool *const has_config);

/** @brief Encode the device status data into a JSON formatted buffer.
 * The include_state flag controls if the "state" JSON key is included in the output.
 * When calling this function to encode data for use with the UpdateDeviceState nRF Cloud
 * REST endpoint, the "state" key should not be included.
 */
int nrf_cloud_device_status_encode(const struct nrf_cloud_device_status * const dev_status,
				   struct nrf_cloud_data * const output, const bool include_state);

/** @brief Free memory allocated by @ref nrf_cloud_device_status_encode. */
void nrf_cloud_device_status_free(struct nrf_cloud_data *status);

/** @brief Frees memory allocated by @ref nrf_cloud_rest_fota_execution_parse */
void nrf_cloud_fota_job_free(struct nrf_cloud_fota_job_info *const job);

/** @brief Parses the response from a FOTA execution request REST call.
 * If successful, memory will be allocated for the data in @ref nrf_cloud_fota_job_info.
 * The user is responsible for freeing the memory by calling @ref nrf_cloud_fota_job_free.
 */
int nrf_cloud_rest_fota_execution_parse(const char *const response,
					struct nrf_cloud_fota_job_info *const job);

#if defined(CONFIG_NRF_CLOUD_PGPS)
/** @brief Parses the PGPS response (REST and MQTT) from nRF Cloud */
int nrf_cloud_parse_pgps_response(const char *const response,
				  struct nrf_cloud_pgps_result *const result);
#endif

/** @brief Adds common [network] modem info to the provided cJSON object */
int nrf_cloud_json_add_modem_info(cJSON * const data_obj);

/** @brief Builds a cellular positioning request string using the provided cell info.
 * If successful, memory will be allocated for the output string and the user is
 * responsible for freeing it using @ref cJSON_free.
 */
int nrf_cloud_format_cell_pos_req(struct lte_lc_cells_info const *const inf,
				  size_t inf_cnt, char **string_out);

/** @brief Builds a cellular positioning request in the provided cJSON object
 * using the provided cell info
 */
int nrf_cloud_format_cell_pos_req_json(struct lte_lc_cells_info const *const inf,
				       size_t inf_cnt, cJSON * const req_obj_out);

/** @brief Builds a [single-cell] cellular positioning request in the provided cJSON object.
 * Function obtains the necessary network info from the modem.
 */
int nrf_cloud_format_single_cell_pos_req_json(cJSON * const req_obj_out);

/** @brief Parses the cellular positioning response (REST and MQTT) from nRF Cloud. */
int nrf_cloud_parse_cell_pos_response(const char *const buf,
				      struct nrf_cloud_cell_pos_result *result);

/** @brief Checks whether the provided MQTT payload is an nRF Cloud disconnection request */
bool nrf_cloud_detect_disconnection_request(const char *const buf);

/** @brief Obtains a pointer to the string at the specified index in the cJSON array.
 * No memory is allocated, pointer is valid as long as the cJSON array is valid.
 */
int get_string_from_array(const cJSON * const array, const int index,
			  char **string_out);

/** @brief Obtains a pointer to the string of the specified key in the cJSON object.
 * No memory is allocated, pointer is valid as long as the cJSON object is valid.
 */
int get_string_from_obj(const cJSON * const obj, const char *const key,
			char **string_out);

/** @brief Sends the cJSON object to nRF Cloud on the d2c topic */
int json_send_to_cloud(cJSON * const request);

/** @brief Creates a cJSON object containing the specified appId and messageType.
 * If successful, user is responsible for calling @ref cJSON_Delete to free
 * the cJSON object's memory.
 */
cJSON *json_create_req_obj(const char *const app_id, const char *const msg_type);

int nrf_cloud_parse_rest_error(const char *const buf, enum nrf_cloud_error *const err);

#ifdef CONFIG_NRF_CLOUD_GATEWAY
typedef int (*gateway_state_handler_t)(void *root_obj);

/** @brief Registers a callback, which is called whenever the shadow changes.
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

#endif /* NRF_CLOUD_CODEC_H__ */
