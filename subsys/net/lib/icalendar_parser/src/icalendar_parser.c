/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <strings.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <toolchain/common.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <net/icalendar_parser.h>
#include <logging/log.h>
#include <nrf_socket.h>

LOG_MODULE_REGISTER(icalendar_parser, CONFIG_ICALENDAR_PARSER_LOG_LEVEL);

#define GET_TEMPLATE		     \
	"GET %s HTTP/1.1\r\n"	     \
	"Host: %s\r\n"		     \
	"Connection: keep-alive\r\n" \
	"Accept: text/calendar\r\n"  \
	"Range: bytes=%u-%u\r\n"     \
	"\r\n"

BUILD_ASSERT_MSG(CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE <=
		 CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE,
		 "The response buffer must accommodate for a full fragment");

#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_IMMEDIATE)
BUILD_ASSERT_MSG(IS_ENABLED(CONFIG_ICALENDAR_PARSER_LOG_HEADERS) ?
		 CONFIG_LOG_BUFFER_SIZE >= 2048 : 1,
		 "Please increase log buffer sizer");
#endif

static int socket_timeout_set(int fd)
{
	int err;

	if (CONFIG_ICALENDAR_PARSER_SOCK_TIMEOUT_MS == K_FOREVER) {
		return 0;
	}

	const u32_t timeout_ms = CONFIG_ICALENDAR_PARSER_SOCK_TIMEOUT_MS;

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
	nrf_sec_session_cache_t sec_session_cache;

	enum {
		NONE            = 0,
		OPTIONAL        = 1,
		REQUIRED        = 2,
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

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, errno %d", errno);
		return -1;
	}

	/* Disable TLE session cache */
	sec_session_cache = 0;
	err = nrf_setsockopt(fd, SOL_TLS, NRF_SO_SEC_SESSION_CACHE, &sec_session_cache, sizeof(sec_session_cache));
	if (err) {
		LOG_ERR("Failed to disable TLE session cache, errno %d", errno);
		return -1;
	}

	return 0;
}

