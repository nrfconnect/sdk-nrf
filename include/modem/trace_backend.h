/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TRACE_BACKEND_H__
#define TRACE_BACKEND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file trace_backend.h
 *
 * @defgroup trace_backend nRF91 Modem trace backend interface.
 * @{
 */

/** @brief callback to signal the trace module that
 * some amount of trace data has been processed.
 */
typedef int (*trace_backend_processed_cb)(size_t len);

/**
 * @brief Initialize the compile-time selected trace backend.
 *
 * @param trace_processed_cb Function callback for signaling how much of the trace data has been
 *			     processed by the backend.
 *
 * @return 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int trace_backend_init(trace_backend_processed_cb trace_processed_cb);

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
 * @returns Number of bytes written if the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int trace_backend_write(const void *data, size_t len);

/**@} */

#ifdef __cplusplus
}
#endif

#endif /* TRACE_BACKEND_H__ */
