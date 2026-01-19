/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#include <net/downloader.h>
#include <net/downloader_transport.h>
#include <net/downloader_transport_http.h>
#include "dl_socket.h"
#include "dl_parse.h"

LOG_MODULE_DECLARE(downloader, CONFIG_DOWNLOADER_LOG_LEVEL);

/* Minimum size for DFU Target library to detect the image type. */
#define MIN_SIZE_IDENTIFY_BUF 32

/* In the nRF91 Series modem, the TLS secure socket buffer is limited to around 2 kB.
 * If the header and data is too large, the downloader will reduce the range requests until
 * the requirements are satisfied. The application can also set the range_override in
 * struct downloader_host_cfg to utilize the size better.
 */
#define TLS_RANGE_MAX 2048

#define DEFAULT_PORT_TLS 443
#define DEFAULT_PORT_TCP 80

#define HTTP_RESPONSE_OK 200
#define HTTP_RESPONSE_PARTIAL_CONTENT 206
#define HTTP_RESPONSE_MOVED_PERMANENTLY 301
#define HTTP_RESPONSE_FOUND 302
#define HTTP_RESPONSE_SEE_OTHER 303
#define HTTP_RESPONSE_TEMPORARY_REDIRECT 307
#define HTTP_RESPONSE_PERMANENT_REDIRECT 308

#define HTTP "http://"
#define HTTPS "https://"

/* Request whole file; use with HTTP */
#define HTTP_GET                                                                                   \
	"GET /%s HTTP/1.1\r\n"                                                                     \
	"Host: %s\r\n"                                                                             \
	"Connection: keep-alive\r\n"                                                               \
	"\r\n"

/* Request remaining bytes from offset; use with HTTP */
#define HTTP_GET_OFFSET                                                                            \
	"GET /%s HTTP/1.1\r\n"                                                                     \
	"Host: %s\r\n"                                                                             \
	"Range: bytes=%u-\r\n"                                                                     \
	"Connection: keep-alive\r\n"                                                               \
	"\r\n"

/* Request a range of bytes; use with HTTPS due to modem limitations */
#define HTTP_GET_RANGE                                                                             \
	"GET /%s HTTP/1.1\r\n"                                                                     \
	"Host: %s\r\n"                                                                             \
	"Range: bytes=%u-%u\r\n"                                                                   \
	"Connection: keep-alive\r\n"                                                               \
	"\r\n"

struct transport_params_http {
	/** Whether transport config has been set by the application. */
	bool cfg_set;
	/** Configuration options */
	struct downloader_transport_http_cfg cfg;

	/* The following fields are reset before each download: */

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
		struct net_sockaddr remote_addr;
	} sock;

	/** Request new data */
	bool new_data_req;
	/** Redirect retries */
	uint8_t redirects;
};

BUILD_ASSERT(CONFIG_DOWNLOADER_TRANSPORT_PARAMS_SIZE >= sizeof(struct transport_params_http));

/* Include size flavour of strstr for safety. */
#if defined(CONFIG_EXTERNAL_LIBC)
/* Pull in memmem and strnstr due to being an extension to the C library and
 * not included by default.
 */
extern void *memmem(const void *haystack, size_t hs_len, const void *needle, size_t ne_len);
extern size_t strnlen(const char *s, size_t maxlen);
static char *strnstr(const char *haystack, const char *needle, size_t haystack_len)
{
	char *x;

	if (!haystack || !needle) {
		return NULL;
	}
	size_t needle_len = strnlen(needle, haystack_len);

	if (needle_len < haystack_len || !needle[needle_len]) {
		x = memmem(haystack, haystack_len, needle, needle_len);
		if (x && !memchr(haystack, 0, x - haystack)) {
			return x;
		}
	}

	return NULL;
}
#else
extern char *strnstr(const char *haystack, const char *needle, size_t haystack_sz);
#endif

static int parse_protocol(struct downloader *dl, const char *url);

