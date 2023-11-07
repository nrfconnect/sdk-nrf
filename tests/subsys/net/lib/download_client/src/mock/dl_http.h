/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DL_HTTP_H_
#define _DL_HTTP_H_

#include <zephyr/kernel.h>

int http_parse(struct download_client *client, size_t len);
int http_get_request_send(struct download_client *client);

#endif /* _DL_HTTP_H_ */
