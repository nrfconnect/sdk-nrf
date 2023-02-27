/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CODEC_H__
#define NRF_CLOUD_CODEC_H__

#include <net/nrf_cloud.h>
//#include <zcbor_common.h>
#include <net/wifi_location_common.h>
#include <modem/lte_lc.h>

#include <cJSON.h>
#include <modem/lte_lc.h>
#include <net/wifi_location_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup nrf_cloud_codec nRF Cloud Codec
 * @{
 */

/** @brief Object types for encoding and decoding */
enum nrf_cloud_obj_type {
	NRF_CLOUD_OBJ_TYPE__UNDEFINED,

	NRF_CLOUD_OBJ_TYPE_JSON,
	NRF_CLOUD_OBJ_TYPE_CBOR,

	NRF_CLOUD_OBJ_TYPE__LAST,
};

/** @brief Encoded data sources */
enum nrf_cloud_enc_src {
	/** Encoded data is not set */
	NRF_CLOUD_ENC_SRC_NONE,
	/** Encoded data was set by @ref nrf_cloud_obj_cloud_encode */
	NRF_CLOUD_ENC_SRC_CLOUD_ENCODED,
	/** Encoded data was set manually or by @ref NRF_CLOUD_OBJ_PRE_ENC_DEFINE */
	NRF_CLOUD_ENC_SRC_PRE_ENCODED,
};

/** @brief Object used for building nRF Cloud messages. */
struct nrf_cloud_obj {

	enum nrf_cloud_obj_type type;
	union {
		cJSON *json;
		// TODO
		void *cbor; //struct zcbor_state_t cbor;
	};

	/** Source of encoded data  */
	enum nrf_cloud_enc_src enc_src;
	/** Encoded data */
	struct nrf_cloud_data encoded_data;
};

/** @brief Define an nRF Cloud JSON object.
 *
 * This macro defines a codec object with the type of NRF_CLOUD_OBJ_TYPE_JSON.
 *
 * @param _name	Name of the object.
 */
#define NRF_CLOUD_OBJ_JSON_DEFINE(_name) \
	struct nrf_cloud_obj _name = { .type = NRF_CLOUD_OBJ_TYPE_JSON, .json = NULL, \
				       .enc_src = NRF_CLOUD_ENC_SRC_NONE, \
				       .encoded_data = { .ptr = NULL, .len = 0 } }

/** @brief Define an nRF Cloud CBOR object.
 *
 * This macro defines a codec object with the type of NRF_CLOUD_OBJ_TYPE_CBOR.
 *
 * @param _name	Name of the object.
 */
#define NRF_CLOUD_OBJ_CBOR_DEFINE(_name) \
	struct nrf_cloud_obj _name = { .type = NRF_CLOUD_OBJ_TYPE_CBOR, \
				       .enc_src = NRF_CLOUD_ENC_SRC_NONE, \
				       .encoded_data = { .ptr = NULL, .len = 0 } }

/** @brief Define an nRF Cloud codec object of the specified type.
 *
 * @param _name	Name of the object.
 * @param _type	Type of the object.
 */
#define NRF_CLOUD_OBJ_DEFINE(_name, _type) \
	struct nrf_cloud_obj _name = { 0 }; \
	_name.type = _type; \
	_name.enc_src = NRF_CLOUD_ENC_SRC_NONE;

/** @brief Define an nRF Cloud object with pre-encoded data.
 *
 * This macro defines a codec object with the type of NRF_CLOUD_OBJ_TYPE__UNDEFINED
 * and initializes the encoded data with the provided parameters.
 *
 * @param _name	Name of the object.
 * @param _data	Pointer to pre-encoded data.
 * @param _len	Size of the pre-encoded data.
 */
#define NRF_CLOUD_OBJ_PRE_ENC_DEFINE(_name, _data, _len) \
	struct nrf_cloud_obj _name = { .type = NRF_CLOUD_OBJ_TYPE__UNDEFINED, \
				       .enc_src = NRF_CLOUD_ENC_SRC_PRE_ENCODED, \
				       .encoded_data = { .ptr = _data, .len = _len } }

/** @brief Check if the provided object is a valid nRF Cloud codec object type.
 *
 * @param _obj_ptr Object to check for a valid type.
 *
 * @retval true  Type is valid.
 * @retval false Type is invalid.
 */
#define NRF_CLOUD_OBJ_TYPE_VALID(_obj_ptr) \
	(bool)((_obj_ptr->type > NRF_CLOUD_OBJ_TYPE__UNDEFINED) && \
	       (_obj_ptr->type < NRF_CLOUD_OBJ_TYPE__LAST))

/**
 * @brief Decode data received from nRF Cloud.
 *
 * @details If successful, memory is allocated for the provided object.
 * 	    The @ref nrf_cloud_obj_free function should be called when finished with the object.
 *
 * @param[out] obj Object to be initialized with the decoded data.
 * @param[in] input Data received from nRF Cloud.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOMSG Unable to decode data to a known type.
 * @retval 0 Data successfully decoded.
 */
int nrf_cloud_obj_input_decode(struct nrf_cloud_obj *const obj,
			       const struct nrf_cloud_data *const input);

/**
 * @brief Check if the object contains the specified app ID and message type.
 *
 * @param[in] obj Object to check.
 * @param[in] app_id Desired app ID. If NULL, app ID is not checked.
 * @param[in] msg_type Desired message type. If NULL, message type is not checked.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval -ENOMSG Object does not contain the specified values.
 * @retval 0 Success; object contains the specified values.
 */
int nrf_cloud_obj_msg_check(const struct nrf_cloud_obj *const obj, const char *const app_id,
			    const char *const msg_type);

/**
 * @brief Get the string value associated with the provided key.
 *
 * @param[in] obj Object containing the key and value.
 * @param[in] key Key.
 * @param[out] str String associated with the provided key.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENODEV Object does not contain the provided key.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval -ENOMSG Value associated with the key is not a string.
 * @retval 0 Success; string found.
 */
int nrf_cloud_obj_str_get(const struct nrf_cloud_obj *const obj, const char *const key,
			  char **str);

/**
 * @brief Initialize an object as an nRF Cloud device message.
 *
 * @details If successful, memory is allocated for the provided object.
 * 	    The @ref nrf_cloud_obj_free function should be called when finished with the object.
 *
 * @param[in/out] obj Object to initialize.
 * @param[in] app_id The desired app ID of the message.
 * @param[in] msg_type The desired message type; can be NULL.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTEMPTY Object already initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; message initialized.
 */
int nrf_cloud_obj_msg_init(struct nrf_cloud_obj *const obj, const char *const app_id,
			   const char *const msg_type);


/**
 * @brief Initialize an object as an nRF Cloud bulk message.
 *
 * @details If successful, memory is allocated for the provided object.
 * 	    The @ref nrf_cloud_obj_free function should be called when finished with the object.
 *
 * @param[in/out] bulk Object to initialize.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTEMPTY Object already initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; bulk message initialized.
 */
int nrf_cloud_obj_bulk_init(struct nrf_cloud_obj *const bulk);

/**
 * @brief Initialize an empty object.
 *
 * @details If successful, memory is allocated for the provided object.
 * 	    The @ref nrf_cloud_obj_free function should be called when finished with the object.
 *
 * @param[in/out] obj Object to initialize.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTEMPTY Object already initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; object initialized.
 */
int nrf_cloud_obj_init(struct nrf_cloud_obj *const obj);

/**
 * @brief Reset the state of an object; does not free memory
 *
 * @param[in/out] obj Object to reset.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; object initialized.
 */
int nrf_cloud_obj_reset(struct nrf_cloud_obj *const obj);

/**
 * @brief Free the memory of an initialized object.
 *
 * @details Frees the object's memory; use @ref nrf_cloud_obj_cloud_encoded_free to free encoded data.
 *
 * @param[in/out] obj Object to initialize.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; memory freed.
 */
int nrf_cloud_obj_free(struct nrf_cloud_obj *const obj);

/**
 * @brief Add an object to a bulk message object.
 *
 * @details If successful, the object belongs to the bulk message and should not be freed directly.
 *
 * @param[in/out] bulk Bulk container object.
 * @param[in] obj Object to add.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENODEV Not a bulk container object.
 * @retval -EIO Error adding object.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_bulk_add(struct nrf_cloud_obj *const bulk, struct nrf_cloud_obj *const obj);

/**
 * @brief Add a timestamp to an object.
 *
 * @param[in/out] bulk Bulk container object.
 * @param[in] obj Object to add.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_ts_add(struct nrf_cloud_obj *const obj, const int64_t time_ms);

/**
 * @brief Add a key string and number value to the provided object.
 *
 * @param[in/out] obj Object to contain key and value.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] val Number value.
 * @param[in] data_child If true, key and number will be added as a child to a "data" object.
 *                       If false, key and number will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_num_add(struct nrf_cloud_obj *const obj, const char *const key,
			  const double val, const bool data_child);

/**
 * @brief Add a key string and string value to the provided object.
 *
 * @param[in/out] obj Object to contain key and value.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] val String value.
 * @param[in] data_child If true, key and value will be added as a child to a "data" object.
 *                       If false, key and value will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_str_add(struct nrf_cloud_obj *const obj, const char *const key,
			  const char *const val, const bool data_child);

/**
 * @brief Add a key string and boolean value to the provided object.
 *
 * @param[in/out] obj Object to contain key and value.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] val Boolean value.
 * @param[in] data_child If true, key and value will be added as a child to a "data" object.
 *                       If false, key and value will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_bool_add(struct nrf_cloud_obj *const obj, const char *const key,
			   const bool val, const bool data_child);

/**
 * @brief Add a key string and null value to the provided object.
 *
 * @param[in/out] obj Object to contain key and value.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] data_child If true, key and value will be added as a child to a "data" object.
 *                       If false, key and value will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_null_add(struct nrf_cloud_obj *const obj, const char *const key,
			   const bool data_child);

/**
 * @brief Add a key string and object to the provided object.
 *
 * @param[in/out] obj Object to contain key and object.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] obj_to_add Object.
 * @param[in] data_child If true, key and value will be added as a child to a "data" object.
 *                       If false, key and value will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_object_add(struct nrf_cloud_obj *const obj, const char *const key,
			     struct nrf_cloud_obj *const obj_to_add, const bool data_child);

/**
 * @brief Add a key string and integer array value to the provided object.
 *
 * @param[in/out] obj Object to contain key and value.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] ints Integer array.
 * @param[in] ints_cnt Number of items in array.
 * @param[in] data_child If true, key and value will be added as a child to a "data" object.
 *                       If false, key and value will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_int_array_add(struct nrf_cloud_obj *const obj, const char *const key,
				const uint32_t ints[], const uint32_t ints_cnt,
				const bool data_child);

/**
 * @brief Add a key string and string array value to the provided object.
 *
 * @param[in/out] obj Object to contain key and value.
 * @param[in] key Key string; must be valid and constant for the life of the object.
 * @param[in] strs String array.
 * @param[in] strs_cnt Number of items in array.
 * @param[in] data_child If true, key and value will be added as a child to a "data" object.
 *                       If false, key and value will be added as a child to the provided object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; item added.
 */
int nrf_cloud_obj_str_array_add(struct nrf_cloud_obj *const obj, const char *const key,
				const char *const strs[], const uint32_t strs_cnt,
				const bool data_child);

/**
 * @brief Encode the object's data for transport to nRF Cloud.
 *
 * @details If successful, memory is allocated for the encoded data.
 *          The @ref nrf_cloud_obj_cloud_encoded_free function should
 *          be called when finished with the object.
 *
 * @param[in/out] obj Object to encode.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; object initialized.
 */
int nrf_cloud_obj_cloud_encode(struct nrf_cloud_obj *const obj);

/**
 * @brief Free the memory of the encoded data in the object.
 *
 * @details Frees the memory allocated by @ref nrf_cloud_obj_cloud_encode.
 *
 * @param[in/out] obj Object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -EACCES Encoded data was not encoded by @ref nrf_cloud_obj_cloud_encode.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval 0 Success; memory freed.
 */
int nrf_cloud_obj_cloud_encoded_free(struct nrf_cloud_obj *const obj);

/**
 * @brief Free the memory of the pre-encoded data in the object.
 *
 * @param[in/out] obj Object.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -EACCES Encoded data was not set with @ref NRF_CLOUD_OBJ_PRE_ENC_DEFINE.
 * @retval 0 Success; memory freed.
 */
int nrf_cloud_obj_cloud_pre_encoded_free(struct nrf_cloud_obj *const obj);

/**
 * @brief Create an nRF Cloud GNSS message object.
 *
 * @details If successful, memory is allocated for the provided object.
 * 	    The @ref nrf_cloud_obj_free function should be called when finished with the object.
 *
 * @param[in/out] obj Uninitialzed object to contain the GNSS message.
 * @param[in] gnss GNSS data.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -EFTYPE Invalid object type.
 * @retval -ENOTEMPTY Object already initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOSYS Modem library (NRF_MODEM) not enabled.
 * @retval -EFBIG NMEA string too large, see NRF_MODEM_GNSS_NMEA_MAX_LEN.
 * @retval -EPROTO Unhandled GNSS type.
 * @retval 0 Success; GNSS message created.
 */
int nrf_cloud_obj_gnss_msg_create(struct nrf_cloud_obj *const obj,
				  const struct nrf_cloud_gnss_data * const gnss);

/**
 * @brief Create an nRF Cloud Location request message object.
 *
 * @details If successful, memory is allocated for the provided object.
 * 	    The @ref nrf_cloud_obj_free function should be called when finished with the object.
 *
 * @param[in/out] obj Uninitialzed object to contain the Location message.
 * @param[in] cells_inf Cellular network data, can be NULL if wifi_inf is provided.
 * @param[in] wifi_inf Wi-Fi network data, can be NULL if cells_inf is provided.
 * @param[in] request_loc If true, nRF Cloud will send the location data to the device.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -EDOM Too few Wi-Fi networks, see NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN.
 * @retval -EFTYPE Invalid object type.
 * @retval -ENOTEMPTY Object already initialized.
 * @retval -ENOMEM Out of memory.
 * @retval 0 Success; GNSS message created.
 */
int nrf_cloud_obj_location_request_create(struct nrf_cloud_obj *const obj,
					  const struct lte_lc_cells_info *const cells_inf,
					  const struct wifi_scan_info *const wifi_inf,
					  const bool request_loc);

/**
 * @brief Add PVT data to the provided object.
 *
 * @param[in/out] obj Object to contain the PVT data.
 * @param[in] pvt PVT data.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -EFTYPE Invalid object type.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval 0 Success; PVT data added to object.
 */
int nrf_cloud_obj_pvt_add(struct nrf_cloud_obj *const obj,
			  const struct nrf_cloud_gnss_pvt * const pvt);

/**
 * @brief Add modem PVT data to the provided object.
 *
 * @param[in/out] obj Object to contain the modem PVT data.
 * @param[in] pvt Modem PVT data.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -EFTYPE Invalid object type.
 * @retval -ENOTSUP Action not supported for the object's type.
 * @retval -ENOENT Object is not initialized.
 * @retval -ENOMEM Out of memory.
 * @retval -ENOSYS Modem library (NRF_MODEM) not enabled.
 * @retval 0 Success; modem PVT data added to object.
 */
int nrf_cloud_obj_modem_pvt_add(struct nrf_cloud_obj *const obj,
				const struct nrf_modem_gnss_pvt_data_frame * const mdm_pvt);

/**
 * @brief Create an nRF Cloud GNSS device message using the provided GNSS data.
 *
 * @param[in]     gnss     Service info to add.
 * @param[in,out] gnss_msg_obj cJSON object to which the GNSS message will be added.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_gnss_msg_json_encode(const struct nrf_cloud_gnss_data * const gnss,
				   cJSON * const gnss_msg_obj);

/**
 * @brief Create an nRF Cloud location request device message using the provided
 *        cellular and/or Wi-Fi data.
 *
 * @param cells_inf Cell info; can be NULL if Wi-Fi info is provided.
 * @param wifi_inf Wi-Fi info; can be NULL if cell info is provided.
 * @param request_loc If true, cloud will send location to the device.
 *                    If false, cloud will not send location to the device.
 * @param loc_req_obj cJSON object to which the location request message will be added.
 *
 * @retval 0 If successful.
 * @retval -EDOM The number of access points in the Wi-Fi-only request was smaller than
 *               the minimum required value NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN.
 * @return A negative value indicates an error.
 */
int nrf_cloud_location_request_msg_json_encode(const struct lte_lc_cells_info * const cells_inf,
					       const struct wifi_scan_info * const wifi_inf,
					       const bool request_loc,
					       cJSON * const loc_req_obj);

/**
 * @brief Add service info into the provided cJSON object.
 *
 * @param[in]     svc_inf     Service info to add.
 * @param[in,out] svc_inf_obj cJSON object to which the service info will be added.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_service_info_json_encode(const struct nrf_cloud_svc_info *const svc_inf,
				       cJSON * const svc_inf_obj);

/**
 * @brief Add modem info into the provided cJSON object.
 *
 * @note To add modem info, CONFIG_MODEM_INFO must be enabled.
 *
 * @param[in]     mod_inf     Modem info to add.
 * @param[in,out] mod_inf_obj cJSON object to which the modem info will be added.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_modem_info_json_encode(const struct nrf_cloud_modem_info *const mod_inf,
				     cJSON * const mod_inf_obj);

/**
 * @brief Check for a JSON error message in the data received from nRF Cloud over MQTT.
 *
 * @param[in] buf Data received from nRF Cloud.
 * @param[in] app_id appId value to check for.
 *                   Set to NULL to skip appID check.
 * @param[in] msg_type messageType value to check for.
 *                     Set to NULL to skip messageType check.
 * @param[out] err Error code found in message.
 *
 * @retval 0 Error code found (and matched app_id and msg_type if provided).
 * @retval -ENOENT Error code found, but did not match specified the app_id and msg_type.
 * @retval -ENOMSG No error code found.
 * @retval -EBADMSG Invalid error code data format.
 * @retval -ENODATA JSON data was not found.
 * @return A negative value indicates an error.
 */
int nrf_cloud_error_msg_decode(const char * const buf,
			       const char * const app_id,
			       const char * const msg_type,
			       enum nrf_cloud_error * const err);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CODEC_H__ */
