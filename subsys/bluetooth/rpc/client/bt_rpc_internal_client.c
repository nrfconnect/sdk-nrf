/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <nrf_rpc_cbor.h>

#include "bt_rpc_common.h"
#include "serialize.h"
#include "cbkproxy.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

bool bt_addr_le_is_bonded(uint8_t id, const bt_addr_le_t *addr)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;
	size_t buffer_size_max = 5;

	buffer_size_max += addr ? sizeof(bt_addr_le_t) : 0;

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, id);
	ser_encode_buffer(&ctx, addr, sizeof(bt_addr_le_t));
	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ADDR_LE_IS_BONDED_CMD,
				&ctx, ser_rsp_decode_bool, &result);

	return result;
}
