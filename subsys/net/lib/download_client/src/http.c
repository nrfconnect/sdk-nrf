/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <net/download_client.h>
#include "download_client_internal.h"

LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE

/* nRF91 modem TLS secure socket buffer limited to 2kB including header */
#define TLS_RANGE_MAX 1024 // TODO can this be more than 1024?

/* Request whole file; use with HTTP */
#define HTTP_GET                                                               \
	"GET /%s HTTP/1.1\r\n"                                                 \
	"Host: %s\r\n"                                                         \
	"Connection: keep-alive\r\n"                                           \
	"\r\n"

/* Request remaining bytes from offset; use with HTTP */
#define HTTP_GET_OFFSET                                                        \
	"GET /%s HTTP/1.1\r\n"                                                 \
	"Host: %s\r\n"                                                         \
	"Range: bytes=%u-\r\n"                                                 \
	"Connection: keep-alive\r\n"                                           \
	"\r\n"

/* Request a range of bytes; use with HTTPS due to modem limitations */
#define HTTP_GET_RANGE                                                         \
	"GET /%s HTTP/1.1\r\n"                                                 \
	"Host: %s\r\n"                                                         \
	"Range: bytes=%u-%u\r\n"                                               \
	"Connection: keep-alive\r\n"                                           \
	"\r\n"

extern char *strnstr(const char *haystack, const char *needle, size_t haystack_sz);

int http_get_request_send(struct download_client *dlc)
{
	int err;
	int len;
	size_t range;
	size_t off = 0;
	char host[HOSTNAME_SIZE];
	char file[FILENAME_SIZE];
	bool tls_use_range;

	__ASSERT_NO_MSG(dlc->host);
	__ASSERT_NO_MSG(dlc->file);

	dlc->http.header.has_end = false;

	err = url_parse_host(dlc->host, host, sizeof(host));
	if (err) {
		return err;
	}

	err = url_parse_file(dlc->file, file, sizeof(file));
	if (err) {
		return err;
	}

	tls_use_range = (dlc->proto == IPPROTO_TLS_1_2 &&
			!dlc->set_native_tls &&
			IS_ENABLED(CONFIG_SOC_SERIES_NRF91X));

	if (tls_use_range || dlc->config.range_override) {
		if (tls_use_range && dlc->config.range_override) {
			range = MIN((TLS_RANGE_MAX - 1), dlc->config.range_override);
		} else if (dlc->config.range_override) {
			range = dlc->config.range_override;
		} else {
			range = TLS_RANGE_MAX - 1;
		}

		off = dlc->progress + range;

		if (dlc->file_size && (off > dlc->file_size - 1)) {
			/* Don't request bytes past the end of file */
			off = dlc->file_size - 1;
		}

		len = snprintf(dlc->config.buf,
			dlc->config.buf_size,
			HTTP_GET_RANGE, file, host, dlc->progress, off);
		dlc->http.ranged = true;
		dlc->http.ranged_progress = 0;
		goto send;
	} else if (dlc->progress) {
		len = snprintf(dlc->config.buf,
			dlc->config.buf_size,
			HTTP_GET_OFFSET, file, host, dlc->progress);
		dlc->http.ranged = false;
	} else {
		len = snprintf(dlc->config.buf,
			dlc->config.buf_size,
			HTTP_GET, file, host);
		dlc->http.ranged = false;
	}

send:
	if (len < 0 || len > dlc->config.buf_size) {
		LOG_ERR("Cannot create GET request, buffer too small");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(dlc->config.buf, len, "HTTP request");
	}

	printk("File size: %d\n", dlc->file_size);
	printk("Progress: %d\n", dlc->progress);
	printk("REQ: %s\n", dlc->config.buf);

	err = client_socket_send(dlc, len, 0);
	if (err) {
		LOG_ERR("Failed to send HTTP request, errno %d", errno);
		return err;
	}

	return 0;
}

/* Returns:
 * Number of bytes parsed on success.
 * Negative errno on error.
 */
