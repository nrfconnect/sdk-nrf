/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_RX_H
#define DECT_MDM_RX_H

#include <zephyr/kernel.h>

#define DECT_MDM_RX_OP_RX_DATA_WITH_PKT_PTR 1

/**
 * @brief Add RX operation to message queue for processing.
 * @param event_id Event type identifier (DECT_MDM_RX_OP_*).
 * @param data Pointer to event data (must not be NULL).
 * @param data_size Size of event data (must be > 0).
 * @return 0 on success, -EINVAL for invalid params, -ENOMEM for allocation failure,
 *         -ENOBUFS if message queue is full.
 */
int dect_mdm_rx_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size);

#endif /* DECT_MDM_RX_H */
