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

static int client_close(struct download_client *dlc);
static int error_evt_send(const struct download_client *dlc, int error);

#if CONFIG_COAP
bool use_coap(struct download_client *dlc)
{
	return (dlc->sock.proto == IPPROTO_UDP || dlc->sock.proto == IPPROTO_DTLS_1_2);
}
#endif

bool use_http(struct download_client *dlc) {
	return (dlc->sock.proto == IPPROTO_TCP || dlc->sock.proto == IPPROTO_TLS_1_2);
}

static bool is_state(struct download_client *dlc, enum download_client_state state)
{
	bool ret;

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	ret = dlc->state == state;
	k_mutex_unlock(&dlc->mutex);
	return ret;
}

#if CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL_DBG
char *state_to_str(int state)
{
	switch (state) {
	case DOWNLOAD_CLIENT_IDLE:
		return "IDLE";
	case DOWNLOAD_CLIENT_CONNECTING:
		return "CONNECTING";
	case DOWNLOAD_CLIENT_CONNECTED:
		return "CONNECTED";
	case DOWNLOAD_CLIENT_DOWNLOADING:
		return "DOWNLOADING";
	case DOWNLOAD_CLIENT_DEINITIALIZING:
		return "DEINITIALIZING";
	case DOWNLOAD_CLIENT_DEINITIALIZED:
		return "DEINITIALIZED";
	}

	return "unknown";
}
#else
char *state_to_str(int state) {
	return "";
}
#endif

static void state_set(struct download_client *dlc, int state)
{
	k_mutex_lock(&dlc->mutex, K_FOREVER);
	dlc->state = state;
	k_mutex_unlock(&dlc->mutex);
	LOG_DBG("state = %d (%s)", state, state_to_str(state));
}

static int client_connect(struct download_client *dlc)
{
	int err;
	int ns_err;

	err = -1;
	ns_err = -1;

	err = client_socket_configure_and_connect(dlc);
	if (err) {
		goto cleanup;
	}

#if CONFIG_COAP
	/* Initialize CoAP */
	if (use_coap(dlc)) {
		coap_block_init(dlc, dlc->progress);
	}
#endif

cleanup:
	if (err) {
		/* Unable to connect, close socket */
		client_close(dlc);
		state_set(dlc, DOWNLOAD_CLIENT_IDLE);
	}

	return err;
}

static int client_close(struct download_client *dlc)
{
	int err;
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_CLOSED,
	};

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	err = client_socket_close(dlc);

	dlc->host_config.hostname = NULL;
	dlc->file = NULL;

	dlc->config.callback(&evt);

	k_mutex_unlock(&dlc->mutex);

	return err;
}

static int request_send(struct download_client *dlc)
{
	if (dlc->sock.fd < 0) {
		return -1;
	}

	if (use_http(dlc)) {
		return http_get_request_send(dlc);
#if CONFIG_COAP
	} else if (use_coap(dlc)) {
		return coap_request_send(dlc);
#endif
	}

	return 0;
}

static int fragment_evt_send(struct download_client *dlc)
{
	__ASSERT(dlc->buf_offset <= dlc->config.buf_size,
		 "Buffer overflow!");

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_FRAGMENT,
		.fragment = {
			.buf = dlc->config.buf,
			.len = dlc->buf_offset,
		}
	};

	dlc->buf_offset = 0;

	return dlc->config.callback(&evt);
}

static int error_evt_send(const struct download_client *dlc, int error)
{
	/* Error will be sent as negative. */
	__ASSERT_NO_MSG(error < 0);

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_ERROR,
		.error = error
	};

	return dlc->config.callback(&evt);
}

static int deinit_evt_send(const struct download_client *dlc)
{
	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_DEINITIALIZED,
	};

	return dlc->config.callback(&evt);
}

static int reconnect(struct download_client *dlc)
{
	int err = 0;

	LOG_INF("Reconnecting...");

	if (dlc->sock.fd >= 0) {
		err = close(dlc->sock.fd);
		if (err) {
			LOG_DBG("disconnect failed, %d", err);
		}
		dlc->sock.fd = -1;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);
	state_set(dlc, DOWNLOAD_CLIENT_CONNECTING);
	k_mutex_unlock(&dlc->mutex);


	return err;
}

