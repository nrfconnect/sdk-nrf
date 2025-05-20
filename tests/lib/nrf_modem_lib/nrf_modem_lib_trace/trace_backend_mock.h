/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <modem/trace_backend.h>

int trace_backend_init(trace_backend_processed_cb trace_processed_cb);

int trace_backend_deinit(void);

int trace_backend_write(const void *data, size_t len);

size_t trace_backend_data_size(void);

int trace_backend_read(void *buf, size_t len);

int trace_backend_clear(void);
