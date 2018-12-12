/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_HIDS_C_H_
#define BT_GATT_HIDS_C_H_

/**
 * @file
 * @defgroup bt_gatt_hids_c BLE GATT HIDS Client API
 * @{
 * @brief API for the BLE GATT HID Service (HIDS) Client.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/common/gatt_dm.h>

struct bt_gatt_hids_c;
struct bt_gatt_hids_c_rep_info;

/**
 * @brief Protocol mode
 *
 * Possible values for protocol mode characteristic value
 */
enum bt_gatt_hids_c_pm {
	BT_GATT_HIDS_C_PM_BOOT   = 0x00,/**< Boot protocol   */
	BT_GATT_HIDS_C_PM_REPORT = 0x01  /**< Report protocol */
};

/**
 * @brief Report types
 *
 * Report types as as defined in Report Reference characteristic descriptor.
 */
enum bt_gatt_hids_c_report_type {
	BT_GATT_HIDS_C_REPORT_TYPE_RESERVED = 0x00, /**< Reserved value */
	BT_GATT_HIDS_C_REPORT_TYPE_INPUT    = 0x01, /**< Input Report   */
	BT_GATT_HIDS_C_REPORT_TYPE_OUTPUT   = 0x02, /**< Output Report  */
	BT_GATT_HIDS_C_REPORT_TYPE_FEATURE  = 0x03  /**< Feature Report */
};

/**
 * @brief Callback function when notification or read response is received
 *
 * Function is called when new data related to the given report object
 * appears.
 * The data size can be obtained from the report object from
 * @ref bt_gatt_hids_c_rep_info::size field.
 *
 * @param hids_c HIDS client object
 * @param rep    Report object
 * @param err    ATT error code
 * @param data   Pointer to the received data
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
typedef u8_t (*bt_gatt_hids_c_read_cb)(struct bt_gatt_hids_c *hids_c,
				       struct bt_gatt_hids_c_rep_info *rep,
				       u8_t err,
				       const u8_t *data);

/**
 * @brief Callback function called when write response is received
 *
 * @param hids_c HIDS client object
 * @param rep    Report object
 * @param err    ATT error code
 */
typedef void (*bt_gatt_hids_c_write_cb)(struct bt_gatt_hids_c *hids_c,
					struct bt_gatt_hids_c_rep_info *rep,
					u8_t err);

/**
 * @brief This function is called when HIDS client is ready
 *
 * When @ref bt_gatt_hids_c_handles_assign function is called this module
 * starts reading all the additional information it needs to work.
 * When this process finish, this callback function would be called.
 *
 * @param hids_c HIDS client object
 *
 * @sa bt_gatt_hids_c_assign_error_cb
 */
typedef void (*bt_gatt_hids_c_ready_cb)(struct bt_gatt_hids_c *hids_c);

/**
 * @brief Function called when error was detected during HIDS client preparation
 *
 * When @ref bt_gatt_hids_c_handles_assign function is called this module
 * starts reading all the additional information it needs to work.
 * When an error was detected during this process,
 * this function would be called.
 *
 * @param hids_c HIDS client object
 * @param err    Error code
 *
 * @sa bt_gatt_hids_c_ready_cb
 */
typedef void (*bt_gatt_hids_c_prep_fail_cb)(struct bt_gatt_hids_c *hids_c,
					    int err);

/**
 * @brief Function called when chunk of report map data is received
 *
 * @param hids_c HIDS client object
 * @param err	 ATT error code.
 * @param data   Pointer to the data or NULL if there is and error
 *               or no more data.
 * @param size   The size of the received data chunk.
 * @param offset Current data chunk offset.
 */
typedef void (*bt_gatt_hids_c_map_cb)(struct bt_gatt_hids_c *hids_c,
				      u8_t err,
				      const u8_t *data,
				      size_t size,
				      size_t offset);

/**
 * @brief Protocol mode was updated
 *
 * Function called when protocol mode read or write finishes.
 *
 * @param hids_c HIDS client object
 */
typedef void (*bt_gatt_hids_c_pm_update_cb)(struct bt_gatt_hids_c *hids_c);

/**
 * @brief Report base information struct
 *
 * The structure that contains record information.
 * This is common structure for Input and Output/Feature reports.
 * Some of the fields are used only for Input report type.
 *
 */
