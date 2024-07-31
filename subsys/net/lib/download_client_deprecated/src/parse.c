/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdbool.h>
#include <zephyr/net/socket.h>
#include <zephyr/types.h>
#include <errno.h>

static int swallow(const char **str, const char *swallow)
{
	const char *p;

	p = strstr(*str, swallow);
	if (!p) {
		return 1;
	}

	*str = p + strlen(swallow);
	return 0;
}

int url_parse_proto(const char *url, int *proto, int *type)
{
	if (strncmp(url, "https://", 8) == 0) {
		*proto = IPPROTO_TLS_1_2;
		*type = SOCK_STREAM;
	} else if (strncmp(url, "http://", 7) == 0) {
		*proto = IPPROTO_TCP;
		*type = SOCK_STREAM;
	} else if (strncmp(url, "coaps://", 8) == 0) {
		*proto = IPPROTO_DTLS_1_2;
		*type = SOCK_DGRAM;
	} else if (strncmp(url, "coap://", 7) == 0) {
		*proto = IPPROTO_UDP;
		*type = SOCK_DGRAM;
	} else if (strstr(url, "://")) {
		return -EPROTONOSUPPORT;
	} else {
		return -EINVAL;
	}
	return 0;
}

int url_parse_host(const char *url, char *host, size_t len)
{
	const char *cur;
	const char *end;

	cur = url;

	(void)swallow(&cur, "://");

	if (cur[0] == '[') {
		/* literal IPv6 address */
		end = strchr(cur, ']');

		if (!end) {
			return -EINVAL;
		}
		++end;
	} else {
		end = strchr(cur, ':');
		if (!end) {
			end = strchr(cur, '/');
			if (!end) {
				end = url + strlen(url) + 1;
			}
		}
	}

	if (end - cur + 1 > len) {
		return -E2BIG;
	}

	len = end - cur;

	memcpy(host, cur, len);
	host[len] = '\0';

	return 0;
}

int url_parse_port(const char *url, uint16_t *port)
{
	int err;
	const char *cur;
	const char *end;
	char aport[8];
	size_t len;

	cur = url;

	(void)swallow(&cur, "://");

	if (cur[0] == '[') {
		/* literal IPv6 address */
		swallow(&cur, "]");
	}

	err = swallow(&cur, ":");
	if (err) {
		return -EINVAL;
	}

	end = strchr(cur, '/');
	if (!end) {
		len = strlen(cur);
	} else {
		len = end - cur;
	}

	len = MIN(len, sizeof(aport) - 1);

	memcpy(aport, cur, len);
	aport[len] = '\0';

	*port = atoi(aport);

	return 0;
}

int url_parse_file(const char *url, char *file, size_t len)
{
	int err;
	const char *cur;

	cur = url;

	if (strstr(url, "//")) {
		err = swallow(&cur, "://");
		if (err) {
			return -EINVAL;
		}
		err = swallow(&cur, "/");
		if (err) {
			return -EINVAL;
		}
	}

	if (strlen(cur) + 1 > len) {
		return -E2BIG;
	}

	len = strlen(cur);

	memcpy(file, cur, len);
	file[len] = '\0';

	return 0;
}