static int request_resend(struct download_client *dlc)
{
#if CONFIG_COAP
	int rc;

	if (use_coap(dlc)) {
		rc = coap_initiate_retransmission(dlc);
			if (rc) {
				error_evt_send(dlc, -ETIMEDOUT);
				return -1;
			}
	}
#endif

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

	if (dlc->sock.fd == -1) {
		/* download was aborted */
		return -1;
	}

	/* If there is a partial data payload in our buffer,
	 * and it has been accounted in our progress, we have
	 * to hand it to the application before discarding it.
	 */
	if ((dlc->buf_offset > 0) && (dlc->http.header.has_end)) {
		rc = fragment_evt_send(dlc);
		if (rc) {
			/* Restart and suspend */
			LOG_INF("Fragment refused, download stopped.");
			return -1;
		}
	}

	rc = -ECONNRESET;

	if (len == -1) {
		if ((errno == ETIMEDOUT) ||
		    (errno == EWOULDBLOCK) ||
		    (errno == EAGAIN)) {
			k_mutex_lock(&dlc->mutex, K_FOREVER);
			rc = request_resend(dlc);
			k_mutex_unlock(&dlc->mutex);
			if (rc == -1) {
				return -1;
			}
			rc = -ETIMEDOUT;
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

	rc = reconnect(dlc);
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

	if (use_http(dlc)) {
		rc = http_parse(dlc, len);
		if (rc) {
			return rc;
		}

		if (dlc->http.header.has_end && dlc->buf_offset) {
			rc = fragment_evt_send(dlc);
			if (rc) {
				/* Restart and suspend */
				LOG_INF("Fragment refused (%d), download stopped.", rc);
				return rc;
			}
		}
#if CONFIG_COAP
	} else if (use_coap(dlc)) {
		rc = coap_parse(dlc, (size_t)len);
		if (rc < 0) {
			return rc;
		}
#endif
	} else {
		return -EBADMSG;
	}

	return 0;
}

static void restart_and_suspend(struct download_client *dlc) {
	if (dlc->host_config.close_when_done) {
		client_close(dlc);
		state_set(dlc, DOWNLOAD_CLIENT_IDLE);
		return;
	}

	state_set(dlc, DOWNLOAD_CLIENT_CONNECTED);
}


void download_thread(void *cli, void *a, void *b)
{
	int rc;
	ssize_t len;
	struct download_client *const dlc = cli;

	while (true) {
		rc = 0;

		if (is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
			/* Client idle, wait for action */
			k_sem_take(&dlc->wait_for_download, K_FOREVER);
		}

		/* Connect to the target host */
		if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTING)) {
			/* Client */
			rc = client_connect(dlc);
			if (rc) {
				error_evt_send(dlc, rc);
				/* Try connecting again */
				continue;
			}

			/* Connection successful */
			k_mutex_lock(&dlc->mutex, K_FOREVER);
			if (dlc->file) {
				dlc->new_data_req = true;
				state_set(dlc, DOWNLOAD_CLIENT_DOWNLOADING);
				k_mutex_unlock(&dlc->mutex);
				continue;
			}

			state_set(dlc, DOWNLOAD_CLIENT_CONNECTED);
			k_mutex_unlock(&dlc->mutex);
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
			/* Client connected, wait for action */
			k_sem_take(&dlc->wait_for_download, K_FOREVER);
		}

		/* Request loop */
		if (is_state(dlc, DOWNLOAD_CLIENT_DOWNLOADING)) {
			if (dlc->new_data_req) {
				/* Request next fragment */
				dlc->buf_offset = 0;
				rc = request_send(dlc);
				if (rc) {
					rc = error_evt_send(dlc, -ECONNRESET);
					if (rc) {
						restart_and_suspend(dlc);
					}

					rc = reconnect(dlc);
					if (rc) {
						restart_and_suspend(dlc);
					}
					continue;
				}

				dlc->new_data_req = false;
			}

			__ASSERT(dlc->buf_offset < dlc->config.buf_size, "Buffer overflow");

			LOG_DBG("Receiving up to %d bytes at %p...",
				(dlc->config.buf_size - dlc->buf_offset),
				(void *)(dlc->config.buf + dlc->buf_offset));

			len = client_socket_recv(dlc);
			if (len <= 0) {
				rc = client_revc_error_handle(dlc, len);
				if (rc < 0) {
					restart_and_suspend(dlc);
				}
			} else {
				/* Accumulate buffer offset */
				dlc->buf_offset += len;
				rc = client_revc_handle(dlc, len);
				if (rc < 0) {
					error_evt_send(dlc, rc);
					restart_and_suspend(dlc);
				}

				if (dlc->file_size) {
					LOG_INF("Downloaded %u/%u bytes (%d%%)",
						dlc->progress, dlc->file_size,
						(dlc->progress * 100) / dlc->file_size);
				} else {
					LOG_INF("Downloaded %u bytes", dlc->progress);
				}

				if (dlc->progress == dlc->file_size) {
					LOG_INF("Download complete");
					const struct download_client_evt evt = {
						.id = DOWNLOAD_CLIENT_EVT_DONE,
					};
					dlc->config.callback(&evt);
					restart_and_suspend(dlc);
				}
			}

			/* Attempt to reconnect if the connection was closed */
			if (dlc->http.connection_close) {
				dlc->http.connection_close = false;
				rc = reconnect(dlc);
				if (rc) {
					error_evt_send(dlc, -EHOSTDOWN);
				}
			}
		}

		if (is_state(dlc, DOWNLOAD_CLIENT_DEINITIALIZING)) {
			client_close(dlc);
			state_set(dlc, DOWNLOAD_CLIENT_DEINITIALIZED);
			deinit_evt_send(dlc);
			return;
		}
	}
}