struct bt_gatt_hids_c_rep_info {
	/** HIDS client object
	 *  @note Do not use it directly as it may be changed to some kind of
	 *        connection context management
	 */
	struct bt_gatt_hids_c *hids_c;
	bt_gatt_hids_c_read_cb  read_cb;   /**< Read function callback        */
	bt_gatt_hids_c_write_cb write_cb;  /**< Write function callback       */
	bt_gatt_hids_c_read_cb  notify_cb; /**< Notification function (Input) */
	struct bt_gatt_read_params      read_params;   /**< Read params   */
	struct bt_gatt_write_params     write_params;  /**< Write params  */
	struct bt_gatt_subscribe_params notify_params; /**< Notify params */

	/** User data to be used freely by the application */
	void *user_data;
	/** Handlers */
	struct {
		u16_t ref; /**< Report Reference handler */
		u16_t val; /**< Value handler            */
		u16_t ccc; /**< CCC handler (Input)      */
	} handlers;
	/** Report reference information */
	struct {
		u8_t id;                              /**< Report identifier */
		enum bt_gatt_hids_c_report_type type; /**< Report type       */
	} ref;
	u8_t size; /**< The size of the value */
};

/**
 * @brief RemoteWake flag position
 *
 * Used in @ref bt_gatt_hids_c_info_val::Flags
 */
#define BT_GATT_HIDS_C_INFO_FLAG_RW_POS 0

/**
 * @brief NormallyConnectable flag position
 *
 * Used in @ref bt_gatt_hids_c_info_val::Flags
 */
#define BT_GATT_HIDS_C_INFO_FLAG_NC_POS 1

/**
 * @brief RemoteWake flag value
 *
 * Used in @ref bt_gatt_hids_c_info_val::Flags
 */
#define BT_GATT_HIDS_C_INFO_FLAG_RW (1U << BT_GATT_HIDS_C_INFO_FLAG_RW_POS)

/**
 * @brief NormallyConnectable flag value
 *
 * Used in @ref bt_gatt_hids_c_info_val::Flags
 */
#define BT_GATT_HIDS_C_INFO_FLAG_NC (1U << BT_GATT_HIDS_C_INFO_FLAG_NC_POS)

/**
 * @brief HID Information Characteristic Value
 */
struct bt_gatt_hids_c_info_val {
	/**
	 * BCD representation of version number of base USB HID Specification
	 * implemented by HID Device.
	 */
	u16_t bcd_hid;
	/**
	 * HID Device hardware localization.
	 * Most hardware is not localized (value 0x00).
	 */
	u8_t  b_country_code;
	/**
	 * Flags to mark available functions.
	 * @sa BT_GATT_HIDS_C_INFO_FLAG_RW, BT_GATT_HIDS_C_INFO_FLAG_NC
	 */
	u8_t  flags;
};

/**
 * @brief HIDS client initialization parameters
 *
 * The parameters for initialization function.
 */
struct bt_gatt_hids_c_init_params {
	/**
	 * @brief Function that would be called when HID client is ready
	 */
	bt_gatt_hids_c_ready_cb ready_cb;
	/**
	 * @brief Function that would be called if HID preparation fails
	 */
	bt_gatt_hids_c_prep_fail_cb prep_error_cb;
	/**
	 * @brief Function that would be called when protocol mode is updated
	 *
	 * This function would be called every time protocol mode is read or
	 * written. Old value is not checked.
	 */
	bt_gatt_hids_c_pm_update_cb pm_update_cb;
};

/**
 * HIDS client structure
 */
struct bt_gatt_hids_c {
	/** Connection object */
	struct bt_conn *conn;
	/** HIDS client information structure */
	struct bt_gatt_hids_c_info_val info_val;
	/** Handlers for descriptors */
	struct {
		u16_t pm;      /**< Protocol Mode characteristic value handle */
		u16_t rep_map; /**< Report Map descriptor handle              */
		u16_t info;    /**< HID information characteristic handle     */
		u16_t cp;      /**< HID control point characteristic handle   */
	} handlers;
	/**
	 * @brief HIDS client ready callback
	 *
	 * For documentation see @ref bt_gatt_hids_c_ready_cb
	 */
	bt_gatt_hids_c_ready_cb ready_cb;
	/**
	 * @brief HIDS client preparation fails
	 */
	bt_gatt_hids_c_prep_fail_cb prep_error_cb;
	/**
	 * @brief Report map data chunk received
	 */
	bt_gatt_hids_c_map_cb map_cb;
	/**
	 * @brief The callback for the Protocol mode updated
	 */
	bt_gatt_hids_c_pm_update_cb pm_update_cb;
	/**
	 * @brief Auxiliary read parameters
	 *
	 * The internal read parameter used when reading
	 * Protocol Mode or Report Map.
	 * It might also be used during initialization.
	 *
	 * @sa bt_gatt_hids_c::read_params_sem
	 */
	struct bt_gatt_read_params read_params;
	/**
	 * @brief The semaphore for common read parameters protection
	 *
	 * This semaphore has to be used when @ref bt_gatt_hids_c::read_params
	 * can be used.
	 */
	struct k_sem read_params_sem;
	/** Boot protocol reports if available.
	 *  Input and Output keyboard reports comes in pairs. It is no valid
	 *  situation if only one of keyboard reports is present.
	 */
	struct {
		/** Keyboard input boot report */
		struct bt_gatt_hids_c_rep_info *kbd_inp;
		/** Keyboard output boot report */
		struct bt_gatt_hids_c_rep_info *kbd_out;
		/** Mouse input boot report */
		struct bt_gatt_hids_c_rep_info *mouse_inp;
	} rep_boot;
	/** Array of reports information structures */
	struct bt_gatt_hids_c_rep_info **rep_info;
	/** Number of records */
	u8_t rep_cnt;
	/** The state */
	bool ready;
	/** Current Protocol mode */
	enum bt_gatt_hids_c_pm pm;
};

