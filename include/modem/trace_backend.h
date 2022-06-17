/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file trace_backend.h
 *
 * @defgroup trace_backend nRF91 Modem trace backend interface.
 * @{
 */
#ifndef TRACE_BACKEND_H__
#define TRACE_BACKEND_H__

/**
 * @brief Initialize the compile-time selected trace backend.
 *
 * @return 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int trace_backend_init(void);

/**
 * @brief Deinitialize the compile-time selected trace backend.
 *
 * @return 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int trace_backend_deinit(void);

/**
 * @brief Write trace data to the compile-time selected trace backend.
 *
 * @param data Memory buffer containing modem trace data.
 * @param len  Memory buffer length.
 *
 * @return Number of bytes written if the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int trace_backend_write(const void *data, size_t len);

#endif /* TRACE_BACKEND_H__ */
/**@} */
