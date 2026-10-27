/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FLATTEN_FAKE_H__
#define FLATTEN_FAKE_H__

#include <zephyr/fff.h>
#include <zephyr/storage/stream_flash.h>

DECLARE_FAKE_VALUE_FUNC2(int, stream_flash_flatten_page, struct stream_flash_ctx *, off_t);

#endif /* FLATTEN_FAKE_H__ */
