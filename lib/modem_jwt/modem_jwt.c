/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <nrf_modem_at.h>
#include <modem/modem_jwt.h>
#include <zephyr/sys/base64.h>

#define JWT_CMD_TEMPLATE "AT%%JWT=%d,%u,\"%s\",\"%s\""
#define JWT_CMD_TEMPLATE_SEC_TAG "AT%%JWT=%d,%u,\"%s\",\"%s\",%u,%d"
#define MAX_INT_PRINT_SZ 11
#define JWT_CMD_PRINT_INT_SZ (MAX_INT_PRINT_SZ * 4)
#define GET_BASE64_DECODED_LEN(n) (3 * n / 4)

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

static void base64_url_format(char *const base64_string)
{
	if (base64_string == NULL) {
		return;
	}

	char *found = NULL;

	/* replace '+' with "-" */
	for (found = base64_string; (found = strchr(found, '+'));) {
		*found = '-';
	}

	/* replace '/' with "_" */
	for (found = base64_string; (found = strchr(found, '/'));) {
		*found = '_';
	}

	/* remove padding '=' */
	found = strchr(base64_string, '=');
	if (found) {
		*found = '\0';
	}
}

int modem_jwt_generate(struct jwt_data *const jwt)
{
	if (!jwt) {
		return -EINVAL;
	} else if ((jwt->jwt_buf) && (jwt->jwt_sz == 0)) {
		return -EMSGSIZE;
	}

	int ret;
	char *cmd_resp;


	/* Allocate response buffer */
	cmd_resp = k_calloc(CONFIG_MODEM_JWT_MAX_LEN, 1);
	if (!cmd_resp) {
		return -ENOMEM;
	}

	if (jwt->sec_tag) {
		ret = nrf_modem_at_cmd(cmd_resp, CONFIG_MODEM_JWT_MAX_LEN,
			JWT_CMD_TEMPLATE_SEC_TAG,
			jwt->alg,
			jwt->exp_delta_s,
			jwt->subject ? jwt->subject : "",
			jwt->audience ? jwt->audience : "",
			jwt->sec_tag,
			jwt->key);
	} else {
		ret = nrf_modem_at_cmd(cmd_resp, CONFIG_MODEM_JWT_MAX_LEN,
			JWT_CMD_TEMPLATE,
			jwt->alg,
			jwt->exp_delta_s,
			jwt->subject ? jwt->subject : "",
			jwt->audience ? jwt->audience : "");
	}

	if (ret) {
		/* when positive, the response was different from "OK" */
		if (ret > 0) {
			ret = -ENOEXEC;
		}
		goto cleanup;
	}

	ret = parse_jwt_at_cmd_resp(cmd_resp);

cleanup:
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
			/* Remove any non-base64url characters */
			base64_url_format(cmd_resp);
			memcpy(jwt->jwt_buf, cmd_resp, jwt_sz);
		}
	}

	if (cmd_resp) {
		k_free(cmd_resp);
	}

	return ret;
}

static int copy_uuid_str(char *dst, const char *src, const char *field)
{
	char *tag;
	char *end;
	char *dot;
	char *uuid;
	size_t len;

	/* nRF modem generated tokens have two fixed fields that contain what we are looking for
	 * "iss":"<chip>.<UUID>"  This is device UUID
	 * "jti":"<chip>.<UUID>.<JTI data>" This is FW UUID
	 * So I need to seek a first dot after the correct tag is found.
	 */
	tag = strstr(src, field);
	if (!tag) {
		return -EINVAL;
	}

	tag += strlen(field);
	end = strchr(tag, '"');
	dot = strchr(tag, '.');
	if (!end || !dot || dot > end) {
		return -EINVAL;
	}

	uuid = dot + 1;
	len = end - uuid;
	if (len < NRF_UUID_V4_STR_LEN) {
		return -EINVAL;
	}

	strncpy(dst, uuid, NRF_UUID_V4_STR_LEN);
	dst[NRF_UUID_V4_STR_LEN] = 0;
	return 0;
}

int modem_jwt_get_uuids(struct nrf_device_uuid *dev,
			struct nrf_modem_fw_uuid *mfw)
{
	struct jwt_data jwt = {0};
	char *payload = NULL;
	char *start, *end;
	size_t slen, len;

	int rc = modem_jwt_generate(&jwt);

	if (rc) {
		goto end;
	}

	start = strchr(jwt.jwt_buf, '.') + 1;
	end = strrchr(jwt.jwt_buf, '.');
	slen = end - start;
	payload =  k_calloc(1, GET_BASE64_DECODED_LEN(slen));
	if (!payload) {
		rc = -ENOMEM;
		goto end;
	}

	rc = base64_decode(payload, GET_BASE64_DECODED_LEN(slen), &len, start, slen);
	if (rc) {
		goto end;
	} else if (len == 0 || len > GET_BASE64_DECODED_LEN(slen)) {
		rc = -EINVAL;
		goto end;
	}

	if (dev && !rc) {
		rc = copy_uuid_str(dev->str, payload, "iss\":\"");
	}
	if (mfw && !rc) {
		rc = copy_uuid_str(mfw->str, payload, "jti\":\"");
	}

end:
	k_free(payload);
	modem_jwt_free(jwt.jwt_buf);
	return rc;
}

void modem_jwt_free(char *const jwt_buf)
{
	if (jwt_buf) {
		k_free(jwt_buf);
	}
}
