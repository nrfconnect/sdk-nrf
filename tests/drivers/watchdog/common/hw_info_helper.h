/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

void print_bar(void);
void get_supported_reset_cause(uint32_t *supported);
void get_current_reset_cause(uint32_t *cause);
void clear_reset_cause(void);
