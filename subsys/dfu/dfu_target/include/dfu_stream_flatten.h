/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/storage/stream_flash.h>

/**
 * @brief Flatten/erase the flash/non-flash storage device page to which a given offset belongs.
 *
 * This function uses the flatten function on supported non-flash memory devices or erase function
 * on flash devices to erase/flatten a flash/non-flash page to which an offset belongs.
 *
 * @param ctx context
 * @param off offset from the base address of the non-flash storage device
 *
 * @return 0 on success
 * @return Negative errno code on error
 */
int stream_flash_flatten_page(struct stream_flash_ctx *ctx, off_t off);
