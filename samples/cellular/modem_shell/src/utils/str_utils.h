/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_STR_UTILS_H
#define MOSH_STR_UTILS_H

int str_hex_to_bytes(char *str, uint32_t str_length, uint8_t *buf, uint16_t buf_length);

/* strdup() C-library function implemented here because there's no such function in newlibc. */
char *mosh_strdup(const char *str);

/* Generates a human readable string of requested A-GNSS data flags into the given buffer. */
void agnss_data_flags_str_get(char *flags_string, uint32_t data_flags);

/* Returns the given GNSS system as a string. */
const char *gnss_system_str_get(uint8_t system_id);

#endif
