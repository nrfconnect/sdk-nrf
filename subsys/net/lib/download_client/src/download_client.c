/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <toolchain/common.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <net/download_client.h>
#include <logging/log.h>

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
int coap_parse(struct download_client *client, size_t len);
int coap_request_send(struct download_client *client);

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

static int socket_timeout_set(int fd, int type)
{
	int err;
	uint32_t timeout_ms;

	if (type == SOCK_STREAM) {
		timeout_ms = CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS;
	} else {
		timeout_ms = CONFIG_DOWNLOAD_CLIENT_UDP_SOCK_TIMEO_MS;
	}

	if (timeout_ms <= 0) {
		return 0;
	}

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};

	LOG_INF("Configuring socket timeout (%ld s)", timeo.tv_sec);

	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
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
			log_strdup(parsed_host), errno);
		return -errno;
	}

	return 0;
}

static int socket_apn_set(int fd, const char *apn)
{
	int err;
	size_t len;

	__ASSERT_NO_MSG(apn);

	len = strlen(apn);
	if (len >= IFNAMSIZ) {
		LOG_ERR("Access point name is too long.");
		return -EINVAL;
	}

	LOG_INF("Setting up APN: %s", log_strdup(apn));

	err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, apn, len);
	if (err) {
		LOG_ERR("Failed to bind socket to network \"%s\", err %d",
			log_strdup(apn), errno);
		return -ENETUNREACH;
	}

	return 0;
}

static int host_lookup(const char *host, int family, const char *apn,
		       struct sockaddr *sa)
{
	int err;
	struct addrinfo *ai;
	char hostname[HOSTNAME_SIZE];

	struct addrinfo hints = {
		.ai_family = family,
		.ai_next = apn ?
			&(struct addrinfo) {
				.ai_family    = AF_LTE,
				.ai_socktype  = SOCK_MGMT,
				.ai_protocol  = NPROTO_PDN,
				.ai_canonname = (char *)apn
			} : NULL,
	};

	/* Extract the hostname, without protocol or port */
	err = url_parse_host(host, hostname, sizeof(hostname));
	if (err) {
		return err;
	}

	err = getaddrinfo(hostname, NULL, &hints, &ai);
	if (err) {
		LOG_WRN("Failed to resolve hostname %s on %s",
			log_strdup(hostname), str_family(family));
		return -EHOSTUNREACH;
	}

	*sa = *(ai->ai_addr);
	freeaddrinfo(ai);

	return 0;
}

static int client_connect(struct download_client *dl, const char *host,
			  struct sockaddr *sa, int *fd)
{
	int err;
	int type;
	uint16_t port;
	socklen_t addrlen;

	err = url_parse_proto(host, &dl->proto, &type);
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

	err = url_parse_port(host, &port);
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

