/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file dfu_client.h
 *
 * @defgroup dfu_client_client DFU client to download and apply new firmware (or
 * patches) to target. Currently, only HTTP protocol is supported for download.
 * @{
 * @brief DFU Client interface to connect to server, download and apply new
 * firmware to a selected target.
 */

#ifndef DFU_CLIENT_H__
#define DFU_CLIENT_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dfu_client_object;

/**@defgroup dfu_client_status DFU download status.
 *
 * @brief Indicates the status of download.
 * @{
 */
enum dfu_client_status {
	DFU_CLIENT_STATUS_IDLE      = 0x00,
	DFU_CLIENT_STATUS_CONNECTED = 0x01,
	DFU_CLIENT_STATUS_DOWNLOAD_INPROGRESS = 0x02,
	DFU_CLIENT_STATUS_DOWNLOAD_COMPLETE = 0x03,
	DFU_CLIENT_STATUS_HALTED = 0x04
};
/* @} */

/**@defgroup dfu_client_evt DFU events.
 * @brief The DFU Events.
 * @{
 */
enum dfu_client_evt {
	DFU_CLIENT_EVT_ERROR = 0x00,
	DFU_CLIENT_EVT_DOWNLOAD_INIT = 0x02,
	DFU_CLIENT_EVT_DOWNLOAD_FRAG = 0x03,
	DFU_CLIENT_EVT_DOWNLOAD_DONE = 0x04,
};
/* @} */

/**@brief Firmware upgrade event handler.
 *
 * @param[in] dfu The DFU instance.
 * @param[in] event     The event.
 */
typedef int (*dfu_client_event_handler_t)(struct dfu_client_object *dfu, enum dfu_client_evt event,
					u32_t status);

/**@brief Firmware download object describing the state of download*/
struct dfu_client_object {
	char resp_buf[CONFIG_NRF_DFU_HTTP_MAX_RESPONSE_SIZE];
	char req_buf[CONFIG_NRF_DFU_HTTP_MAX_REQUEST_SIZE];
	char * fragment;
	int fragment_size;

	int fd; 		/**< Transport file descriptor.
		  	          *  If negative the transport is disconnected. */
	int target_fd; 	/**< Target file descriptor.
			 	  	 *  If negative, target could not be reached.
			 	  	 */
	int firmware_size;	/**< Total size of firmware being downloaded.
			     	 	 * If negative, download is in progress.
						 * If zero, the size is unknown.
				  		 */
	volatile int download_size;	/**< Size of firmware being downloaded. */
	volatile int status; 		/**< Status of transfer. Will be one of \ref dfu_client_status. */
	u32_t available; 	/**< Available resources for firmware download.
				  		 */
	u32_t target_state;			   /**< Current state of the target. */
	const char *host; 			   /**< The server that hosts the firmwares. */
	const char *resource;          /**< Resource to be downloaded. */

	const dfu_client_event_handler_t callback; /**< Event handler.
					      						* Shall not be NULL.
					      						*/
};

/**@brief Initialize firmware upgrade for the target identified in the target
 * field of dfu. The server to connect to for firmware download is identified by
 * the the host. The caller of this method must register an event handler in
 * callback field of the instance.
 *
 * @param[inout] dfu The DFU instance. Shall not be NULL.
 *		     The target, host and callback fields should be
 *		     correctly initialized in the instance.
 *		     The fd, target_state, target_fd and available fields
 * are out parameters.
 *
 * @retval 0 if the procedure succeeded, else, -1.
 * @note If this method fails, no other APIs shall be called for the instance.
 */
int dfu_client_init(struct dfu_client_object *dfu);

/**@brief Establish connection to the firmware server for the target.
 *
 * @param[in] dfu The DFU instance.
 *
 * @retval 0 if the procedure request succeeded, else, -1.
 *
 * @note DFU_CLIENT_EVT_CONNECTION event marks the completion of this procedure.
 *       The status associated with the event indicates if the procedure
 * succeeded or not.
 */
int dfu_client_connect(struct dfu_client_object *dfu);

/**@brief Start download of firmware.
 *
 * @param[in] dfu The DFU instance.
 *
 * @retval 0 if the procedure succeeded, else, -1.
 *
 * @note The DFU_CLIENT_EVT_DOWNLOAD_INIT event indicates the download procedure was
 * initialized. If the status of this event indicates a failure, use @ref
 * dfu_client_abort and restart the procedure. Else, one or more
 * DFU_CLIENT_EVT_DOWNLOAD_FRAGMENT events can be expected. If any of
 * DFU_CLIENT_EVT_DOWNLOAD_COMPLETE event is generated when the entire firmware has
 * been downloaded. The @ref dfu_client_apply must be called to apply the downloaded
 * image on the target.
 */
int dfu_client_download(struct dfu_client_object *dfu);

/**@brief Method to continue any pending process based on the state.
 *
 * @param[in] dfu The DFU instance.
 *
 * @note This API shall be called to advance the DFU state and allow for
 * asynchronous behavior. The calling application may use poll on the fd of the
 * dfu to decide whether to call this method or not.
 */
void dfu_client_process(struct dfu_client_object *dfu);

/**@brief Disconnect connection to the server.
 *
 * @param[in] dfu The DFU instance.
 *
 * @note DFU_CLIENT_EVT_DISCONNECTION marks the completion of this procedure.
 * @note Its is recommended to disconnect connection to the server before
 * applying the downloaded image.
 */
void dfu_client_disconnect(struct dfu_client_object *dfu);

/**@brief Method to request apply the downloaded firmware on target.
 *
 * @param[in] dfu The DFU instance.
 *
 * @note The DFU_CLIENT_EVT_APPLY event marks the end of this procedure.
 *       This API may result in resetting the target.
 *       Therefore, it is recommended that the application calling this method
 * gracefully closes any procedures that depend on the functionality of target
 * before calling this API.
 *
 */
int dfu_client_apply(struct dfu_client_object *dfu);

/**@brief Method to request abort of the firmware upgrade procedure.
 *
 * @param[in] dfu The DFU instance.
 *
 * @note This API shall be called to advance the DFU state and allow for
 * asynchronous behavior. The calling application may use nrf_poll on the fd of
 * the dfu to decide whether to call this method or not.
 */
void dfu_client_abort(struct dfu_client_object *dfu);

#ifdef __cplusplus
}
#endif

#endif // DFU_CLIENT_H__

/**@} */
