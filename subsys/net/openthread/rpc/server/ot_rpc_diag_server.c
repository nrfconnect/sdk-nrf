/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>

#include <nrf_rpc_cbor.h>
#include <string.h>

#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/netdata.h>
#include <zephyr/net/openthread.h>

NRF_RPC_GROUP_DECLARE(ot_group);

static size_t ret_size_str(const char *in)
{
	return strlen(in);
}

static size_t ret_size_ext_addr(const otExtAddress *in)
{
	return OT_EXT_ADDRESS_SIZE;
}

static size_t ret_size_ext_panid(const otExtendedPanId *in)
{
	return OT_EXT_PAN_ID_SIZE;
}

static size_t ret_size_mesh_local_prefix(const otMeshLocalPrefix *in)
{
	return OT_IP6_PREFIX_SIZE;
}

#define ret_size(X)                                                                               \
	_Generic((X),                                                                             \
		const char * : ret_size_str,                                                       \
		const otExtAddress * : ret_size_ext_addr,                                          \
		const otExtendedPanId * : ret_size_ext_panid,                                      \
		const otMeshLocalPrefix * : ret_size_mesh_local_prefix)(X)

static bool serialize_str(zcbor_state_t *state, const char *ser)
{
	size_t length = ret_size(ser);

	return zcbor_bstr_encode_ptr(state, ser, length);
}

static bool serialize_extaddr(zcbor_state_t *state, const otExtAddress *ser)
{
	size_t length = ret_size(ser);

	return zcbor_bstr_encode_ptr(state, ser->m8, length);
}

static bool serialize_ext_panid(zcbor_state_t *state, const otExtendedPanId *ser)
{
	size_t length = ret_size(ser);

	return zcbor_bstr_encode_ptr(state, ser->m8, length);
}

static bool serialize_mesh_local_prefix(zcbor_state_t *state, const otMeshLocalPrefix *ser)
{
	size_t length = ret_size(ser);

	return zcbor_bstr_encode_ptr(state, ser->m8, length);
}

#define serialize_bstr(Z, X)                                                                       \
	_Generic((X),                                                                              \
		const char * : serialize_str,                                                      \
		const otExtAddress * : serialize_extaddr,                                          \
		const otMeshLocalPrefix * : serialize_mesh_local_prefix,                           \
		const otExtendedPanId  *  : serialize_ext_panid)(Z, X)

#define NRF_RPC_CBOR_CMD_STRING_GETTER(func_name, func_to_call, call_id, type, ptr_call)           \
	static void func_name(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,     \
			      void *handler_data)                                                  \
	{                                                                                          \
		size_t length = 1;                                                                \
		const type *data;                                                                  \
		struct nrf_rpc_cbor_ctx rsp_ctx;                                                   \
                                                                                                   \
		nrf_rpc_cbor_decoding_done(group, ctx);                                            \
                                                                                                   \
		openthread_api_mutex_lock(openthread_get_default_context());                       \
		data = func_to_call(ptr_call);                                                     \
		openthread_api_mutex_unlock(openthread_get_default_context());                     \
                                                                                                   \
		length += ret_size(data);                                                          \
                                                                                                   \
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, length);                                        \
		serialize_bstr(rsp_ctx.zs, data);                                                  \
		nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);                                          \
	}                                                                                          \
	NRF_RPC_CBOR_CMD_DECODER(ot_group, func_name, call_id, func_name, NULL)

NRF_RPC_CBOR_CMD_STRING_GETTER(ot_get_version_string_getter, otGetVersionString,
			       OT_RPC_CMD_GET_VERSION_STRING, char,);

NRF_RPC_CBOR_CMD_STRING_GETTER(ot_get_extaddr, otLinkGetExtendedAddress,
			       OT_RPC_CMD_LINK_GET_EXTENDED_ADDRESS, otExtAddress,
			       openthread_get_default_instance());

NRF_RPC_CBOR_CMD_STRING_GETTER(ot_get_network_name, otThreadGetNetworkName,
			       OT_RPC_CMD_THREAD_GET_NETWORK_NAME, char,
			       openthread_get_default_instance());

NRF_RPC_CBOR_CMD_STRING_GETTER(ot_get_extpanid, otThreadGetExtendedPanId,
			       OT_RPC_CMD_THREAD_GET_EXTENDED_PANID, otExtendedPanId,
			       openthread_get_default_instance());

NRF_RPC_CBOR_CMD_STRING_GETTER(ot_get_local_prefix, otThreadGetMeshLocalPrefix,
			       OT_RPC_CMD_THREAD_GET_MESH_LOCAL_PREFIX, otMeshLocalPrefix,
			       openthread_get_default_instance());

#define NRF_RPC_CBOR_CMD_UINT_GETTER(func_name, func_to_call, call_id, type)                       \
	static void func_name(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,     \
			      void *handler_data)                                                  \
	{                                                                                          \
		size_t length = 1;                                                                \
		type data;                                                                         \
		struct nrf_rpc_cbor_ctx rsp_ctx;                                                   \
                                                                                                   \
		nrf_rpc_cbor_decoding_done(group, ctx);                                            \
                                                                                                   \
		openthread_api_mutex_lock(openthread_get_default_context());                       \
		data = func_to_call(openthread_get_default_instance());                   \
		openthread_api_mutex_unlock(openthread_get_default_context());                     \
                                                                                                   \
		length += sizeof(data);                                                            \
                                                                                                   \
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, length);                                        \
		zcbor_uint_encode(rsp_ctx.zs, (const void *)&data, sizeof(data));                  \
		nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);                                          \
	}                                                                                          \
	NRF_RPC_CBOR_CMD_DECODER(ot_group, func_name, call_id, func_name, NULL)

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_channel, otLinkGetChannel, OT_RPC_CMD_LINK_GET_CHANNEL,
			     otPanId);

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_panid, otLinkGetPanId, OT_RPC_CMD_LINK_GET_PAN_ID,
			     otPanId);

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_netdata_stable_version, otNetDataGetStableVersion,
			     OT_RPC_CMD_NET_DATA_GET_STABLE_VERSION, uint8_t);

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_net_data_version, otNetDataGetVersion,
			     OT_RPC_CMD_NET_DATA_GET_VERSION, uint8_t);

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_leader_id, otThreadGetLeaderRouterId,
			     OT_RPC_CMD_THREAD_GET_LEADER_ROUTER_ID, uint8_t);

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_leader_weight, otThreadGetLeaderWeight,
			     OT_RPC_CMD_THREAD_GET_LEADER_WEIGHT, uint8_t);

NRF_RPC_CBOR_CMD_UINT_GETTER(ot_link_get_partition_id, otThreadGetPartitionId,
			     OT_RPC_CMD_THREAD_GET_PARTITION_ID, uint32_t);
