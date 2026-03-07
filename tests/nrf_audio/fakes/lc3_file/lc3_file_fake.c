/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lc3_file_fake.h"
#include "lc3_file_fake_data.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

DEFINE_FAKE_VALUE_FUNC(int, lc3_file_frame_get, struct lc3_file_ctx *, uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, lc3_file_open, struct lc3_file_ctx *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, lc3_file_close, struct lc3_file_ctx *);
DEFINE_FAKE_VALUE_FUNC(int, lc3_file_init);

int lc3_file_frame_get_fake_valid(struct lc3_file_ctx *ctx, uint8_t *buf, size_t size)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(size);

	memcpy(buf, fake_dataset1_valid, fake_dataset1_valid_size);

	return 0;
}

int lc3_file_frame_get_fake_enodata(struct lc3_file_ctx *ctx, uint8_t *buf, size_t size)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);

	return -ENODATA;
}
