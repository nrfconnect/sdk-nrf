/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef __cplusplus
extern "C" {
#endif

#define ATTRIBUTE_ID_ONOFF 1
#define ATTRIBUTE_ID_LEVEL_CONTROL 2

struct aws_iot_integration_cb_data {
	uint32_t attribute_id;
	uint32_t value;

	/* Error code set if an irrecoverable error has ocurred in the library. (errno.h) */
	int error;
};

/** @brief Handler used to receive attribute change request from AWS IoT.
 *
 *  @param[in] event Pointer to a structure containing attribute change details.
 *
 *  @retval true if the attribute change was successfully applied.
 *  @retval false if the attribute change failed to be applied.
 */
typedef bool (*aws_iot_integration_evt_handler_t)(struct aws_iot_integration_cb_data *data);

/** @brief Function used to register a callback to receive attribute changes.
 *
 *  @param[in] callback Pointer the callback.
 *
 *  @returns -EINVAL if callback is NULL.
 */
int aws_iot_integration_register_callback(aws_iot_integration_evt_handler_t callback);

/** @brief Function that updates the supported attribute's state and notifies AWS IoT.
 *
 *  @param[in] id Attribute ID.
 *  @param[in] value Attribute value.
 */
void aws_iot_integration_attribute_set(uint32_t id, uint32_t value);

#ifdef __cplusplus
}
#endif
