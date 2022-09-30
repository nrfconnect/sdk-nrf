/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>

#include <download_client.h>

#include "mock/dl_http.h"

int http_parse(struct download_client *client, size_t len)
{
	return 0;
}

int http_get_request_send(struct download_client *client)
{
	return 0;
}
