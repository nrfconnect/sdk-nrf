/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_STR_UTILS_H
#define MOSH_STR_UTILS_H

int str_hex_to_bytes(char *str, uint32_t str_length, uint8_t *buf, uint16_t buf_length);
char *mosh_strdup(const char *str);

#endif
