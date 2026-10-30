/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SM_XDFU_H_
#define SM_XDFU_H_

#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

/**
 * Transfer a signed nRF91 application from a file range using AT#XDFU.
 *
 * The caller must suspend the cellular driver and stop networking first.
 * Returns after scheduling the image, resetting the modem, and receiving Ready.
 * The new application is responsible for confirming its MCUboot image.
 *
 * @return 0 on success, negative errno on failure.
 */
int sm_xdfu_app_run(const struct device *modem, const char *file, off_t offset, size_t length);

#endif