/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_ENVELOPE_INFO_H__
#define SUIT_ENVELOPE_INFO_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>
#include <suit_memory_layout.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gets information about the stored candidate envelope.
 *
 * @param[out] address The address of the stored envelope.
 * @param[out] size The size of the stored envelope.
 *
 * @return suit_plat_success on success, error code otherwise.
 */
suit_plat_err_t suit_dfu_partition_envelope_info_get(const uint8_t **address, size_t *size);

/**
 * @brief Gets information about dfu partition characteristics.
 *
 * @return suit_plat_success on success, error code otherwise.
 */
suit_plat_err_t suit_dfu_partition_device_info_get(struct suit_nvm_device_info *device_info);

/**
 * @brief Chcecks if dfu partition is empty.
 *
 * @return true if empty, false otherwise
 */
bool suit_dfu_partition_is_empty(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_ENVELOPE_INFO_H__ */
