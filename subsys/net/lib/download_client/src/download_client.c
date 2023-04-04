/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain/common.h>
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <zephyr/net/tls_credentials.h>
#include <net/download_client.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define SIN6(A) ((struct sockaddr_in6 *)(A))
#define SIN(A) ((struct sockaddr_in *)(A))

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE

int url_parse_port(const char *url, uint16_t *port);
int url_parse_proto(const char *url, int *proto, int *type);
int url_parse_host(const char *url, char *host, size_t len);

int http_parse(struct download_client *client, size_t len);
int http_get_request_send(struct download_client *client);

int coap_block_init(struct download_client *client, size_t from);
int coap_get_recv_timeout(struct download_client *dl);
int coap_initiate_retransmission(struct download_client *dl);
int coap_parse(struct download_client *client, size_t len);
int coap_request_send(struct download_client *client);

static int handle_disconnect(struct download_client *client);

static bool is_idle(struct download_client *client)
{
	bool ret;

	k_mutex_lock(&client->mutex, K_FOREVER);
	ret = client->state == DOWNLOAD_CLIENT_IDLE;
	k_mutex_unlock(&client->mutex);
	return ret;
}

static bool is_connecting(struct download_client *client)
{
	bool ret;

	k_mutex_lock(&client->mutex, K_FOREVER);
	ret = client->state == DOWNLOAD_CLIENT_CONNECTING;
	k_mutex_unlock(&client->mutex);
	return ret;
}

static bool is_downloading(struct download_client *client)
{
	bool ret;

	k_mutex_lock(&client->mutex, K_FOREVER);
	ret = client->state == DOWNLOAD_CLIENT_DOWNLOADING;
	k_mutex_unlock(&client->mutex);
	return ret;
}

static bool is_closing(struct download_client *client)
{
	bool ret;

	k_mutex_lock(&client->mutex, K_FOREVER);
	ret = client->state == DOWNLOAD_CLIENT_CLOSING;
	k_mutex_unlock(&client->mutex);
	return ret;
}

static bool is_finished(struct download_client *client)
{
	bool ret;

	k_mutex_lock(&client->mutex, K_FOREVER);
	ret = client->state == DOWNLOAD_CLIENT_FINNISHED;
	k_mutex_unlock(&client->mutex);
	return ret;
}

static void set_state(struct download_client *client, int state)
{
	k_mutex_lock(&client->mutex, K_FOREVER);
	/* Prevent moving back to CLOSING from IDLE */
	if ((state != DOWNLOAD_CLIENT_CLOSING) || (client->state != DOWNLOAD_CLIENT_IDLE)) {
		client->state = state;
	}
	k_mutex_unlock(&client->mutex);
	LOG_DBG("state = %d", state);
}

static const char *str_family(int family)
{
	switch (family) {
	case AF_INET:
		return "IPv4";
	case AF_INET6:
		return "IPv6";
	default:
		__ASSERT(false, "Unsupported family");
		return NULL;
	}
}

static int set_recv_socket_timeout(int fd, int timeout_ms)
{
	int err;

	if (timeout_ms <= 0) {
		return 0;
	}

	if (fd == -1) {
		return -1;
	}

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};

	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -1;
	}

	return 0;
}

static int set_snd_socket_timeout(int fd, int timeout_ms)
{
	int err;

	if (timeout_ms <= 0) {
		return 0;
	}

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};

	err = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -1;
	}

	return 0;
}

static int socket_sectag_set(int fd, int sec_tag)
{
	int err;
	int verify;
	sec_tag_t sec_tag_list[] = { sec_tag };

	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, errno %d", errno);
		return -errno;
	}

	LOG_INF("Setting up TLS credentials, tag %d", sec_tag);
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
	if (err) {
		LOG_ERR("Failed to setup socket security tag, errno %d", errno);
		return -errno;
	}

	return 0;
}

