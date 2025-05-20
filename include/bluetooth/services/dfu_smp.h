/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_DFU_SMP_H_
#define BT_DFU_SMP_H_

/**
 * @file
 * @defgroup bt_dfu_smp Bluetooth LE GATT DFU SMP Client API
 * @{
 * @brief API for the Bluetooth LE GATT DFU SMP (DFU_SMP) Client.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>


/** @brief SMP Service.
 *
 *  The 128-bit service UUID is 8D53DC1D-1DB7-4CD3-868B-8A527460AA84.
 */
#define BT_UUID_DFU_SMP_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x8D53DC1D, 0x1DB7, 0x4CD3, 0x868B, 0x8A527460AA84)

#define BT_UUID_DFU_SMP_SERVICE BT_UUID_DECLARE_128(BT_UUID_DFU_SMP_SERVICE_VAL)

/** @brief SMP Characteristic.
 *
 *  The characteristic is used for both requests and responses.
 *  The UUID is DA2E7828-FBCE-4E01-AE9E-261174997C48.
 */
#define BT_UUID_DFU_SMP_CHAR_VAL \
	BT_UUID_128_ENCODE(0xDA2E7828, 0xFBCE, 0x4E01, 0xAE9E, 0x261174997C48)


#define BT_UUID_DFU_SMP_CHAR BT_UUID_DECLARE_128(BT_UUID_DFU_SMP_CHAR_VAL)

/* Forward declaration required for function handlers. */
struct bt_dfu_smp;

/** @brief Header used internally by dfu_smp.
 */
struct bt_dfu_smp_header {
	/** Operation. */
	uint8_t op;
	/** Operational flags. */
	uint8_t flags;
	/** Payload length high byte. */
	uint8_t len_h8;
	/** Payload length low byte. */
	uint8_t len_l8;
	/** Group ID high byte. */
	uint8_t group_h8;
	/** Group ID low byte. */
	uint8_t group_l8;
	/** Sequence number. */
	uint8_t seq;
	/** Command ID. */
	uint8_t id;
};

/** @brief Current response state.
 */
struct bt_dfu_smp_rsp_state {
	/** @brief Total size of the response.
	 *
	 *  Total expected size of the response.
	 *  This value is retrieved from the first part of the
	 *  response, where the SMP header is located.
	 */
	size_t total_size;
	/** @brief Current data chunk offset.  */
	size_t offset;
	/** @brief The size of the data chunk. */
	size_t chunk_size;
	/** @brief Pointer to the data.        */
	const uint8_t *data;
};

/** @brief Handle part of the response.
 *
 *  @param dfu_smp DFU SMP Client instance.
 */
typedef void (*bt_dfu_smp_rsp_part_cb)(struct bt_dfu_smp *dfu_smp);

/** @brief Global function that is called when an error occurs.
 *
 *  @param dfu_smp DFU SMP Client instance.
 *  @param err       Negative internal error code or positive GATT error code.
 */
typedef void (*bt_dfu_smp_error_cb)(struct bt_dfu_smp *dfu_smp,
					 int err);

/**
 * @brief DFU SMP Client parameters for the initialization function.
 */
struct bt_dfu_smp_init_params {
	/** @brief Callback pointer for error handler.
	 *
	 *  This function is called when an error occurs.
	 */
	bt_dfu_smp_error_cb error_cb;
};

/**
 * @brief DFU SMP Client structure.
 */
struct bt_dfu_smp {
	/** Connection object. */
	struct bt_conn *conn;
	/** Handles. */
	struct bt_dfu_smp_handles {
		/** SMP characteristic value handle. */
		uint16_t smp;
		/** SMP characteristic CCC handle. */
		uint16_t smp_ccc;
	} handles;
	/** Current response state. */
	struct bt_dfu_smp_rsp_state rsp_state;
	/** Callbacks. */
	struct bt_dfu_smp_cbs {
		/** @brief Callback pointer for global error.
		 *
		 *  This function is called when a global error occurs.
		 */
		bt_dfu_smp_error_cb error_cb;
		/** @brief Callback pointer for receiving the response.
		 *
		 * The pointer is set to NULL when the response is
		 * finished.
		 */
		bt_dfu_smp_rsp_part_cb rsp_part;
	} cbs;
	/** @brief Response notification parameters.
	 *
	 *  These parameters are set for the SMP response notification.
	 *
	 *  The status of the notification is reflected by the @p notify
	 *  field. Notifications must be enabled before the command is sent.
	 *  This is handled automatically by the library.
	 */
	struct bt_gatt_subscribe_params notification_params;
};

/** @brief Initialize the DFU SMP Client module.
 *
 *  @param[out] dfu_smp DFU SMP Client instance.
 *  @param[in]  params Initialization parameter.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int bt_dfu_smp_init(struct bt_dfu_smp *dfu_smp,
		    const struct bt_dfu_smp_init_params *params);

/** @brief Assign handles to the DFU SMP Client instance.
 *
 *  @param[in,out] dm Discovery object.
 *  @param[in,out] dfu_smp DFU SMP Client instance.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 *  @retval (-ENOTSUP) Special error code used when the UUID
 *          of the service does not match the expected UUID.
 */
int bt_dfu_smp_handles_assign(struct bt_gatt_dm *dm,
			      struct bt_dfu_smp *dfu_smp);

/** @brief Execute a command.
 *
 *  @param[in,out] dfu_smp DFU SMP Client instance.
 *  @param[in]     rsp_cb    Callback function to process the response.
 *  @param[in]     cmd_size  Size of the command data buffer.
 *  @param[in]     cmd_data  Data buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int bt_dfu_smp_command(struct bt_dfu_smp *dfu_smp,
		       bt_dfu_smp_rsp_part_cb rsp_cb,
		       size_t cmd_size, const void *cmd_data);

/** @brief Get the connection object that is used with the DFU SMP Client.
 *
 *  @param[in] dfu_smp DFU SMP Client instance.
 *
 *  @return Connection object.
 */
struct bt_conn *bt_dfu_smp_conn(const struct bt_dfu_smp *dfu_smp);

/** @brief Get the current response state.
 *
 *  @note
 *  This function should be used inside a response callback function
 *  that is provided to @ref bt_dfu_smp_command.
 *
 *  @param dfu_smp DFU SMP Client instance.
 *
 *  @return The pointer to the response state structure.
 */
const struct bt_dfu_smp_rsp_state *bt_dfu_smp_rsp_state(
	const struct bt_dfu_smp *dfu_smp);

/** @brief Check if all response parts have been received.
 *
 *  This auxiliary function checks if the current response part is
 *  the final one.
 *
 *  @note
 *  This function should be used inside a response callback function
 *  that is provided to @ref bt_dfu_smp_command.
 *
 *  @param dfu_smp DFU SMP Client instance.
 *
 *  @retval true  If the current part is the final one.
 *  @retval false If the current part is not the final one.
 */
bool bt_dfu_smp_rsp_total_check(const struct bt_dfu_smp *dfu_smp);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_DFU_SMP_H_ */
