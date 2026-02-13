/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

#include "dm_rpc_common.h"

NRF_RPC_IPC_TRANSPORT(dm_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "dm_rpc_ept");
NRF_RPC_GROUP_DEFINE(dm_rpc_grp, "dm_rpc_grp", &dm_rpc_tr, NULL, NULL, NULL);
