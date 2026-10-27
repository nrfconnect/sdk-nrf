/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <flatten_fake.h>

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC2(int, stream_flash_flatten_page, struct stream_flash_ctx *, off_t);
