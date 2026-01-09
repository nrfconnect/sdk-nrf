/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file downloader.h
 *
 * @defgroup downloader Downloader
 * @{
 * @brief Client for downloading a file.
 *
 * @details The downloader provides APIs for:
 *  - downloading a file from the server,
 *  - receiving asynchronous event notifications on the download status.
 */

#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/coap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Downloader event IDs.
 */
enum downloader_evt_id {
	/**
	 * Event contains a fragment of data received from the server.
	 * When using range requests the amount of data per fragment may be less than the
	 * range requested.
	 * The application may return any non-zero value to stop the download.
	 */
	DOWNLOADER_EVT_FRAGMENT,
	/**
	 * An error has occurred during download and
	 * the connection to the server has been lost.
	 *
	 * Error reason may be one of the following:
	 * - -ECONNRESET: Socket error, peer closed connection.
	 * - -ECONNREFUSED: Socket error, connection refused by server.
	 * - -ENETDOWN: Socket error, network down.
	 * - -ETIMEDOUT: Socket error, connection timed out.
	 * - -EHOSTDOWN: Host went down during download.
	 * - -EBADMSG: HTTP response header not as expected.
	 * - -ERANGE: HTTP response does not support range requests.
	 * - -E2BIG: HTTP response header could not fit in buffer.
	 * - -EPROTONOSUPPORT: Protocol is not supported.
	 * - -EINVAL: Invalid configuration.
	 * - -EAFNOSUPPORT: Unsupported address family (IPv4/IPv6).
	 * - -EHOSTUNREACH: Failed to resolve the target address.
	 * - -EMSGSIZE: TLS packet is larger than the nRF91 Modem can handle.
	 * - -EMLINK: Maximum number of redirects reached.
	 *
	 * In case of @c ECONNRESET errors, returning zero from the callback will let the
	 * library attempt to reconnect to the server and download the last fragment again.
	 * Otherwise, the application may return any non-zero value to stop the download.
	 * On any other error code than @c ECONNRESET, the downloader will not attempt to reconnect
	 * and will ignore the return value.
	 *
	 * In case the download is stopped or completed, and the
	 * @c downloader_host_cfg.keep_connection flag is set, the downloader will stay
	 * connected to the server. If a new download is initiated towards a different server, the
	 * current connection is closed and the downloader will connect to the new server. The
	 * connection can be closed by deinitializing the downloader, which will also free its
	 * resources.
	 *
	 * If the @c downloader_host_cfg.keep_connection flag is not set, the downloader
	 * will automatically close the connection. The application should wait for the
	 * @c DOWNLOADER_EVT_STOPPED event before attempting another download.
	 */
	DOWNLOADER_EVT_ERROR,
	/** Download complete. Downloader is ready for new download. */
	DOWNLOADER_EVT_DONE,
	/** Download has been stopped. Downloader is ready for new download. */
	DOWNLOADER_EVT_STOPPED,
	/** Downloader deinitialized. Memory can be freed. */
	DOWNLOADER_EVT_DEINITIALIZED,
};

/**
 * @brief Downloader data fragment.
 */
struct downloader_fragment {
	/** Fragment buffer. */
	const void *buf;
	/** Length of fragment. */
	size_t len;
};

/**
 * @brief Downloader event.
 */
struct downloader_evt {
	/** Event ID. */
	enum downloader_evt_id id;

	union {
		/** Error cause. */
		int error;
		/** Fragment data. */
		struct downloader_fragment fragment;
	};
};

/**
 * @brief Downloader asynchronous event handler.
 *
 * Through this callback, the application receives events, such as
 * download of a fragment, download completion, or errors.
 *
 * On a @c DOWNLOADER_EVT_ERROR event with error @c ECONNRESET,
 * returning zero from the callback will let the library attempt
 * to reconnect to the server and continue the download.
 * Otherwise, the callback may return any non-zero value
 * to stop the download. On any other error code than @c ECONNRESET, the downloader
 * will not attempt to reconnect and will ignore the return value.
 * To resume the download, use @ref downloader_get().
 *
 * @param[in] event	The event.
 *
 * @return Zero to continue the download, non-zero otherwise.
 */
typedef int (*downloader_callback_t)(const struct downloader_evt *event);

/**
 * @brief Downloader configuration options.
 */
struct downloader_cfg {
	/** Event handler. */
	downloader_callback_t callback;
	/** Downloader buffer. */
	char *buf;
	/** Downloader buffer size. */
	size_t buf_size;
};

/**
 * @brief Downloader host configuration options.
 */
struct downloader_host_cfg {
	/**
	 * TLS security tag list.
	 * Pass NULL to disable TLS.
	 * The list must be kept in scope while download is going on.
	 */
	const int *sec_tag_list;
	/**
	 * Number of TLS security tags in list.
	 * Set to 0 to disable TLS.
	 */
	uint8_t sec_tag_count;
	/**
	 * PDN ID to be used for the download.
	 * Zero is the default PDN.
	 */
	uint8_t pdn_id;
	/**
	 * Range override.
	 * Request a number of bytes from the server at a time.
	 * 0 disables the range override, and the downloader will ask for the whole file.
	 * The nRF91 series has a limitation of decoding ~2k of data at once when using TLS, hence
	 * range override will be used in this case regardless of the value here.
	 */
	size_t range_override;
	/** Use native TLS. */
	bool set_native_tls;
	/**
	 * Keep connection to server when done.
	 * Server is disconnected if a file is requested from another server, and when the
	 * downloader is deinitialized.
	 */
	bool keep_connection;
	/**
	 * Enable DTLS connection identifier (CID) feature.
	 * This option requires modem firmware version >= 1.3.5.
	 */
	bool cid;
	/**
	 * Address family to be used for the download, @c AF_INET6 or @c AF_INET.
	 * Set to @c AF_UNSPEC (0) to fallback to @c AF_INET if @c AF_INET6 does not work.
	 */
	int family;
	/**
	 * Callback to do client authentication.
	 * This is called after connecting.
	 */
	int (*auth_cb)(int sock);
	/**
	 * CoAP Proxy-URI option.
	 * This string is used in case you are requesting a proxied file from a CoAP server.
	 */
	const char *proxy_uri;
	/**
	 * Maximum number of HTTP redirects.
	 * Use 0 to set the value of CONFIG_DOWNLOADER_MAX_REDIRECTS.
	 */
	uint8_t redirects_max;
	/** Name of the interface that the downloader instance should be bound to.
	 *  Leave as NULL if not specified.
	 */
	const char *if_name;
};

/**
 * @brief Downloader internal state.
 */
enum downloader_state {
	DOWNLOADER_DEINITIALIZED,
	DOWNLOADER_IDLE,
	DOWNLOADER_CONNECTING,
	DOWNLOADER_CONNECTED,
	DOWNLOADER_DOWNLOADING,
	DOWNLOADER_STOPPING,
	DOWNLOADER_DEINITIALIZING,
};

/**
 * @brief Downloader instance.
 *
 * Members are set internally by the downloader.
 */
struct downloader {
	/** Downloader configuration options. */
	struct downloader_cfg cfg;
	/** Host configuration options. */
	struct downloader_host_cfg host_cfg;
	/** Host name, null-terminated. */
	char hostname[CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE];
	/** File name, null-terminated. */
	char file[CONFIG_DOWNLOADER_MAX_FILENAME_SIZE];
	/** Size of the file being downloaded, in bytes. */
	size_t file_size;
	/** Download progress, in number of bytes downloaded. */
	size_t progress;
	/** Buffer offset. */
	size_t buf_offset;
	/** Flag to signal that the download is complete. */
	bool complete;
	/**
	 * Downloader transport, http, CoAP, MQTT, ...
	 * Store a pointer to the selected transport per downloader instance to avoid looking it up
	 * each call.
	 */
	const struct dl_transport *transport;
	/** Transport parameters. */
	uint8_t transport_internal[CONFIG_DOWNLOADER_TRANSPORT_PARAMS_SIZE];

	/** Ensure that thread is ready for download. */
	struct k_sem event_sem;
	/** Protect shared variables. */
	struct k_mutex mutex;
	/** Downloader state. */
	enum downloader_state state;
	/** Internal download thread. */
	struct k_thread thread;
	/** Internal thread ID. */
	k_tid_t tid;
	/** Internal thread stack. */
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_DOWNLOADER_STACK_SIZE);
};

