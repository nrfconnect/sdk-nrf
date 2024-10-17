/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <net/download_client.h>
#include <net/download_client_transport.h>
#include "download_client_internal.h"

LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE

/* nRF91 modem TLS secure socket buffer limited to 2kB including header */
#define TLS_RANGE_MAX 2048

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

struct transport_params_http {
	/** The server has closed the connection. */
	bool connection_close;
	/** Is using ranged query. */
	bool ranged;
	/** Ranged progress */
	size_t ranged_progress;
	/** HTTP header */
	struct {
		/** Header length */
		size_t hdr_len;
		/** Status code */
		unsigned long status_code;
		/** Whether the HTTP header for
		 * the current fragment has been processed.
		 */
		bool has_end;
	} header;

	struct {
		/** Socket descriptor. */
		int fd;
		/** Protocol for current download. */
		int proto;
		/** Socket type */
		int type;
		/** Port */
		uint16_t port;
		/** Destination address storage */
		struct sockaddr remote_addr;
	} sock;

	/** Request new data */
	bool new_data_req;
};

BUILD_ASSERT(CONFIG_DOWNLOAD_CLIENT_TRANSPORT_PARAMS_SIZE >= sizeof(struct transport_params_http));

extern char *strnstr(const char *haystack, const char *needle, size_t haystack_sz);

