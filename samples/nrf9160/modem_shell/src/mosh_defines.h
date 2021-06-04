/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_DEFINES_H
#define MOSH_DEFINES_H

#define MOSH_EMPTY_STRING "\0"

#define MOSH_APN_STR_MAX_LEN (64)
#define MOSH_ARG_NOT_SET -6

#define MOSH_STRING_NULL_CHECK(string) \
	((string != NULL) ? string : MOSH_EMPTY_STRING)

#define MOSH_AT_CMD_RESPONSE_MAX_LEN 2700

#endif /* MOSH_DEFINES_H */
