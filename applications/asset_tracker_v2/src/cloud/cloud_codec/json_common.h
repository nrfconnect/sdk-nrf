/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 * @brief JSON common library header.
 */

#ifndef JSON_COMMON_H__
#define JSON_COMMON_H__

/**@file
 *
 * @defgroup JSON common json_common
 * @brief    Module containing common JSON encoding functions.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <cJSON.h>

#include "cloud_codec.h"
#include "json_protocol_names.h"

/** @brief Type of data to be handled by the respective API. Used to signify what data structure
 *         that is passed in to the function.
 */
enum json_common_buffer_type {
	JSON_COMMON_UI,
	JSON_COMMON_MODEM_STATIC,
	JSON_COMMON_MODEM_DYNAMIC,
	JSON_COMMON_GPS,
	JSON_COMMON_SENSOR,
	JSON_COMMON_ACCELEROMETER,
	JSON_COMMON_BATTERY,

	JSON_COMMON_COUNT
};

/** @brief Operation to be carried out with the passed in data. */
enum json_common_op_code {
	JSON_COMMON_INVALID,
	/** Encode data and add it to a passed in parent array object. This option does not
	 *  label the encoded data.
	 */
	JSON_COMMON_ADD_DATA_TO_ARRAY,
	/** Encode data and add it to a passed in parent object. */
	JSON_COMMON_ADD_DATA_TO_OBJECT,
	/** Encode data and set the passed in object pointer to point to it. */
	JSON_COMMON_GET_POINTER_TO_OBJECT
};

/**
 * @brief Encode and add static modem data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_modem_static_data_add(cJSON *parent,
				      struct cloud_data_modem_static *data,
				      enum json_common_op_code op,
				      const char *object_label,
				      cJSON **parent_ref);

/**
 * @brief Encode and add dynamic modem data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_modem_dynamic_data_add(cJSON *parent,
				       struct cloud_data_modem_dynamic *data,
				       enum json_common_op_code op,
				       const char *object_label,
				       cJSON **parent_ref);

/**
 * @brief Encode and add environmental sensor data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_sensor_data_add(cJSON *parent,
				struct cloud_data_sensors *data,
				enum json_common_op_code op,
				const char *object_label,
				cJSON **parent_ref);

/**
 * @brief Encode and add GPS data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_gps_data_add(cJSON *parent,
			     struct cloud_data_gps *data,
			     enum json_common_op_code op,
			     const char *object_label,
			     cJSON **parent_ref);

/**
 * @brief Encode and add accelerometer data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_accel_data_add(cJSON *parent,
			       struct cloud_data_accelerometer *data,
			       enum json_common_op_code op,
			       const char *object_label,
			       cJSON **parent_ref);

/**
 * @brief Encode and add User Interface data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_ui_data_add(cJSON *parent,
			    struct cloud_data_ui *data,
			    enum json_common_op_code op,
			    const char *object_label,
			    cJSON **parent_ref);

/**
 * @brief Encode and add neighbor cell data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 *
 * @retval 0 on success.
 * @retval -ENODATA if the passed in data is not queued or has an invalid timestamp value.
 * @retval -EINVAL if the passed in input arguments does not match whats required by the OP code.
 * @retval -ENOMEM if the function fails to allocate memory.
 */
int json_common_neighbor_cells_data_add(cJSON *parent,
					struct cloud_data_neighbor_cells *data,
					enum json_common_op_code op);

/**
 * @brief Encode and add battery data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] op Operation that is to be carried out.
 * @param[in] object_label Name of the encoded object.
 * @param[out] parent_ref Reference to an unallocated parent object pointer. Used when getting the
 *			  pointer to the encoded data object when setting
 *			  JSON_COMMON_GET_POINTER_TO_OBJECT as the opcode. The cJSON object pointed
 *			  to after this function call must be manually freed after use.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_battery_data_add(cJSON *parent,
				 struct cloud_data_battery *data,
				 enum json_common_op_code op,
				 const char *object_label,
				 cJSON **parent_ref);

/**
 * @brief Encode and add configuration data to the parent object.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] data Pointer to data that is to be encoded.
 * @param[in] object_label Name of the encoded object.
 *
 * @return 0 on success. Otherwise a negative error code is returned.
 */
int json_common_config_add(cJSON *parent, struct cloud_data_cfg *data, const char *object_label);

/**
 * @brief Extract configuration values from parent object.
 *
 * @param[in] parent Pointer to object that the configuration is to be extracted from.
 * @param[out] data Pointer to data structure that will be populated with the extracted
 *                  configuration values.
 */
void json_common_config_get(cJSON *parent, struct cloud_data_cfg *data);

/**
 * @brief Encode all queued entries in the passed in buffer and add it to the parent object
 *        as an array.
 *
 * @param[out] parent Pointer to object that the encoded data is added to.
 * @param[in] type Type of data passed in to the function.
 * @param[in] buf Pointer to data buffer that is to be encoded.
 * @param[in] buf_count Number of entries in passed in data buffer.
 * @param[in] object_label Name of the array entry that is added to the parent object.
 *
 * @return 0 on success. -ENODATA if the passed in data is not valid. Otherwise a negative error
 *         code is returned.
 */
int json_common_batch_data_add(cJSON *parent, enum json_common_buffer_type type, void *buf,
			       size_t buf_count, const char *object_label);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* JSON_COMMON_H__ */
