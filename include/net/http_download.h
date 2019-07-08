/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file http_download.h
 *
 * @defgroup http_downloader HTTP downloader
 * @{
 * @brief Library for downloading files over HTTP or HTTPS.
 *
 * @details The HTTP downloader provides APIs for:
 *  - connecting to a remote server,
 *  - downloading a file from the server,
 *  - disconnecting from the server,
 *  - receiving asynchronous event notifications on the download progress.
 */

#ifndef HTTP_DOWNLOAD_H__
#define HTTP_DOWNLOAD_H__

#include <zephyr.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP download event IDs.
 */
enum http_download_evt_id {
	/** Event contains a fragment. */
	HTTP_DOWNLOAD_EVT_FRAGMENT,
	/** Download complete. */
	HTTP_DOWNLOAD_EVT_DONE,
	/**
	 * An error has occurred during download and
	 * the connection to the server has been lost.
	 * - ENOTCONN: error reading from socket
	 * - ECONNRESET: peer closed connection
	 *
	 * In both cases, the application should
	 * disconnect (@ref http_download_disconnect)
	 * and connect (@ref http_download_connect)
	 * before reattempting the download, to
	 * reinitialize the network socket.
	 */
	HTTP_DOWNLOAD_EVT_ERROR,
};

/**
 * @brief HTTP download event.
 */
struct http_download_evt {
	/** Event ID. */
	enum http_download_evt_id id;
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
 * @brief HTTP download TLS and APN configuration options.
 */
struct http_download_cfg {
	/** TLS security tag. If -1, TLS is disabled. */
	int sec_tag;
	/** Access point name identifying a packet data network.
	 * Must be a null-terminated string or NULL to use the default APN.
	 */
	const char *apn;
};

/**
 * @brief HTTP download asynchronous event handler.
 *
 * Through this callback, the application receives events, such as
 * download of a fragment, download completion, or errors.
 *
 * If the callback returns a non-zero value, the download stops.
 * To resume the download, use @ref http_download_start().
 *
 * @param[in] event	The event.
 *
 * @return Zero to continue the download, non-zero otherwise.
 */
typedef int (*http_download_callback_t)(const struct http_download_evt *event);

/**
 * @brief HTTP download instance.
 */
struct http_download {
	/** HTTP socket. */
	int fd;
	/** HTTP response buffer. */
	char buf[CONFIG_HTTP_DOWNLOAD_MAX_RESPONSE_SIZE];
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
	/** The server has closed the connection. */
	bool connection_close;

	/** Server hosting the file, null-terminated. */
	char *host;
	/** File name, null-terminated. */
	char *file;
	/** TLS and APN configuration options. */
	struct http_download_cfg config;

	/** Internal thread ID. */
	k_tid_t tid;
	/** Internal download thread. */
	struct k_thread thread;
	/** Internal thread stack. */
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_HTTP_DOWNLOAD_STACK_SIZE);

	/** Event handler. */
	http_download_callback_t callback;
};

/**
 * @brief Initialize the HTTP downloader.
 *
 * @param[in] client	Client instance.
 * @param[in] callback	Callback function.
 *
 * @retval int Zero on success, otherwise a negative error code.
 */
int http_download_init(struct http_download *client,
		       http_download_callback_t callback);

/**
 * @brief Establish a connection to the server.
 *
 * @param[in] client	Client instance.
 * @param[in] host	HTTP server to connect to, null-terminated.
 * @param[in] config	TLS and APN configuration options.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int http_download_connect(struct http_download *client, char *host,
			  struct http_download_cfg *config);

/**
 * @brief Download a file.
 *
 * The download is carried out in fragments of @c
 * CONFIG_HTTP_DOWNLOAD_MAX_FRAGMENT_SIZE bytes,
 * which are delivered to the application
 * via @ref HTTP_DOWNLOAD_EVT_FRAGMENT events.
 *
 * @param[in] client	Client instance.
 * @param[in] file	File to download, null-terminated.
 * @param[in] from	Offset from where to resume the download,
 *			or zero to download from the beginning.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int http_download_start(struct http_download *client, char *file, size_t from);

/**
 * @brief Pause the download.
 *
 * @param[in] client	Client instance.
 */
void http_download_pause(struct http_download *client);

/**
 * @brief Resume the download.
 *
 * @param[in] client	Client instance.
 */
void http_download_resume(struct http_download *client);

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
int http_download_file_size_get(struct http_download *client, size_t *size);

/**
 * @brief Disconnect from the server.
 *
 * @param[in] client	Client instance.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int http_download_disconnect(struct http_download *client);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_DOWNLOAD_H__ */

/**@} */
