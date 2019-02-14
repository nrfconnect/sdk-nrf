/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_DFU_SMP_C_H_
#define BT_GATT_DFU_SMP_C_H_

/**
 * @file
 * @defgroup bt_gatt_dfu_smp_c BLE GATT DFU SMP Client API
 * @{
 * @brief API for the BLE GATT DFU SMP (DFU_SMP) Client.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>


/** @brief SMP Service.
 *
 *  The 128-bit service UUID is 8D53DC1D-1DB7-4CD3-868B-8A527460AA84.
 */
#define DFU_SMP_UUID_SERVICE BT_UUID_DECLARE_128( \
				0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,\
				0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d)

/** @brief SMP Characteristic.
 *
 *  The characteristic is used for both requests and responses.
 *  The UUID is DA2E7828-FBCE-4E01-AE9E-261174997C48.
 */
#define DFU_SMP_UUID_CHAR BT_UUID_DECLARE_128( \
				0x48, 0x7c, 0x99, 0x74, 0x11, 0x26, 0x9e, 0xae,\
				0x01, 0x4e, 0xce, 0xfb, 0x28, 0x78, 0x2e, 0xda)


/* Forward declaration required for function handlers. */
struct bt_gatt_dfu_smp_c;

/** @brief Header used internally by dfu_smp.
 */
struct dfu_smp_header {
	/** Operation. */
	u8_t op;
	/** Operational flags. */
	u8_t flags;
	/** Payload length high byte. */
	u8_t len_h8;
	/** Payload length low byte. */
	u8_t len_l8;
	/** Group ID high byte. */
	u8_t group_h8;
	/** Group ID low byte. */
	u8_t group_l8;
	/** Sequence number. */
	u8_t seq;
	/** Command ID. */
	u8_t id;
};

/** @brief Current response state.
 */
struct bt_gatt_dfu_smp_rsp_state {
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
	const u8_t *data;
};

/** @brief Handle part of the response.
 *
 *  @param dfu_smp_c DFU SMP Client instance.
 */
typedef void (*bt_gatt_dfu_smp_rsp_part_cb)(
		struct bt_gatt_dfu_smp_c *dfu_smp_c);

/** @brief Global function that is called when an error occurs.
 *
 *  @param dfu_smp_c DFU SMP Client instance.
 *  @param err       Negative internal error code or positive GATT error code.
 */
typedef void (*bt_gatt_dfu_smp_error_cb)(
		struct bt_gatt_dfu_smp_c *dfu_smp_c, int err);

/**
 * @brief DFU SMP Client parameters for the initialization function.
 */
struct bt_gatt_dfu_smp_c_init_params {
	/** @brief Callback pointer for error handler.
	 *
	 *  This function is called when an error occurs.
	 */
	bt_gatt_dfu_smp_error_cb error_cb;
};

/**
 * @brief DFU SMP Client structure.
 */
struct bt_gatt_dfu_smp_c {
	/** Connection object. */
	struct bt_conn *conn;
	/** Handles. */
	struct bt_gatt_dfu_smp_c_handles {
		/** SMP characteristic value handle. */
		u16_t smp;
		/** SMP characteristic CCC handle. */
		u16_t smp_ccc;
	} handles;
	/** Current response state. */
	struct bt_gatt_dfu_smp_rsp_state rsp_state;
	/** Callbacks. */
	struct bt_gatt_dfu_smp_c_cbs {
		/** @brief Callback pointer for global error.
		 *
		 *  This function is called when a global error occurs.
		 */
		bt_gatt_dfu_smp_error_cb error_cb;
		/** @brief Callback pointer for receiving the response.
		 *
		 * The pointer is set to NULL when the response is
		 * finished.
		 */
		bt_gatt_dfu_smp_rsp_part_cb rsp_part;
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
 *  @param[out] dfu_smp_c DFU SMP Client instance.
 *  @param[in]  params Initialization parameter.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dfu_smp_c_init(struct bt_gatt_dfu_smp_c *dfu_smp_c,
			   const struct bt_gatt_dfu_smp_c_init_params *params);

/** @brief Assign handles to the DFU SMP Client instance.
 *
 *  @param[in,out] dm Discovery object.
 *  @param[in,out] dfu_smp_c DFU SMP Client instance.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 *  @retval (-ENOTSUP) Special error code used when the UUID
 *          of the service does not match the expected UUID.
 */
int bt_gatt_dfu_smp_c_handles_assign(struct bt_gatt_dm *dm,
				     struct bt_gatt_dfu_smp_c *dfu_smp_c);

/** @brief Execute a command.
 *
 *  @param[in,out] dfu_smp_c DFU SMP Client instance.
 *  @param[in]     rsp_cb    Callback function to process the response.
 *  @param[in]     cmd_size  Size of the command data buffer.
 *  @param[in]     cmd_data  Data buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dfu_smp_c_command(struct bt_gatt_dfu_smp_c *dfu_smp_c,
			      bt_gatt_dfu_smp_rsp_part_cb rsp_cb,
			      size_t cmd_size,
			      const void *cmd_data);

/** @brief Get the connection object that is used with the DFU SMP Client.
 *
 *  @param[in] dfu_smp_c DFU SMP Client instance.
 *
 *  @return Connection object.
 */
struct bt_conn *bt_gatt_dfu_smp_c_conn(
		const struct bt_gatt_dfu_smp_c *dfu_smp_c);

/** @brief Get the current response state.
 *
 *  @note
 *  This function should be used inside a response callback function
 *  that is provided to @ref bt_gatt_dfu_smp_c_command.
 *
 *  @param dfu_smp_c DFU SMP Client instance.
 *
 *  @return The pointer to the response state structure.
 */
const struct bt_gatt_dfu_smp_rsp_state *bt_gatt_dfu_smp_c_rsp_state(
		const struct bt_gatt_dfu_smp_c *dfu_smp_c);

/** @brief Check if all response parts have been received.
 *
 *  This auxiliary function checks if the current response part is
 *  the final one.
 *
 *  @note
 *  This function should be used inside a response callback function
 *  that is provided to @ref bt_gatt_dfu_smp_c_command.
 *
 *  @param dfu_smp_c DFU SMP Client instance.
 *
 *  @retval true  If the current part is the final one.
 *  @retval false If the current part is not the final one.
 */
bool bt_gatt_dfu_smp_c_rsp_total_check(
		const struct bt_gatt_dfu_smp_c *dfu_smp_c);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_DFU_SMP_C_H_ */
