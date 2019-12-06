/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <toolchain/common.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <net/download_client.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define GET_TEMPLATE                                                           \
	"GET /%s HTTP/1.1\r\n"                                                 \
	"Host: %s\r\n"                                                         \
	"Connection: keep-alive\r\n"                                           \
	"Range: bytes=%u-%u\r\n"                                               \
	"\r\n"

BUILD_ASSERT_MSG(CONFIG_DOWNLOAD_CLIENT_MAX_FRAGMENT_SIZE <=
		 CONFIG_DOWNLOAD_CLIENT_MAX_RESPONSE_SIZE,
		 "The response buffer must accommodate for a full fragment");

#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_IMMEDIATE)
BUILD_ASSERT_MSG(IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS) ?
			 CONFIG_LOG_BUFFER_SIZE >= 2048 : 1,
		 "Please increase log buffer sizer");
#endif

static int socket_timeout_set(int fd)
{
	int err;

	if (CONFIG_DOWNLOAD_CLIENT_SOCK_TIMEOUT_MS == K_FOREVER) {
		return 0;
	}

	const u32_t timeout_ms = CONFIG_DOWNLOAD_CLIENT_SOCK_TIMEOUT_MS;

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
		return -1;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
	if (err) {
		LOG_ERR("Failed to setup socket security tag, errno %d", errno);
		return -1;
	}

	return 0;
}

static int socket_apn_set(int fd, const char *apn)
{
	int err;
	size_t len;
	struct ifreq ifr = {0};

	__ASSERT_NO_MSG(apn);

	len = strlen(apn);
	if (len >= sizeof(ifr.ifr_name)) {
		LOG_ERR("Access point name is too long.");
		return -1;
	}

	memcpy(ifr.ifr_name, apn, len);
	err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, len);
	if (err) {
		LOG_ERR("Failed to bind socket, errno %d", errno);
		return -1;
	}

	return 0;
}

static int resolve_and_connect(int family, const char *host,
			       const struct download_client_cfg *cfg)
{
	int fd;
	int err;
	int proto;
	u16_t port;
	struct addrinfo *addr;
	struct addrinfo *info;

	__ASSERT_NO_MSG(host);
	__ASSERT_NO_MSG(cfg);

	/* Set up port and protocol */
	if (cfg->sec_tag == -1) {
		/* HTTP, port 80 */
		proto = IPPROTO_TCP;
		port = htons(80);
	} else {
		/* HTTPS, port 443 */
		proto = IPPROTO_TLS_1_2;
		port = htons(443);
	}

	/* Lookup host */
	struct addrinfo hints = {
		.ai_family = family,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = proto,
		/* Either a valid, NULL-terminated access point name or NULL. */
		.ai_canonname = (char *)cfg->apn
	};

	err = getaddrinfo(host, NULL, &hints, &info);
	if (err) {
		LOG_WRN("Failed to resolve hostname %s on %s", log_strdup(host),
			family == AF_INET ? "IPv4" : "IPv6");
		return -1;
	}

	LOG_INF("Attempting to connect over %s",
		family == AF_INET ? log_strdup("IPv4") : log_strdup("IPv6"));

	fd = socket(family, SOCK_STREAM, proto);
	if (fd < 0) {
		LOG_ERR("Failed to create socket, errno %d", errno);
		goto cleanup;
	}

	if (cfg->apn != NULL) {
		LOG_INF("Setting up APN: %s", log_strdup(cfg->apn));
		err = socket_apn_set(fd, cfg->apn);
		if (err) {
			goto cleanup;
		}
	}

	if (proto == IPPROTO_TLS_1_2) {
		LOG_INF("Setting up TLS credentials");
		err = socket_sectag_set(fd, cfg->sec_tag);
		if (err) {
			goto cleanup;
		}
	}

	/* Not connected */
	err = -1;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr * const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = port;
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			break;
		}

		err = connect(fd, sa, addr->ai_addrlen);
		if (err) {
			/* Try next address */
			LOG_ERR("Unable to connect, errno %d", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		close(fd);
		fd = -1;
	}

	return fd;
}