/**
 * @brief Initialize HIDS client instance
 *
 * This function has to be called on HIDS client object before
 * any other function.
 *
 * @note This function clears whole memory occupied by the object.
 *       Do not call it on the object in use.
 *
 * @param hids_c HIDS client object
 * @param params Initialization parameters.
 */
void bt_gatt_hids_c_init(struct bt_gatt_hids_c *hids_c,
			const struct bt_gatt_hids_c_init_params *params);

/**
 * @brief Assign handlers to the HIDS client instance
 *
 * This function should be called when a link with a peer has been established
 * to associate the link to this instance of the module. This makes it
 * possible to handle several links and associate each link to a particular
 * instance of this module. The GATT attribute handles are provided by the
 * GATT DB discovery module.
 *
 * @note
 * This function would start report discovery process.
 * Wait for one of the functions in @ref bt_gatt_hids_c_init_params
 * before using HIDS object.
 *
 * @param dm     Discovery object.
 * @param hids_c HIDS client object.
 *
 * @retval 0 If the operation was successful.
 * @retval (-ENOTSUP) Special error code used when UUID
 *         of the service does not match the expected UUID.
 * @retval Otherwise, a negative error code is returned.
 */
int bt_gatt_hids_c_handles_assign(struct bt_gatt_dm *dm,
				  struct bt_gatt_hids_c *hids_c);

/**
 * @brief Release the HIDS client instance
 *
 * Function releases all the dynamic memory allocated by the HIDS client
 * instance and clears all the instance data.
 *
 * @note
 * Do not call this function on instance that was not initialized
 * (see @ref bt_gatt_hids_c_init).
 * After the instance is initialized it is safe to call this function
 * multiple times.
 *
 * @param hids_c The HIDS client object.
 */
void bt_gatt_hids_c_release(struct bt_gatt_hids_c *hids_c);

/**
 * @brief Abort all pending transfers
 *
 * Call this function @b when server was disconnected.
 * In other case if there is any pending read or write transfer
 * it might permanently return EBUSY error on next transfer.
 *
 * @note
 * This function disables transfers inside the HIDS client library.
 * It would not abort any pending transfers in BLE stack.
 *
 * @param hids_c The HIDS client object.
 */
void bt_gatt_hids_c_abort_all(struct bt_gatt_hids_c *hids_c);

/**
 * @brief Check if assignment function was called
 *
 * @param hids_c HIDS client object.
 *
 * @retval true  HIDS client has handlers assigned already.
 * @retval false HIDS client is released.
 */
bool bt_gatt_hids_c_assign_check(const struct bt_gatt_hids_c *hids_c);

/**
 * @brief Check if HIDS client is ready to work
 *
 * @param hids_c HIDS client object.
 *
 * @retval true  HIDS client is ready to work.
 * @retval false HIDS client is not ready.
 *
 * @note
 * If @ref bt_gatt_hids_c_assign_check returns @c true while this function
 * returns @c false it means that initialization is in progress.
 */
bool bt_gatt_hids_ready_check(const struct bt_gatt_hids_c *hids_c);

