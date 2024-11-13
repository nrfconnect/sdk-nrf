/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DL_PARSE_H
#define DL_PARSE_H

#include <sys/types.h>

int dl_parse_url_port(const char *url, uint16_t *port);
int dl_parse_url_host(const char *url, char *host, size_t len);
int dl_parse_url_file(const char *url, char *file, size_t len);

#endif /* DL_PARSE_H */
