/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file http_client.h
 *
 * @defgroup http_client Simple HTTP client.
 * @{
 * @brief Simple HTTP Client.
 */

#ifndef HTTP_CLIENT_H__
#define HTTP_CLIENT_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int httpc_connect(const char *host, const char *port, u32_t family, u32_t proto);

void httpc_disconnect(int fd);

int httpc_request(int fd, char *req, u32_t len);

int httpc_recv(int fd, char *buf, u32_t len, u32_t flags);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_H__

/**@} */