/**
 * @brief Send read request addressing report value descriptor
 *
 * This function can be used on any type of report.
 *
 * @param hids_c HIDS client object.
 * @param rep    Report object.
 * @param func   Function that would be called to process the response.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_rep_read(struct bt_gatt_hids_c *hids_c,
			    struct bt_gatt_hids_c_rep_info *rep,
			    bt_gatt_hids_c_read_cb func);

/**
 * @brief Send write request addressing report value descriptor
 *
 * Note that In reports may not support this function.
 * In such situation this function may succeed but ATT error code
 * would we passed to callback function.
 *
 * @param hids_c HIDS client object.
 * @param rep    Report object.
 * @param func   Function that would be called to process the response.
 * @param data   Data to be sent.
 * @param length Data size.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_rep_write(struct bt_gatt_hids_c *hids_c,
			     struct bt_gatt_hids_c_rep_info *rep,
			     bt_gatt_hids_c_write_cb func,
			     const void *data, u8_t length);

/**
 * @brief Send write command addressing report value descriptor
 *
 * Note that there would not be error response from the server received.
 * If server cannot execute the command it would be just ignored.
 *
 * @param hids_c HIDS client object.
 * @param rep    Report object.
 * @param data   Data to be sent.
 * @param length Data size.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_rep_write_wo_rsp(struct bt_gatt_hids_c *hids_c,
				    struct bt_gatt_hids_c_rep_info *rep,
				    const void *data, u8_t length);

/**
 * @brief Subscribe into report notification
 *
 * This function may be called only on Input type reports.
 *
 * @param hids_c HIDS client object.
 * @param rep    Report object.
 * @param func   Function that would be called to handle the notificated value.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_rep_subscribe(struct bt_gatt_hids_c *hids_c,
				 struct bt_gatt_hids_c_rep_info *rep,
				 bt_gatt_hids_c_read_cb func);

/**
 * @brief Remove the subscription
 *
 * Abort the subscription for selected report.
 *
 * @param hids_c HIDS client object.
 * @param rep    Report object.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_rep_unsubscribe(struct bt_gatt_hids_c *hids_c,
				   struct bt_gatt_hids_c_rep_info *rep);

/**
 * @brief Reads part of report map
 *
 * Function sends read request to the report map value.
 * Depending on @c offset read or read blob is sent.
 *
 * Map may not fit into single PDU.
 * To read the whole map call this function again with offset set.
 *
 * @note
 * This function uses common read parameters structure inside HIDS
 * client object. This object may be used by other functions and is
 * protected by semaphore.
 *
 * @param hids_c  HIDS client object.
 * @param func    Function that would be called to process the response.
 * @param offset  Byte offset where to start data read.
 * @param timeout Time out for the common read structure usage.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_map_read(struct bt_gatt_hids_c *hids_c,
			    bt_gatt_hids_c_map_cb func,
			    size_t offset,
			    s32_t timeout);

/**
 * @brief Read current protocol mode from server
 *
 * The callback would be called when read response is received.
 *
 * @note
 * In normal situation the protocol mode value is read during HIDS client
 * preparation and its value is followed after that.
 * This function may be used in error recovery situation and should not be
 * required in normal usage.
 *
 * @note
 * This function uses common read parameters structure inside HIDS
 * client object. This object may be used by other functions and is
 * protected by semaphore.
 *
 * @sa bt_gatt_hids_c_pm_get
 *
 * @param hids_c HIDS client object.
 * @param timeout Time out for the common read structure usage.
 *
 * @return 0 or negative error code.
 */
int bt_gatt_hids_c_pm_update(struct bt_gatt_hids_c *hids_c, s32_t timeout);

/**
 * @brief Get current protocol mode
 *
 * Function returns the protocol mode stored in HIDS client object.
 * It does not generate any traffic on the radio.
 *
 * @param hids_c HIDS client object.
 *
 * @return Current protocol mode.
 */
enum bt_gatt_hids_c_pm bt_gatt_hids_c_pm_get(
	const struct bt_gatt_hids_c *hids_c);

/**
 * @brief Set current protocol mode
 *
 * Function send write command to the protocol mode value.
 *
 * @note
 * For Protocol Mode value only Write Without Response is used,
 * so there is no control over the real PM value if write did not succeed.
 *
 * @param hids_c HIDS client object.
 * @param pm     Protocol mode to select.
 *
 * @return 0 or negative error value.
 */
int bt_gatt_hids_c_pm_write(struct bt_gatt_hids_c *hids_c,
			    enum bt_gatt_hids_c_pm pm);

/**
 * @brief Suspend the device
 *
 * Function sends SUSPEND command using control point characteristic.
 * This function should be called when HOST enters SUSPEND state.
 *
 * @param hids_c HIDS client object.
 *
 * @return 0 or negative error value.
 */
int bt_gatt_hids_c_suspend(struct bt_gatt_hids_c *hids_c);

/**
 * @brief Exit suspend
 *
 * Function used to inform the device that HOST exits SUSPEND state.
 *
 * @param hids_c HIDS client object
 *
 * @return 0 or negative error value.
 */
int bt_gatt_hids_c_exit_suspend(struct bt_gatt_hids_c *hids_c);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_HIDS_C_H_ */
