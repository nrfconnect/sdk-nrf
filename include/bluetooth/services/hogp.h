/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_HOGP_H_
#define BT_HOGP_H_

/**
 * @file
 * @defgroup bt_hogp Bluetooth LE GATT HIDS Client API
 * @{
 * @brief API for the Bluetooth LE GATT HID Service (HIDS) Client.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/hids.h>

struct bt_hogp;
struct bt_hogp_rep_info;

/**
 * @brief Callback function that is called when a notification or read response
 *        is received.
 *
 * This function is called when new data related to the given report object
 * appears.
 * The data size can be obtained from the report object from the
 * @em bt_hogp_rep_info::size field.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 * @param err    ATT error code.
 * @param data   Pointer to the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification.
 * @retval BT_GATT_ITER_CONTINUE Continue notification.
 */
typedef uint8_t (*bt_hogp_read_cb)(struct bt_hogp *hogp,
				   struct bt_hogp_rep_info *rep,
				   uint8_t err,
				   const uint8_t *data);

/**
 * @brief Callback function that is called when a write response is received.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 * @param err    ATT error code.
 */
typedef void (*bt_hogp_write_cb)(struct bt_hogp *hogp,
				 struct bt_hogp_rep_info *rep,
				 uint8_t err);

/**
 * @brief Callback function that is called when the HIDS client is ready.
 *
 * When the @ref bt_hogp_handles_assign function is called, the module
 * starts reading all additional information it needs to work.
 * When this process is finished, this callback function is called.
 *
 * @param hogp HOGP object.
 *
 * @sa bt_hogp_assign_error_cb
 */
typedef void (*bt_hogp_ready_cb)(struct bt_hogp *hogp);

/**
 * @brief Callback function that is called when an error is detected during
 *        HIDS client preparation.
 *
 * When the @ref bt_hogp_handles_assign function is called, the module
 * starts reading all additional information it needs to work.
 * If an error is detected during this process, this callback function is
 * called.
 *
 * @param hogp   HOGP object.
 * @param err    Negative internal error code or positive ATT error code.
 *
 * @sa bt_hogp_ready_cb
 */
typedef void (*bt_hogp_prep_fail_cb)(struct bt_hogp *hogp,
					    int err);

/**
 * @brief Callback function that is called when a chunk of a report map
 *        data is received.
 *
 * @param hogp   HOGP object.
 * @param err	 ATT error code.
 * @param data   Pointer to the data, or NULL if there is an error
 *               or no more data.
 * @param size   The size of the received data chunk.
 * @param offset Current data chunk offset.
 */
typedef void (*bt_hogp_map_cb)(struct bt_hogp *hogp, uint8_t err,
			       const uint8_t *data, size_t size, size_t offset);

/**
 * @brief Callback function that is called when the protocol mode is updated.
 *
 * This function is called when a protocol mode read or write finishes.
 *
 * @param hogp HOGP object.
 */
typedef void (*bt_hogp_pm_update_cb)(struct bt_hogp *hogp);

/**
 * @brief HIDS client parameters for the initialization function.
 */
struct bt_hogp_init_params {
	/**
	 * @brief Function to call when the HID client is ready.
	 */
	bt_hogp_ready_cb ready_cb;
	/**
	 * @brief Function to call if HID preparation fails.
	 */
	bt_hogp_prep_fail_cb prep_error_cb;
	/**
	 * @brief Function to call when the protocol mode is updated.
	 *
	 * This function is called every time the protocol mode is read or
	 * written. The old value is not checked.
	 */
	bt_hogp_pm_update_cb pm_update_cb;
};

/**
 * @brief Report base information.
 *
 * This structure is common for Input and Output/Feature reports.
 * Some of the fields are used only for the Input report type.
 *
 */
struct bt_hogp_rep_info;

/**
 * @brief HOGP object.
 *
 * @note
 * This structure is defined here to allow the user to allocate the memory
 * for it in an application-dependent manner.
 * Do not use any of the fields here directly, but use the accessor functions.
 * There are accessors for every field you might need.
 */
