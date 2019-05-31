/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file download_client.h
 *
 * @defgroup dl_client Download client
 * @{
 * @brief Client for downloading an object.
 *
 * @details The download client provides APIs for:
 *  - connecting to a remote server,
 *  - downloading an object from the server,
 *  - disconnecting from the server,
 *  - receiving asynchronous event notifications on the download status.
 */

#ifndef DOWNLOAD_CLIENT_H__
#define DOWNLOAD_CLIENT_H__

#include <zephyr.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Download client event IDs.
 */
enum download_client_evt_id {
	/** Event contains a fragment. */
	DOWNLOAD_CLIENT_EVT_FRAGMENT,
	/** Download complete. */
	DOWNLOAD_CLIENT_EVT_DONE,
	/**
	 * An error has occurred during download and
	 * the connection to the server has been lost.
	 * - ENOTCONN: error reading from socket
	 * - ECONNRESET: peer closed connection
	 *
	 * In both cases, the application should
	 * disconnect (@ref download_client_disconnect)
	 * and connect (@ref download_client_connect)
	 * before reattempting the download, to
	 * reinitialize the network socket.
	 */
	DOWNLOAD_CLIENT_EVT_ERROR,
};

/**
 * @brief Download client event.
 */
struct download_client_evt {
	/** Event ID. */
	enum download_client_evt_id id;
	union {
		/** Error cause. */
		int error;
		/** Fragment data. */
		struct fragment {
			const void *buf;
			size_t len;
		} fragment;
	};
};

/**
 * @brief Download client asynchronous event handler.
 *
 * Through this callback, the application receives events, such as
 * download of a fragment, download completion, or errors.
 *
 * If the callback returns a non-zero value, the download stops.
 * To resume the download, use @ref download_client_start().
 *
 * @param[in] event	The event.
 *
 * @return Zero to continue the download, non-zero otherwise.
 */
typedef int (*download_client_callback_t)(
	const struct download_client_evt *event);

/**
 * @brief Download client instance.
 */
struct download_client {
	/** Transport file descriptor. */
	int fd;
	/** HTTP response buffer. */
	char buf[CONFIG_NRF_DOWNLOAD_MAX_RESPONSE_SIZE];
	/** Buffer offset. */
	size_t offset;

	/** Size of the file being downloaded, in bytes. */
	size_t file_size;
	/** Download progress, number of bytes downloaded. */
	size_t progress;
	/** Whether the HTTP header for
	 * the current fragment has been processed.
	 */
	bool has_header;

	/** Server hosting the file, null-terminated. */
	char *host;
	/** File name, null-terminated. */
	char *file;

	/** Internal thread ID. */
	k_tid_t tid;
	/** Internal download thread. */
	struct k_thread thread;
	/** Internal thread stack. */
	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_NRF_DOWNLOAD_CLIENT_STACK_SIZE);

	/** Event handler. */
	download_client_callback_t callback;
};

/**
 * @brief Initialize the download client.
 *
 * @param[in] client	Client instance.
 * @param[in] callback	Callback function.
 *
 * @retval int Zero on success, otherwise a negative error code.
 */
int download_client_init(struct download_client *client,
			 download_client_callback_t callback);

/**
 * @brief Establish a connection to the server.
 *
 * @param[in] client	Client instance.
 * @param[in] host	HTTP server to connect to, null-terminated.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int download_client_connect(struct download_client *client, char *host);

/**
 * @brief Download a file.
 *
 * The download is carried out in fragments of @c
 * CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE bytes,
 * which are delivered to the application
 * via @ref DOWNLOAD_CLIENT_EVT_FRAGMENT events.
 *
 * @param[in] client	Client instance.
 * @param[in] file	File to download, null-terminated.
 * @param[in] from	Offset from where to resume the download,
 *			or zero to download from the beginning.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int download_client_start(struct download_client *client, char *file,
			  size_t from);

/**
 * @brief Pause the download.
 *
 * @param[in] client	Client instance.
 */
void download_client_pause(struct download_client *client);

/**
 * @brief Resume the download.
 *
 * @param[in] client	Client instance.
 */
void download_client_resume(struct download_client *client);

/**
 * @brief Retrieve the size of the file being downloaded, in bytes.
 *
 * The file size is only available after the download has begun.
 *
 * @param[in]  client	Client instance.
 * @param[out] size	File size.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int download_client_file_size_get(struct download_client *client, size_t *size);

/**
 * @brief Disconnect from the server.
 *
 * @param[in] client	Client instance.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int download_client_disconnect(struct download_client *client);

#ifdef __cplusplus
}
#endif

#endif /* DOWNLOAD_CLIENT_H__ */

/**@} */