static int socket_send(const struct download_client *client, size_t len)
{
	int sent;
	size_t off = 0;

	while (len) {
		sent = send(client->fd, client->buf + off, len, 0);
		if (sent <= 0) {
			return -EIO;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

static int get_request_send(struct download_client *client)
{
	int err;
	int len;
	size_t off;

	__ASSERT_NO_MSG(client);
	__ASSERT_NO_MSG(client->host);
	__ASSERT_NO_MSG(client->file);

	/* Offset of last byte in range (Content-Range) */
	off = client->progress + CONFIG_DOWNLOAD_CLIENT_MAX_FRAGMENT_SIZE - 1;

	if (client->file_size != 0) {
		/* Don't request bytes past the end of file */
		off = MIN(off, client->file_size);
	}

	len = snprintf(client->buf, CONFIG_DOWNLOAD_CLIENT_MAX_RESPONSE_SIZE,
		       GET_TEMPLATE, client->file, client->host,
		       client->progress, off);

	if (len < 0 || len > CONFIG_DOWNLOAD_CLIENT_MAX_RESPONSE_SIZE) {
		LOG_ERR("Cannot create GET request, buffer too small");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(client->buf, len, "HTTP request");
	}

	LOG_DBG("Sending HTTP request");
	err = socket_send(client, len);
	if (err) {
		LOG_ERR("Failed to send HTTP request, errno %d", errno);
		return err;
	}

	return 0;
}

static int my_strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n != 0) {
		do {
			if (tolower(*s1) != tolower(*s2++))
				return (tolower(*s1) - tolower(*--s2));
			if (*s1++ == '\0')
				break;
		} while (--n != 0);
	}
	return (0);
}

static char *my_strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;
	if ((c = *find++) != 0) {
		c = (char)tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (my_strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

/* Returns:
 *  1 while the header is being received
 *  0 if the header has been fully received
 * -1 on error
 */
static int header_parse(struct download_client *client)
{
	char *p;
	size_t hdr;

	p = strstr(client->buf, "\r\n\r\n");
	if (!p) {
		/* Awaiting full GET response */
		LOG_DBG("Awaiting full header in response");
		return 1;
	}

	/* Offset of the end of the HTTP header in the buffer */
	hdr = p + strlen("\r\n\r\n") - client->buf;

	__ASSERT(hdr < sizeof(client->buf), "Buffer overflow");

	LOG_DBG("GET header size: %u", hdr);

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(client->buf, hdr, "GET");
	}

	/* If file size is not known, read it from the header */
	if (client->file_size == 0) {
		if (NULL !=
		    (p = my_strcasestr(client->buf, "Content-Range: bytes"))) {
			p = strstr(p, "/");
			if (!p) {
				/* Cannot continue */
				LOG_ERR("Server did not send file size in response");
				return -1;
			}

			client->file_size = atoi(p + 1);

			LOG_DBG("File size = %d", client->file_size);
		} else if (NULL != my_strcasestr(client->buf,
						 "Accept-Range: bytes") &&
			   NULL != (p = my_strcasestr(client->buf,
						      "Content-Length"))) {
			p = strstr(p, ":");
			if (!p) {
				/* Cannot continue */
				LOG_ERR("Server did not send file size in response");
				return -1;
			}

			client->file_size = atoi(p + 1);

			LOG_DBG("File size = %d", client->file_size);
		} else {
			/* Cannot continue */
			LOG_ERR("Server did not send "
				"\"Content-Range\" or \"Content-Length\" "
				"in response");
			return -1;
		}
	}

	p = strstr(client->buf, "Connection: close");
	if (p) {
		LOG_WRN("Peer closed connection, will attempt to re-connect");
		client->connection_close = true;
	}

	if (client->offset != hdr) {
		/* The current buffer contains some payload bytes.
		 * Copy them at the beginning of the buffer
		 * then update the offset.
		 */
		LOG_WRN("Copying %u payload bytes", client->offset - hdr);
		memcpy(client->buf, client->buf + hdr, client->offset - hdr);

		client->offset -= hdr;
	} else {
		/* Reset the offset.
		 * The payload is received in an empty buffer.
		 */
		client->offset = 0;
	}

	return 0;
}

static int fragment_evt_send(const struct download_client *client)
{
	__ASSERT(client->offset <= CONFIG_DOWNLOAD_CLIENT_MAX_FRAGMENT_SIZE,
		 "Fragment overflow!");

	__ASSERT(client->offset <= CONFIG_DOWNLOAD_CLIENT_MAX_RESPONSE_SIZE,
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
	int rc;
	size_t len;
	struct download_client *const dl = client;

restart_and_suspend:
	k_thread_suspend(dl->tid);

	while (true) {
		__ASSERT(dl->offset < sizeof(dl->buf), "Buffer overflow");

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
			if ((dl->offset > 0) && (dl->has_header)) {
				rc = fragment_evt_send(dl);
				if (rc) {
					/* Restart and suspend */
					LOG_INF("Fragment refused, download "
						"stopped.");
					break;
				}
			}

			if (len == -1) {
				LOG_ERR("Error in recv(), errno %d", errno);
				rc = error_evt_send(dl, ENOTCONN);
			}

			if (len == 0) {
				LOG_WRN("Peer closed connection!");
				rc = error_evt_send(dl, ECONNRESET);
			}

			if (rc) {
				/* Restart and suspend */
				break;
			}
			reconnect(dl);
			goto send_again;
		}

		LOG_DBG("Read %d bytes from socket", len);

		/* Accumulate buffer offset */
		dl->offset += len;

		if (!dl->has_header) {
			rc = header_parse(dl);
			if (rc > 0) {
				/* Wait for payload */
				continue;
			}
			if (rc < 0) {
				/* Something was wrong with the header.
				 * Restart and suspend, no point in retrying.
				 */
				error_evt_send(dl, EBADMSG);
				break;
			}

			dl->has_header = true;
		}

		/* Accumulate overall file progress.
		 *
		 * If the last recv() call read an HTTP header,
		 * the offset has been moved to the end of the header in
		 * header_parse(). Thus, we accumulate the offset
		 * to the progress.
		 *
		 * If the last recv() call received only a HTTP message body,
		 * then we accumulate 'len'.
		 *
		 */
		dl->progress += MIN(dl->offset, len);

		/* Have we received a whole fragment or the whole file? */
		if ((dl->offset < CONFIG_DOWNLOAD_CLIENT_MAX_FRAGMENT_SIZE) &&
		    (dl->progress != dl->file_size)) {
			LOG_DBG("Awaiting full fragment (%u)", dl->offset);
			continue;
		}

		LOG_INF("Downloaded %u/%u bytes (%d%%)", dl->progress,
			dl->file_size, (dl->progress * 100) / dl->file_size);

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
		if (dl->connection_close) {
			dl->connection_close = false;
			reconnect(dl);
		}

		/* Request next fragment */
		/* Send a GET request for the next bytes */
send_again:
		dl->offset = 0;
		dl->has_header = false;

		rc = get_request_send(dl);
		if (rc) {
			rc = error_evt_send(dl, ECONNRESET);
			if (rc) {
				/* Restart and suspend */
				break;
			}
			reconnect(dl);
			goto send_again;
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

	return 0;
}

int download_client_connect(struct download_client *client, const char *host,
			    const struct download_client_cfg *config)
{
	int err;

	if (client == NULL || host == NULL || config == NULL) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_TLS)) {
		if (config->sec_tag != -1) {
			return -EINVAL;
		}
	}

	if (client->fd != -1) {
		/* Already connected */
		return 0;
	}

	/* Attempt IPv6 connection if configured, fallback to IPv4 */
	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_IPV6)) {
		client->fd =
			resolve_and_connect(AF_INET6, host, config);
	}
	if (client->fd < 0) {
		client->fd =
			resolve_and_connect(AF_INET, host, config);
	}

	if (client->fd < 0) {
		return -EINVAL;
	}

	client->host = host;
	client->config = *config;

	LOG_INF("Connected to %s", log_strdup(host));

	/* Set socket timeout, if configured */
	err = socket_timeout_set(client->fd);
	if (err) {
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

	if (client == NULL || client->fd < 0) {
		return -EINVAL;
	}

	client->file = file;
	client->file_size = 0;
	client->progress = from;

	client->offset = 0;
	client->has_header = false;

	LOG_INF("Downloading: %s [%u]", log_strdup(client->file),
		client->progress);

	err = get_request_send(client);
	if (err) {
		return err;
	}

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
