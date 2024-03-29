/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SINK_SELECTOR_H__
#define SINK_SELECTOR_H__

#include <suit_sink.h>
#include <suit_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get sink based on component handle
 *
 * @param dst_handle Component handle
 * @param sink Pointer to sink structure to be filled
 * @return int 0 in case of success, otherwise error code
 */
int suit_sink_select(suit_component_t dst_handle, struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* SINK_SELECTOR_H__ */
