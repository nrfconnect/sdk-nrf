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

enum sm_xdfu_image_type {
	SM_XDFU_TYPE_APP = 0,
	SM_XDFU_TYPE_DELTA_MFW = 1,
	SM_XDFU_TYPE_FULL_MFW = 2,
};

/**
 * Transfer an nRF91 update from a file range using AT#XDFU.
 *
 * The caller must suspend the cellular driver and stop networking first.
 * Application and delta updates are scheduled and followed by a modem reset.
 * Full-modem updates decode a Nordic CBOR package, verify its firmware blob,
 * enter bootloader mode, and write addressed segments. Final apply reboots the
 * modem automatically. Returns after receiving Ready on success.
 * A failed full update may require a complete retry before the modem is usable.
 * A new application is responsible for confirming its MCUboot image.
 *
 * @return 0 on success, negative errno on failure.
 */
int sm_xdfu_run(const struct device *modem, enum sm_xdfu_image_type type,
	       const char *file, off_t offset, size_t length);

/** Query the Serial Modem application version using AT#XSMVER.
 *
 * The modem cellular driver and UART must be suspended by the caller.
 */
int sm_xdfu_get_slm_version(const struct device *modem, char *version, size_t version_size);

#endif