struct bt_hogp {
	/** Connection object. */
	struct bt_conn *conn;
	/** HIDS client information. */
	struct bt_hids_info info_val;
	/** Handlers for descriptors */
	struct bt_hogp_handlers {
		/** Protocol Mode Characteristic value handle. */
		uint16_t pm;
		/** Report Map descriptor handle. */
		uint16_t rep_map;
		/** HID Information Characteristic handle. */
		uint16_t info;
		/** HID Control Point Characteristic handle. */
		uint16_t cp;
	} handlers;
	/**
	 * @brief Callback for HIDS client ready
	 *       (see @ref bt_hogp_ready_cb).
	 */
	bt_hogp_ready_cb ready_cb;
	/**
	 * @brief Callback for HIDS client preparation failed.
	 */
	bt_hogp_prep_fail_cb prep_error_cb;
	/**
	 * @brief Callback for protocol mode updated.
	 */
	bt_hogp_pm_update_cb pm_update_cb;
	/**
	 * @brief Callback for report map data chunk received.
	 */
	bt_hogp_map_cb map_cb;
	/**
	 * @brief Internal read parameters used when reading
	 * Protocol Mode or Report Map.
	 * This struct might also be used during initialization.
	 *
	 * @sa bt_hogp::read_params_sem
	 */
	struct bt_gatt_read_params read_params;
	/**
	 * @brief The semaphore for common read parameters protection.
	 *
	 * This semaphore must be used when @ref bt_hogp::read_params
	 * can be used.
	 */
	struct k_sem read_params_sem;

	struct {
		/**
		 * During the initialization process, all reports reference
		 * information is read. This structure helps tracking the
		 * current state of this process.
		 */
		uint8_t rep_idx;
	} init_repref;

	struct {
		/** Keyboard input boot report. Input and Output keyboard
		 *  reports come in pairs.
		 *  It is not valid that only one of the keyboard
		 *  reports is present.
		 */
		struct bt_hogp_rep_info *kbd_inp;
		/** Keyboard output boot report. Input and Output keyboard
		 *  reports come in pairs. It is not valid that only one of
		 *  the keyboard reports is present.
		 */
		struct bt_hogp_rep_info *kbd_out;
		/** Mouse input boot report. */
		struct bt_hogp_rep_info *mouse_inp;
	} rep_boot;
	/** Array of report information structures. */
	struct bt_hogp_rep_info **rep_info;
	/** Number of records. */
	uint8_t rep_count;
	/** Current state. */
	bool ready;
	/** Current protocol mode. */
	enum bt_hids_pm pm;
};

/**
 * @brief Initialize the HIDS client instance.
 *
 * You must call this function on the HOGP object before
 * any other function.
 *
 * @note This function clears the whole memory occupied by the object.
 *       Do not call it on the object in use.
 *
 * @param hogp   HOGP object.
 * @param params Initialization parameters.
 */
void bt_hogp_init(struct bt_hogp *hogp,
		  const struct bt_hogp_init_params *params);

/**
 * @brief Assign handlers to the HIDS client instance.
 *
 * This function should be called when a link with a peer has been established,
 * to associate the link to this instance of the module. This makes it
 * possible to handle several links and associate each link to a particular
 * instance of this module. The GATT attribute handles are provided by the
 * GATT Discovery Manager.
 *
 * @note
 * This function starts the report discovery process.
 * Wait for one of the functions in @ref bt_hogp_init_params
 * before using the HIDS object.
 *
 * @param dm     Discovery object.
 * @param hogp   HOGP object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval (-ENOTSUP) Special error code used when the UUID
 *         of the service does not match the expected UUID.
 */
int bt_hogp_handles_assign(struct bt_gatt_dm *dm, struct bt_hogp *hogp);

/**
 * @brief Release the HIDS client instance.
 *
 * This function releases all dynamic memory allocated by the HIDS client
 * instance and clears all the instance data.
 *
 * @note
 * Do not call this function on an instance that was not initialized
 * (see @ref bt_hogp_init).
 * After the instance is initialized, it is safe to call this function
 * multiple times.
 *
 * @param hogp HOGP object.
 */
void bt_hogp_release(struct bt_hogp *hogp);

/**
 * @brief Abort all pending transfers.
 *
 * Call this function only when the server is disconnected.
 * Otherwise, if any read or write transfers are pending,
 * the HOGP object might permanently return an EBUSY error
 * on the following transfer attempts.
 *
 * @note
 * This function disables transfers inside the HIDS client library.
 * It does not abort any pending transfers in the Bluetooth LE stack.
 *
 * @param hogp HOGP object.
 */
void bt_hogp_abort_all(struct bt_hogp *hogp);

/**
 * @brief Check if the assignment function was called.
 *
 * @param hogp HOGP object.
 *
 * @retval true  If the HIDS client has handlers assigned already.
 * @retval false If the HIDS client is released.
 */
bool bt_hogp_assign_check(const struct bt_hogp *hogp);