static int socket_tls_hostname_set(int fd, const char * const hostname)
{
	__ASSERT_NO_MSG(hostname);

	char parsed_host[HOSTNAME_SIZE];
	int err;

	err = url_parse_host(hostname, parsed_host, sizeof(parsed_host));
	if (err) {
		LOG_ERR("Failed to parse host, err %d", err);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, parsed_host,
			 strlen(parsed_host));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname (%s), errno %d",
			parsed_host, errno);
		return -errno;
	}

	return 0;
}

static int socket_pdn_id_set(int fd, uint8_t pdn_id)
{
	int err;
	char buf[8] = {0};

	(void) snprintf(buf, sizeof(buf), "pdn%d", pdn_id);

	LOG_INF("Binding to PDN ID: %s", buf);
	err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &buf, strlen(buf));
	if (err) {
		LOG_ERR("Failed to bind socket to PDN ID %d, err %d",
			pdn_id, errno);
		return -ENETDOWN;
	}

	return 0;
}

static int host_lookup(const char *host, int family, uint8_t pdn_id,
		       struct sockaddr *sa)
{
	int err;
	char pdnserv[4];
	char hostname[HOSTNAME_SIZE];
	struct addrinfo *ai;

	struct addrinfo hints = {
		.ai_family = family,
	};

	/* Extract the hostname, without protocol or port */
	err = url_parse_host(host, hostname, sizeof(hostname));
	if (err) {
		return err;
	}

	if (pdn_id) {
		hints.ai_flags = AI_PDNSERV;
		(void)snprintf(pdnserv, sizeof(pdnserv), "%d", pdn_id);
		err = getaddrinfo(hostname, pdnserv, &hints, &ai);
	} else {
		err = getaddrinfo(hostname, NULL, &hints, &ai);
	}

	if (err) {
		LOG_WRN("Failed to resolve hostname %s on %s",
			hostname, str_family(family));
		return -EHOSTUNREACH;
	}

	*sa = *(ai->ai_addr);
	freeaddrinfo(ai);

	return 0;
}