static int http_get_request_send(struct downloader *dl)
{
	int err;
	int len;
	size_t off = 0;
	bool tls_force_range;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	http->header.has_end = false;

	/* nRF91 series has a limitation of decoding ~2k of data at once when using TLS */
	tls_force_range = (http->sock.proto == NET_IPPROTO_TLS_1_2 &&
			   !dl->host_cfg.set_native_tls && IS_ENABLED(CONFIG_SOC_SERIES_NRF91X));
	if (tls_force_range) {
		if (dl->host_cfg.range_override > TLS_RANGE_MAX) {
			LOG_WRN("Range override > TLS max range, setting to TLS max range");
			dl->host_cfg.range_override = TLS_RANGE_MAX;
		} else if (dl->host_cfg.range_override == 0) {
			dl->host_cfg.range_override = TLS_RANGE_MAX;
		}
	}

	if (dl->host_cfg.range_override) {
		off = dl->progress + dl->host_cfg.range_override - 1;

		if (dl->file_size) {
			/* Don't request bytes past the end of file */
			off = MIN(off, dl->file_size - 1);
		}

		len = snprintf(dl->cfg.buf, dl->cfg.buf_size, HTTP_GET_RANGE, dl->file,
			       dl->hostname, dl->progress, off);
		http->ranged = true;
		http->ranged_progress = 0;
		LOG_DBG("Range request up to %d bytes", dl->host_cfg.range_override);
		goto send;
	} else if (dl->progress) {
		len = snprintf(dl->cfg.buf, dl->cfg.buf_size, HTTP_GET_OFFSET, dl->file,
			       dl->hostname, dl->progress);
		http->ranged = false;
	} else {
		len = snprintf(dl->cfg.buf, dl->cfg.buf_size, HTTP_GET, dl->file,
			       dl->hostname);
		http->ranged = false;
	}

send:
	if (len < 0 || len > dl->cfg.buf_size) {
		LOG_ERR("Cannot create GET request, buffer too small");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_DOWNLOADER_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(dl->cfg.buf, len, "HTTP request");
	}

	LOG_DBG("http request:\n%s", dl->cfg.buf);

	err = dl_socket_send(http->sock.fd, dl->cfg.buf, len);
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
static int http_header_parse(struct downloader *dl, size_t buf_len)
{
	int err;
	char *p;
	char *q;
	size_t parse_len;
	unsigned int expected_status;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	LOG_DBG("(partial) http header response:\n%.*s", buf_len, dl->cfg.buf);

	p = strnstr(dl->cfg.buf, "\r\n\r\n", buf_len);
	if (p) {
		/* End of header received */
		http->header.has_end = true;
		parse_len = p + strlen("\r\n\r\n") - (char *)dl->cfg.buf;
	} else {
		parse_len = buf_len;
	}

	/* Convert HTTP headers to lowercase, but not the values (for example URI) */
	bool value = false;
	for (size_t i = 0; i < parse_len; i++) {
		if (dl->cfg.buf[i] == '\r' || dl->cfg.buf[i] == '\n') {
			value = false;
		}
		if (value) {
			continue;
		}
		if (dl->cfg.buf[i] == ':') {
			value = true;
			continue;
		}
		dl->cfg.buf[i] = tolower(dl->cfg.buf[i]);
	}

	/* Look for the status code just after "http/1.1 " */
	p = strnstr(dl->cfg.buf, "http/1.1 ", parse_len);
	if (p) {
		q = strnstr(p, "\r\n", parse_len - (p - dl->cfg.buf));
		if (q) {
			/* Received entire line */
			p += strlen("http/1.1 ");
			http->header.status_code = strtoul(p, &q, 10);
		}
	}

	if (http->header.status_code == HTTP_RESPONSE_MOVED_PERMANENTLY ||
	    http->header.status_code == HTTP_RESPONSE_FOUND ||
	    http->header.status_code == HTTP_RESPONSE_SEE_OTHER ||
	    http->header.status_code == HTTP_RESPONSE_TEMPORARY_REDIRECT ||
	    http->header.status_code == HTTP_RESPONSE_PERMANENT_REDIRECT) {
		/* Resource is moved, update host and file before reconnecting. */
		p = strnstr(dl->cfg.buf, "\r\nlocation:", parse_len);
		if (p) {
			q = strnstr((p + 1), "\r\n", parse_len - ((p + 1) - dl->cfg.buf));
			if (q) {

				/* Received entire line */
				p += strlen("\r\nlocation: ");
				*q = '\0';

				LOG_INF("Resource moved to %s", p);

				err = parse_protocol(dl, p);
				if (err) {
					LOG_ERR("Failed to parse protocol, err %d, url %s", err, p);
					return -EBADMSG;
				}

				err = dl_parse_url_host(p, dl->hostname, sizeof(dl->hostname));
				if (err) {
					LOG_ERR("Failed to parse hostname, err %d, url %s", err, p);
					return -EBADMSG;
				}

				err = dl_parse_url_file(p, dl->file, sizeof(dl->file));
				if (err) {
					LOG_ERR("Failed to parse filename, err %d, url %s", err, p);
					k_mutex_unlock(&dl->mutex);
					return -EBADMSG;
				}

				http->redirects++;

				if (http->redirects > dl->host_cfg.redirects_max) {
					LOG_ERR("Maximum redirections reached, aborting");
					return -EMLINK;
				}

				return -ECONNRESET;
			}
		}
	}

	/* The file size is returned via "Content-Length" in case of HTTP,
	 * and via "Content-Range" in case of HTTPS with range requests.
	 */
	do {
		if (dl->file_size == 0) {
			if (http->ranged) {
				p = strnstr(dl->cfg.buf, "\r\ncontent-range", parse_len);
				if (!p) {
					break;
				}
				p = strnstr(p, "/", parse_len - (p - dl->cfg.buf));
				if (!p) {
					break;
				}
				q = strnstr(p, "\r\n", parse_len - (p - dl->cfg.buf));
				if (!q) {
					/* Missing end of line */
					break;
				}
			} else { /* proto == PROTO_HTTP */
				p = strnstr(dl->cfg.buf, "\r\ncontent-length", parse_len);
				if (!p) {
					break;
				}
				p = strstr(p, ":");
				if (!p) {
					break;
				}
				q = strnstr(p, "\r\n", parse_len - (p - dl->cfg.buf));
				if (!q) {
					/* Missing end of line */
					break;
				}
				/* Accumulate any eventual progress (starting offset)
				 * when reading the file size from Content-Length
				 */
				dl->file_size = dl->progress;
			}

			dl->file_size += atoi(p + 1);
			LOG_DBG("File size = %u", dl->file_size);
		}
	} while (0);

	p = strnstr(dl->cfg.buf, "\r\nconnection: close", parse_len);
	if (p) {
		LOG_WRN("Peer closed connection, will re-connect");
		http->connection_close = true;
	}

	if (http->header.has_end) {
		/* We have received the end of the header.
		 * Verify that we have received everything that we need.
		 */

		if (!http->header.status_code) {
			LOG_ERR("Server response malformed: status code not found");
			return -EBADMSG;
		}

		expected_status = (http->ranged || dl->progress) ? HTTP_RESPONSE_PARTIAL_CONTENT :
								   HTTP_RESPONSE_OK;
		if (http->header.status_code != expected_status) {
			LOG_ERR("Unexpected HTTP response code %ld", http->header.status_code);
			return -EBADMSG;
		}

		if (!dl->file_size) {
			LOG_ERR("File size not set");
			return -EBADMSG;
		}

		return parse_len;
	}

	q = dl->cfg.buf + buf_len;
	/* We are still missing part of the header.
	 * Return the lines (in number of bytes) that we have parsed.
	 */
	while (q > dl->cfg.buf && (*q != '\r') && (*q != '\n')) {
		q--;
	}

	/* Keep \r and \n in the buffer in case it is part of the header ending. */
	while (*(q - 1) == '\r' || *(q - 1) == '\n') {
		q--;
	}

	parse_len = (q - dl->cfg.buf);

	return parse_len;
}

/** Separate HTTP headers and data.
 * Update the buf_offset pointer to end of content.
 *
 * @return Length of data payload left to process on success,
 *         negative errno on error.
 */
static int http_parse(struct downloader *dl, size_t len)
{
	int parsed_len;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	if (!http->header.has_end) {
		if (len <= 0) {
			return -ECONNRESET;
		}

		/* Parse what we can from the header */
		parsed_len = http_header_parse(dl, len);
		if (parsed_len < 0) {
			/* Something is wrong with the header */
			return parsed_len;
		}

		if (parsed_len == len) {
			dl->buf_offset = 0;
			return 0;
		} else if (parsed_len) {
			/* Keep remaining payload */
			len = len - parsed_len;
			memmove(dl->cfg.buf, dl->cfg.buf + parsed_len, len);
			dl->buf_offset = len;
		}

		if (!http->header.has_end) {
			if (dl->cfg.buf_size == dl->buf_offset) {
				LOG_ERR("Could not parse HTTP header lines from server (> %d)",
					dl->cfg.buf_size);
				return -E2BIG;
			}
			/* Wait for rest of header */
			return 0;
		}
	} else {
		/* Forward the offset pointer, so we could cumulate data */
		dl->buf_offset = len;
	}

	return len;
}

static bool dl_http_proto_supported(struct downloader *dl, const char *url)
{
	if (strncmp(url, HTTPS, (sizeof(HTTPS) - 1)) == 0) {
		return true;
	}

	if (strncmp(url, HTTP, (sizeof(HTTP) - 1)) == 0) {
		return true;
	}

	return false;
}

static int parse_protocol(struct downloader *dl, const char *url)
{
	int err;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;


	http->sock.proto = NET_IPPROTO_TCP;
	http->sock.type = NET_SOCK_STREAM;

	if (strncmp(url, HTTPS, (sizeof(HTTPS) - 1)) == 0 ||
	    (strncmp(url, HTTP, (sizeof(HTTP) - 1)) != 0 &&
	     (dl->host_cfg.sec_tag_count != 0 && dl->host_cfg.sec_tag_list != NULL))) {
		http->sock.proto = NET_IPPROTO_TLS_1_2;
		http->sock.type = NET_SOCK_STREAM;

		if (dl->host_cfg.sec_tag_list == NULL || dl->host_cfg.sec_tag_count == 0) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			return -EINVAL;
		}
	}

	err = dl_parse_url_port(url, &http->sock.port);
	if (err) {
		switch (http->sock.proto) {
		case NET_IPPROTO_TLS_1_2:
			http->sock.port = DEFAULT_PORT_TLS;
			break;
		case NET_IPPROTO_TCP:
			http->sock.port = DEFAULT_PORT_TCP;
			break;
		}
		LOG_DBG("Port not specified, using default: %d", http->sock.port);
	}

	if (dl->host_cfg.set_native_tls) {
		LOG_DBG("Enabled native TLS");
		http->sock.type |= SOCK_NATIVE_TLS;
	}

	return 0;
}