/**
 * @brief Check if the HIDS client is ready to work.
 *
 * @note
 * If @ref bt_hogp_assign_check returns @c true while this function
 * returns @c false, it means that initialization is in progress.
 *
 * @param hogp HOGP object.
 *
 * @retval true  If the HIDS client is ready to work.
 * @retval false If the HIDS client is not ready.
 *
 */
bool bt_hogp_ready_check(const struct bt_hogp *hogp);

/**
 * @brief Send a read request addressing the report value descriptor.
 *
 * This function can be used on any type of report.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 * @param func   Function to be called to process the response.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_rep_read(struct bt_hogp *hogp,
		     struct bt_hogp_rep_info *rep,
		     bt_hogp_read_cb func);

/**
 * @brief Send a write request addressing the report value descriptor.
 *
 * Note that In reports might not support this function.
 * In this case, this function may succeed but pass an ATT error code
 * to the callback function.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 * @param func   Function to be called to process the response.
 * @param data   Data to be sent.
 * @param length Data size.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_rep_write(struct bt_hogp *hogp,
		      struct bt_hogp_rep_info *rep,
		      bt_hogp_write_cb func,
		      const void *data, uint8_t length);

/**
 * @brief Send a write command addressing the report value descriptor.
 *
 * Note that in case of an error, no error response from the server
 * is received.
 * If the server cannot execute the command, it is just ignored.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 * @param data   Data to be sent.
 * @param length Data size.
 * @param func   Function to be called when operation is completed.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_rep_write_wo_rsp(struct bt_hogp *hogp,
			     struct bt_hogp_rep_info *rep,
			     const void *data, uint8_t length,
			     bt_hogp_write_cb func);

/**
 * @brief Subscribe to report notifications.
 *
 * This function may be called only on Input type reports.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 * @param func   Function to be called to handle the notificated value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_rep_subscribe(struct bt_hogp *hogp,
			  struct bt_hogp_rep_info *rep,
			  bt_hogp_read_cb func);

/**
 * @brief Remove the subscription for a selected report.
 *
 * @param hogp   HOGP object.
 * @param rep    Report object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_rep_unsubscribe(struct bt_hogp *hogp,
			    struct bt_hogp_rep_info *rep);

/**
 * @brief Read part of the report map.
 *
 * This function sends a read request to the report map value.
 * Depending on the @c offset, either read or read blob is sent.
 *
 * The report map might not fit into a single PDU.
 * To read the whole map, call this function repeatedly with a different
 * offset.
 *
 * @note
 * This function uses the common read parameters structure inside the HIDS
 * client object. This object may be used by other functions and is
 * protected by semaphore.
 *
 * @param hogp    HOGP object.
 * @param func    Function to be called to process the response.
 * @param offset  Byte offset where to start data read.
 * @param timeout Time-out for the common read structure usage.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_map_read(struct bt_hogp *hogp,
		     bt_hogp_map_cb func,
		     size_t offset,
		     k_timeout_t timeout);

/**
 * @brief Read the current protocol mode from the server.
 *
 * The callback is called when a read response is received.
 *
 * @note
 * - Usually, the protocol mode value is read during HIDS client
 * preparation, and its value is followed after that.
 * This function may be used in an error recovery situation and
 * should not be required in normal usage.
 * - This function uses the common read parameters structure inside
 * the HOGP object. This object may be used by other functions
 * and is protected by semaphore.
 *
 * @sa bt_hogp_pm_get
 *
 * @param hogp    HOGP object.
 * @param timeout Time-out for the common read structure usage.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_pm_update(struct bt_hogp *hogp, k_timeout_t timeout);

/**
 * @brief Get the current protocol mode.
 *
 * This function returns the protocol mode stored in the HOGP object.
 * It does not generate any traffic on the radio.
 *
 * @param hogp HOGP object.
 *
 * @return Current protocol mode.
 */
enum bt_hids_pm bt_hogp_pm_get(const struct bt_hogp *hogp);

/**
 * @brief Set the current protocol mode.
 *
 * This function sends a write command to the Protocol Mode value.
 *
 * @note
 * For the Protocol Mode value, only Write Without Response is used,
 * so there is no control over the real Protocol Mode value if the write
 * does not succeed.
 *
 * @param hogp   HOGP object.
 * @param pm     Protocol mode to select.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_pm_write(struct bt_hogp *hogp, enum bt_hids_pm pm);

/**
 * @brief Suspend the device.
 *
 * This function sends a SUSPEND command using the Control Point
 *  Characteristic. It should be called when the HOST enters SUSPEND state.
 *
 * @param hogp HOGP object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_suspend(struct bt_hogp *hogp);

/**
 * @brief Exit suspend and resume.
 *
 * This function is used to inform the device that the HOST exits
 * SUSPEND state.
 *
 * @param hogp HOGP object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hogp_exit_suspend(struct bt_hogp *hogp);


/**
 * @brief Get the connection object from the HIDS client.
 *
 * This function gets the connection object that is used with a given
 * HIDS client.
 *
 * @param hogp HOGP object.
 *
 * @return Connection object.
 */
