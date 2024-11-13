/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdbool.h>
#include <zephyr/net/socket.h>
#include <zephyr/types.h>
#include <errno.h>

#include "dl_parse.h"

/*
 * Trim the string to point to the first character after the substring.
 * Returns 0 on success.
 */
static int trim_to_after(const char **str, const char *substr)
{
	const char *p;

	p = strstr(*str, substr);
	if (!p) {
		return -EINVAL;
	}

	*str = p + strlen(substr);
	return 0;
}

int dl_parse_url_host(const char *url, char *host, size_t len)
{
	const char *cur;
	const char *end;

	cur = url;

	(void)trim_to_after(&cur, "://");

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

int dl_parse_url_port(const char *url, uint16_t *port)
{
	int err;
	const char *cur;
	const char *end;
	char aport[8];
	size_t len;

	cur = url;

	(void)trim_to_after(&cur, "://");

	if (cur[0] == '[') {
		/* literal IPv6 address */
		(void)trim_to_after(&cur, "]");
	}

	err = trim_to_after(&cur, ":");
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

int dl_parse_url_file(const char *url, char *file, size_t len)
{
	int err;
	const char *cur;

	cur = url;

	if (strstr(url, "//")) {
		err = trim_to_after(&cur, "://");
		if (err) {
			return -EINVAL;
		}
	}

	err = trim_to_after(&cur, "/");
	if (err) {
		return -EINVAL;
	}

	if (strlen(cur) + 1 > len) {
		return -E2BIG;
	}

	len = strlen(cur);

	memcpy(file, cur, len);
	file[len] = '\0';

	return 0;
}
