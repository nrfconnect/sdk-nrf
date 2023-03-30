/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_BMS_H_
#define BT_BMS_H_

/**@file
 * @defgroup bt_bms Bond Management Service API
 * @{
 * @brief API for the Bond Management Service (BMS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <zephyr/types.h>

/** @brief BMS Control Point operation codes (LE transport). */
enum bt_bms_op {
	/** Initiates the procedure to delete the bond of
	 *  the requesting device.
	 */
	BT_BMS_OP_DEL_REQ_BOND = 0x03,

	/** Initiates the procedure to delete all bonds on the device. */
	BT_BMS_OP_DEL_ALL_BONDS = 0x06,

	/** Initiates the procedure to delete all bonds except for the one of
	 *  the requesting device.
	 */
	BT_BMS_OP_DEL_REST_BONDS = 0x09,
};

/** @brief BMS authorization callback parameters. */
struct bt_bms_authorize_params {
	/** Operation code. */
	enum bt_bms_op op_code;

	/** Authorization Code. */
	const uint8_t *code;

	/** Length of Authorization Code. */
	uint16_t code_len;
};

/** @brief BMS server callback structure. */
struct bt_bms_cb {
	/** Operation authorization callback
	 *
	 *  If this callback is set, support for BMS Authorization Code
	 *  functionality is enabled. This function is called whenever
	 *  an authorized operation is requested by the client.
	 *
	 *  @param conn   Connection object.
	 *  @param params Authorization parameters.
	 *
	 * @retval true   If the authorization was successful.
	 * @retval false  Otherwise.
	 */
	bool (*authorize)(struct bt_conn *conn,
			  struct bt_bms_authorize_params *params);
};

/** @brief Bitmask of supported features. */
struct bt_bms_feature {
	/** Support the feature. */
	uint8_t supported : 1;

	/** Enable authorization. */
	uint8_t authorize : 1;
};

/** @brief Bitmask set of supported features. */
struct bt_bms_features {
	/** Support the operation to delete all bonds. */
	struct bt_bms_feature delete_all;

	/** Support the operation to delete the bonds of
	 *  the requesting device.
	 */
	struct bt_bms_feature delete_requesting;

	/** Support the operation to delete all bonds except for the bond of
	 *  the requesting device.
	 */
	struct bt_bms_feature delete_rest;
};

/** @brief BMS initialization parameters. */
struct bt_bms_init_params {
	/** Callbacks. */
	struct bt_bms_cb *cbs;
	/** Features. */
	struct bt_bms_features features;
};

/** @brief Initialize the BMS Service.
 *
 *  Initialize the BMS Service by specifying a list of supported operations.
 *  If any operation is configured as authorized, you need to provide
 *  authorize() callback.
 *
 *  @param init_params Initialization parameters.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int bt_bms_init(const struct bt_bms_init_params *init_params);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_BMS_H_ */