struct bt_conn *bt_hogp_conn(const struct bt_hogp *hogp);

/**
 * @brief Access the HID information value.
 *
 * This function accesses the structure with the decoded HID Information
 * Characteristic value.
 *
 * @param hogp HOGP object.
 *
 * @return Pointer to the decoded HID information structure.
 */
const struct bt_hids_info *bt_hogp_conn_info_val(const struct bt_hogp *hogp);

/**
 * @brief Access boot keyboard input report.
 *
 * @param hogp HOGP object.
 *
 * @return The report pointer, or NULL if the device does not support
 *         boot keyboard protocol mode.
 */
struct bt_hogp_rep_info *bt_hogp_rep_boot_kbd_in(struct bt_hogp *hogp);

/**
 * @brief Access boot keyboard output report.
 *
 * @param hogp HOGP object.
 *
 * @return The report pointer, or NULL if the device does not support
 *         boot keyboard protocol mode.
 */
struct bt_hogp_rep_info *bt_hogp_rep_boot_kbd_out(struct bt_hogp *hogp);

/**
 * @brief Access boot mouse input report.
 *
 * @param hogp HOGP object.
 *
 * @return The report pointer, or NULL if the device does not support
 *         boot mouse protocol mode.
 */
struct bt_hogp_rep_info *bt_hogp_rep_boot_mouse_in(struct bt_hogp *hogp);

/**
 * @brief Get the number of HID reports in the device.
 *
 * @note
 * This function does not take boot records into account.
 * They can be accessed using separate functions.
 *
 * @param hogp HOGP object.
 *
 * @return The number of records.
 */
size_t bt_hogp_rep_count(const struct bt_hogp *hogp);

/**
 * @brief Get the next report in the HOGP object.
 *
 * @param hogp   HOGP object.
 * @param rep    The record before the one to access, or NULL
 *               to access the first record.
 *
 * @return The next report according to @c rep, or NULL if no more reports
 *         are available.
 */
struct bt_hogp_rep_info *bt_hogp_rep_next(struct bt_hogp *hogp,
					  const struct bt_hogp_rep_info *rep);

/**
 * @brief Find a report.
 *
 * This function searches for a report with the given type and identifier.
 *
 * @param hogp   HOGP object.
 * @param type   The report type to find.
 * @param id     The identifier to find.
 *               Note that valid identifiers start from 1, but the value 0 may
 *               be used for devices that have one report of each type and
 *               there is no report ID inside the report map.
 *
 * @return The report that was found, or NULL.
 */
struct bt_hogp_rep_info *bt_hogp_rep_find(struct bt_hogp *hogp,
					  enum bt_hids_report_type type,
					  uint8_t id);

/**
 * @brief Set user data in report.
 *
 * @param rep  Report object.
 * @param data User data pointer to set.
 */
void bt_hogp_rep_user_data_set(struct bt_hogp_rep_info *rep, void *data);

/**
 * @brief Get user data from report.
 *
 * @param rep Report object.
 *
 * @return User data pointer.
 */
void *bt_hogp_rep_user_data(const struct bt_hogp_rep_info *rep);

/**
 * @brief Get report identifier.
 *
 * @param rep Report object.
 *
 * @return Report identifier.
 */
uint8_t bt_hogp_rep_id(const struct bt_hogp_rep_info *rep);

/**
 * @brief Get report type.
 *
 * @param rep Report object.
 *
 * @return Report type.
 */
enum bt_hids_report_type bt_hogp_rep_type(const struct bt_hogp_rep_info *rep);

/**
 * @brief Get report size.
 *
 * @note You can rely on this function after completing the read procedure for
 *       your target HID report with the @ref bt_hogp_rep_read API. The completion
 *       is reported by the appropriate callback. Alternatively, you can subscribe
 *       to your target report with @ref bt_hogp_rep_subscribe API and start
 *       using the function below once you receive the first notification. In
 *       all other cases, the size information is not available.
 *
 * @param rep Report object.
 *
 * @return The size of the report or 0 if not available.
 */
size_t bt_hogp_rep_size(const struct bt_hogp_rep_info *rep);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_HOGP_H_ */
