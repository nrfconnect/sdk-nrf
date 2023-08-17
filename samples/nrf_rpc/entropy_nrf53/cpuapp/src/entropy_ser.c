/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <zephyr/init.h>
#include <string.h>

#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

#include "../../common_ids.h"
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>


#define CBOR_BUF_SIZE 16


struct entropy_get_result {
	uint8_t *buffer;
	size_t length;
	int result;
};


static void (*result_callback)(int result, uint8_t *buffer, size_t length);

NRF_RPC_IPC_TRANSPORT(entropy_group_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "nrf_rpc_ept");
NRF_RPC_GROUP_DEFINE(entropy_group, "nrf_sample_entropy", &entropy_group_tr, NULL, NULL, NULL);


void rsp_error_code_handle(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data)
{
	int32_t val;

	if (zcbor_int32_decode(ctx->zs, &val)) {
		*(int *)handler_data = (int)val;
	} else {
		*(int *)handler_data = -NRF_EINVAL;
	}
}


int entropy_remote_init(void)
{
	int result;
	int err;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&entropy_group, ctx, CBOR_BUF_SIZE);

	err = nrf_rpc_cbor_cmd(&entropy_group, RPC_COMMAND_ENTROPY_INIT, &ctx,
			       rsp_error_code_handle, &result);
	if (err < 0) {
		return err;
	}

	return result;
}


static void entropy_get_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	struct zcbor_string zst;
	struct entropy_get_result *result =
		(struct entropy_get_result *)handler_data;

	if (!zcbor_int32_decode(ctx->zs, &result->result)) {
		goto cbor_error_exit;
	}

	if (zcbor_bstr_decode(ctx->zs, &zst) && result->length == zst.len) {
		memcpy(result->buffer, zst.value, zst.len);
		return;
	}

cbor_error_exit:
	result->result = -NRF_EINVAL;
}


int entropy_remote_get(uint8_t *buffer, size_t length)
{
	int err;
	struct nrf_rpc_cbor_ctx ctx;
	struct entropy_get_result result = {
		.buffer = buffer,
		.length = length,
	};

	if (!buffer || length < 1) {
		return -NRF_EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&entropy_group, ctx, CBOR_BUF_SIZE);

	if (zcbor_uint32_put(ctx.zs, length)) {
		err = nrf_rpc_cbor_cmd(&entropy_group, RPC_COMMAND_ENTROPY_GET, &ctx,
			       entropy_get_rsp, &result);
		if (err) {
			return -NRF_EINVAL;
		}
		return result.result;
	}
	return -NRF_EINVAL;
}


int entropy_remote_get_inline(uint8_t *buffer, size_t length)
{
	int err;
	struct nrf_rpc_cbor_ctx ctx;
	struct zcbor_string zst;
	int result = -NRF_EINVAL;

	if (!buffer || length < 1) {
		return -NRF_EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&entropy_group, ctx, CBOR_BUF_SIZE);

	if (!zcbor_int32_put(ctx.zs, (int)length)) {
		return -NRF_EINVAL;
	}

	err = nrf_rpc_cbor_cmd_rsp(&entropy_group, RPC_COMMAND_ENTROPY_GET,
				   &ctx);
	if (err) {
		return -NRF_EINVAL;
	}

	if (!zcbor_int32_decode(ctx.zs, &result)) {
		goto cbor_error_exit;
	}

	if (!zcbor_bstr_decode(ctx.zs, &zst) || length != zst.len) {
		goto cbor_error_exit;
	}

	memcpy(buffer, zst.value, zst.len);
	result = 0;

cbor_error_exit:
	nrf_rpc_cbor_decoding_done(&entropy_group, &ctx);
	return result;
}


int entropy_remote_get_async(uint16_t length, void (*callback)(int result,
							       uint8_t *buffer,
							       size_t length))
{
	int err;
	struct nrf_rpc_cbor_ctx ctx;

	if (length < 1 || callback == NULL) {
		return -NRF_EINVAL;
	}

	result_callback = callback;

	NRF_RPC_CBOR_ALLOC(&entropy_group, ctx, CBOR_BUF_SIZE);

	if (!zcbor_int32_put(ctx.zs, (int)length)) {
		return -NRF_EINVAL;
	}

	err = nrf_rpc_cbor_evt(&entropy_group, RPC_EVENT_ENTROPY_GET_ASYNC,
			       &ctx);
	if (err) {
		return -NRF_EINVAL;
	}

	return 0;
}


int entropy_remote_get_cbk(uint16_t length, void (*callback)(int result,
							     uint8_t *buffer,
							     size_t length))
{
	int err;
	int result;
	struct nrf_rpc_cbor_ctx ctx;

	if (length < 1 || callback == NULL) {
		return -NRF_EINVAL;
	}

	result_callback = callback;

	NRF_RPC_CBOR_ALLOC(&entropy_group, ctx, CBOR_BUF_SIZE);

	if (!zcbor_int32_put(ctx.zs, (int)length)) {
		return -NRF_EINVAL;
	}

	err = nrf_rpc_cbor_cmd(&entropy_group, RPC_COMMAND_ENTROPY_GET_CBK,
			       &ctx, rsp_error_code_handle, &result);
	if (err) {
		return -NRF_EINVAL;
	}

	return result;
}



static void entropy_get_result_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool is_command = (handler_data != NULL);
	int err_code;
	size_t length;
	uint8_t buf[32];
	struct zcbor_string zst;

	if (result_callback == NULL) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		return;
	}

	if (!zcbor_int32_decode(ctx->zs, &err_code)) {
		goto cbor_error_exit;
	}

	length = ARRAY_SIZE(buf);

	if (!zcbor_bstr_decode(ctx->zs, &zst) || zst.len > length) {
		goto cbor_error_exit;
	}

	memcpy(buf, zst.value, zst.len);
	length = zst.len;

	nrf_rpc_cbor_decoding_done(group, ctx);

	result_callback(err_code, buf, length);

	if (is_command) {
		struct nrf_rpc_cbor_ctx nctx;

		NRF_RPC_CBOR_ALLOC(group, nctx, 0);
		nrf_rpc_cbor_rsp_no_err(group, &nctx);
	}

	return;

cbor_error_exit:
	nrf_rpc_cbor_decoding_done(group, ctx);
	result_callback(-NRF_EINVAL, buf, 0);
}


NRF_RPC_CBOR_CMD_DECODER(entropy_group, entropy_get_cbk_result,
			 RPC_COMMAND_ENTROPY_GET_CBK_RESULT,
			 entropy_get_result_handler, (void *)1);

NRF_RPC_CBOR_EVT_DECODER(entropy_group, entropy_get_async_result,
			 RPC_EVENT_ENTROPY_GET_ASYNC_RESULT,
			 entropy_get_result_handler, NULL);


static void err_handler(const struct nrf_rpc_err_report *report)
{
	printk("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
	       report->code);
	k_oops();
}


static int serialization_init(void)
{

	int err;

	printk("Init begin\n");

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -NRF_EINVAL;
	}

	printk("Init done\n");

	return 0;
}


SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
