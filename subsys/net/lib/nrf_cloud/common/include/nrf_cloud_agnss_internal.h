/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_AGNSS_INTERNAL_H_
#define NRF_CLOUD_AGNSS_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "nrf_cloud_agnss_schema_v1.h"

/**
 * @brief Notify common A-GNSS module that a request is in progress.
 *
 * @note Resets the "processed" data if in_progress is true.
 *
 * @param in_progress whether an A-GNSS request is in progress.
 */
void nrf_cloud_agnss_set_request_in_progress(bool in_progress);

/**
 * @brief Debug-print one decoded A-GNSS element in modem format.
 *
 * @param type Modem A-GNSS data type.
 * @param data Pointer to the modem-format payload for @p type.
 */
void agnss_print(uint16_t type, void *data);

/**
 * @brief Callback used by parse_agnss_block().
 *
 * @param e Parsed A-GNSS element descriptor and payload pointer.
 *
 * @retval 0 Continue parsing.
 * @retval negative Abort parsing and propagate error.
 */
typedef int (*agnss_block_cb_t)(const struct nrf_cloud_agnss_element *e);

/**
 * @brief Parse binary A-GNSS elements and invoke a callback for each element.
 *
 * The input must begin at the element block header (type/count) and may contain
 * one or more elements.
 *
 * @param buf Pointer to encoded A-GNSS element block.
 * @param buf_len Length of @p buf in bytes.
 * @param cb Callback invoked once per parsed element, return non-zero to abort.
 *
 * @retval 0 Success.
 * @retval -ENOENT Truncated or unsupported data.
 * @retval -EIO if aborted by @p cb.
 * @retval -ERANGE if element type is out of range.
 */
int parse_agnss_block(const char *buf, size_t buf_len, agnss_block_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_AGNSS_INTERNAL_H_ */
