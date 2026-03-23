/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LC3_FILE_FAKE_H__
#define LC3_FILE_FAKE_H__

#include <zephyr/fff.h>
#include <zephyr/types.h>

DECLARE_FAKE_VALUE_FUNC(int, lc3_file_frame_get, struct lc3_file_ctx *, uint8_t *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, lc3_file_open, struct lc3_file_ctx *, const char *);
DECLARE_FAKE_VALUE_FUNC(int, lc3_file_close, struct lc3_file_ctx *);
DECLARE_FAKE_VALUE_FUNC(int, lc3_file_init);

/* List of fakes used by this unit tester */
#define DO_FOREACH_LC3_FILE_FAKE(FUNC)                                                             \
	do {                                                                                       \
		FUNC(lc3_file_frame_get)                                                           \
		FUNC(lc3_file_open)                                                                \
		FUNC(lc3_file_close)                                                               \
		FUNC(lc3_file_init)                                                                \
	} while (0)

#endif /* LC3_FILE_FAKE_H__ */

int lc3_file_frame_get_fake_valid(struct lc3_file_ctx *ctx, uint8_t *buf, size_t size);

int lc3_file_frame_get_fake_enodata(struct lc3_file_ctx *ctx, uint8_t *buf, size_t size);
