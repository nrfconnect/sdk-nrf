/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/error.h>
#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <zephyr/logging/log.h>

NRF_RPC_GROUP_DECLARE(ot_group);
LOG_MODULE_DECLARE(ot_rpc, LOG_LEVEL_DBG);

static char version[256];
static otExtAddress mac = {.m8 = {0xF4, 0xCE, 0x36, 0x24, 0x95, 0x6A, 0xD1, 0xD0}};
static otExtendedPanId ext_pan_id;
static char thread_network_name[OT_NETWORK_NAME_MAX_SIZE + 1];
static otMeshLocalPrefix mesh_prefix;

static void get_uint_t(int id, void *result, size_t result_size)
{
	const size_t cbor_buffer_size = 0;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, id, &ctx);

	if (!zcbor_uint_decode(ctx.zs, result, result_size)) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		ot_rpc_report_decoding_error(id);
		return;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
}

static int get_string(int id, char *buffer, size_t buffer_size)
{
	const size_t cbor_buffer_size = 0;
	size_t bytes_copied = 0;
	struct zcbor_string zst;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, id, &ctx);

	if (!zcbor_bstr_decode(ctx.zs, &zst)) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		ot_rpc_report_decoding_error(id);
		goto exit;
	}

	if (buffer_size >= zst.len) {
		bytes_copied = zst.len;
	} else {
		bytes_copied = 0;
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		ot_rpc_report_decoding_error(id);
		goto exit;
	}

	memcpy(buffer, zst.value, bytes_copied);

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

exit:
	return bytes_copied;
}

const char *otGetVersionString(void)
{
	get_string(OT_RPC_CMD_GET_VERSION_STRING, version, sizeof(version));

	return version;
}

otError otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset)
{
	return 0;
}

uint8_t otLinkGetChannel(otInstance *aInstance)
{
	uint8_t ret;

	get_uint_t(OT_RPC_CMD_LINK_GET_CHANNEL, &ret, sizeof(ret));

	return ret;
}

const otMacCounters *otLinkGetCounters(otInstance *aInstance)
{
	return 0;
}

const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance)
{
	int ret_size = get_string(OT_RPC_CMD_LINK_GET_EXTENDED_ADDRESS, mac.m8, sizeof(mac.m8));

	if (ret_size != sizeof(mac.m8)) {
		LOG_ERR("Received mac addr size is too short");
		ot_rpc_report_decoding_error(OT_RPC_CMD_LINK_GET_EXTENDED_ADDRESS);
	}

	return &mac;
}

otPanId otLinkGetPanId(otInstance *aInstance)
{
	otPanId ret;

	get_uint_t(OT_RPC_CMD_LINK_GET_PAN_ID, &ret, sizeof(ret));

	return ret;
}

uint8_t otNetDataGetStableVersion(otInstance *aInstance)
{
	uint8_t ret;

	get_uint_t(OT_RPC_CMD_NET_DATA_GET_STABLE_VERSION, &ret, sizeof(ret));

	return ret;
}

uint8_t otNetDataGetVersion(otInstance *aInstance)
{
	uint8_t ret;

	get_uint_t(OT_RPC_CMD_NET_DATA_GET_VERSION, &ret, sizeof(ret));

	return ret;
}

const otExtendedPanId *otThreadGetExtendedPanId(otInstance *aInstance)
{
	get_string(OT_RPC_CMD_THREAD_GET_EXTENDED_PANID, ext_pan_id.m8, sizeof(ext_pan_id.m8));

	return &ext_pan_id;
}

uint8_t otThreadGetLeaderRouterId(otInstance *aInstance)
{
	uint8_t ret;

	get_uint_t(OT_RPC_CMD_THREAD_GET_LEADER_ROUTER_ID, &ret, sizeof(ret));

	return ret;
}

uint8_t otThreadGetLeaderWeight(otInstance *aInstance)
{
	uint8_t ret;

	get_uint_t(OT_RPC_CMD_THREAD_GET_LEADER_WEIGHT, &ret, sizeof(ret));

	return ret;
}

const otMeshLocalPrefix *otThreadGetMeshLocalPrefix(otInstance *aInstance)
{
	get_string(OT_RPC_CMD_THREAD_GET_MESH_LOCAL_PREFIX, mesh_prefix.m8, sizeof(mesh_prefix));

	return &mesh_prefix;
}

const otMleCounters *otThreadGetMleCounters(otInstance *aInstance)
{
	return 0;
}

const char *otThreadGetNetworkName(otInstance *aInstance)
{
	get_string(OT_RPC_CMD_THREAD_GET_NETWORK_NAME, thread_network_name,
		   sizeof(thread_network_name));

	return thread_network_name;
}

otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
	return 0;
}

uint32_t otThreadGetPartitionId(otInstance *aInstance)
{
	uint32_t ret;

	get_uint_t(OT_RPC_CMD_THREAD_GET_PARTITION_ID, &ret, sizeof(ret));

	return ret;
}
