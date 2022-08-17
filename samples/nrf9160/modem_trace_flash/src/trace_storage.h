/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>

/**
 * @brief Initialize trace storage
 *
 * @details Initialize the flash device and erase the flash area.
 *
 * @return 0 on success, negative errno code on fail
 */
int trace_storage_init(void);

/**
 * @brief Read trace data from flash storage
 *
 * @details Read out the trace data from flash memory. It is required to call trace_storage_flush()
 * first.
 *
 * @param buf Output buffer
 * @param len Size of output buffer
 * @param offset Read offset
 *
 * @return 0 on success, negative errno code on fail
 */
int trace_storage_read(uint8_t *buf, size_t len, size_t offset);

/**
 * @brief Write trace data to flash storage
 *
 * @param buf Buffer to write
 * @param len Size of data in the buffer
 *
 * @return 0 on success, negative errno on fail
 */
int trace_storage_write(const void *buf, size_t len);

/**
 * @brief Flush traces to flash storage
 *
 * @details Forces write of the remaining block buffer to flash. Call this function after modem
 * traces has been turned off and no more trace data is expected.
 *
 * @return 0 on success, negative errno code on fail
 */
int trace_storage_flush(void);
