/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_FW_LOADER_SETTINGS_H_
#define DFU_FW_LOADER_SETTINGS_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup fw_loader_settings Firmware loader settings
 * @{
 */

/**
 * @brief Load the firmware loader advertising name from Settings storage.
 *
 * Reads the fw_loader/adv_name key directly from persistent storage.
 *
 * @param buf      Buffer for the NUL-terminated name.
 * @param buf_len  Size of @p buf.
 *
 * @return 0 on success, -EINVAL if @p buf_len is zero, -ENOENT if the key
 *         is not stored.
 */
int fw_loader_settings_adv_name_load(char *buf, size_t buf_len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DFU_FW_LOADER_SETTINGS_H_ */
