/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SDFW_RECOVERY_SINK_H__
#define SDFW_RECOVERY_SINK_H__

#include <suit_sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the sdfw_recovery_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @return SUIT_PLAT_SUCCESS if success, error code otherwise
 */
suit_plat_err_t suit_sdfw_recovery_sink_get(struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* SDFW_RECOVERY_SINK_H__ */