	switch (sa->sa_family) {
	case AF_INET6:
		SIN6(sa)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		SIN(sa)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	LOG_DBG("family: %d, type: %d, proto: %d",
		sa->sa_family, type, dl->proto);

	*fd = socket(sa->sa_family, type, dl->proto);
	if (*fd < 0) {
		LOG_ERR("Failed to create socket, err %d", errno);
		return -errno;
	}

	if (dl->config.apn != NULL && strlen(dl->config.apn)) {
		err = socket_apn_set(*fd, dl->config.apn);
		if (err) {
			goto cleanup;
		}
	}

	if ((dl->proto == IPPROTO_TLS_1_2 || dl->proto == IPPROTO_DTLS_1_2)
	     && (dl->config.sec_tag != -1)) {
		err = socket_sectag_set(*fd, dl->config.sec_tag);
		if (err) {
			goto cleanup;
		}

		if (dl->config.set_tls_hostname) {
			err = socket_tls_hostname_set(*fd, host);
			if (err) {
				goto cleanup;
			}
		}
	}

	/* Set socket timeout, if configured */
	err = socket_timeout_set(*fd, type);
	if (err) {
		goto cleanup;
	}

	LOG_INF("Connecting to %s", log_strdup(host));
	LOG_DBG("fd %d, addrlen %d, fam %s, port %d",
		*fd, addrlen, str_family(sa->sa_family), port);

	err = connect(*fd, sa, addrlen);
	if (err) {
		LOG_ERR("Unable to connect, errno %d", errno);
		err = -errno;
	}

cleanup:
	if (err) {
		/* Unable to connect, close socket */
		close(*fd);
		*fd = -1;
	}

	return err;
}

int socket_send(const struct download_client *client, size_t len)
{
	int sent;
	size_t off = 0;

	while (len) {
		sent = send(client->fd, client->buf + off, len, 0);
		if (sent <= 0) {
			return -errno;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

static int request_send(struct download_client *dl)
{
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

static int fragment_evt_send(const struct download_client *client)
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
	err = download_client_disconnect(dl);
	if (err) {
		return err;
	}

	err = download_client_connect(dl, dl->host, &dl->config);
	if (err) {
		return err;
	}

	return 0;
}

void download_thread(void *client, void *a, void *b)
{
	int rc = 0;
	int error_cause;
	size_t len;
	struct download_client *const dl = client;

restart_and_suspend:
	k_thread_suspend(dl->tid);

	while (true) {
		__ASSERT(dl->offset < sizeof(dl->buf), "Buffer overflow");

		if (sizeof(dl->buf) - dl->offset == 0) {
			LOG_ERR("Could not fit HTTP header from server (> %d)",
				sizeof(dl->buf));
			error_evt_send(dl, E2BIG);
			break;
		}

		LOG_DBG("Receiving up to %d bytes at %p...",
			(sizeof(dl->buf) - dl->offset), (dl->buf + dl->offset));

		len = recv(dl->fd, dl->buf + dl->offset,
			   sizeof(dl->buf) - dl->offset, 0);

		if ((len == 0) || (len == -1)) {
			/* We just had an unexpected socket error or closure */

			/* If there is a partial data payload in our buffer,
			 * and it has been accounted in our progress, we have
			 * to hand it to the application before discarding it.
			 */
			if ((dl->offset > 0) && (dl->http.has_header)) {
				rc = fragment_evt_send(dl);
				if (rc) {
					/* Restart and suspend */
					LOG_INF("Fragment refused, download stopped.");
					break;
				}
			}

			error_cause = ECONNRESET;

			if (len == -1) {
				if ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK) ||
				    (errno == EAGAIN)) {
					if (dl->proto == IPPROTO_UDP ||
					    dl->proto == IPPROTO_DTLS_1_2) {
						LOG_DBG("Socket timeout, resending");
						goto send_again;
					}
					error_cause = ETIMEDOUT;
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
			rc = error_evt_send(dl, error_cause);
			if (rc) {
				/* Restart and suspend */
				break;
			}

			rc = reconnect(dl);
			if (rc) {
				error_evt_send(dl, EHOSTDOWN);
				break;
			}

			goto send_again;
		}

		LOG_DBG("Read %d bytes from socket", len);

		if (dl->proto == IPPROTO_TCP || dl->proto == IPPROTO_TLS_1_2) {
			rc = http_parse(client, len);
			if (rc > 0) {
				/* Wait for more data (fragment/header) */
				continue;
			}
		} else if (IS_ENABLED(CONFIG_COAP)) {
			rc = coap_parse(client, len);
		}

		if (rc < 0) {
			/* Something was wrong with the packet
			 * Restart and suspend
			 */
			error_evt_send(dl, EBADMSG);
			break;
		}

		if (dl->file_size) {
			LOG_INF("Downloaded %u/%u bytes (%d%%)",
				dl->progress, dl->file_size,
				(dl->progress * 100) / dl->file_size);
		} else {
			LOG_INF("Downloaded %u bytes", dl->progress);
		}

		/* Send fragment to application.
		 * If the application callback returns non-zero, stop.
		 */
		rc = fragment_evt_send(dl);
		if (rc) {
			/* Restart and suspend */
			LOG_INF("Fragment refused, download stopped.");
			break;
		}

		if (dl->progress == dl->file_size) {
			LOG_INF("Download complete");
			const struct download_client_evt evt = {
				.id = DOWNLOAD_CLIENT_EVT_DONE,
			};
			dl->callback(&evt);
			/* Restart and suspend */
			break;
		}

		/* Attempt to reconnect if the connection was closed */
		if (dl->http.connection_close) {
			dl->http.connection_close = false;
			reconnect(dl);
		}

send_again:
		dl->offset = 0;
		/* Request next fragment, if necessary (HTTPS/CoAP) */
		if (dl->proto != IPPROTO_TCP || len == 0
		   || IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS)) {
			dl->http.has_header = false;

			rc = request_send(dl);
			if (rc) {
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

				goto send_again;
			}
		}
	}

	/* Do not let the thread return, since it can't be restarted */
	goto restart_and_suspend;
}

int download_client_init(struct download_client *const client,
			 download_client_callback_t callback)
{
	if (client == NULL || callback == NULL) {
		return -EINVAL;
	}

	client->fd = -1;
	client->callback = callback;

	/* The thread is spawned now, but it will suspend itself;
	 * it is resumed when the download is started via the API.
	 */
	client->tid =
		k_thread_create(&client->thread, client->thread_stack,
				K_THREAD_STACK_SIZEOF(client->thread_stack),
				download_thread, client, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(client->tid, "download_client");

	return 0;
}

int download_client_connect(struct download_client *client, const char *host,
			    const struct download_client_cfg *config)
{
	int err;
	struct sockaddr sa;

	if (client == NULL || host == NULL || config == NULL) {
		return -EINVAL;
	}

	if (client->fd != -1) {
		/* Already connected */
		return 0;
	}

	if (config->frag_size_override > CONFIG_DOWNLOAD_CLIENT_BUF_SIZE) {
		LOG_ERR("The configured fragment size is larger than buffer");
		return -E2BIG;
	}

	/* Attempt IPv6 connection if configured, fallback to IPv4 */
	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_IPV6)) {
		err = host_lookup(host, AF_INET6, config->apn, &sa);
	}
	if (err || !IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_IPV6)) {
		err = host_lookup(host, AF_INET, config->apn, &sa);
	}

	if (err) {
		return err;
	}

	client->config = *config;
	client->host = host;

	err = client_connect(client, host, &sa, &client->fd);
	if (client->fd < 0) {
		return err;
	}

	return 0;
}

int download_client_disconnect(struct download_client *const client)
{
	int err;

	if (client == NULL || client->fd < 0) {
		return -EINVAL;
	}

	err = close(client->fd);
	if (err) {
		LOG_ERR("Failed to close socket, errno %d", errno);
		return -errno;
	}

	client->fd = -1;

	return 0;
}

int download_client_start(struct download_client *client, const char *file,
			  size_t from)
{
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	if (client->fd < 0) {
		return -ENOTCONN;
	}

	client->file = file;
	client->file_size = 0;
	client->progress = from;

	client->offset = 0;
	client->http.has_header = false;

	if (client->proto == IPPROTO_UDP || client->proto == IPPROTO_DTLS_1_2) {
		if (IS_ENABLED(CONFIG_COAP)) {
			coap_block_init(client, from);
		}
	}

	err = request_send(client);
	if (err) {
		return err;
	}

	LOG_INF("Downloading: %s [%u]", log_strdup(client->file),
		client->progress);

	/* Let the thread run */
	k_thread_resume(client->tid);

	return 0;
}

void download_client_pause(struct download_client *client)
{
	k_thread_suspend(client->tid);
}

void download_client_resume(struct download_client *client)
{
	k_thread_resume(client->tid);
}

int download_client_file_size_get(struct download_client *client, size_t *size)
{
	if (!client || !size) {
		return -EINVAL;
	}

	*size = client->file_size;

	return 0;
}