static int client_connect(struct download_client *dl)
{
	int err;
	int type;
	uint16_t port;
	socklen_t addrlen;

	/* Attempt IPv6 connection if configured, fallback to IPv4 */
	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_IPV6)) {
		err = host_lookup(dl->host, AF_INET6, dl->config.pdn_id, &dl->remote_addr);
	}
	if (err || !IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_IPV6)) {
		err = host_lookup(dl->host, AF_INET, dl->config.pdn_id, &dl->remote_addr);
	}
	if (err) {
		return err;
	}

	err = url_parse_proto(dl->host, &dl->proto, &type);
	if (err) {
		LOG_DBG("Protocol not specified, defaulting to HTTP(S)");
		type = SOCK_STREAM;
		if (dl->config.sec_tag != -1) {
			dl->proto = IPPROTO_TLS_1_2;
		} else {
			dl->proto = IPPROTO_TCP;
		}
	}

	if (dl->proto == IPPROTO_UDP || dl->proto == IPPROTO_DTLS_1_2) {
		if (!IS_ENABLED(CONFIG_COAP)) {
			return -EPROTONOSUPPORT;
		}
	}

	if (dl->proto == IPPROTO_TLS_1_2 || dl->proto == IPPROTO_DTLS_1_2) {
		if (dl->config.sec_tag == -1) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			return -EINVAL;
		}
	}

	if (dl->config.sec_tag == -1 && dl->config.set_tls_hostname) {
		LOG_WRN("set_tls_hostname flag is set for non-TLS connection");
		return -EINVAL;
	}

	err = url_parse_port(dl->host, &port);
	if (err) {
		switch (dl->proto) {
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

	switch (dl->remote_addr.sa_family) {
	case AF_INET6:
		SIN6(&dl->remote_addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		SIN(&dl->remote_addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	if (dl->set_native_tls) {
		LOG_DBG("Enabled native TLS");
		type |= SOCK_NATIVE_TLS;
	}

	LOG_DBG("family: %d, type: %d, proto: %d",
		dl->remote_addr.sa_family, type, dl->proto);

	dl->fd = socket(dl->remote_addr.sa_family, type, dl->proto);
	if (dl->fd < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		return -errno;
	}

	if (dl->config.pdn_id) {
		err = socket_pdn_id_set(dl->fd, dl->config.pdn_id);
		if (err) {
			goto cleanup;
		}
	}

	if ((dl->proto == IPPROTO_TLS_1_2 || dl->proto == IPPROTO_DTLS_1_2)
	     && (dl->config.sec_tag != -1)) {
		err = socket_sectag_set(dl->fd, dl->config.sec_tag);
		if (err) {
			goto cleanup;
		}

		if (dl->config.set_tls_hostname) {
			err = socket_tls_hostname_set(dl->fd, dl->host);
			if (err) {
				goto cleanup;
			}
		}
	}

	LOG_INF("Connecting to %s", dl->host);
	LOG_DBG("fd %d, addrlen %d, fam %s, port %d",
		dl->fd, addrlen, str_family(dl->remote_addr.sa_family), port);

	err = connect(dl->fd, &dl->remote_addr, addrlen);
	if (err) {
		err = -errno;
		LOG_ERR("Unable to connect, errno %d", -err);
	}

cleanup:
	if (err) {
		/* Unable to connect, close socket */
		download_client_disconnect(dl);
	}

	return err;
}

int socket_send(const struct download_client *client, size_t len, int timeout)
{
	int err;
	int sent;
	size_t off = 0;

	err = set_snd_socket_timeout(client->fd, timeout);
	if (err) {
		return -errno;
	}

	while (len) {
		sent = send(client->fd, client->buf + off, len, 0);
		if (sent < 0) {
			return -errno;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

static int request_send(struct download_client *dl)
{
	if (dl->fd < 0) {
		return -1;
	}
	switch (dl->proto) {
	case IPPROTO_TCP:
	case IPPROTO_TLS_1_2:
		return http_get_request_send(dl);
	case IPPROTO_UDP:
	case IPPROTO_DTLS_1_2:
		if (IS_ENABLED(CONFIG_COAP)) {
			return coap_request_send(dl);
		}
	}

	return 0;
}

static int fragment_evt_send(struct download_client *client)
{
	__ASSERT(client->offset <= CONFIG_DOWNLOAD_CLIENT_BUF_SIZE,
		 "Buffer overflow!");

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_FRAGMENT,
		.fragment = {
			.buf = client->buf,
			.len = client->offset,
		}
	};

	client->offset = 0;

	return client->callback(&evt);
}

static int error_evt_send(const struct download_client *dl, int error)
{
	/* Error will be sent as negative. */
	__ASSERT_NO_MSG(error > 0);

	const struct download_client_evt evt = {
		.id = DOWNLOAD_CLIENT_EVT_ERROR,
		.error = -error
	};

	return dl->callback(&evt);
}

static int reconnect(struct download_client *dl)
{
	int err;

	LOG_INF("Reconnecting..");
	if (dl->fd >= 0) {
		err = close(dl->fd);
		if (err) {
			LOG_DBG("disconnect failed, %d", err);
			return err;
		}
		dl->fd = -1;
	}
	err = client_connect(dl);

	return err;
}

static ssize_t socket_recv(struct download_client *dl)
{
	int err, timeout = 0;

	switch (dl->proto) {
	case IPPROTO_TCP:
	case IPPROTO_TLS_1_2:
		timeout = CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS;
		break;
	case IPPROTO_UDP:
	case IPPROTO_DTLS_1_2:
		if (IS_ENABLED(CONFIG_COAP)) {
			timeout = coap_get_recv_timeout(dl);
			if (timeout == 0) {
				errno = ETIMEDOUT;
				return -1;
			}
			break;
		}
	default:
		LOG_ERR("unhandled proto");
		return -1;
	}

	err = set_recv_socket_timeout(dl->fd, timeout);
	if (err) {
		return -1;
	}

	return recv(dl->fd, dl->buf + dl->offset, sizeof(dl->buf) - dl->offset, 0);
}

static int request_resend(struct download_client *dl)
{
	int rc;

	if (dl->proto == IPPROTO_UDP || dl->proto == IPPROTO_DTLS_1_2) {
		LOG_DBG("Socket timeout, resending");

		if (IS_ENABLED(CONFIG_COAP)) {
			rc = coap_initiate_retransmission(dl);
			if (rc) {
				error_evt_send(dl, ETIMEDOUT);
				return -1;
			}
		}

		return 1;
	}

	return 0;
}

/**
 * @brief Handle socket errors.
 *
 * @param dl
 * @return negative value to stop the handler loop, zero to rettry the query.
 */
static int handle_socket_error(struct download_client *dl, ssize_t len)
{
	int rc;

	if (dl->fd == -1) {
		/* download was aborted */
		return -1;
	}

	/* If there is a partial data payload in our buffer,
	 * and it has been accounted in our progress, we have
	 * to hand it to the application before discarding it.
	 */
	if ((dl->offset > 0) && (dl->http.has_header)) {
		rc = fragment_evt_send(dl);
		if (rc) {
			/* Restart and suspend */
			LOG_INF("Fragment refused, download stopped.");
			return -1;
		}
	}

	rc = ECONNRESET;

	if (len == -1) {
		if ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK) ||
			(errno == EAGAIN)) {
			k_mutex_lock(&dl->mutex, K_FOREVER);
			rc = request_resend(dl);
			k_mutex_unlock(&dl->mutex);
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
	rc = error_evt_send(dl, rc);
	if (rc) {
		/* Restart and suspend */
		return -1;
	}

	k_mutex_lock(&dl->mutex, K_FOREVER);
	rc = reconnect(dl);
	k_mutex_unlock(&dl->mutex);
	if (rc) {
		error_evt_send(dl, EHOSTDOWN);
		return -1;
	}

	return 0;
}

/* Return:
 * 1 wait for more data,
 * -1 to stop
 * 0 to send a next request
 */
static int handle_received(struct download_client *dl, ssize_t len)
{
	int rc;

	LOG_DBG("Read %d bytes from socket", len);

	if (dl->proto == IPPROTO_TCP || dl->proto == IPPROTO_TLS_1_2) {
		rc = http_parse(dl, len);
		if (rc > 0 &&
		    (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS) || !dl->http.has_header)) {
			/* Wait for more data (fragment/header).
			 * Unranged reguest (normal GET) should start forwarding the data
			 * once HTTP headers are received. Ranged-GET should receive full
			 * buffer(fragment) before sending an event.
			 */
			return 1;
		}
	} else if (IS_ENABLED(CONFIG_COAP)) {
		rc = coap_parse(dl, (size_t)len);
		if (rc == 1) {
			/* Duplicate packet received */
			return 1;
		}
	} else {
		/* Unknown protocol */
		rc = -1;
	}

	if (rc < 0) {
		/* Something was wrong with the packet
		 * Restart and suspend
		 */
		error_evt_send(dl, EBADMSG);
		return -1;
	}

	if (dl->file_size) {
		LOG_INF("Downloaded %u/%u bytes (%d%%)", dl->progress, dl->file_size,
			(dl->progress * 100) / dl->file_size);
	} else {
		LOG_INF("Downloaded %u bytes", dl->progress);
	}

	/* Send fragment to application.
	 * If the application callback returns non-zero, stop.
	 */
	if (fragment_evt_send(dl)) {
		/* Restart and suspend */
		LOG_INF("Fragment refused, download stopped.");
		rc = -1;
	}

	if (dl->progress == dl->file_size) {
		LOG_INF("Download complete");
		const struct download_client_evt evt = {
			.id = DOWNLOAD_CLIENT_EVT_DONE,
		};
		dl->callback(&evt);
		/* Restart and suspend */
		rc = -1;
	} else if ((dl->proto == IPPROTO_TCP || dl->proto == IPPROTO_TLS_1_2) &&
		   IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS)) {
		/* Request a next range */
		rc = 0;
	}

	/* Attempt to reconnect if the connection was closed */
	if (dl->http.connection_close && rc >= 0) {
		dl->http.connection_close = false;
		k_mutex_lock(&dl->mutex, K_FOREVER);
		rc = reconnect(dl);
		k_mutex_unlock(&dl->mutex);
		if (rc) {
			error_evt_send(dl, EHOSTDOWN);
			rc = -1;
		}
	}
	return rc;
}

void download_thread(void *client, void *a, void *b)
{
	int rc;
	ssize_t len;
	struct download_client *const dl = client;
	bool send_request;

	while (true) {
		rc = 0;

		/* Wait for action */
		k_sem_take(&dl->wait_for_download, K_FOREVER);

		/* Connect to the target host */
		k_mutex_lock(&dl->mutex, K_FOREVER);
		if (dl->fd == -1 && dl->host != NULL) {
			rc = client_connect(dl);
		}
		k_mutex_unlock(&dl->mutex);

		if (rc) {
			error_evt_send(dl, -rc);
			continue;
		}

		/* Initialize CoAP */
		k_mutex_lock(&dl->mutex, K_FOREVER);
		if (IS_ENABLED(CONFIG_COAP) &&
			(dl->proto == IPPROTO_UDP || dl->proto == IPPROTO_DTLS_1_2)) {
			coap_block_init(client, dl->progress);
		}
		k_mutex_unlock(&dl->mutex);

		len = 0;
		send_request = true;

		/* Request loop */
		while (true) {
			if (send_request) {
				/* Request next fragment */
				k_mutex_lock(&dl->mutex, K_FOREVER);
				dl->offset = 0;
				rc = request_send(dl);
				k_mutex_unlock(&dl->mutex);
				send_request = false;
				if (rc) {
					if (rc == -1) {
						/* download was aborted */
						break;
					}

					rc = error_evt_send(dl, ECONNRESET);
					if (rc) {
						/* Restart and suspend */
						break;
					}

					rc = reconnect(dl);
					if (rc) {
						error_evt_send(dl, EHOSTDOWN);
						break;
					}
					/* Send request again */
					send_request = true;
					continue;
				}
			}

			__ASSERT(dl->offset < sizeof(dl->buf), "Buffer overflow");

			if (sizeof(dl->buf) - dl->offset == 0) {
				LOG_ERR("Could not fit HTTP header from server (> %d)",
					sizeof(dl->buf));
				error_evt_send(dl, E2BIG);
				break;
			}

			LOG_DBG("Receiving up to %d bytes at %p...", (sizeof(dl->buf) - dl->offset),
				(void *)(dl->buf + dl->offset));

			k_mutex_lock(&dl->mutex, K_FOREVER);
			len = socket_recv(dl);
			k_mutex_unlock(&dl->mutex);

			if ((len == 0) || (len == -1)) {
				/* We just had an unexpected socket error or closure */
				rc = handle_socket_error(dl, len);
				if (rc) {
					break;
				}
				/* Send request again */
				send_request = true;

			} else {
				rc = handle_received(dl, len);
				if (rc < 0) {
					break;
				} else if (rc == 0) {
					/* Send request again */
					send_request = true;
				}
			}
		}

		k_mutex_lock(&dl->mutex, K_FOREVER);
		dl->file = NULL;
		if (dl->close_when_done && dl->fd != -1) {
			download_client_disconnect(dl);
			LOG_DBG("Connection closed automatically");
		}
		k_mutex_unlock(&dl->mutex);

		/* Do not let the thread return, since it can't be restarted */
	}
}

int download_client_init(struct download_client *const client,
			 download_client_callback_t callback)
{
	if (client == NULL || callback == NULL) {
		return -EINVAL;
	}

	memset(client, 0, sizeof(*client));
	client->fd = -1;
	client->callback = callback;
	k_sem_init(&client->wait_for_download, 0, 1);
	k_mutex_init(&client->mutex);

	k_mutex_lock(&client->mutex, K_FOREVER);

	/* The thread is spawned now, but it will suspend itself;
	 * it is resumed when the download is started via the API.
	 */
	client->tid =
		k_thread_create(&client->thread, client->thread_stack,
				K_THREAD_STACK_SIZEOF(client->thread_stack),
				download_thread, client, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(client->tid, "download_client");

	k_mutex_unlock(&client->mutex);

	return 0;
}

int download_client_set_host(struct download_client *client, const char *host,
			    const struct download_client_cfg *config)
{
	if (client == NULL || host == NULL || config == NULL) {
		return -EINVAL;
	}

	if (config->frag_size_override > CONFIG_DOWNLOAD_CLIENT_BUF_SIZE) {
		LOG_ERR("The configured fragment size is larger than buffer");
		return -E2BIG;
	}

	k_mutex_lock(&client->mutex, K_FOREVER);

	if (client->fd != -1) {
		k_mutex_unlock(&client->mutex);
		/* Already connected */
		return -EALREADY;
	}

	client->config = *config;
	client->host = host;
	client->close_when_done = false;
	k_mutex_unlock(&client->mutex);
	return 0;
}

int download_client_connect(struct download_client *client, const char *host,
			    const struct download_client_cfg *config)
{
	return download_client_set_host(client, host, config);
}

int download_client_disconnect(struct download_client *const client)
{
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->mutex, K_FOREVER);

	if (client->fd < 0) {
		k_mutex_unlock(&client->mutex);
		return -EINVAL;
	}

	err = close(client->fd);
	if (err) {
		err = errno;
		k_mutex_unlock(&client->mutex);
		LOG_ERR("Failed to close socket, errno %d", err);
		return -err;
	}

	client->fd = -1;
	client->close_when_done = false;
	client->host = NULL;
	client->file = NULL;

	k_mutex_unlock(&client->mutex);
	return 0;
}

int download_client_start(struct download_client *client, const char *file,
			  size_t from)
{
	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->mutex, K_FOREVER);

	if (client->host == NULL) {
		k_mutex_unlock(&client->mutex);
		return -EINVAL;
	}

	if (!is_idle(client) && !is_finished(client)) {
		k_mutex_unlock(&client->mutex);
		return -EALREADY;
	}

	client->file = file;
	client->file_size = 0;
	client->progress = from;
	client->offset = 0;
	client->http.has_header = false;

	k_mutex_unlock(&client->mutex);

	LOG_INF("Downloading: %s [%u]", client->file, client->progress);

	/* Let the thread run */
	k_sem_give(&client->wait_for_download);
	return 0;
}

int download_client_get(struct download_client *client, const char *host,
			const struct download_client_cfg *config, const char *file, size_t from)
{
	int rc;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->mutex, K_FOREVER);

	rc = download_client_set_host(client, host, config);

	if (rc == 0) {
		client->close_when_done = true;
		rc = download_client_start(client, file, from);
	}

	k_mutex_unlock(&client->mutex);

	return rc;
}

int download_client_file_size_get(struct download_client *client, size_t *size)
{
	if (!client || !size) {
		return -EINVAL;
	}

	k_mutex_lock(&client->mutex, K_FOREVER);
	*size = client->file_size;
	k_mutex_unlock(&client->mutex);

	return 0;
}
