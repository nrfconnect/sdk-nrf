/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief IPC-only device: rpu_dev for nRF71 Wi-Fi on IPC.
 */

#include <zephyr/kernel.h>

#include "ipc_if.h"

static struct rpu_dev ipc = {
	.init = ipc_init,
	.deinit = ipc_deinit,
	.send = ipc_send,
	.recv = ipc_recv,
	.register_rx_cb = ipc_register_rx_cb,
};

struct rpu_dev *rpu_dev(void)
{
	return &ipc;
}
