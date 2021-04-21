/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <modem/at_cmd.h>
#include <modem/modem_jwt.h>

#define JWT_CMD_TEMPLATE "AT%%JWT=%d,%u,\"%s\",\"%s\",%d,%d"
#define MAX_INT_PRINT_SZ 11
#define JWT_CMD_PRINT_INT_SZ (MAX_INT_PRINT_SZ * 4)

/* Minimum JWT response is ~420 bytes.
 * With very long (128 bytes) values for sub and aud claims
 * response is ~820.
 */
BUILD_ASSERT(CONFIG_MODEM_JWT_MAX_LEN <= CONFIG_AT_CMD_RESPONSE_MAX_LEN,
	     "Max JWT length exceeds max AT cmd response length");

static int parse_jwt_at_cmd_resp(char *const jwt_resp)
{
	if (!jwt_resp) {
		return -EINVAL;
	}

	char *jwt_start = NULL;
	char *jwt_end = NULL;
	size_t jwt_len;

	/* Response format is:
	 *	%JWT: "<base64_JWT>"\r\n
	 */

	/* A quotation mark indicates the start of the JWT string */
	jwt_start = strchr(jwt_resp, '"');
	if (!jwt_start) {
		return -EBADMSG;
	}

	/* Move beyond first quotation mark and find second */
	jwt_end = strchr(++jwt_start, '"');
	if (!jwt_end) {
		return -EBADMSG;
	}

	*jwt_end = 0;
	jwt_len = strlen(jwt_start) + 1;

	/* Move the now null-terminated jwt to the front of the buffer */
	memmove(jwt_resp, jwt_start, jwt_len);

	return 0;
}

int modem_jwt_generate(struct jwt_data *const jwt)
{
	if (!jwt) {
		return -EINVAL;
	} else if ((jwt->jwt_buf) && (jwt->jwt_sz == 0)) {
		return -EMSGSIZE;
	}

	int ret = 0;
	enum at_cmd_state state;
	char *cmd;
	char *cmd_resp = NULL;
	size_t cmd_sz = sizeof(JWT_CMD_TEMPLATE) + JWT_CMD_PRINT_INT_SZ +
			(jwt->subject ? strlen(jwt->subject) : 0) +
			(jwt->audience ? strlen(jwt->audience) : 0);

	ret = at_cmd_init();
	if (ret) {
		return -EPIPE;
	}

	/* Allocate and format the JWT request cmd */
	cmd = k_calloc(cmd_sz, 1);
	if (!cmd) {
		return -ENOMEM;
	}

	ret = snprintf(cmd, cmd_sz, JWT_CMD_TEMPLATE,
		       jwt->alg,
		       jwt->exp_delta_s,
		       jwt->subject ? jwt->subject : "",
		       jwt->audience ? jwt->audience : "",
		       jwt->sec_tag,
		       jwt->key);

	if ((ret < 0) || (ret >= cmd_sz)) {
		ret = -EIO;
		goto cleanup;
	}

	/* Allocate response buffer and send cmd */
	cmd_resp = k_calloc(CONFIG_MODEM_JWT_MAX_LEN, 1);
	if (!cmd_resp) {
		ret = -ENOMEM;
		goto cleanup;
	}

	ret = at_cmd_write(cmd, cmd_resp, CONFIG_MODEM_JWT_MAX_LEN, &state);
	if (ret) {
		ret = -EBADMSG;
		goto cleanup;
	}

	ret = parse_jwt_at_cmd_resp(cmd_resp);
	if (ret) {
		ret = -ENOMSG;
		goto cleanup;
	}

cleanup:

	if (cmd) {
		k_free(cmd);
	}

	if (ret == 0) {
		size_t jwt_sz = strlen(cmd_resp) + 1;

		if (!jwt->jwt_buf) {
			jwt->jwt_buf = k_calloc(jwt_sz, 1);
			if (!jwt->jwt_buf) {
				ret = -ENOMEM;
			} else {
				jwt->jwt_sz = jwt_sz;
			}
		} else if (jwt_sz > jwt->jwt_sz) {
			ret = -E2BIG;
		}

		if (ret == 0) {
			memcpy(jwt->jwt_buf, cmd_resp, jwt_sz);
		}
	}

	if (cmd_resp) {
		k_free(cmd_resp);
	}

	return ret;
}

void modem_jwt_free(char *const jwt_buf)
{
	if (jwt_buf) {
		k_free(jwt_buf);
	}
}