int download_client_init(struct download_client *const dlc,
			 struct download_client_cfg *config)
{
	if (dlc == NULL || config == NULL || config->callback == NULL ||
	    config->buf == NULL || config->buf_size == 0) {
		return -EINVAL;
	}

	memset(dlc, 0, sizeof(*dlc));
	dlc->sock.fd = -1;
	dlc->config = *config;
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

int download_client_deinit(struct download_client *const dlc)
{
	if (dlc == NULL) {
		return -EINVAL;
	}

	state_set(dlc, DOWNLOAD_CLIENT_DEINITIALIZING);

	return 0;
}

int download_client_start(struct download_client *dlc,
			  const struct download_client_host_cfg *host_config,
			  const char *file, size_t from)
{
	int err;
	bool host_connected;

	if (dlc == NULL || host_config == NULL || host_config->hostname == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	if (!is_state(dlc, DOWNLOAD_CLIENT_IDLE) &&
	    !is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
		k_mutex_unlock(&dlc->mutex);
		return -EPERM;
	}

	host_connected = is_state(dlc, DOWNLOAD_CLIENT_CONNECTED) &&
			 dlc->host_config.hostname == host_config->hostname;

	dlc->host_config = *host_config;
	dlc->file = file;
	dlc->file_size = 0;
	dlc->progress = from;
	dlc->buf_offset = 0;

	/* Socket configuration */
	err = url_parse_proto(dlc->host_config.hostname, &dlc->sock.proto, &dlc->sock.type);
	if (err == -EINVAL) {
		LOG_DBG("Protocol not specified, defaulting to HTTP(S)");
		dlc->sock.type = SOCK_STREAM;
		if (dlc->host_config.sec_tag_list && (dlc->host_config.sec_tag_count > 0)) {
			dlc->sock.proto = IPPROTO_TLS_1_2;
		} else {
			dlc->sock.proto = IPPROTO_TCP;
		}
	} else if (err) {
		return err;
	}

	if ((dlc->sock.proto == IPPROTO_UDP || dlc->sock.proto == IPPROTO_DTLS_1_2) &&
	    (!IS_ENABLED(CONFIG_COAP))) {
		return -EPROTONOSUPPORT;
	}

	if (dlc->sock.proto == IPPROTO_TLS_1_2 || dlc->sock.proto == IPPROTO_DTLS_1_2) {
		if (dlc->host_config.sec_tag_list == NULL || dlc->host_config.sec_tag_count == 0) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			return -EINVAL;
		}
	}

	if ((dlc->host_config.sec_tag_list == NULL || dlc->host_config.sec_tag_count == 0) &&
	    dlc->host_config.set_tls_hostname) {
		LOG_WRN("set_tls_hostname flag is set for non-TLS connection");
		return -EINVAL;
	}

	err = url_parse_port(dlc->host_config.hostname, &dlc->sock.port);
	if (err) {
		switch (dlc->sock.proto) {
		case IPPROTO_TLS_1_2:
			dlc->sock.port = 443;
			break;
		case IPPROTO_TCP:
		 	dlc->sock.port = 80;
			break;
		case IPPROTO_DTLS_1_2:
			dlc->sock.port = 5684;
			break;
		case IPPROTO_UDP:
			dlc->sock.port = 5683;
			break;
		}
		LOG_DBG("Port not specified, using default: %d", dlc->sock.port);
	}

	if (dlc->host_config.set_native_tls) {
		LOG_DBG("Enabled native TLS");
		dlc->sock.type |= SOCK_NATIVE_TLS;
	}

	if (use_http(dlc)) {
		dlc->http.header.has_end = false;
		dlc->http.header.hdr_len = 0;
		dlc->http.header.status_code = 0;
	}

	if (is_state(dlc, DOWNLOAD_CLIENT_CONNECTED)) {
		if (host_connected) {
			state_set(dlc, DOWNLOAD_CLIENT_DOWNLOADING);
		} else {
			/* We are connected to the wrong host */
			LOG_DBG("Closing connection to connect to different host");
			client_socket_close(dlc);
			state_set(dlc, DOWNLOAD_CLIENT_CONNECTING);
		}
	} else {
		/* IDLE */
		state_set(dlc, DOWNLOAD_CLIENT_CONNECTING);
	}

	k_mutex_unlock(&dlc->mutex);

	/* Let the thread run */
	k_sem_give(&dlc->wait_for_download);
	return 0;
}

int download_client_stop(struct download_client *const dlc)
{
	if (dlc == NULL || is_state(dlc, DOWNLOAD_CLIENT_IDLE)) {
		return -EINVAL;
	}

	client_close(dlc);
	state_set(dlc, DOWNLOAD_CLIENT_IDLE);

	return 0;
}

int download_client_get(struct download_client *dlc,
			const struct download_client_host_cfg *host_config,
			const char *file, size_t from)
{
	int rc;

	if (dlc == NULL) {
		return -EINVAL;
	}

	if (file == NULL) {
		file = host_config->hostname;
	}

	k_mutex_lock(&dlc->mutex, K_FOREVER);

	dlc->host_config.close_when_done = true;
	rc = download_client_start(dlc, host_config, file, from);

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
