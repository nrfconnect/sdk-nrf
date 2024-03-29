/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMPTR_SINK_H__
#define MEMPTR_SINK_H__

#include <suit_sink.h>
#include <suit_memptr_storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the memptr sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param handle Handle to storage object
 * @return SUIT_PLAT_SUCCESS if success otherwise error code.
 */
suit_plat_err_t suit_memptr_sink_get(struct stream_sink *sink, memptr_storage_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* MEMPTR_SINK_H__ */
