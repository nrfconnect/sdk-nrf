/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_DISCOVERY_MOCK_H_
#define BT_GATT_DISCOVERY_MOCK_H_

#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


/**
 * @file
 * @defgroup bt_gatt_discover_mock API
 * @{
 * @brief The API used to setup the mock for bt_gatt_discover
 */

/**
 * @brief Service definition
 *
 * Macro to set attribute that stands for the service header.
 *
 * @note
 * This macro is used only for attribute created for bt_gatt_discover mock.
 *
 * @param _handle     The handler of the service attribute.
 * @param _uuid       The UUID of the service itself.
 * @param _end_handle The value of the end handle.
 *
 * @note
 * The @c user_data field is set to point to const struct.
 * This way we can assume that created structure would be placed in FLASH.
 * Then if we try to write it, the test would hard fault.
 */
#define BT_GATT_DISCOVER_MOCK_SERV(_handle, _uuid, _end_handle) {         \
		.uuid = BT_UUID_GATT_PRIMARY,                             \
		.handle = _handle,                                        \
		.user_data = (void *)(&(const struct bt_gatt_service_val) \
			{ .uuid = _uuid, .end_handle = _end_handle})      \
	}

/**
 * @brief Characteristic definition
 *
 * Macro to set attribute that stands for characteristic handler.
 *
 * @note
 * This macro is used only for attribute created for bt_gatt_discover mock.
 *
 * @note The value of the characteristic should be added using
 * @ref BT_GATT_DISCOVER_MOCK_DESC macro.
 *
 * @param _handle The handler of the characteristic attribute.
 * @param _uuid   The UUID of the characteristic itself.
 * @param _props  Characteristic properties.
 */
#define BT_GATT_DISCOVER_MOCK_CHRC(_handle, _uuid, _props) {       \
		.uuid = BT_UUID_GATT_CHRC,                         \
		.handle = _handle,                                 \
		.user_data = (void *)(&(const struct bt_gatt_chrc) \
			{ .uuid = _uuid, .properties = _props })   \
	}

/**
 * @brief Descriptor definition
 *
 * Macro to set attribute that stands for descriptor handler.
 *
 * @note
 * This macro is used only for attribute created for bt_gatt_discover mock.
 *
 * @param _handle The handler of the characteristic attribute.
 * @param _uuid   The UUID of the descriptor.
 */
#define BT_GATT_DISCOVER_MOCK_DESC(_handle, _uuid) { \
		.uuid = _uuid,                       \
		.handle = _handle                    \
	}

/**
 * @brief GATT discover mock setup
 *
 * This function setups the mock for @ref bt_gatt_discover function.
 *
 * @param attr The array of the attribute
 * @param len  The size of the array
 */
void bt_gatt_discover_mock_setup(const struct bt_gatt_attr *attr, size_t len);

/** @} */
#endif /* #define BT_GATT_DISCOVERY_MOCK_H_ */
