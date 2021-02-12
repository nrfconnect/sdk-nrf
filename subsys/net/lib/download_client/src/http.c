/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <logging/log.h>
#include <sys/__assert.h>
#include <net/download_client.h>

LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE

#define GET_HTTP_TEMPLATE                                                      \
	"GET /%s HTTP/1.1\r\n"                                                 \
	"Host: %s\r\n"                                                         \
	"Range: bytes=%u-\r\n"                                                 \
	"Connection: keep-alive\r\n"                                           \
	"\r\n"

#define GET_HTTPS_TEMPLATE                                                     \
	"GET /%s HTTP/1.1\r\n"                                                 \
	"Host: %s\r\n"                                                         \
	"Range: bytes=%u-%u\r\n"                                               \
	"Connection: keep-alive\r\n"                                           \
	"\r\n"

int url_parse_host(const char *url, char *host, size_t len);
int url_parse_file(const char *url, char *file, size_t len);
int socket_send(const struct download_client *client, size_t len);

int http_get_request_send(struct download_client *client)
{
	int err;
	int len;
	size_t off;
	char host[HOSTNAME_SIZE];
	char file[FILENAME_SIZE];

	__ASSERT_NO_MSG(client->host);
	__ASSERT_NO_MSG(client->file);

	err = url_parse_host(client->host, host, sizeof(host));
	if (err) {
		return err;
	}

	err = url_parse_file(client->file, file, sizeof(file));
	if (err) {
		return err;
	}

	/* Offset of last byte in range (Content-Range) */
	if (client->config.frag_size_override) {
		off = client->progress + client->config.frag_size_override - 1;
	} else {
		off = client->progress +
			CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE - 1;
	}

	if (client->file_size != 0) {
		/* Don't request bytes past the end of file */
		off = MIN(off, client->file_size);
	}

	/* We use range requests only for HTTPS, due to memory limitations.
	 * When using HTTP, we request the whole resource to minimize
	 * network usage (only one request/response are sent).
	 */
	if (client->proto == IPPROTO_TLS_1_2
	   || IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS)) {
		len = snprintf(client->buf,
			CONFIG_DOWNLOAD_CLIENT_BUF_SIZE,
			GET_HTTPS_TEMPLATE, file, host, client->progress, off);
	} else {
		len = snprintf(client->buf,
			CONFIG_DOWNLOAD_CLIENT_BUF_SIZE,
			GET_HTTP_TEMPLATE, file, host, client->progress);
	}

	if (len < 0 || len > CONFIG_DOWNLOAD_CLIENT_BUF_SIZE) {
		LOG_ERR("Cannot create GET request, buffer too small");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(client->buf, len, "HTTP request");
	}

	err = socket_send(client, len);
	if (err) {
		LOG_ERR("Failed to send HTTP request, errno %d", errno);
		return err;
	}

	return 0;
}

/* Returns:
 *  1 while the header is being received
 *  0 if the header has been fully received
 * -1 on error
 */
static int http_header_parse(struct download_client *client, size_t *hdr_len)
{
	char *p;

	p = strstr(client->buf, "\r\n\r\n");
	if (!p) {
		/* Waiting full HTTP header */
		LOG_DBG("Waiting full header in response");
		return 1;
	}

	/* Offset of the end of the HTTP header in the buffer */
	*hdr_len = p + strlen("\r\n\r\n") - client->buf;

	LOG_DBG("GET header size: %u", *hdr_len);
	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(client->buf, *hdr_len, "HTTP response");
	}

	for (size_t i = 0; i < *hdr_len; i++) {
		client->buf[i] = tolower(client->buf[i]);
	}

	p = strstr(client->buf, "http/1.1 206");
	if (!p) {
		if (client->proto == IPPROTO_TLS_1_2
		   || IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS)) {
			LOG_ERR("Server did not honor partial content request");
			return -1;
		}
		p = strstr(client->buf, "http/1.1 200");
		if (!p) {
			LOG_ERR("Server response is not 200 Success");
			return -1;
		}
	}

	/* The file size is returned via "Content-Length" in case of HTTP,
	 * and via "Content-Range" in case of HTTPS with range requests.
	 */
	if (client->file_size == 0) {
		if (client->proto == IPPROTO_TLS_1_2
		   || IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS)) {
			p = strstr(client->buf, "content-range");
			if (!p) {
				LOG_ERR("Server did not send "
					"\"Content-Range\" in response");
				return -1;
			}
			p = strstr(p, "/");
			if (!p) {
				LOG_ERR("No file size in response");
				return -1;
			}
		} else { /* proto == PROTO_HTTP */
			p = strstr(client->buf, "content-length");
			if (!p) {
				LOG_WRN("Server did not send "
					"\"Content-Length\" in response");
					return -1;
			}
			p = strstr(p, ":");
			if (!p) {
				LOG_ERR("No file size in response");
				return -1;
			}
			/* Accumulate any eventual progress (starting offset)
			 * when reading the file size from Content-Length
			 */
			client->file_size = client->progress;
		}

		client->file_size += atoi(p + 1);
		LOG_DBG("File size = %u", client->file_size);
	}

	p = strstr(client->buf, "connection: close");
	if (p) {
		LOG_WRN("Peer closed connection, will re-connect");
		client->http.connection_close = true;
	}

	client->http.has_header = true;

	return 0;
}

/* Returns:
 *  1 if more data is expected
 *  0 if a whole fragment has been received
 * -1 on error
 */
int http_parse(struct download_client *client, size_t len)
{
	int rc;
	size_t hdr_len;

	/* Accumulate buffer offset */
	client->offset += len;

	if (!client->http.has_header) {
		rc = http_header_parse(client, &hdr_len);
		if (rc > 0) {
			/* Wait for header */
			return 1;
		}
		if (rc < 0) {
			/* Something is wrong with the header */
			return -1;
		}

		if (client->offset != hdr_len) {
			/* The buffer contains some payload bytes,
			 * copy them at the beginning of the buffer
			 * and update the offset.
			 */
			LOG_DBG("Copying %u payload bytes",
				client->offset - hdr_len);
			memcpy(client->buf, client->buf + hdr_len,
			       client->offset - hdr_len);

			client->offset -= hdr_len;
		} else {
			/* Reset the offset.
			 * The payload is received in an empty buffer.
			 */
			client->offset = 0;
		}
	}

	/* Accumulate overall file progress.
	 * If the last recv() call read an HTTP header,
	 * `offset` has been moved at the end of any trailing
	 * payload bytes by http_header_parse(). In this case,
	 * `offset` is less than `len` and it represents
	 * the actual payload bytes.
	 */
	client->progress += MIN(client->offset, len);

	/* Have we received a whole fragment or the whole file? */
	if (client->progress != client->file_size &&
	    client->offset < (client->config.frag_size_override != 0 ?
			      client->config.frag_size_override :
			      CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE)) {
		return 1;
	}

	return 0;
}
