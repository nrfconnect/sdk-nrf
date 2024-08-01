/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#include <arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <zephyr/net/socket_ncs.h>
#include <zephyr/net/tls_credentials.h>
#include <net/download_client.h>
#include <zephyr/logging/log.h>
#include "download_client_internal.h"

LOG_MODULE_REGISTER(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE

static int handle_disconnect(struct download_client *dlc);
static int error_evt_send(const struct download_client *dlc, int error);

static bool use_coap(struct download_client *dlc)
{
	return (IS_ENABLED(CONFIG_COAP) &&
		(dlc->proto == IPPROTO_UDP || dlc->proto == IPPROTO_DTLS_1_2));
}

static bool is_state(struct download_client *dlc, enum download_client_state state)
{
	bool ret;

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	ret = dlc->state == state;
	k_mutex_unlock(&dlc->mutex);
	return ret;
}

static void set_state(struct download_client *dlc, int state)
{
	k_mutex_lock(&dlc->mutex, K_FOREVER);
	/* Prevent moving back to CLOSING from IDLE */
	if ((state != DOWNLOAD_CLIENT_CLOSING) || (dlc->state != DOWNLOAD_CLIENT_IDLE)) {
		dlc->state = state;
	}
	k_mutex_unlock(&dlc->mutex);
	LOG_DBG("state = %d", state);
}

static int client_connect(struct download_client *dlc)
{
	int err;
	int ns_err;
	int type;
	uint16_t port;

	err = url_parse_proto(dlc->host, &dlc->proto, &type);
	if (err == -EINVAL) {
		LOG_DBG("Protocol not specified, defaulting to HTTP(S)");
		type = SOCK_STREAM;
		if (dlc->config.sec_tag_list && (dlc->config.sec_tag_count > 0)) {
			dlc->proto = IPPROTO_TLS_1_2;
		} else {
			dlc->proto = IPPROTO_TCP;
		}
	} else if (err) {
		goto cleanup;
	}

	if ((dlc->proto == IPPROTO_UDP || dlc->proto == IPPROTO_DTLS_1_2) &&
	    (!IS_ENABLED(CONFIG_COAP))) {
		err = -EPROTONOSUPPORT;
		goto cleanup;
	}

	if (dlc->proto == IPPROTO_TLS_1_2 || dlc->proto == IPPROTO_DTLS_1_2) {
		if (dlc->config.sec_tag_list == NULL || dlc->config.sec_tag_count == 0) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			err = -EINVAL;
			goto cleanup;
		}
	}

	if ((dlc->config.sec_tag_list == NULL || dlc->config.sec_tag_count == 0) &&
	    dlc->config.set_tls_hostname) {
		LOG_WRN("set_tls_hostname flag is set for non-TLS connection");
		err = -EINVAL;
		goto cleanup;
	}

	err = url_parse_port(dlc->host, &port);
	if (err) {
		switch (dlc->proto) {
		case IPPROTO_TLS_1_2:
			port = 443;
			break;
		case IPPROTO_TCP:
			port = 80;
			break;
		case IPPROTO_DTLS_1_2:
			port = 5684;
			break;
		case IPPROTO_UDP:
			port = 5683;
			break;
		}
		LOG_DBG("Port not specified, using default: %d", port);
	}

	if (dlc->set_native_tls) {
		LOG_DBG("Enabled native TLS");
		type |= SOCK_NATIVE_TLS;
	}

	err = -1;
	ns_err = -1;

	err = client_socket_configure_and_connect(dlc, type, port);

cleanup:
	if (err) {
		error_evt_send(dlc, -err);

		/* Unable to connect, close socket */
		handle_disconnect(dlc);
	}

	return err;
}

static int request_send(struct download_client *dlc)
{
	if (dlc->fd < 0) {
		return -1;
	}

	switch (dlc->proto) {
	case IPPROTO_TCP:
	case IPPROTO_TLS_1_2:
		return http_get_request_send(dlc);
	case IPPROTO_UDP:
	case IPPROTO_DTLS_1_2:
		if (IS_ENABLED(CONFIG_COAP)) {
			return coap_request_send(dlc);
		}
	}

	return 0;
}

