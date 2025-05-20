/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_MEMPTR_SIZE_UPDATE_H__
#define SUIT_PLAT_MEMPTR_SIZE_UPDATE_H__

#include <suit_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update size in memptr attached to supplied handle
 *
 * @param handle Handle to component with attached memptr
 * @param size New size to be set in memptr
 * @return int SUIT_SUCCESS in case of success, otherwise error code
 */
int suit_plat_memptr_size_update(suit_component_t handle, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_MEMPTR_SIZE_UPDATE_H__ */