static int dl_http_init(struct downloader *dl, struct downloader_host_cfg *dl_host_cfg,
			const char *url)
{
	struct transport_params_http *http;
	uint8_t *reset_ptr;

	http = (struct transport_params_http *)dl->transport_internal;

	if (!http->cfg_set) {
		/* Set socket receive timeout to 30s by default. */
		http->cfg.sock_recv_timeo_ms = 30 * MSEC_PER_SEC;
	}

	/* Reset all fields after the config. */
	reset_ptr = (uint8_t *)&http->cfg + sizeof(http->cfg);
	memset(reset_ptr,
	       0,
	       sizeof(struct transport_params_http) - ((uint8_t *)reset_ptr - (uint8_t *)http));

	return parse_protocol(dl, url);
}

static int dl_http_deinit(struct downloader *dl)
{
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	if (http->sock.fd != -1) {
		dl_socket_close(&http->sock.fd);
	}

	return 0;
}

static int dl_http_connect(struct downloader *dl)
{
	int err;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	err = -1;

	err = dl_socket_configure_and_connect(&http->sock.fd, http->sock.proto, http->sock.type,
					      http->sock.port, &http->sock.remote_addr,
					      dl->hostname, &dl->host_cfg);
	if (err) {
		return err;
	}

	err = dl_socket_recv_timeout_set(http->sock.fd, http->cfg.sock_recv_timeo_ms);
	if (err) {
		/* Unable to set timeout, close socket */
		LOG_ERR("Failed to set http recv timeout, err %d", err);
		dl_socket_close(&http->sock.fd);
		return err;
	}

	http->connection_close = false;
	http->new_data_req = true;

	return err;
}