static int fragment_evt_send(struct download_client *dlc)
{
	__ASSERT(dlc->offset <= CONFIG_DOWNLOAD_CLIENT_BUF_SIZE,
		 "Buffer overflow!");

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_FRAGMENT,
		.fragment = {
			.buf = dlc->buf,
			.len = dlc->offset,
		}
	};

	dlc->offset = 0;

	return dlc->callback(&evt);
}

static int error_evt_send(const struct download_client *dlc, int error)
{
	/* Error will be sent as negative. */
	__ASSERT_NO_MSG(error > 0);

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_ERROR,
		.error = -error
	};

	return dlc->callback(&evt);
}

static int reconnect(struct download_client *dlc)
{
	int err;

	LOG_INF("Reconnecting...");
	if (dlc->fd >= 0) {
		err = close(dlc->fd);
		if (err) {
			LOG_DBG("disconnect failed, %d", err);
		}
		dlc->fd = -1;
	}
	err = client_connect(dlc);

	return err;
}

static int request_resend(struct download_client *dlc)
{
	int rc;

	switch (dlc->proto) {
	case IPPROTO_UDP:
	case IPPROTO_DTLS_1_2:
		if (IS_ENABLED(CONFIG_COAP)) {
			rc = coap_initiate_retransmission(dlc);
			if (rc) {
				error_evt_send(dlc, ETIMEDOUT);
				return -1;
			}
		}
		break;
	case IPPROTO_TCP:
	case IPPROTO_TLS_1_2:
		break;
	}

	return 0;
}

/**
 * @brief Handle socket errors.
 *
 * @param dlc
 * @return negative value to stop the handler loop, zero to retry the query.
 */
static int client_revc_error_handle(struct download_client *dlc, ssize_t len)
{
	int rc;

	if (dlc->fd == -1) {
		/* download was aborted */
		return -1;
	}

	/* If there is a partial data payload in our buffer,
	 * and it has been accounted in our progress, we have
	 * to hand it to the application before discarding it.
	 */
	if ((dlc->offset > 0) && (dlc->http.has_header)) {
		rc = fragment_evt_send(dlc);
		if (rc) {
			/* Restart and suspend */
			LOG_INF("Fragment refused, download stopped.");
			return -1;
		}
	}

	rc = ECONNRESET;

	if (len == -1) {
		if ((errno == ETIMEDOUT) ||
		    (errno == EWOULDBLOCK) ||
		    (errno == EAGAIN)) {
			k_mutex_lock(&dlc->mutex, K_FOREVER);
			rc = request_resend(dlc);
			k_mutex_unlock(&dlc->mutex);
			if (rc == -1) {
				return -1;
			} else if (rc == 1) {
				return 0;
			}
			rc = ETIMEDOUT;
		}
		LOG_ERR("Error in recv(), errno %d", errno);
	}

	if (len == 0) {
		LOG_WRN("Peer closed connection!");
	}

	/* Notify the application of the error via en event.
	 * Attempt to reconnect and resume the download
	 * if the application returns Zero via the event.
	 */
	rc = error_evt_send(dlc, rc);
	if (rc) {
		/* Restart and suspend */
		return -1;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	rc = reconnect(dlc);
	k_mutex_unlock(&dlc->mutex);
	if (rc) {
		return -1;
	}

	return 0;
}

/* Return:
 * 1 wait for more data,
 * -1 to stop
 * 0 to send a next request
 */
static int client_revc_handle(struct download_client *dlc, ssize_t len)
{
	int rc;

	LOG_DBG("Read %d bytes from socket", len);

	if (dlc->proto == IPPROTO_TCP || dlc->proto == IPPROTO_TLS_1_2) {
		rc = http_parse(dlc, len);
		if (rc > 0 &&
		    (!dlc->http.has_header || dlc->offset < sizeof(dlc->buf))) {
			/* Wait for more data (full buffer).
			 * Forward only full buffers to callback.
			 */
			return 1;
		}
	} else if (IS_ENABLED(CONFIG_COAP)) {
		rc = coap_parse(dlc, (size_t)len);
		if (rc == 1) {
			/* Duplicate packet received */
			return 1;
		}
	} else {
		/* Unknown protocol */
		rc = -EBADMSG;
	}

	if (rc < 0) {
		/* Something was wrong with the packet
		 * Restart and suspend
		 */
		error_evt_send(dlc, -rc);
		return -1;
	}

	if (dlc->file_size) {
		LOG_INF("Downloaded %u/%u bytes (%d%%)", dlc->progress, dlc->file_size,
			(dlc->progress * 100) / dlc->file_size);
	} else {
		LOG_INF("Downloaded %u bytes", dlc->progress);
	}

	/* Send fragment to application.
	 * If the application callback returns non-zero, stop.
	 */
	if (fragment_evt_send(dlc)) {
		/* Restart and suspend */
		LOG_INF("Fragment refused, download stopped.");
		rc = -1;
	}

	if (dlc->progress == dlc->file_size) {
		LOG_INF("Download complete");
		const struct download_client_evt evt = {
			.id = DOWNLOAD_CLIENT_EVT_DONE,
		};
		dlc->callback(&evt);
		/* Restart and suspend */
		rc = -1;
	} else if ((dlc->proto == IPPROTO_TCP || dlc->proto == IPPROTO_TLS_1_2) &&
		   IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS)) {
		/* Request a next range */
		rc = 0;
	}

	/* Attempt to reconnect if the connection was closed */
	if (dlc->http.connection_close && rc >= 0) {
		dlc->http.connection_close = false;
		k_mutex_lock(&dlc->mutex, K_FOREVER);
		rc = reconnect(dlc);
		k_mutex_unlock(&dlc->mutex);
		if (rc) {
			error_evt_send(dlc, EHOSTDOWN);
			rc = -1;
		}
	}
	return rc;
}

