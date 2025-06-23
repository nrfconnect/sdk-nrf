/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/openthread.h>
#include <zephyr/sys/byteorder.h>

#include <openthread/link.h>

#ifdef CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE
#ifdef CONFIG_NRFX_RRAMC
#include <nrfx_rramc.h>
#endif
#endif

static void ot_rpc_cmd_set_poll_period(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t poll_period;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	poll_period = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_LINK_SET_POLL_PERIOD);
		return;
	}

	ot_rpc_mutex_lock();
	error = otLinkSetPollPeriod(openthread_get_default_instance(), poll_period);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_poll_period(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t poll_period;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	poll_period = otLinkGetPollPeriod(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	nrf_rpc_encode_uint(&rsp_ctx, poll_period);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_set_max_frame_retries_direct(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	uint8_t max_retries = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT);
		return;
	}

	ot_rpc_mutex_lock();
	otLinkSetMaxFrameRetriesDirect(openthread_get_default_instance(), max_retries);
	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_cmd_set_max_frame_retries_indirect(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	uint8_t max_retries = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT);
		return;
	}

	ot_rpc_mutex_lock();
	otLinkSetMaxFrameRetriesIndirect(openthread_get_default_instance(), max_retries);
	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_cmd_set_link_enabled(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	bool enabled = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_LINK_SET_ENABLED);
		return;
	}

	ot_rpc_mutex_lock();
	error = otLinkSetEnabled(openthread_get_default_instance(), enabled);
	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_get_factory_assigned_eui64(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otExtAddress ext_addr;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	otLinkGetFactoryAssignedIeeeEui64(openthread_get_default_instance(), &ext_addr);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(ext_addr.m8) + 1);
	nrf_rpc_encode_buffer(&rsp_ctx, &ext_addr, sizeof(ext_addr.m8));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_counters(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otMacCounters *counters;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	counters = otLinkGetCounters(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx,
			   sizeof(*counters) / sizeof(uint32_t) * (1 + sizeof(uint32_t)));
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxTotal);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxUnicast);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxBroadcast);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxAckRequested);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxAcked);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxNoAckRequested);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxData);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxDataPoll);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxBeacon);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxBeaconRequest);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxOther);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxRetry);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxDirectMaxRetryExpiry);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxIndirectMaxRetryExpiry);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxErrCca);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxErrAbort);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mTxErrBusyChannel);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxTotal);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxUnicast);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxBroadcast);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxData);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxDataPoll);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxBeacon);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxBeaconRequest);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxOther);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxAddressFiltered);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxDestAddrFiltered);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxDuplicated);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxErrNoFrame);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxErrUnknownNeighbor);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxErrInvalidSrcAddr);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxErrSec);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxErrFcs);
	nrf_rpc_encode_uint(&rsp_ctx, counters->mRxErrOther);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_set_factory_assigned_eui64(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error = OT_ERROR_NONE;
	otExtAddress ext_addr;

	nrf_rpc_decode_buffer(ctx, ext_addr.m8, OT_EXT_ADDRESS_SIZE);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_LINK_SET_FACTORY_ASSIGNED_EUI64);
		return;
	}

#ifdef CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE
#ifdef CONFIG_NRFX_RRAMC
	if (nrfx_rramc_otp_word_write(CONFIG_IEEE802154_NRF5_UICR_EUI64_REG + 1,
				      sys_get_le32(ext_addr.m8)) &&
	    nrfx_rramc_otp_word_write(CONFIG_IEEE802154_NRF5_UICR_EUI64_REG,
				      sys_get_le32(ext_addr.m8 + 4))) {
		error = OT_ERROR_NONE;
	} else {
		error = OT_ERROR_FAILED;
	}
#else
	error = OT_ERROR_NOT_CAPABLE;
#endif
#endif

	/*
	 * Update the network interface regardless of whether EUI64 comes from UICR of not.
	 * Even if it does, it is only read from UICR once, during the network interface
	 * initialization, and we want the EUI64 change to have an immediate effect.
	 */
	if (IS_ENABLED(CONFIG_NET_L2_OPENTHREAD) && error == OT_ERROR_NONE) {
		struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
		struct net_linkaddr *addr;

		__ASSERT_NO_MSG(iface != NULL);

		net_if_lock(iface);
		addr = net_if_get_link_addr(iface);

		__ASSERT_NO_MSG(addr != NULL && addr->len == OT_EXT_ADDRESS_SIZE);

		memcpy(addr->addr, ext_addr.m8, OT_EXT_ADDRESS_SIZE);
		net_if_unlock(iface);
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_poll_period, OT_RPC_CMD_LINK_SET_POLL_PERIOD,
			 ot_rpc_cmd_set_poll_period, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_poll_period, OT_RPC_CMD_LINK_GET_POLL_PERIOD,
			 ot_rpc_cmd_get_poll_period, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_max_frame_retries_direct,
			 OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT,
			 ot_rpc_cmd_set_max_frame_retries_direct, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_max_frame_retries_indirect,
			 OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT,
			 ot_rpc_cmd_set_max_frame_retries_indirect, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_link_enabled, OT_RPC_CMD_LINK_SET_ENABLED,
			 ot_rpc_cmd_set_link_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_factory_assigned_eui64,
			 OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64,
			 ot_rpc_cmd_get_factory_assigned_eui64, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_counters, OT_RPC_CMD_LINK_GET_COUNTERS,
			 ot_rpc_cmd_get_counters, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_factory_assigned_eui64,
			 OT_RPC_CMD_LINK_SET_FACTORY_ASSIGNED_EUI64,
			 ot_rpc_cmd_set_factory_assigned_eui64, NULL);
