/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MDS_H_
#define MDS_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

/**@file
 * @defgroup bt_mds Memfault Diagnostic GATT service API
 * @{
 * @brief API for the Memfault Diagnostic GATT service (MDS).
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the Memfault Diagnostic Service. */
#define BT_UUID_MDS_VAL \
	BT_UUID_128_ENCODE(0x54220000, 0xf6a5, 0x4007, 0xa371, 0x722f4ebd8436)

/** @brief UUID of the MDS Supported Features Characteristic. */
#define BT_UUID_MDS_SUPPORTED_FEATURES_VAL \
	BT_UUID_128_ENCODE(0x54220001, 0xf6a5, 0x4007, 0xa371, 0x722f4ebd8436)

/** @brief UUID of the MDS Device Identifier Characteristic. */
#define BT_UUID_MDS_DEVICE_IDENTIFIER_VAL \
	BT_UUID_128_ENCODE(0x54220002, 0xf6a5, 0x4007, 0xa371, 0x722f4ebd8436)

/** @brief UUID of the MDS Data URI Characteristic. */
#define BT_UUID_MDS_DATA_URI_VAL \
	BT_UUID_128_ENCODE(0x54220003, 0xf6a5, 0x4007, 0xa371, 0x722f4ebd8436)

/** @brief UUID of the MDS Authorization Characteristic. */
#define BT_UUID_MDS_AUTHORIZATION_VAL \
	BT_UUID_128_ENCODE(0x54220004, 0xf6a5, 0x4007, 0xa371, 0x722f4ebd8436)

/** @brief UUID of the MDS Data Export Characteristic. */
#define BT_UUID_MDS_DATA_EXPORT_VAL \
	BT_UUID_128_ENCODE(0x54220005, 0xf6a5, 0x4007, 0xa371, 0x722f4ebd8436)

#define BT_UUID_MEMFAULT_DIAG          BT_UUID_DECLARE_128(BT_UUID_MDS_VAL)
#define BT_UUID_MDS_SUPPORTED_FEATURES BT_UUID_DECLARE_128(BT_UUID_MDS_SUPPORTED_FEATURES_VAL)
#define BT_UUID_MDS_DEVICE_IDENTIFIER  BT_UUID_DECLARE_128(BT_UUID_MDS_DEVICE_IDENTIFIER_VAL)
#define BT_UUID_MDS_DATA_URI           BT_UUID_DECLARE_128(BT_UUID_MDS_DATA_URI_VAL)
#define BT_UUID_MDS_AUTHORIZATION      BT_UUID_DECLARE_128(BT_UUID_MDS_AUTHORIZATION_VAL)
#define BT_UUID_MDS_DATA_EXPORT        BT_UUID_DECLARE_128(BT_UUID_MDS_DATA_EXPORT_VAL)

/** @brief Memfault Diagnostic Service callback structure.
 */
struct bt_mds_cb {
	/** @brief A callback for enabling Memfault access.
	 *
	 * If this callback is not implemented, any connected peer can access the Memfault
	 * characteristics and descriptors data, including write access to them, without
	 * authentication.
	 *
	 * The Memfault Diagnostic service allows only one client subscription.
	 *
	 * @param[in] conn Connection object.
	 *
	 * @retval True if peer associated with connection object has granted access to
	 *         the Memfault data.
	 *	   False if access to the Memfault data is forbidden.
	 */
	bool (*access_enable)(struct bt_conn *conn);
};

/** @brief Register the Memfault Diagnostic service callback.
 *
 * This function should be called before enabling Bluetooth stack to ensure proper access grating
 * to MDS characteristics data through @p access_enable callback.
 *
 * @param[in] cb Memfault callback structure. This parameter can be set to NULL but it will let the
 *               first connected client access the Memfault data without authentication.
 *
 *  @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned
 */
int bt_mds_cb_register(const struct bt_mds_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* MDS_H_ */