static int handle_disconnect(struct download_client *dlc)
{
	int err = 0;

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	if (dlc->fd != -1) {
		err = close(dlc->fd);
		if (err) {
			err = -errno;
			LOG_ERR("Failed to close socket, errno %d", -err);
		}
	}

	dlc->fd = -1;
	dlc->close_when_done = false;
	dlc->host = NULL;
	dlc->file = NULL;
	set_state(dlc, DOWNLOAD_CLIENT_IDLE);
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_CLOSED,
	};
	dlc->callback(&evt);

	k_mutex_unlock(&dlc->mutex);

	return err;
}

void download_thread(void *cli, void *a, void *b)
{
	int rc;
	ssize_t len;
	struct download_client *const dlc = cli;
	bool send_request = false;

	while (true) {
		rc = 0;

		/* Wait for action */
		k_sem_take(&dlc->wait_for_download, K_FOREVER);

		/* Connect to the target host */
		if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTING)) {
			rc = client_connect(dlc);

			if (rc) {
				continue;
			}

			/* Initialize CoAP */
			if (use_coap(dlc)) {
				coap_block_init(dlc, dlc->progress);
			}

			len = 0;
			send_request = true;

			set_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING);
		}

		/* Request loop */
		while (is_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING)) {
			if (send_request) {
				/* Request next fragment */
				dlc->offset = 0;
				rc = request_send(dlc);
				send_request = false;
				if (rc) {
					rc = error_evt_send(dlc, ECONNRESET);
					if (rc) {
						/* Restart and suspend */
						break;
					}

					rc = reconnect(dlc);
					if (rc) {
						break;
					}
					/* Send request again */
					send_request = true;
					continue;
				}
			}

			__ASSERT(dlc->offset < sizeof(dlc->buf), "Buffer overflow");

			if (sizeof(dlc->buf) - dlc->offset == 0) {
				LOG_ERR("Could not fit HTTP header from server (> %d)",
					sizeof(dlc->buf));
				error_evt_send(dlc, E2BIG);
				break;
			}

			LOG_DBG("Receiving up to %d bytes at %p...", (sizeof(dlc->buf) - dlc->offset),
				(void *)(dlc->buf + dlc->offset));

			len = client_socket_recv(dlc);
			if (len <= 0) {
				rc = client_revc_error_handle(dlc, len);
			} else {
				rc = client_revc_handle(dlc, len);
			}

			if (rc < 0) {
				break;
			} else if (rc == 0) {
				/* Send request again */
				send_request = true;
			}
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING)) {
			if (dlc->close_when_done) {
				set_state(dlc, DOWNLOAD_CLIENT_CLOSING);
			} else {
				set_state(dlc, DOWNLOAD_CLIENT_FINISHED);
			}
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_CLOSING)) {
			handle_disconnect(dlc);
			LOG_DBG("Connection closed");
		}

		/* Do not let the thread return, since it can't be restarted */
	}
}

