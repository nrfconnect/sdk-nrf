/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Notify common A-GNSS module that a request is in progress.
 *
 * @note Resets the "processed" data if in_progress is true.
 *
 * @param in_progress whether an A-GNSS request is in progress.
 */
void nrf_cloud_agnss_set_request_in_progress(bool in_progress);