static int dl_http_close(struct downloader *dl)
{
	int err;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	if (http->sock.fd != -1) {
		err = dl_socket_close(&http->sock.fd);
		return err;
	}

	memset(&http->sock.remote_addr, 0, sizeof(http->sock.remote_addr));

	return -EBADF;
}

static int dl_http_download(struct downloader *dl)
{
	int ret, recv_len, data_len, expected_len;
	struct transport_params_http *http;

	http = (struct transport_params_http *)dl->transport_internal;

	if (http->new_data_req) {
		/* Request next fragment */
		dl->buf_offset = 0;
		ret = http_get_request_send(dl);
		if (ret) {
			LOG_DBG("data_req failed, err %d", ret);
			/** Attempt reconnection. */
			return -ECONNRESET;
		}

		http->new_data_req = false;
	}

	__ASSERT(dl->buf_offset < dl->cfg.buf_size, "Buffer overflow");

	LOG_DBG("Receiving up to %d bytes at %p...", (dl->cfg.buf_size - dl->buf_offset),
		(void *)(dl->cfg.buf + dl->buf_offset));

	recv_len = dl_socket_recv(http->sock.fd, dl->cfg.buf + dl->buf_offset,
			     dl->cfg.buf_size - dl->buf_offset);

	if (recv_len < 0) {
		if (recv_len == -EMSGSIZE && dl->host_cfg.range_override) {
			/* We do not have enough space for the http header and requested data,
			 * reattempt with shorter range request.
			 */
			dl->host_cfg.range_override -=
				((dl->host_cfg.range_override > 256) ? 128 : 8);
			if (dl->host_cfg.range_override <= 8) {
				return -EMSGSIZE;
			}
			LOG_DBG("Message size too big, reattempting with range size %d",
				dl->host_cfg.range_override);
			return -ECONNRESET;
		}
		if (http->connection_close) {
			return -ECONNRESET;
		}

		return recv_len;
	}

	data_len = http_parse(dl, recv_len + dl->buf_offset);
	if (data_len < 0) {
		return data_len;
	}

	expected_len = MIN(MIN_SIZE_IDENTIFY_BUF, dl->file_size - dl->progress);

	if (data_len < expected_len) {
		/* Wait for more data after the HTTP headers,
		 * so we don't end up forwarding too small chunks to FOTA library.
		 */
		return recv_len > 0 ? 0 : -ECONNRESET; /* Fail if closed while expecting more */
	}

	/* Accumulate progress */
	dl->progress += data_len;
	if (data_len) {
		dl_transport_evt_data(dl, dl->cfg.buf, data_len);
	}
	if (http->ranged) {
		http->ranged_progress += data_len;
		if (http->ranged_progress < dl->host_cfg.range_override) {
			/* Ranged query: read until a full fragment is received */
		} else {
			/* Ranged query: request next fragment */
			http->new_data_req = true;
		}
	}
	if (dl->progress == dl->file_size) {
		/* A full file has been received */
		dl->complete = true;
		http->new_data_req = true;
	}
	dl->buf_offset = 0;

	if (dl->complete) {
		return 0;
	}
	/* Continue reading, unless connection is closed */
	return recv_len > 0 ? 0 : -ECONNRESET;
}

static const struct dl_transport dl_transport_http = {
	.proto_supported = dl_http_proto_supported,
	.init = dl_http_init,
	.deinit = dl_http_deinit,
	.connect = dl_http_connect,
	.close = dl_http_close,
	.download = dl_http_download,
};

DL_TRANSPORT(http, &dl_transport_http);

int downloader_transport_http_set_config(struct downloader *dl,
					 struct downloader_transport_http_cfg *cfg)
{
	struct transport_params_http *http;

	if (!dl || !cfg) {
		return -EINVAL;
	}

	http = (struct transport_params_http *)dl->transport_internal;
	http->cfg_set = true;
	http->cfg = *cfg;

	return 0;
}