static int http_header_parse(struct download_client *dlc, size_t buf_len)
{
	char *p;
	char *q;
	size_t parse_len;
	unsigned int expected_status;

	p = strnstr(dlc->config.buf, "\r\n\r\n", dlc->config.buf_size);
	if (p && p <= dlc->config.buf + dlc->buf_offset) {
		/* End of header received */
		dlc->http.header.has_end = true;
		parse_len = p + strlen("\r\n\r\n") - (char *)dlc->config.buf;
	} else {
		parse_len = buf_len;
	}

	for (size_t i = 0; i < parse_len; i++) {
		dlc->config.buf[i] = tolower(dlc->config.buf[i]);
	}

	/* Look for the status code just after "http/1.1 " */
	p = strnstr(dlc->config.buf, "http/1.1 ", parse_len);
	if (p) {
		q = strnstr(p, "\r\n", parse_len - (p - dlc->config.buf));
		if (q) {
			/* Received entire line */
			p += strlen("http/1.1 ");
			dlc->http.header.status_code = strtoul(p, &q, 10);
		}

	}

	/* The file size is returned via "Content-Length" in case of HTTP,
	 * and via "Content-Range" in case of HTTPS with range requests.
	 */
	do {
		if (dlc->file_size == 0) {
			if (dlc->http.ranged) {
				p = strnstr(dlc->config.buf, "\r\ncontent-range", parse_len);
				if (!p) {
					break;
				}
				p = strnstr(p, "/", parse_len - (p - dlc->config.buf));
				if (!p) {
					break;
				}
				q = strnstr(p, "\r\n", parse_len - (p - dlc->config.buf));
				if (!q) {
					/* Missing end of line */
					break;
				}
			} else { /* proto == PROTO_HTTP */
				p = strnstr(dlc->config.buf, "\r\ncontent-length", parse_len);
				if (!p) {
					break;
				}
				p = strstr(p, ":");
				if (!p) {
					break;
				}
				q = strnstr(p, "\r\n", parse_len - (p - dlc->config.buf));
				if (!q) {
					/* Missing end of line */
					break;
				}
				/* Accumulate any eventual progress (starting offset)
				* when reading the file size from Content-Length
				*/
				dlc->file_size = dlc->progress;
			}

			dlc->file_size += atoi(p + 1);
			LOG_DBG("File size = %u", dlc->file_size);
		}
	} while (0);

	p = strnstr(dlc->config.buf, "\r\nconnection: close", parse_len);
	if (p) {
		LOG_WRN("Peer closed connection, will re-connect");
		dlc->http.connection_close = true;
	}

	if (dlc->http.header.has_end) {
		/* We have received the end of the header.
		 * Verify that we have received everything that we need.
		 */

		if (!dlc->file_size) {
			LOG_ERR("File size not set");
			return -EBADMSG;
		}

		if (!dlc->http.header.status_code) {
			LOG_ERR("Server response malformed: status code not found");
			return -EBADMSG;
		}

		expected_status = (dlc->http.ranged || dlc->progress) ? 206 : 200;
		if (dlc->http.header.status_code != expected_status) {
			LOG_ERR("Unexpected HTTP response code %ld", dlc->http.header.status_code);
			return -EBADMSG;
		}

		return parse_len;
	}

	q = dlc->config.buf + buf_len;
	/* We are still missing part of the header.
	 * Return the lines (in number of bytes) that we have parsed.
	 */
	while (q > dlc->config.buf && (*q != '\r') && (*q != '\n')) {
		q--;
	}

	/* Keep \r and \n in the buffer in case it is part of the header ending. */
	while (*(q - 1) == '\r' || *(q - 1) == '\n') {
		q--;
	}

	parse_len = (q - dlc->config.buf);

	return parse_len;
}

/* Returns:
 * 0 on success
 * Negative errno on error.
 */
int http_parse(struct download_client *dlc, size_t len)
{
	int parsed_len;

	LOG_DBG("RES: %s", dlc->config.buf);

	if (!dlc->http.header.has_end) {
		/* Parse what we can from the header */
		parsed_len = http_header_parse(dlc, len);
		if (parsed_len < 0) {
			/* Something is wrong with the header */
			return parsed_len;
		}

		if (parsed_len == len) {
			len = 0;
			dlc->buf_offset = 0;
		} else if (parsed_len) {
			/* Keep remaining payload */
			len = len - parsed_len;
			memmove(dlc->config.buf, dlc->config.buf + parsed_len, dlc->buf_offset);
			dlc->buf_offset = len;
		}

		if (!dlc->http.header.has_end) {
			/* Wait for rest of header */
			return 0;
		}
	}

	/* Accumulate overall file progress. */
	dlc->progress += len;

	/* Have we received a whole fragment or the whole file? */
	if (dlc->progress != dlc->file_size) {
		if (dlc->http.ranged) {
			dlc->http.ranged_progress += len;
			if (dlc->http.ranged_progress < (dlc->config.range_override ?
					   dlc->config.range_override :
					   TLS_RANGE_MAX)) {
				/* Ranged query: read until a full fragment */
				return 0;
			}
		} else {
			/* Non-ranged query: just keep on reading, ignore fragment size */
			return 0;
		}
	}

	/* Either we have a full file, or we need to request a next fragment */
	dlc->new_data_req = true;
	return 0;
}
