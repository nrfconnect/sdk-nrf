/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file download_client.h
 *
 * @defgroup dl_client Client to download an object.
 * @{
 * @brief Client to download an object.
 */
/* The download client provides APIs to:
 *  - connect to a remote server
 *  - download an object from the server
 *  - disconnect from the server
 *  - receive asynchronous event notification on the status of
 *    download
 *
 * Currently, only the HTTP protocol is supported for download.
 */

#ifndef DOWNLOAD_CLIENT_H__
#define DOWNLOAD_CLIENT_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct download_client;

/** @brief Download states. */
enum download_client_status {
	/** Indicates that the client was either never connected
	 *  to the server or is now disconnected.
	 */
	DOWNLOAD_CLIENT_STATUS_IDLE      = 0x00,
	/** Indicates that the client is connected to the server
	 *  and there is no ongoing download.
	 */
	DOWNLOAD_CLIENT_STATUS_CONNECTED = 0x01,
	/** Indicates that the client is connected to the server
	 *  and download is in progress.
	 */
	DOWNLOAD_CLIENT_STATUS_DOWNLOAD_INPROGRESS = 0x02,
	/** Indicates that the client is connected to the server
	 *  and download is complete.
	 */
	DOWNLOAD_CLIENT_STATUS_DOWNLOAD_COMPLETE = 0x03,
	/** Indicates that the object download is halted by the
	 *  application. This status indicates that the application
	 *  identified a failure when handling the
	 *  @ref DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG event.
	 */
	DOWNLOAD_CLIENT_STATUS_HALTED = 0x04,
	/** Indicates that an error occurred and the download
	 *  cannot continue.
	 */
	DOWNLOAD_CLIENT_ERROR = 0xFF
};


/** @brief Download events. */
enum download_client_evt {
	/** Indicates an error during download.
	 *  The application should disconnect and retry the operation
	 *  when receiving this event.
	 */
	DOWNLOAD_CLIENT_EVT_ERROR = 0x00,
	/** Indicates reception of a fragment during download.
	 *  The @p fragment field of the @ref download_client object
	 *  points to the object fragment, and the fragment size
	 *  indicates the size of the fragment.
	 */
	DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG = 0x01,
	/** Indicates that the download is complete.
	 */
	DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE = 0x02,
};

/** @brief Download client asynchronous event handler.
 *
 * The application is notified of the status of the object download
 * through this event handler.
 * The application can use the return value to indicate if it handled
 * the event successfully or not.
 * This feedback is useful when, for example, a faulty fragment was
 * received or the application was unable to process a object fragment.
 *
 * @param[in] client	The client instance.
 * @param[in] event     The event.
 * @param[in] status    Event status (either 0 or an errno value).
 *
 * @retval 0 If the event was handled successfully.
 *           Other values indicate that the application failed to handle
 *           the event.
 */
typedef int (*download_client_event_handler_t)(struct download_client *client,
				enum download_client_evt event, u32_t status);

/** @brief Object download client instance that describes the state of download.
 */
struct download_client {
	/** Buffer used to receive responses from the server.
	 *  This buffer can be read by the application if necessary,
	 *  but must never be written to.
	 */
	char resp_buf[CONFIG_NRF_DOWNLOAD_MAX_RESPONSE_SIZE];
	/** Buffer used to create requests to the server.
	 *  This buffer can be read by the application if necessary,
	 *  but must never be written to.
	 */
	char req_buf[CONFIG_NRF_DOWNLOAD_MAX_REQUEST_SIZE];
	/** Pointer to the object fragment in @p resp_buf.
	 *  The response from the server contains protocol metadata
	 *  in addition to the object fragment. On every
	 *  DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG event, this pointer is
	 *  updated to point to the latest object fragment.
	 *  This pointer shall not be updated by the application.
	 */
	char *fragment;
	/** Size of the fragment. The size is updated on every
	 *  DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG event.
	 */
	int fragment_size;
	/** Transport file descriptor.
	 *  If negative, the transport is disconnected.
	 */
	int fd;
	/** Total size of the object being downloaded.
	 * If negative, the download is in progress.
	 * If zero, the size is unknown.
	 */
	int object_size;
	/** Current size of the object being downloaded. */
	volatile int download_size;
	/** Status of the transfer (see @ref download_client_status). */
	volatile int status;
	/** Server that hosts the object. */
	const char *host;
	/** Resource to be downloaded. */
	const char *resource;
	/** Event handler. Must not be NULL. */
	const download_client_event_handler_t callback;
};

/** @brief Initialize the download client object for a given host and resource.
 *
 * The server to connect to for the object download is identified by
 * the @p host field of @p client. The @p callback field of @p client
 * must contain an event handler.
 *
 * @note If this method fails, do no call any other APIs for the instance.
 *
 * @param[in,out] client The client instance. Must not be NULL.
 *		      The target, host, resource, and callback fields must be
 *		      correctly initialized in the object instance.
 *		      The fd, status, fragment, fragment_size, download_size,
 *                    and object_size fields are out parameters.
 *
 * @retval 0  If the operation was successful.
 * @retval -1 Otherwise.
 */
int download_client_init(struct download_client *client);

/**@brief Establish a connection to the server.
 *
 * The server to connect to for the object download is identified by
 * the @p host field of @p client. The @p host field is expected to be a
 * that can be resolved to an IP address using DNS.
 *
 * @note This is a blocking call.
 *       Do not initiate a @ref download_client_start if this procedure fails.
 *
 * @param[in] client The client instance.
 *
 * @retval 0  If the operation was successful.
 * @retval -1 Otherwise.
 */
int download_client_connect(struct download_client *client);

/**@brief Start downloading the object.
 *
 * This is a blocking call used to trigger the download of an object
 * identified by the @p resource field of @p client. The download is
 * requested from the server in chunks of CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE.
 *
 * This API may be used to resume an interrupted download by setting the @p
 * download_size field of @p client to the last successfully downloaded fragment
 * of the object.
 *
 * If the API succeeds, use @ref download_client_process to advance the download
 * until a @ref DOWNLOAD_CLIENT_EVT_ERROR or a
 * @ref DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE event is received in the registered
 * callback.
 * If the API fails, disconnect using @ref download_client_disconnect, then
 * reconnect using @ref download_client_connect and restart the procedure.
 *
 * @param[in] client The client instance.
 *
 * @retval 0  If the operation was successful.
 * @retval -1 Otherwise.
 */
int download_client_start(struct download_client *client);

/**@brief Advance the object download.
 *
 * Call this API to advance the download state identified by the @p status
 * field of @p client. This is a blocking call. You can poll the @p fd field of
 * @p client to decide whether to call this method.
 *
 * @param[in] client The client instance.
 */
void download_client_process(struct download_client *client);

/**@brief Disconnect from the server.
 *
 * This API terminates the connection to the server. If called before
 * the download is complete, it is possible to resume the interrupted transfer
 * by reconnecting to the server using the @ref download_client_connect API and
 * calling @ref download_client_start to continue the download.
 * If you want to resume after a power cycle, you must store the download size
 * persistently and supply this value in the next connection.
 *
 * @note You should disconnect from the server as soon as the download
 * is complete.
 *
 * @param[in] client The client instance.
 *
 */
void download_client_disconnect(struct download_client *client);

#ifdef __cplusplus
}
#endif

#endif /* DOWNLOAD_CLIENT_H__ */

/**@} */