int http_get_request_send(struct download_client *dlc)
{
	int err;
	int len;
	size_t off = 0;
	bool tls_force_range;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	http->header.has_end = false;

	/* nRF91 series has a limitation of decoding ~2k of data at once when using TLS */
	tls_force_range = (http->sock.proto == IPPROTO_TLS_1_2 &&
			!dlc->host_config.set_native_tls &&
			IS_ENABLED(CONFIG_SOC_SERIES_NRF91X));

	if (dlc->host_config.range_override) {
		if (tls_force_range && dlc->host_config.range_override > (TLS_RANGE_MAX - 1)) {
			LOG_WRN("Range override > TLS max range, setting to TLS max range");
			dlc->host_config.range_override = (TLS_RANGE_MAX - 1);
		}
	} else if (tls_force_range) {
		dlc->host_config.range_override = TLS_RANGE_MAX - 1;
	}

	if (dlc->host_config.range_override) {
		off = dlc->progress + dlc->host_config.range_override;

		if (dlc->file_size && (off > dlc->file_size - 1)) {
			/* Don't request bytes past the end of file */
			off = dlc->file_size - 1;
		}

		len = snprintf(dlc->config.buf,
			dlc->config.buf_size,
			HTTP_GET_RANGE, dlc->file, dlc->hostname, dlc->progress, off);
		http->ranged = true;
		http->ranged_progress = 0;
		LOG_DBG("Range request up to %d bytes", dlc->host_config.range_override);
		goto send;
	} else if (dlc->progress) {
		len = snprintf(dlc->config.buf,
			dlc->config.buf_size,
			HTTP_GET_OFFSET, dlc->file, dlc->hostname, dlc->progress);
		http->ranged = false;
	} else {
		len = snprintf(dlc->config.buf,
			dlc->config.buf_size,
			HTTP_GET, dlc->file, dlc->hostname);
		http->ranged = false;
	}

send:
	if (len < 0 || len > dlc->config.buf_size) {
		LOG_ERR("Cannot create GET request, buffer too small");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(dlc->config.buf, len, "HTTP request");
	}

	err = client_socket_send(http->sock.fd, dlc->config.buf, len);
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
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	p = strnstr(dlc->config.buf, "\r\n\r\n", dlc->config.buf_size);
	if (p) {
		/* End of header received */
		http->header.has_end = true;
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
			http->header.status_code = strtoul(p, &q, 10);
		}

	}

	/* The file size is returned via "Content-Length" in case of HTTP,
	 * and via "Content-Range" in case of HTTPS with range requests.
	 */
	do {
		if (dlc->file_size == 0) {
			if (http->ranged) {
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
		http->connection_close = true;
	}

	if (http->header.has_end) {
		/* We have received the end of the header.
		 * Verify that we have received everything that we need.
		 */

		if (!dlc->file_size) {
			LOG_ERR("File size not set");
			return -EBADMSG;
		}

		if (!http->header.status_code) {
			LOG_ERR("Server response malformed: status code not found");
			return -EBADMSG;
		}

		expected_status = (http->ranged || dlc->progress) ? 206 : 200;
		if (http->header.status_code != expected_status) {
			LOG_ERR("Unexpected HTTP response code %ld", http->header.status_code);
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
 * length of data payload received on success
 * Negative errno on error.
 */
int http_parse(struct download_client *dlc, size_t len)
{
	int parsed_len;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	if (!http->header.has_end) {
		/* Parse what we can from the header */
		parsed_len = http_header_parse(dlc, len);
		if (parsed_len < 0) {
			/* Something is wrong with the header */
			return -EBADMSG;
		}

		LOG_DBG("Parsed len: %d", parsed_len);

		if (parsed_len == len) {
			len = 0;
			dlc->buf_offset = 0;
		} else if (parsed_len) {
			/* Keep remaining payload */
			len = len - parsed_len;
			memmove(dlc->config.buf, dlc->config.buf + parsed_len, dlc->buf_offset);
			dlc->buf_offset = len;
		}

		if (!http->header.has_end) {
			if (dlc->config.buf_size == dlc->buf_offset) {
				LOG_ERR("Could not parse HTTP header lines from server (> %d)",
					dlc->config.buf_size);
				return -E2BIG;
			}
			/* Wait for rest of header */
			return 0;
		}
	}

	/* Have we received a whole fragment or the whole file? */
	if (dlc->progress + len != dlc->file_size) {
		if (http->ranged) {
			http->ranged_progress += len;
			if (http->ranged_progress < (dlc->host_config.range_override ?
					   dlc->host_config.range_override :
					   TLS_RANGE_MAX - 1)) {
				/* Ranged query: read until a full fragment */
				return len;
			}
		} else {
			/* Non-ranged query: just keep on reading, ignore fragment size */
			return len;
		}
	}

	/* Either we have a full file, or we need to request a next fragment */
	http->new_data_req = true;
	return len;
}

static bool dlc_http_proto_supported(struct download_client *dlc, const char *uri)
{
	if (strncmp(uri, "https://", 8) == 0) {
		return true;
	}

	if (strncmp(uri, "http://", 7) == 0) {
		return true;
	}

	return false;
}

static int dlc_http_init(struct download_client *dlc, struct download_client_host_cfg *host_cfg, const char *uri)
{
	int err;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	http->sock.proto = IPPROTO_TCP;
	http->sock.type = SOCK_STREAM;

	if (strncmp(uri, "https://", 8) == 0) {
		http->sock.proto = IPPROTO_TLS_1_2;
		http->sock.type = SOCK_STREAM;

		if (host_cfg->sec_tag_list == NULL || host_cfg->sec_tag_count == 0) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			return -EINVAL;
		}
	}

	err = url_parse_port(uri, &http->sock.port);
	if (err) {
		switch (http->sock.proto) {
		case IPPROTO_TLS_1_2:
			http->sock.port = 443;
			break;
		case IPPROTO_TCP:
		 	http->sock.port = 80;
			break;
		}
		LOG_DBG("Port not specified, using default: %d", http->sock.port);
	}

	if (host_cfg->set_native_tls) {
		LOG_DBG("Enabled native TLS");
		http->sock.type |= SOCK_NATIVE_TLS;
	}

	return 0;
}

static int dlc_http_deinit(struct download_client *dlc)
{
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	if (http->sock.fd != -1) {
		client_socket_close(&http->sock.fd);
		dlc_transport_evt_disconnected(dlc);
	}

	return 0;
}

static int dlc_http_connect(struct download_client *dlc)
{
	int err;
	int ns_err;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	err = -1;
	ns_err = -1;

	if (!http->sock.remote_addr.sa_family) {
		err = client_socket_host_lookup(dlc->hostname, dlc->host_config.pdn_id, &http->sock.remote_addr);
		if (err) {
			return err;
		}
	}

	err = client_socket_configure_and_connect(
		&http->sock.fd, http->sock.proto, http->sock.type, http->sock.port,
		&http->sock.remote_addr, dlc->hostname, &dlc->host_config);
	if (err) {
		return err;
	}

	err = client_socket_recv_timeout_set(http->sock.fd,
					     CONFIG_DOWNLOAD_CLIENT_HTTP_TIMEO_MS);
	if (err) {
		/* Unable to connect, close socket */
		client_socket_close(&http->sock.fd);
		return err;
	}

	http->new_data_req = true;

	dlc_transport_evt_connected(dlc);

	return err;
}

static int dlc_http_close(struct download_client *dlc)
{
	int err;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	if (http->sock.fd != -1) {
		err = client_socket_close(&http->sock.fd);
		dlc_transport_evt_disconnected(dlc);
		return err;
	}

	return -EBADF;
}

static int dlc_http_download(struct download_client *dlc)
{
	int ret, len;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dlc->transport_internal;

	if (http->connection_close) {
		return -ECONNRESET;
	}

	if (http->new_data_req) {
		/* Request next fragment */
		dlc->buf_offset = 0;
		ret = http_get_request_send(dlc);
		if (ret) {
			LOG_DBG("data_req failed, err %d", ret);
			/** Attempt reconnection. */
			return -ECONNRESET;
		}

		http->new_data_req = false;
	}

	__ASSERT(dlc->buf_offset < dlc->config.buf_size, "Buffer overflow");

	LOG_DBG("Receiving up to %d bytes at %p...",
		(dlc->config.buf_size - dlc->buf_offset),
		(void *)(dlc->config.buf + dlc->buf_offset));

	len = client_socket_recv(http->sock.fd,
				 dlc->config.buf + dlc->buf_offset,
				 dlc->config.buf_size - dlc->buf_offset);
	if (len < 0) {
		return len;
	}

	if (len == 0) {
		return -ECONNRESET;
	}

	ret = http_parse(dlc, len);
	if (ret <= 0) {
		return ret;
	}

	if (http->header.has_end ) {
		/* Accumulate progress */
		dlc->progress += ret;
		dlc_transport_evt_data(dlc, dlc->config.buf, ret);
		if (dlc->progress == dlc->file_size) {
			dlc_transport_evt_download_complete(dlc);
		}
		dlc->buf_offset = 0;
	}

	return 0;
}

static struct dlc_transport dlc_transport_http = {
	.proto_supported = dlc_http_proto_supported,
	.init = dlc_http_init,
	.deinit = dlc_http_deinit,
	.connect = dlc_http_connect,
	.close = dlc_http_close,
	.download = dlc_http_download,
};

DLC_TRANSPORT(http, &dlc_transport_http);