static int resolve_and_connect(int family, const char *host, int sec_tag)
{
	int fd;
	int err;
	int proto;
	u16_t port;
	struct addrinfo *addr;
	struct addrinfo *info;

	__ASSERT_NO_MSG(host);

	/* Set up port and protocol */
	if (sec_tag == -1) {
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

	if (proto == IPPROTO_TLS_1_2) {
		LOG_INF("Setting up TLS credentials");
		err = socket_sectag_set(fd, sec_tag);
		if (err) {
			goto cleanup;
		}
	}

	/* Not connected */
	err = -1;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr *const sa = addr->ai_addr;

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

static int socket_send(const struct icalendar_parser *ical, size_t len)
{
	int sent;
	size_t off = 0;

	while (len) {
		sent = send(ical->fd, ical->http_tx_buf + off, len, 0);
		if (sent <= 0) {
			return -EIO;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

static int get_request_send(struct icalendar_parser *ical)
{
	int err;
	int len;
	size_t off;

	__ASSERT_NO_MSG(ical);
	__ASSERT_NO_MSG(ical->host);
	__ASSERT_NO_MSG(ical->file);

	memset(ical->http_tx_buf, 0, sizeof(ical->http_tx_buf));

	/* Offset of last byte in range (Content-Range) */
	off = ical->progress + CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE - 1 - ical->offset;

	len = snprintf(ical->http_tx_buf, CONFIG_ICALENDAR_PARSER_MAX_TX_SIZE,
		       GET_TEMPLATE, ical->file, ical->host,
		       ical->progress, off);

	if (len < 0 || len > CONFIG_ICALENDAR_PARSER_MAX_TX_SIZE) {
		LOG_ERR("Cannot create GET request, buffer too small");
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_ICALENDAR_PARSER_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(ical->http_tx_buf, len, "HTTP request");
	}

	LOG_DBG("Sending HTTP request");
	err = socket_send(ical, len);
	if (err) {
		LOG_ERR("Failed to send HTTP request, errno %d", errno);
		return err;
	}

	return 0;
}

static size_t unfold_contentline(const char *p_buf, char *p_ical_prop_buf)
{
	size_t unfold_size = 0;
	const char *p_sol = p_buf;
	char *p_eol;
	u16_t total_line_len = 0, single_line_len = 0;

	while (1) {
		p_eol = strstr(p_sol, "\r\n");
		if (!p_eol) {
			return 0;
		}
		single_line_len = p_eol - p_sol;
		if ((total_line_len + single_line_len) <= CONFIG_MAX_PROPERTY_SIZE) {
			memcpy(p_ical_prop_buf + total_line_len, p_sol, single_line_len);
			total_line_len += single_line_len;
		} else {
			LOG_DBG("Calendar property is too large. Try increase max property length in config.");
			return 0;
		}
		if (!strncmp(p_eol, "\r\n ", strlen("\r\n "))) {
			/* Long content line is split into multiple lines.
			 * Copy the folded part to unfolded buffer.
			 */
			p_sol = p_eol + strlen("\r\n ");
		} else {
			/* Content line is delimited. Count parsed bytes */
			unfold_size = p_eol - p_buf;
			break;
		}
	}

	return unfold_size;
}

static bool parse_descriptive_props(const char *p_buf, const char *p_name,
				    size_t name_size, char *p_value, size_t max_value_len)
{
	size_t unfold_size;
	char ical_prop_buf[CONFIG_MAX_PROPERTY_SIZE];

	unfold_size = unfold_contentline(p_buf, ical_prop_buf);
	if (unfold_size > 0) {
		if (ical_prop_buf[name_size] == ':') {
			size_t value_len = unfold_size - name_size - 1;
			if (value_len <= max_value_len) {
				memcpy(p_value, ical_prop_buf + name_size + 1, value_len);
				return true;
			} else {
				/* Property value overflow. */
				LOG_ERR("%s value overflow.", p_name);
				return false;
			}
		} else if (ical_prop_buf[name_size] == ';') {
			/* Does not support property parameter. */
			LOG_ERR("%s param not supported.", p_name);
			return false;
		} else {
			/* Property wrong format - no parameter or value. */
			LOG_ERR("%s wrong format.", p_name);
			return false;
		}
	} else {
		/* Property wrong format - no parameter or value. */
		LOG_ERR("%s no value/param.", p_name);
		return false;
	}
}

static bool parse_datetime_props(const char *p_buf, const char *p_name,
				 size_t name_size, char *p_value, size_t max_value_len)
{
	size_t unfold_size;
	char ical_prop_buf[CONFIG_MAX_PROPERTY_SIZE];

	memset(ical_prop_buf, 0, sizeof(ical_prop_buf));
	unfold_size = unfold_contentline(p_buf, ical_prop_buf);
	if (unfold_size > 0) {
		if (ical_prop_buf[name_size] == ':') {
			size_t value_len = unfold_size - name_size - 1;
			if (value_len <= max_value_len) {
				memcpy(p_value, ical_prop_buf + name_size + 1, value_len);
				return true;
			} else {
				/* Property value overflow. */
				LOG_ERR("%s value overflow.", p_name);
				return false;
			}
		} else if (ical_prop_buf[name_size] == ';') {
			char *p_dtvalue;
			p_dtvalue = strchr(ical_prop_buf, ':');
			if (p_dtvalue) {
				p_dtvalue = p_dtvalue + 1;
				size_t value_len = unfold_size - (p_dtvalue - ical_prop_buf);
				if (value_len <= max_value_len) {
					memcpy(p_value, p_dtvalue, value_len);
					return true;
				} else {
					/* Property value overflow. */
					LOG_ERR("%s value overflow.", p_name);
					return false;
				}
			} else {
				/* Property wrong format - no value. */
				LOG_ERR("%s wrong format - no value.", p_name);
				return false;
			}
		} else {
			/* Property wrong format - no parameter or value. */
			LOG_ERR("%s wrong format.", p_name);
			return false;
		}
	} else {
		/* Property wrong format - fail to unfold property. */
		LOG_ERR("%s wrong format. Fail to unfold property", p_name);
		return false;
	}
}

static size_t parse_calprops(const char *p_buf)
{
	const char *p_parsed = p_buf;
	char prop_value[64];

	p_parsed = strstr(p_parsed, "BEGIN:VCALENDAR\r\n");
	if (p_parsed) {
		char *p_end;
		p_parsed += strlen("BEGIN:VCALENDAR\r\n");
		p_end = strstr(p_parsed, "\r\nBEGIN:");
		if (p_end) {
			p_end += strlen("\r\n");
			while (p_parsed < p_end) {
				memset(prop_value, 0, sizeof(prop_value));
				if (!strncmp(p_parsed, "PRODID", strlen("PRODID"))) {
					if (!parse_descriptive_props(p_parsed, "PRODID", 6, prop_value, sizeof(prop_value))) {
						LOG_ERR("Wrong PRODID property");
					}
				} else if (!strncmp(p_parsed, "VERSION", strlen("VERSION"))) {
					if (!parse_descriptive_props(p_parsed, "VERSION", 7, prop_value, sizeof(prop_value))) {
						LOG_ERR("Wrong VERSION property");
					}
				}
				p_parsed = strstr(p_parsed, "\r\n");
				p_parsed += strlen("\r\n");
			}
		}
	} else {
		return 0;
	}

	return (p_parsed - p_buf);
}

static size_t parse_eventprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;

	if (!strncmp(p_buf, "BEGIN:VEVENT\r\n", strlen("BEGIN:VEVENT\r\n"))) {
		char *p_com_end;
		p_com_end = strstr(p_buf, "END:VEVENT\r\n");
		if (p_com_end) {
			p_com_end += strlen("END:VEVENT\r\n");
			while (p_parsed < p_com_end) {
				if (!strncasecmp(p_parsed, "SUMMARY", strlen("SUMMARY"))) {
					if (!parse_descriptive_props(p_parsed, "SUMMARY", 7, p_ical_com->summary, CONFIG_MAX_SUMMARY_PROPERTY_SIZE)) {
						LOG_ERR("Wrong SUMMARY property");
					}
				} else if (!strncasecmp(p_parsed, "LOCATION", strlen("LOCATION"))) {
					if (!parse_descriptive_props(p_parsed, "LOCATION", 8, p_ical_com->location, CONFIG_MAX_LOCATION_PROPERTY_SIZE)) {
						LOG_ERR("Wrong LOCATION property");
					}
				} else if (!strncasecmp(p_parsed, "DESCRIPTION", strlen("DESCRIPTION"))) {
					if (!parse_descriptive_props(p_parsed, "DESCRIPTION", 11, p_ical_com->description, CONFIG_MAX_DESCRIPTION_PROPERTY_SIZE)) {
						LOG_ERR("Wrong DESCRIPTION property");
					}
				} else if (!strncasecmp(p_parsed, "DTSTART", strlen("DTSTART"))) {
					if (!parse_datetime_props(p_parsed, "DTSTART", 7, p_ical_com->dtstart, CONFIG_MAX_DTSTART_PROPERTY_SIZE)) {
						LOG_ERR("Wrong DTSTART property");
					}
				} else if (!strncasecmp(p_parsed, "DTEND", strlen("DTEND"))) {
					if (!parse_datetime_props(p_parsed, "DTEND", 5, p_ical_com->dtend, CONFIG_MAX_DTEND_PROPERTY_SIZE)) {
						LOG_ERR("Wrong DTEND property");
					}
				}

				p_parsed = strstr(p_parsed, "\r\n") + strlen("\r\n");
			}
		}
	}

	return p_parsed - p_buf;
}

static size_t parse_todoprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VTODO\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VTODO\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_jourprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VJOURNAL\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VJOURNAL\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_fbprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VFREEBUSY\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VFREEBUSY\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_tzprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VTIMEZONE\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VTIMEZONE\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_component(const char *p_buf, struct ical_component *p_ical_com)
{
	char *p_com_begin;
	size_t ret = 0;

	/* Search begin of component */
	p_com_begin = strstr(p_buf, "BEGIN:");
	if (p_com_begin == NULL) {
		return ret;
	}

	ret = p_com_begin - p_buf;
	if (!strncmp(p_com_begin, "BEGIN:VEVENT\r\n", strlen("BEGIN:VEVENT\r\n"))) {
		ret += parse_eventprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VTODO\r\n", strlen("BEGIN:VTODO\r\n"))) {
		ret += parse_todoprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VJOURNAL\r\n", strlen("BEGIN:VJOURNAL\r\n"))) {
		ret += parse_jourprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VFREEBUSY\r\n", strlen("BEGIN:VFREEBUSY\r\n"))) {
		ret += parse_fbprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VTIMEZONE\r\n", strlen("BEGIN:VTIMEZONE\r\n"))) {
		ret += parse_tzprop(p_com_begin, p_ical_com);
	}

	return ret;
}

static size_t parse_icalbody(const char *p_buf, icalendar_parser_callback_t callback)
{
	size_t parsed_bytes = 0, parsed_offset = 0;

	do {
		struct ical_component ical_com;
		memset(&ical_com, 0, sizeof(ical_com));
		parsed_bytes = parse_component(p_buf + parsed_offset, &ical_com);
		if (parsed_bytes > 0) {
			const struct icalendar_parser_evt evt = {
				.id = ICAL_PARSER_EVT_COMPONENT,
			};
			callback(&evt, &ical_com);
		}
		parsed_offset += parsed_bytes;
	} while (parsed_bytes > 0);

	return parsed_offset;
}

static size_t parse_icalstream(struct icalendar_parser *ical)
{
	size_t parsed_offset = 0;

	/* Check begin of iCalendar object delimiter
	 * Reference: RFC 5545 3.4 iCalendar Object
	 */
	if (!ical->icalobject_begin) {
		/* The body of the iCalendar object consists of a sequence of calendar
		 * properties and one or more calendar components.
		 *
		 * Reference: RFC 5545 3.6 Calendar Components
		 */
		parsed_offset += parse_calprops(ical->http_rx_buf);
		if (parsed_offset > 0) {
			LOG_DBG("Found a calendar stream");
			ical->icalobject_begin = true;
			const struct icalendar_parser_evt evt = {
				.id = ICAL_PARSER_EVT_CALENDAR,
			};
			ical->callback(&evt, NULL);
		}
	}

	/* If we got a calendar property, start parsing calendar body. */
	if (ical->icalobject_begin) {
		/* Parse recevied buffer either BEGIN and END of iCalendar object is matched, or
		 * when HTTP RX buffer is full.
		 */
		if (strstr(ical->http_rx_buf, "END:VCALENDAR\r\n") != NULL) {
			/* Receive pair of iCalendar object delimiter */
			parsed_offset += parse_icalbody(ical->http_rx_buf, ical->callback);
			ical->icalobject_end = true;
		} else if (ical->offset == CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE) {
			/* HTTP RX buffer is full */
			parsed_offset += parse_icalbody(ical->http_rx_buf, ical->callback);
		}
	}

	return parsed_offset;
}

/* Returns:
 *  1 while the header is being received but not complete
 *  0 if the header has been fully received
 * -1 on error
 */
static int header_parse(struct icalendar_parser *ical)
{
	char *p;
	size_t hdr;

	p = strstr(ical->http_rx_buf, "\r\n\r\n");
	if (!p) {
		/* Awaiting full GET response */
		LOG_DBG("Awaiting full header in response");
		return 1;
	}

	/* Offset of the end of the HTTP header in the buffer */
	hdr = p + strlen("\r\n\r\n") - ical->http_rx_buf;

	__ASSERT(hdr < sizeof(ical->http_rx_buf), "Buffer overflow");

	LOG_DBG("GET header size: %u", hdr);

	if (IS_ENABLED(CONFIG_ICALENDAR_PARSER_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(ical->http_rx_buf, hdr, "GET");
	}

	if (!strstr(ical->http_rx_buf, "Content-Type: text/calendar")) {
		LOG_WRN("Content type is not text/calendar");
	}

	p = strstr(ical->http_rx_buf, "Connection: close");
	if (p) {
		LOG_WRN("Peer closed connection, will attempt to re-connect");
		ical->connection_close = true;
	}

	if (ical->offset != hdr) {
		/* The current buffer contains some payload bytes.
		 * Copy them at the beginning of the buffer
		 * then update the offset.
		 */
		LOG_DBG("Copying %u payload bytes", ical->offset - hdr);
		memcpy(ical->http_rx_buf, ical->http_rx_buf + hdr, ical->offset - hdr);
		ical->offset -= hdr;
	} else {
		/* Reset the offset.
		 * The payload is received in an empty buffer.
		 */
		ical->offset = 0;
	}

	return 0;
}

static void error_evt_send(const struct icalendar_parser *ical, int error)
{
	/* Error will be sent as negative. */
	__ASSERT_NO_MSG(error > 0);

	const struct icalendar_parser_evt evt = {
		.id = ICAL_PARSER_EVT_ERROR,
		.error = -error
	};

	ical->callback(&evt, NULL);
}

void download_thread(void *icalendar, void *a, void *b)
{
	int rc;
	size_t len;
	struct icalendar_parser *const ical = icalendar;

restart_and_suspend:
	ical->state = ICAL_PARSER_STATE_STOPPED;
	k_thread_suspend(ical->tid);

	while (true) {
		__ASSERT(ical->offset < sizeof(ical->http_rx_buf), "Buffer overflow");

		LOG_DBG("Receiving bytes.. ical offset:%d", ical->offset);
		len = recv(ical->fd, ical->http_rx_buf + ical->offset,
			   sizeof(ical->http_rx_buf) - ical->offset, 0);

		LOG_DBG("%d bytes received", len);
		if (len == -1) {
			LOG_ERR("Error reading from socket, errno %d", errno);
			error_evt_send(ical, ENOTCONN);
			/* Restart and suspend */
			break;
		} else if (len == 0) {
			LOG_WRN("Peer closed connection!");
			error_evt_send(ical, ECONNRESET);
			/* Restart and suspend */
			break;
		}

		/* Accumulate buffer offset */
		ical->offset += len;

		/* Verify HTTP header */
		if (!ical->has_header) {
			rc = header_parse(ical);
			if (rc > 0) {
				/* Wait for complete header */
				continue;
			} else if (rc < 0) {
				/* Wrong header. Restart and suspend */
				LOG_WRN("Header parsing fail!");
				error_evt_send(ical, ENOMSG);
				break;
			} else {
				/* Correct header received. */
				ical->has_header = true;
			}
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
		ical->progress += MIN(ical->offset, len);

		if (ical->progress > 0) {
			size_t ret;
			ret = parse_icalstream(ical);
			if (ret > 0) {
				if (ical->icalobject_end) {
					/* Download complete. Report complete event. */
					LOG_DBG("Download complete");
					const struct icalendar_parser_evt evt = {
						.id = ICAL_PARSER_EVT_COMPLETE,
					};
					ical->callback(&evt, NULL);
					/* Restart and suspend */
					break;
				} else {
					/* Not finished yet. Copy unparsed buffer to the begining of buffer */
					memcpy(ical->http_rx_buf, ical->http_rx_buf + ret,
					       ical->offset - ret);
					/* Clear rest of HTTP buffer */
					memset(ical->http_rx_buf + (ical->offset - ret), 0, CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE - (ical->offset - ret));
					ical->offset = ical->offset - ret;
				}
			} else {
				if (ical->offset < CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE) {
					LOG_DBG("Awaiting full fragment (%u)", ical->offset);
					continue;
				} else {
					LOG_ERR("Get full fragment (%u) but parse fail. Increase rx buffer or check calendar format", ical->offset);
					error_evt_send(ical, ENOBUFS);
					break;
				}
			}
		}

		/* Attempt to reconnect if the connection was closed */
		if (ical->connection_close) {
			ical->connection_close = false;
			ical_parser_disconnect(ical);
			ical_parser_connect(ical, ical->host, ical->sec_tag);
		}

		/* Download next fragment */
		rc = get_request_send(ical);
		if (rc) {
			error_evt_send(ical, ECONNRESET);
			/* Restart and suspend */
			break;
		}
	}

	/* Do not let the thread return, since it can't be restarted */
	goto restart_and_suspend;
}

int ical_parser_init(struct icalendar_parser *const ical,
		     icalendar_parser_callback_t callback)
{
	if (ical == NULL || callback == NULL) {
		return -EINVAL;
	}

	ical->state = ICAL_PARSER_STATE_STOPPED;
	ical->fd = -1;
	ical->callback = callback;

	/* The thread is spawned now, but it will suspend itself;
	 * it is resumed when the download is started via the API.
	 */
	ical->tid =
		k_thread_create(&ical->thread, ical->thread_stack,
				K_THREAD_STACK_SIZEOF(ical->thread_stack),
				download_thread, ical, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}

int ical_parser_connect(struct icalendar_parser *ical, const char *host,
			int sec_tag)
{
	int err;

	if (ical == NULL || host == NULL) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_ICALENDAR_PARSER_TLS)) {
		LOG_INF("HTTP Connection");
		if (sec_tag != -1) {
			return -EINVAL;
		}
	} else {
		LOG_INF("HTTPS Connection");
		if (sec_tag == -1) {
			return -EINVAL;
		}
	}

	if (ical->state == ICAL_PARSER_STATE_DOWNLOADING) {
		return -EINVAL;
	} else {
		ical->state = ICAL_PARSER_STATE_DOWNLOADING;
	}

	if (ical->fd != -1) {
		/* Already connected */
		return 0;
	}

	/* Attempt IPv6 connection if configured, fallback to IPv4 */
	if (IS_ENABLED(CONFIG_ICALENDAR_PARSER_IPV6)) {
		ical->fd =
			resolve_and_connect(AF_INET6, host, sec_tag);
	}
	if (ical->fd < 0) {
		ical->fd =
			resolve_and_connect(AF_INET, host, sec_tag);
	}

	if (ical->fd < 0) {
		LOG_INF("Fail to resolve and connect");
		ical->state = ICAL_PARSER_STATE_STOPPED;
		return -EINVAL;
	}

	ical->host = host;
	ical->sec_tag = sec_tag;

	LOG_INF("Connected to %s", log_strdup(host));

	/* Set socket timeout, if configured */
	err = socket_timeout_set(ical->fd);
	if (err) {
		return err;
	}

	return 0;
}

int ical_parser_disconnect(struct icalendar_parser *const ical)
{
	int err;

	if (ical == NULL || ical->fd < 0) {
		return -EINVAL;
	}

	err = close(ical->fd);
	if (err) {
		LOG_ERR("Failed to close socket, errno %d", errno);
		return -errno;
	}

	ical->fd = -1;
	ical->state = ICAL_PARSER_STATE_STOPPED;

	return 0;
}

int ical_parser_start(struct icalendar_parser *ical, const char *file)
{
	int err;

	if (ical == NULL || file == NULL || (ical->fd < 0)) {
		ical->state = ICAL_PARSER_STATE_STOPPED;
		return -EINVAL;
	} else {
		ical->state = ICAL_PARSER_STATE_DOWNLOADING;
	}
	ical->file = file;

	ical->offset = 0;
	ical->progress = 0;
	ical->has_header = false;
	ical->icalobject_begin = false;
	ical->icalobject_end = false;

	memset(ical->http_rx_buf, 0, sizeof(ical->http_rx_buf));
	memset(ical->http_tx_buf, 0, sizeof(ical->http_tx_buf));

	LOG_INF("Downloading: %s [%u]", log_strdup(ical->file),
		ical->progress);

	err = get_request_send(ical);
	if (err) {
		ical->state = ICAL_PARSER_STATE_STOPPED;
		return err;
	}

	/* Let the receiving thread run */
	k_thread_resume(ical->tid);

	return 0;
}

