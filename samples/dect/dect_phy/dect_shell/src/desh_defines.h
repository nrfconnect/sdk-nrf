/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DESH_DEFINES_H
#define DESH_DEFINES_H

#define DESH_EMPTY_STRING "\0"

#define DESH_STRING_NULL_CHECK(string) \
	((string != NULL) ? string : DESH_EMPTY_STRING)

#define DESH_AT_CMD_RESPONSE_MAX_LEN 2701

#endif /* DESH_DEFINES_H */
