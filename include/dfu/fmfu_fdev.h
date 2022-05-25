/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FMFU_FDEV_H__
#define FMFU_FDEV_H__

#include <zephyr/device.h>

/**
 * @defgroup fmfu_fdev Full Modem Firmware Update from flash device
 * @{
 *
 * @brief Functions for applying a full modem firmware update from
 *        flash device.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load the modem firmware update from the specified flash device to the
 *        modem.
 *
 * The modem library must be initialized in DFU mode before calling this
 * function.
 *
 * @param[in] buf Pointer to buffer used to read data from external flash.
 * @param[in] buf_len Length of provided buffer.
 * @param[in] fdev Flash device to read modem firmware from.
 * @param[in] offset Offset within configured flash device to first byte of
 *                   modem update data.
 *
 * @return non-negative on success, negative error code on failure.
 */
int fmfu_fdev_load(uint8_t *buf, size_t buf_len,
		   const struct device *fdev, size_t offset);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* FMFU_FDEV_H__ */