/**
 * @brief Initialize the downloader.
 *
 * @param[in] dl	Downloader instance.
 * @param[in] cfg	Downloader configuration options.
 *
 * @return Zero on success, otherwise a negative error code.
 */
int downloader_init(struct downloader *dl, struct downloader_cfg *cfg);

/**
 * @brief Deinitialize the downloader.
 *
 * @param[in] dl Downloader instance.
 *
 * @return Zero on success.
 */
int downloader_deinit(struct downloader *dl);

/**
 * @brief Download a file asynchronously.
 *
 * This initiates an asynchronous connect-download-disconnect sequence to the target
 * host.
 *
 * Downloads are handled one at a time. If previous download is not finished
 * this returns -EALREADY.
 *
 * @param[in] dl		Downloader instance.
 * @param[in] host_cfg		Host configuration options.
 * @param[in] url		URL of the host to connect to.
 *				Can include scheme, port number and full file path, defaults to
 *				HTTP or HTTPS if no scheme is provided.
 * @param[in] from		Offset from where to resume the download,
 *				or zero to download from the beginning.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int downloader_get(struct downloader *dl, const struct downloader_host_cfg *host_cfg,
		   const char *url, size_t from);

/**
 * @brief Download a file asynchronously with host and file as separate parameters.
 *
 * This initiates an asynchronous connect-download-disconnect sequence to the target
 * host.
 *
 * Downloads are handled one at a time. If previous download is not finished,
 * this returns -EALREADY.
 *
 * @param[in] dl		Downloader instance.
 * @param[in] host_cfg		Host configuration options.
 * @param[in] host		URL of the host to connect to.
 *				Can include scheme and port number, defaults to
 *				HTTP or HTTPS if no scheme is provided.
 * @param[in] file		File to download
 * @param[in] from		Offset from where to resume the download,
 *				or zero to download from the beginning.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int downloader_get_with_host_and_file(struct downloader *dl,
				      const struct downloader_host_cfg *host_cfg,
				      const char *host, const char *file, size_t from);

/**
 * @brief Cancel file download.
 *
 * Request downloader to stop the download. This does not block.
 * If the @c downloader_host_cfg.keep_connection flag is set the downloader remains connected to
 * the server. Else the downloader is disconnected.
 * When the download is canceled an @c DOWNLOADER_EVT_STOPPED event is sent.
 *
 * @param[in] dl Downloader instance.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int downloader_cancel(struct downloader *dl);

/**
 * @brief Retrieve the size of the file being downloaded, in bytes.
 *
 * The file size is only available after the download has begun.
 *
 * @param[in]  dl	Downloader instance.
 * @param[out] size	File size.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int downloader_file_size_get(struct downloader *dl, size_t *size);

/**
 * @brief Retrieve the number of bytes downloaded so far.
 *
 * The progress is only available after the download has begun.
 *
 * @param[in]  dl	Downloader instance.
 * @param[out] size	Number of bytes downloaded so far.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int downloader_downloaded_size_get(struct downloader *dl, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* __DOWNLOADER_H__ */

/**@} */