int download_client_init(struct download_client *const dlc,
			 download_client_callback_t callback)
{
	if (dlc == NULL || callback == NULL) {
		return -EINVAL;
	}

	memset(dlc, 0, sizeof(*dlc));
	dlc->fd = -1;
	dlc->callback = callback;
	k_sem_init(&dlc->wait_for_download, 0, 1);
	k_mutex_init(&dlc->mutex);

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	/* The thread is spawned now, but it will suspend itself;
	 * it is resumed when the download is started via the API.
	 */
	dlc->tid =
		k_thread_create(&dlc->thread, dlc->thread_stack,
				K_THREAD_STACK_SIZEOF(dlc->thread_stack),
				download_thread, dlc, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(dlc->tid, "download_client");

	k_mutex_unlock(&dlc->mutex);

	return 0;
}

int download_client_set_host(struct download_client *dlc, const char *host,
			    const struct download_client_cfg *config)
{
	if (dlc == NULL || host == NULL || config == NULL) {
		return -EINVAL;
	}

	if (config->frag_size_override > CONFIG_DOWNLOAD_CLIENT_BUF_SIZE) {
		LOG_ERR("The configured fragment size is larger than buffer");
		return -E2BIG;
	}


	k_mutex_lock(&dlc->mutex, K_FOREVER);

	if (!is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
		k_mutex_unlock(&dlc->mutex);
		return -EALREADY;
	}

	dlc->config = *config;
	dlc->host = host;
	dlc->close_when_done = false;
	k_mutex_unlock(&dlc->mutex);
	return 0;
}

int download_client_disconnect(struct download_client *const dlc)
{
	if (dlc == NULL || is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
		return -EINVAL;
	}

	set_state(dlc, DOWNLOAD_CLIENT_CLOSING);
	k_sem_give(&dlc->wait_for_download);

	return 0;
}

int download_client_start(struct download_client *dlc, const char *file,
			  size_t from)
{
	if (dlc == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	if (dlc->host == NULL) {
		k_mutex_unlock(&dlc->mutex);
		return -EINVAL;
	}

	if (!is_state(dlc, DOWNLOAD_CLIENT_IDLE) && !is_state(dlc, DOWNLOAD_CLIENT_FINISHED)) {
		k_mutex_unlock(&dlc->mutex);
		return -EALREADY;
	}

	dlc->file = file;
	dlc->file_size = 0;
	dlc->progress = from;
	dlc->offset = 0;
	dlc->http.has_header = false;
	if (is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
		set_state(dlc, DOWNLOAD_CLIENT_CONNECTING);
	} else {
		set_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING);
	}

	k_mutex_unlock(&dlc->mutex);

	LOG_INF("Downloading: %s [%u]", dlc->file, dlc->progress);

	/* Let the thread run */
	k_sem_give(&dlc->wait_for_download);
	return 0;
}

int download_client_get(struct download_client *dlc, const char *host,
			const struct download_client_cfg *config, const char *file, size_t from)
{
	int rc;

	if (dlc == NULL) {
		return -EINVAL;
	}

	if (file == NULL) {
		file = host;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	rc = download_client_set_host(dlc, host, config);

	if (rc == 0) {
		dlc->close_when_done = true;
		rc = download_client_start(dlc, file, from);
	}

	k_mutex_unlock(&dlc->mutex);

	return rc;
}

int download_client_file_size_get(struct download_client *dlc, size_t *size)
{
	if (!dlc || !size) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	*size = dlc->file_size;
	k_mutex_unlock(&dlc->mutex);

	return 0;
}

int download_client_downloaded_size_get(struct download_client *dlc, size_t *size)
{
	if (!dlc || !size) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	*size = dlc->progress;
	k_mutex_unlock(&dlc->mutex);

	return 0;
}
