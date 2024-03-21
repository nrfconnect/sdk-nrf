/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

NRF_RPC_IPC_TRANSPORT(ot_group_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "ot_rpc_ept");
NRF_RPC_GROUP_DEFINE(ot_group, "ot", &ot_group_tr, NULL, NULL, NULL);
LOG_MODULE_REGISTER(ot_rpc, LOG_LEVEL_DBG);
